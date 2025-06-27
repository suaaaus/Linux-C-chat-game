import socket
from gensim.models.fasttext import load_facebook_vectors

MODEL_PATH = 'cc.ko.300.bin'   # 압축 해제한 .bin 파일

# 여러 라운드를 위한 정답 리스트
ANSWERS = ['통신', '바늘', '용도', '등장하다', '대화']
current_index = 0

def get_current_answer():
    return ANSWERS[current_index]

# 모델 로드
print(f"Loading FastText model from {MODEL_PATH} ...")
kv = load_facebook_vectors(MODEL_PATH)
print("Model loaded, starting TCP service.")

# TCP 서버 설정
HOST, PORT = '0.0.0.0', 9000
sock = socket.socket()
sock.bind((HOST, PORT))
sock.listen(5)
print(f"유사도 서비스 대기 중: {PORT}")
print(f"[ROUND] 첫 번째 정답: '{get_current_answer()}'")

# 메인 루프: 클라이언트 요청 처리
while True:
    conn, addr = sock.accept()
    word = conn.recv(256).decode().strip()
    ans = get_current_answer()
    try:
        sim  = kv.similarity(word, ans) * 100
        topn = kv.most_similar(ans, topn=1000)
        rank = next((i+1 for i,(w,_) in enumerate(topn) if w == word), 1001)
        resp = f"{word}|{sim:.2f}|{rank}"
    except KeyError:
        resp = "ERROR|단어 없음"

    conn.sendall(resp.encode())
    conn.close()

    # 정답 확인 후 다음 라운드로 이동
    if word == ans:
        print(f"[ROUND] '{ans}' 정답! 다음 단어로 이동합니다.")
        current_index = (current_index + 1) % len(ANSWERS)
        print(f"[ROUND] 새로운 정답: '{get_current_answer()}'")
