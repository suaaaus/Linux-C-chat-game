# python-model/check_similarity_tcp.py

import socket
from gensim.models.fasttext import load_facebook_vectors

MODEL_PATH = 'cc.ko.300.bin'   # 압축 해제한 .bin 파일
ANSWER     = '군데'

print(f"Loading FastText model from {MODEL_PATH} ...")
# Facebook FastText 바이너리 포맷 바로 로드
kv = load_facebook_vectors(MODEL_PATH)
print("Model loaded, starting TCP service.")

HOST, PORT = '0.0.0.0', 9000
sock = socket.socket()
sock.bind((HOST, PORT))
sock.listen(5)
print(f"유사도 서비스 대기 중: {PORT}")

while True:
    conn, addr = sock.accept()
    word = conn.recv(256).decode().strip()
    try:
        sim  = kv.similarity(word, ANSWER) * 100
        topn = kv.most_similar(ANSWER, topn=1000)
        rank = next((i+1 for i,(w,_) in enumerate(topn) if w == word), 1001)
        resp = f"{word}|{sim:.2f}|{rank}"
    except KeyError:
        resp = "ERROR|단어 없음"

    conn.sendall(resp.encode())
    conn.close()
