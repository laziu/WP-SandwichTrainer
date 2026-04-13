# 🥪 Sandwich Trainer: FastAPI + MediaPipe 최소 구조 가이드

## 📋 목차
1. [프로젝트 구조](#프로젝트-구조)
2. [요구사항](#요구사항)
3. [설치](#설치)
4. [실행](#실행)
5. [사용 방법](#사용-방법)
6. [코드 설명](#코드-설명)
7. [트러블슈팅](#트러블슈팅)

---

## 📁 프로젝트 구조

```
WP-SandwichTrainer/
├── server/                 # 백엔드 서버
│   ├── main.py            # FastAPI 메인 파일 (앱 초기화)
│   └── app/
│       ├── hand_router.py # WebSocket 라우터 (손 추적 API)
│       └── schemas.py     # Pydantic 스키마 (데이터 검증)
│
├── client/                # 클라이언트
│   └── mediapipe_client.py # MediaPipe Hands 클라이언트
│
└── requirements.txt       # 패키지 의존성
```

---

## ✅ 요구사항

### 하드웨어
- **웹캠**: USB 웹캠 또는 노트북 내장 카메라
- **프로세서**: Intel i5 이상 (또는 동급 AMD)
- **메모리**: 8GB 이상 권장

### 소프트웨어
- **Python**: 3.8 이상
- **pip**: Python 패키지 매니저

### 운영체제 테스트됨
- Windows 10/11
- macOS 11+
- Ubuntu 20.04+

---

## 🚀 설치

### 1단계: Python 확인
```bash
python --version
# 또는
python3 --version
```

3.8 이상 필요합니다.

### 2단계: 프로젝트 디렉토리로 이동
```bash
cd c:\potenup3\WP-SandwichTrainer
# 또는 (Mac/Linux)
cd /path/to/WP-SandwichTrainer
```

### 3단계: 가상 환경 생성 (권장)
```bash
# Windows
python -m venv venv
venv\Scripts\activate

# Mac/Linux
python3 -m venv venv
source venv/bin/activate
```

### 4단계: 패키지 설치
```bash
pip install -r requirements.txt
```

> ⚠️ **주의**: 처음 설치 시 mediapipe 다운로드에 5~10분이 걸릴 수 있습니다.

---

## ▶️ 실행

### 터미널 1: 서버 실행

```bash
cd server
uvicorn main:app --reload --port 8888
```

**성공 메시지:**
```
INFO:     Uvicorn running on http://127.0.0.1:8888
INFO:     Application startup complete
```

서버 API 문서: [http://127.0.0.1:8888/docs](http://127.0.0.1:8888/docs)

### 터미널 2: 클라이언트 실행

새로운 터미널을 열고:

```bash
cd client
python mediapipe_client.py
```

**성공 메시지:**
```
2024-XX-XX XX:XX:XX,XXX - INFO - ============================================================
2024-XX-XX XX:XX:XX,XXX - INFO - MediaPipe Hands 클라이언트 시작
2024-XX-XX XX:XX:XX,XXX - INFO - 서버 주소: ws://127.0.0.1:8888/ws/hand
2024-XX-XX XX:XX:XX,XXX - INFO - 웹캠 해상도: 1280x720
```

---

## 💡 사용 방법

### 화면 인터페이스

클라이언트를 실행하면 다음 정보가 표시됩니다:

```
┌─────────────────────────────────────────┐
│  Gesture: open                          │
│  Action: hover                          │
│                                         │
│  [웹캠 영상]                              │
│  - 초록 점: 손의 21개 랜드마크              │
│  - 파란 선: 손의 뼈대                      │
│  - 청록 선: 엄지-검지 거리                  │
│  - 청록 텍스트: 거리 수치                    │
└─────────────────────────────────────────┘
```

### 제스처 종류

| 제스처 | 설명 | 액션 |
|--------|------|------|
| **open** | 손가락 펼친 상태 | `hover` |
| **pinch** | 엄지-검지 모음 (집기) | `grab_start` → `grab_hold` |
| **drop** | 손가락 펼침 (놓기) | `drop` |

### 액션 상태

| 액션 | 의미 | 색상 |
|------|------|------|
| `idle` | 손 미감지 | 회색 |
| `hover` | 마우스 호버 (손 열린 상태) | 초록 |
| `grab_start` | 집기 시작 | 주황 |
| `grab_hold` | 집기 유지 | 파란색 |
| `drop` | 놓기 | 분홍 |

### 종료

- 클라이언트 창에서 **'q' 키** 누르기
- 터미널에서 **Ctrl+C** 누르기

---

## 📖 코드 설명

### 1. 서버 아키텍처

```
클라이언트 (WebSocket)
    ↓
main.py (FastAPI 앱)
    ↓
hand_router.py (라우터)
    ↓
schemas.py (검증) ← Pydantic Validation
    ↓
decide_action() (로직)
    ↓
응답 (JSON)
```

### 2. 클라이언트 프로세스

```
웹캠 프레임
    ↓ (RGB 변환)
MediaPipe Hands 모델
    ↓ (손 인식)
21개 랜드마크 추출
    ↓ (거리 계산)
제스처 판정 (open/pinch)
    ↓
페이로드 생성 (JSON)
    ↓ (WebSocket)
서버 전송
    ↓
액션 응답 수신
    ↓
화면 표시 & 다음 프레임
```

### 3. MediaPipe Hands 랜드마크

손의 21개 점 (인덱스):

```
     8 (검지 끝)
     │
     6
     │
     5
     │
     4 (엄지 끝)
  3─2─1─0 (손목)
```

**주요 인덱스:**
- `0`: 손목 (WRIST)
- `4`: 엄지 끝 (THUMB_TIP)
- `8`: 검지 끝 (INDEX_TIP)

---

## 🔧 커스터마이징

### 거리 임계값 변경

제스처 인식 민감도를 조정하려면:

**`client/mediapipe_client.py`:**
```python
PINCH_DISTANCE_THRESHOLD = 0.05  # 기본값
# 작게: pinch 더 쉽게 (예: 0.03)
# 크게: pinch 어렵게 (예: 0.08)
```

### 다른 제스처 추가

**`server/app/hand_router.py`의 `decide_action()` 함수:**

```python
def decide_action(data: HandPayload) -> str:
    # 기존 코드...
    
    elif data.gesture == "grab":  # 새로운 제스처
        return "grab_start"
    
    # ...
```

### 포트 번호 변경

**`client/mediapipe_client.py`:**
```python
WS_URL = "ws://127.0.0.1:9000/ws/hand"  # 포트 변경
```

**`server/main.py` 실행:**
```bash
uvicorn main:app --port 9000
```

---

## 🐛 트러블슈팅

### 1. 웹캠이 작동하지 않음

**증상**: `웹캠을 열 수 없습니다!` 에러

**해결방법**:
```bash
# 다른 카메라 번호 시도
# mediapipe_client.py 수정:
cap = cv2.VideoCapture(1)  # 0 → 1로 변경 (또는 2, 3...)
```

### 2. "ModuleNotFoundError" 에러

**증상**: `ModuleNotFoundError: No module named 'mediapipe'`

**해결방법**:
```bash
# 설치 확인
pip list | grep mediapipe

# 재설치
pip install --upgrade mediapipe
```

### 3. WebSocket 연결 실패

**증상**: `WebSocket 에러: connection closed`

**해결방법**:
1. 서버가 실행 중인지 확인: `http://127.0.0.1:8888/health`
2. 방화벽 설정 확인
3. 포트 충돌 확인:
   ```bash
   # Windows
   netstat -ano | findstr :8888
   
   # Mac/Linux
   lsof -i :8888
   ```

### 4. 느린 처리 속도

**증상**: 화면이 끊기거나 지연

**해결방법**:
```python
# 다른 프로그램 닫기
# 웹캠 해상도 낮추기:
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)   # 1280 → 640
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)  # 720 → 480
```

### 5. 손 인식이 안 됨

**증상**: `hand_detected: False` 계속 출력

**해결방법**:
- 밝은 환경에서 테스트
- 손을 카메라 가까이 이동
- 신뢰도 임계값 낮추기:
  ```python
  with mp_hands.Hands(
      min_detection_confidence=0.5,  # 0.7 → 0.5로 낮춤
  ) as hands:
  ```

---

## 📚 다음 단계

1. **제스처 추가**: 다양한 손-손가락 조합 인식
2. **좌표 기반 로직**: 화면 구역별 액션
3. **양손 지원**: 두 손 동시 추적
4. **기계학습**: 커스텀 제스처 학습
5. **UI 개선**: 실시간 대시보드, 웹 인터페이스

---

## 📝 라이선스 & 참고

- **MediaPipe**: Google © (Apache 2.0)
- **FastAPI**: Sebastián Ramírez © (MIT)
- **OpenCV**: Intel © (Apache 2.0)

---

## ❓ FAQ

**Q: 여러 손을 동시에 추적할 수 있나요?**
A: 현재는 1개 손만 추적합니다. `max_num_hands=2`로 변경하면 2개 손 추적 가능합니다.

**Q: CPU vs GPU 성능은?**
A: MediaPipe는 기본적으로 CPU 사용. GPU 사용은 특별 설정 필요합니다.

**Q: 마우스 제어까지 가능한가요?**
A: 가능합니다! `pyautogui` 라이브러리로 마우스 제어 기능을 추가할 수 있습니다.

---

## 💬 문의

기술 문의는 MediaPipe 공식 문서를 참고하세요:
- [MediaPipe Hands](https://mediapipe.dev/solutions/hands)
- [FastAPI 문서](https://fastapi.tiangolo.com/)
- [WebSocket 개념](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)
