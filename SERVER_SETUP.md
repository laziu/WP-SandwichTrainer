# 서버 기반 MediaPipe 손 인식 시스템

## 변경 사항

원래는 클라이언트가 카메라와 MediaPipe를 처리했으나, 이제 **서버에서 처리**하고 **클라이언트에게 JSON으로 전송**합니다.

### 구조 변화

```
이전 (클라이언트 기반):
클라이언트 → [웹캠 + MediaPipe 처리] → 서버로 JSON 전송 → 서버 (액션 판정)

현재 (서버 기반):
서버 → [웹캠 + MediaPipe 처리] → 클라이언트에게 JSON 전송
```

---

## 서버 실행

### 1. 필수 패키지 설치

```bash
cd server
pip install -r requirements.txt
# 또는
pip install fastapi uvicorn pydantic websockets mediapipe opencv-contrib-python
```

### 2. 서버 시작

```bash
# 방법 1: uvicorn 명령어
cd server
uvicorn main:app --reload --port 8888

# 방법 2: Python 직접 실행
cd server
python main.py
```

서버는 자동으로 웹캠을 열고 MediaPipe로 손을 인식합니다.

**로그 예시:**
```
INFO:     Uvicorn running on http://0.0.0.0:8888
✓ CameraProcessor 시작됨
프레임 처리 시작
```

---

## 클라이언트 사용

### 옵션 1: REST API (권장 - 간단함)

```bash
cd client
python client_rest.py
```

**특징:**
- 클라이언트가 주기적으로 `GET /data` 호출
- 약 30fps (0.033초마다 요청)
- 간단하고 동기적

**로그 예시:**
```
INFO:root:서버 주소: http://127.0.0.1:8888
INFO:root:엔드포인트: http://127.0.0.1:8888/data
Frame #10: hand=True, gesture=open, action=hover
Frame #20: hand=True, gesture=pinch, action=grab_start
```

### 옵션 2: WebSocket 스트리밍 (실시간 스트림)

```bash
cd client
python client_websocket.py
```

**특징:**
- WebSocket 연결 유지
- 서버에서 ~30fps로 자동으로 데이터 스트림 전송
- 더 효율적이고 실시간성 우수

**로그 예시:**
```
INFO:root:서버 주소: ws://127.0.0.1:8888
INFO:root:엔드포인트: ws://127.0.0.1:8888/ws/hand/stream
✓ WebSocket 연결됨
Frame #10: hand=True, gesture=open, action=hover
```

---

## API 엔드포인트

### 1. 헬스 체크

```bash
curl http://localhost:8888/health
```

응답:
```json
{
  "ok": true,
  "message": "Server is running"
}
```

### 2. 최신 손 정보 조회 (REST)

```bash
curl http://localhost:8888/data
```

응답 (손 감지됨):
```json
{
  "ok": true,
  "hand_detected": true,
  "gesture": "open",
  "index_tip": {"x": 0.5, "y": 0.4},
  "thumb_tip": {"x": 0.48, "y": 0.42},
  "landmarks": [...],
  "timestamp": 1704067200.123,
  "action": "hover"
}
```

응답 (손 미감지):
```json
{
  "ok": true,
  "hand_detected": false,
  "gesture": "none",
  "action": "idle"
}
```

### 3. 손 정보 스트리밍 (WebSocket)

```bash
websocat ws://localhost:8888/ws/hand/stream
```

연결 후 ~30fps로 데이터가 계속 전송됩니다.

---

## JSON 데이터 형식

모든 클라이언트 응답은 다음 구조를 따릅니다:

```json
{
  "ok": true,
  "hand_detected": true,
  "gesture": "open|pinch|none",
  "index_tip": {
    "x": 0.0,
    "y": 0.0
  },
  "thumb_tip": {
    "x": 0.0,
    "y": 0.0
  },
  "landmarks": [
    {"x": 0.1, "y": 0.2},
    ...
  ],
  "timestamp": 1704067200.123,
  "action": "idle|hover|grab_start|grab_hold|drop"
}
```

**필드 설명:**
- `hand_detected`: 손 감지 여부
- `gesture`: 제스처 ("open", "pinch", "none")
- `index_tip`: 검지 끝 좌표 (0~1 정규화)
- `thumb_tip`: 엄지 끝 좌표 (0~1 정규화)
- `landmarks`: 21개 손 포인트 좌표 배열
- `timestamp`: Unix 타임스탐프
- `action`: 수행할 액션
  - `idle`: 손 미감지
  - `hover`: 손 열린 상태
  - `grab_start`: 손가락 모음 시작
  - `grab_hold`: 손가락 모음 유지
  - `drop`: 손가락 펼침

---

## 커스터마이징

### 카메라 ID 변경

[server/main.py](server/main.py)의 `CameraProcessor(camera_id=0)` 부분 수정:

```python
# 웹캠이 여러 개일 경우 다른 ID 사용
camera_processor = CameraProcessor(camera_id=1)  # ID 1, 2, ... 등
```

### 제스처 감지 임계값 변경

[server/app/camera_processor.py](server/app/camera_processor.py)의 `PINCH_DISTANCE_THRESHOLD` 수정:

```python
PINCH_DISTANCE_THRESHOLD = 0.05  # 엄지-검지 거리 기준 (작을수록 까다로움)
```

### MediaPipe 모델 설정

[server/app/camera_processor.py](server/app/camera_processor.py)의 `Hands()` 초기화 부분 수정:

```python
self.hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=2,  # 한 손이 아닌 두 손 추적
    min_detection_confidence=0.7,
    min_tracking_confidence=0.5
)
```

---

## 트러블슈팅

### 1. "카메라를 열 수 없음" 오류

```
logger.error(f"카메라 {self.camera_id} 열기 실패")
```

**해결방법:**
- 카메라 권한 확인
- 다른 프로그램에서 카메라 사용 중 아닌지 확인
- 카메라 ID 변경 (0 → 1)

### 2. WebSocket 연결 실패

```
websockets.exceptions.WebSocketException
```

**해결방법:**
- 서버가 실행 중인지 확인
- 포트 8888이 이미 사용 중인지 확인
- 방화벽 설정 확인

### 3. 느린 응답

**REST API의 경우:**
- `POLL_INTERVAL` 감소 (기본값: 0.033초)

**WebSocket의 경우:**
- 네트워크 연결 확인
- 카메라 처리 부하 확인 (CPU 사용률)

---

## 포트 변경

포트를 다르게 사용하려면:

```bash
# 서버: 포트 9000 사용
uvicorn main:app --port 9000

# 클라이언트: client_rest.py에서
SERVER_URL = "http://127.0.0.1:9000"

# 클라이언트: client_websocket.py에서
SERVER_URL = "ws://127.0.0.1:9000"
```

---

## 성능 최적화

1. **카메라 해상도 감소** (camera_processor.py):
   ```python
   self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)   # 1280 → 640
   self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 360)  # 720 → 360
   ```

2. **FPS 감소**:
   ```python
   self.cap.set(cv2.CAP_PROP_FPS, 15)  # 30 → 15
   ```

3. **MediaPipe 신뢰도 하향** (손 감지 속도 증가):
   ```python
   min_detection_confidence=0.5,  # 0.7 → 0.5
   ```

---

## 참고사항

- 현재 한 손만 추적합니다 (`max_num_hands=1`)
- 두 손 추적이 필요하면 `camera_processor.py` 수정
- 정규화된 좌표 (0~1)는 화면 크기에 관계없이 일관성 유지
