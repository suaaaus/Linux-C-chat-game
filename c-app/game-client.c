// client.c
// Chat + Semantle-KO Guess Client (닉네임, 헤더 유지, /guess UI)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXLINE   512
#define BAR_SLOTS 10

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *sv_ip   = argv[1];
    int         sv_port = atoi(argv[2]);

    // 1) 서버 연결
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }
    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port   = htons(sv_port);
    inet_pton(AF_INET, sv_ip, &serv.sin_addr);
    if (connect(sockfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect"); exit(1);
    }

    // 2) 닉네임 입력 & 전송
    char nick[MAXLINE];
    printf("Enter your nick: ");
    fflush(stdout);
    if (!fgets(nick, sizeof(nick), stdin)) exit(1);
    nick[strcspn(nick, "\r\n")] = '\0';
    send(sockfd, nick, strlen(nick), 0);

    // 3) 화면 헤더
    printf("\nSemantle-KO 서버에 연결: %s:%d\n", sv_ip, sv_port);
    printf("일반 메시지 → broadcast\n");
    printf("/guess <word> → 유사도 확인\n");
    printf("Ctrl+D → 종료\n\n");
    printf("추측한 단어 |   유사도   |    순위    |\n");
    printf("-----------------------------------------------------\n");

    // 4) select 루프
    int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
    fd_set rset;
    char sendbuf[MAXLINE], recvbuf[MAXLINE];

    while (1) {
        FD_ZERO(&rset);
        FD_SET(STDIN_FILENO, &rset);
        FD_SET(sockfd,       &rset);
        if (select(maxfd+1, &rset, NULL, NULL, NULL) < 0) {
            if (errno==EINTR) continue;
            perror("select"); break;
        }

        // (A) 사용자 입력 → 서버
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            if (!fgets(sendbuf, sizeof(sendbuf), stdin)) {
                printf("\n프로그램 종료\n");
                break;
            }
            sendbuf[strcspn(sendbuf, "\r\n")] = '\0';
            if (sendbuf[0]=='\0') continue;
            write(sockfd, sendbuf, strlen(sendbuf));
        }

        // (B) 서버 메시지 수신
        if (FD_ISSET(sockfd, &rset)) {
            ssize_t n = read(sockfd, recvbuf, sizeof(recvbuf)-1);
            if (n <= 0) {
                printf("서버 연결 종료\n");
                break;
            }
            recvbuf[n] = '\0';

            // “word|sim|rank” 포맷인지 검사
            if (strchr(recvbuf, '|')) {
                // 파싱
                char *rest = recvbuf, *tok;
                char word[64], bar[BAR_SLOTS+1], rankstr[16];
                double sim;
                int   rank;

                tok = strtok_r(rest, "|", &rest);
                strncpy(word, tok, sizeof(word)-1);
                tok = strtok_r(NULL, "|", &rest);
                sim = atof(tok);
                tok = strtok_r(NULL, "|", &rest);
                rank = atoi(tok);

                // 순위 문자열
                if (sim >= 100.0) {
                    strcpy(rankstr, "정답!");
                } else if (rank > 1000) {
                    strcpy(rankstr, "1000위 이상");
                } else {
                    snprintf(rankstr, sizeof(rankstr), "%d위", rank);
                }

                // 막대 생성
                memset(bar, '-', BAR_SLOTS);
                bar[BAR_SLOTS] = '\0';
                if (sim < 100.0) {
                    int bucket = (rank - 1) / 100;
                    if (bucket < 0) bucket = 9;
                    if (bucket > 9) bucket = 9;
                    int filled = BAR_SLOTS - bucket;
                    for (int i = 0; i < filled; i++) bar[i] = '#';
                }

                // 출력
                if (sim >= 100.0) {
                    printf("%-10s | %7.2f%%   | %-10s |\n",
                           word, sim, rankstr);
                } else {
                    printf("%-10s | %7.2f%%   | %-10s | %s\n",
                           word, sim, rankstr, bar);
                }
                printf("-----------------------------------------------------\n");
            }
            else {
                // 일반 채팅 메시지
                fputs(recvbuf, stdout);
                if (recvbuf[n-1]!='\n') putchar('\n');
            }
        }
    }

    close(sockfd);
    return 0;
}

