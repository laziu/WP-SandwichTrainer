# 🚀 빠른 시작 가이드 (5분)

## 1️⃣ 준비물 확인

- ✅ Python 3.8+ 설치 여부 확인
- ✅ 웹캠 연결
- ✅ 인터넷 연결

## 2️⃣ 한 줄 설치

```bash
# 프로젝트 디렉토리로 이동
cd WP-SandwichTrainer

# 가상 환경 생성 및 활성화
python -m venv venv
venv\Scripts\activate  # Windows

# 또는 Mac/Linux:
# source venv/bin/activate

# 패키지 설치
pip install -r requirements.txt
```

⏱️ 약 5~10분 소요 (처음엔 mediapipe 다운로드)

## 3️⃣ 한 번에 실행

### 터미널 1 (서버)
```bash
cd server
uvicorn main:app --reload --port 8888
```

### 터미널 2 (클라이언트)
```bash
cd client
python mediapipe_client.py
```

## 4️⃣ 화면에서 보면...

✅ **웹캠 영상 표시**
✅ **초록 점들**: 손의 21개 점들 (랜드마크)
✅ **파란 선**: 손의 뼈대
✅ **텍스트**: 현재 제스처와 액션

## 5️⃣ 손을 움직여보기

1. 손을 카메라 앞에 들기
2. 손가락을 펼쳤다 모았다 반복
3. 화면의 텍스트가 바뀌는 것을 확인

| 동작 | 화면 표시 |
|------|---------|
| 손가락 펼침 | `Gesture: open` → `Action: hover` |
| 엄지-검지 모음 | `Gesture: pinch` → `Action: grab_hold` |
| 손가락 다시 펼침 | `Action: drop` |

## 6️⃣ 종료하기

- 클라이언트 창에서 **'q' 키** 누르기
- 또는 터미널에서 **Ctrl+C**

---

완료! 🎉

더 자세한 설명은 `INSTALLATION_GUIDE.md`를 읽어보세요.
