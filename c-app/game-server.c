// server.c
// Chat + Semantle-KO Guess Server (닉네임, 입/퇴장 알림, 정답 브로드캐스트)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_CLIENTS   100
#define MAX_NICKLEN   32
#define MAXLINE      512

// Python similarity service info
#define PY_HOST     "127.0.0.1"
#define PY_PORT      9000

typedef struct {
    int  sockfd;
    char nick[MAX_NICKLEN];
} client_t;

static client_t *clients[MAX_CLIENTS];
static pthread_mutex_t clients_mtx = PTHREAD_MUTEX_INITIALIZER;

// except_fd를 제외한 모두에게 msg 전송
void broadcast(const char *msg, int except_fd) {
    pthread_mutex_lock(&clients_mtx);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->sockfd != except_fd) {
            send(clients[i]->sockfd, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mtx);
}

// Python 서비스에 단어 보내고 "word|sim|rank" 한 줄 읽기
int get_similarity(const char *word, char *buf, size_t buflen) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port   = htons(PY_PORT);
    inet_pton(AF_INET, PY_HOST, &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        close(sock);
        return -1;
    }
    dprintf(sock, "%s\n", word);
    ssize_t n = read(sock, buf, buflen-1);
    close(sock);
    if (n <= 0) return -1;
    buf[n] = '\0';
    if (buf[n-1]=='\n') buf[n-1]='\0';
    return 0;
}

void *handle_client(void *arg) {
    client_t *ci = arg;
    char line[MAXLINE];

    // —— 1) 닉네임 받기
    int n = recv(ci->sockfd, ci->nick, MAX_NICKLEN-1, 0);
    if (n <= 0) { close(ci->sockfd); free(ci); return NULL; }
    ci->nick[n] = '\0';
    // 입장 메시지
    snprintf(line, sizeof(line),
             "[SERVER] %s님이 입장하셨습니다.\n",
             ci->nick);
    broadcast(line, -1);

    // —— 2) 메시지 루프
    while ((n = recv(ci->sockfd, line, sizeof(line)-1, 0)) > 0) {
        line[n] = '\0';
        // '/guess ' 처리
        if (strncmp(line, "/guess ", 7) == 0) {
            char *word = line + 7;
            // 공백·개행 제거
            word[strcspn(word, "\r\n")] = '\0';

            char resp[MAXLINE];
            if (get_similarity(word, resp, sizeof(resp)) == 0) {
                // 1) 요청자에게 결과 전송
                send(ci->sockfd, resp, strlen(resp), 0);
                send(ci->sockfd, "\n", 1, 0);
                // 2) resp = "word|sim|rank"
                //    sim 부분 파싱하여 100.00 이상이면 '정답'으로 판단
                char *p = strchr(resp, '|');
                if (p) {
                    double sim = atof(p+1);
                    if (sim >= 100.0) {
                        // 정답 맞춘 사람 브로드캐스트
                        snprintf(line, sizeof(line),
                                 "[SERVER] %s님이 정답을 맞췄습니다!\n",
                                 ci->nick);
                        broadcast(line, -1);
                    }
                }
            } else {
                const char *err = "ERROR|서비스 오류\n";
                send(ci->sockfd, err, strlen(err), 0);
            }
        }
        else {
            // 일반 채팅: "[nick] message"
            char out[MAX_NICKLEN+MAXLINE];
            snprintf(out, sizeof(out),
                     "[%s] %s", ci->nick, line);
            broadcast(out, ci->sockfd);
        }
    }

    // —— 3) 퇴장
    snprintf(line, sizeof(line),
             "[SERVER] %s님이 퇴장하셨습니다.\n",
             ci->nick);
    broadcast(line, -1);

    close(ci->sockfd);
    pthread_mutex_lock(&clients_mtx);
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (clients[i] == ci) { clients[i] = NULL; break; }
    }
    pthread_mutex_unlock(&clients_mtx);
    free(ci);
    return NULL;
}

int main(int argc, char *argv[]) {
    int port = (argc==2 ? atoi(argv[1]) : 6667);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv = {0};
    serv.sin_family      = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port        = htons(port);

    bind(listenfd, (struct sockaddr*)&serv, sizeof(serv));
    listen(listenfd, 10);
    printf("Server listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int conn = accept(listenfd,
                          (struct sockaddr*)&cli, &clilen);
        if (conn < 0) continue;

        client_t *ci = malloc(sizeof(*ci));
        ci->sockfd = conn;

        pthread_mutex_lock(&clients_mtx);
        int added = 0;
        for (int i = 0; i < MAX_CLIENTS; i++){
            if (!clients[i]) {
                clients[i] = ci;
                added = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mtx);

        if (!added) {
            const char *full = "[SERVER] 방이 가득 찼습니다.\n";
            send(conn, full, strlen(full), 0);
            close(conn);
            free(ci);
        } else {
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, ci);
            pthread_detach(tid);
        }
    }
    return 0;
}

