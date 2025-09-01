# 🧠 Semantle-KO Chat Game (C + Python)


## 📝 개요 

**Python** 기반 FastText 유사도 계산 서비스와 **C** 언어 기반 TCP 소켓 다중 클라이언트 채팅 기능을 결합하여,

NewsJelly의 [Semantle-KO](https://semantle-ko.newsjel.ly/) 게임을 구현한 네트워크 프로그램입니다.


+ 사용자는 실시간 채팅을 하며 /guess <단어> 로 정답을 추측할 수 있습니다.
  
+ 서버는 FastText 모델을 통해 단어 유사도와 순위를 계산하고, 추측 결과를 시각화합니다.
  



## 📂 디렉터리 구조
```bash
chat-semantle/
├── c-app/                      # C 기반 채팅 & 게임 클라이언트/서버
│   ├── game-server.c         
│   └── game-client.c         
│    
│
├── python-model/                # Python 유사도 모델 서비스
│   ├── check_similarity_tcp.py  # FastText 기반 TCP 유사도 서버
│   └── cc.ko.300.bin            # FastText 한글 사전 모델 (6.8GB)
```




## ⚙️ 설치 및 실행
**1. Python 유사도 서버 실행**
```bash
cd python-model
python3 -m venv myenv
source myenv/bin/activate
pip install gensim
python3 check_similarity_tcp.py
```
  ⚠️ cc.ko.300.bin 파일은 FastText 공식(https://fasttext.cc/docs/en/crawl-vectors.html) 에서 다운로드


**2. C 채팅/게임 서버 실행**
```bash
cd c-app
gcc game-server.c -o game-server
./game-server 6667
```


**3. C 클라이언트 실행 (동일 LAN의 다른 기기에서 가능)**
```bash
gcc game-client.c -o semantle-client
./semantle-client <서버_IP> 6667
```




## 💡 사용법
서버에 접속하면 닉네임을 입력합니다.

일반 채팅 메시지를 입력하거나,

/guess <단어> 로 꼬맨틀 게임 정답 단어를 추측 입력


정답 유사도와 순위는 막대로 시각화되어 출력됩니다.


+ 예시 화면
  ```bash
  추측한 단어 |   유사도   |    순위    |
  ------------------------------------------
  강아지     |   39.60%  | 70위      | #########-
  식물      |   24.85%  | 780위     | ####-------
  정답      |  100.00%  | 1위       | 정답입니다!
  ```




## 🎯 주요 기능

+ FastText 유사도 계산 기반 순위 표시
  
+ 유사도 시각화 바 출력 (10단계)
  
+ 정답 맞춘 사용자 전체 채팅방에 자동 알림
  
+ 닉네임 기반 입장/퇴장 알림 및 채팅



## 🔧 설정 변경 (Configuration)

Python 코드의 꼬맨틀 정답 단어는 check_similarity_tcp.py 에서 ANSWER_LIST 수정

C 서버/클라이언트 포트는 실행 시 인자(./game-server 6667)로 설정
