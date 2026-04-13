# 📚 파일별 상세 설명

초보자가 이해하기 쉽도록 각 파일의 역할을 설명합니다.

---

## 🏗️ 전체 아키텍처

```
┌─────────────────────────────────────────────────────────┐
│ 클라이언트 (client/mediapipe_client.py)                 │
│                                                         │
│ 역할: 웹캠에서 손 추적, 제스처 판정, 서버와 통신         │
│                                                         │
│ 1. 웹캠에서 프레임 읽기                                   │
│ 2. MediaPipe로 손 인식 (21개 점)                         │
│ 3. 엄지-검지 거리로 제스처 판정                          │
│ 4. JSON 페이로드 생성                                    │
│ 5. WebSocket으로 서버에 전송                            │
│ 6. 서버 응답 수신 (액션)                                 │
│ 7. 웹캠 영상에 정보 표시                                 │
└─────────────────────────────────────────────────────────┘
           ↕ (WebSocket 통신)
┌─────────────────────────────────────────────────────────┐
│ 서버 (server/)                                          │
│                                                         │
│ ┌──────────────────────────────────────────────────┐   │
│ │ main.py                                          │   │
│ │ 역할: FastAPI 앱 초기화, CORS 설정, 라우터 등록  │   │
│ │                                                  │   │
│ │ - FastAPI 인스턴스 생성: app = FastAPI()         │   │
│ │ - CORS 미들웨어 추가                              │   │
│ │ - 라우터 등록: app.include_router(hand_router)   │   │
│ │ - 루트 엔드포인트: GET /                         │   │
│ └──────────────────────────────────────────────────┘   │
│          ↓                                              │
│ ┌──────────────────────────────────────────────────┐   │
│ │ app/hand_router.py                               │   │
│ │ 역할: WebSocket 핸들러, 액션 판정 로직            │   │
│ │                                                  │   │
│ │ - /health: 건강 상태 체크 (REST)                 │   │
│ │ - /ws/hand: WebSocket 핸들러 (실시간 통신)       │   │
│ │   - 클라이언트 연결 수용                           │   │
│ │   - JSON 데이터 검증 (Pydantic)                  │   │
│ │   - 액션 판정                                     │   │
│ │   - 응답 전송                                     │   │
│ │   - 예외 처리                                     │   │
│ └──────────────────────────────────────────────────┘   │
│          ↕ (데이터 검증)                                │
│ ┌──────────────────────────────────────────────────┐   │
│ │ app/schemas.py                                   │   │
│ │ 역할: Pydantic 데이터 스키마 (검증)               │   │
│ │                                                  │   │
│ │ - Point2D: 좌표 (x, y)                          │   │
│ │ - HandPayload: 클라이언트 입력 검증              │   │
│ │ - ServerResponse: 서버 응답 형식 정의            │   │
│ └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

---

## 📄 파일 1: `server/main.py`

### 목적
FastAPI 웹 애플리케이션 초기화 및 설정

### 역할
- ✅ FastAPI 인스턴스 생성
- ✅ CORS 미들웨어 추가 (클라이언트에서 다른 도메인 요청 허용)
- ✅ 라우터 등록
- ✅ 루트 엔드포인트 제공

### 주요 변수

| 변수 | 의미 |
|------|------|
| `app` | FastAPI 애플리케이션 인스턴스 |
| `title` | API 문서 제목 |
| `version` | API 버전 |

### 주요 함수

| 함수 | 목적 | 반환 |
|------|------|------|
| `root()` | 루트 엔드포인트 | API 정보 JSON |

### 실행 방법

```bash
cd server
uvicorn main:app --reload --port 8888
```

- `uvicorn`: ASGI 웹서버
- `main:app`: main.py의 app 인스턴스 실행
- `--reload`: 코드 변경 시 자동 재시작
- `--port 8888`: 포트 번호

### API 접속
- 메인 API: http://127.0.0.1:8888
- Swagger 문서: http://127.0.0.1:8888/docs
- ReDoc 문서: http://127.0.0.1:8888/redoc

---

## 📄 파일 2: `server/app/schemas.py`

### 목적
데이터 검증 및 타입 안정성 보장 (Pydantic)

### 개념: Pydantic이란?
- Python에서 데이터 검증하는 라이브러리
- JSON 데이터가 올바른 형식인지 자동 확인
- 타입 오류 방지

### 예시
```python
# 올바른 JSON (통과)
data = {
    "x": 0.5,
    "y": 0.3
}
point = Point2D(**data)  # ✅ OK

# 잘못된 JSON (실패)
data = {
    "x": 1.5,      # ❌ 1.0 초과
    "y": 0.3
}
point = Point2D(**data)  # ValueError 발생
```

### 스키마 3가지

#### 1. `Point2D`
```python
class Point2D(BaseModel):
    x: float = Field(..., ge=0.0, le=1.0)
    y: float = Field(..., ge=0.0, le=1.0)
```
- 용도: 2D 좌표
- `x, y`: 0.0 ~ 1.0 사이의 정규화된 값
- `Field(..., ge=0.0, le=1.0)`: 유효성 검사

#### 2. `HandPayload`
```python
class HandPayload(BaseModel):
    hand_detected: bool
    gesture: Literal["open", "pinch", "grab", "none"] = "none"
    index_tip: Optional[Point2D] = None
    thumb_tip: Optional[Point2D] = None
    landmarks: Optional[List[Point2D]] = None
    timestamp: float
```
- 용도: 클라이언트에서 서버로 보내는 데이터
- `hand_detected`: 손 감지 여부
- `gesture`: "open", "pinch" 중 하나만 가능 (Literal)
- `index_tip`, `thumb_tip`: Point2D 객체 (또는 None)
- `landmarks`: 21개 좌표 리스트
- `timestamp`: 데이터 생성 시간

#### 3. `ServerResponse`
```python
class ServerResponse(BaseModel):
    ok: bool
    message: str
    action: Literal["idle", "hover", "grab_start", "grab_hold", "drop"] = "idle"
```
- 용도: 서버에서 클라이언트로 보내는 응답
- `ok`: 성공/실패 여부
- `message`: 상태 메시지
- `action`: 수행할 액션

### Pydantic 이점

| 이점 | 설명 |
|------|------|
| **타입 안정성** | JSON이 정의된 타입과 맞는지 자동 확인 |
| **자동 검증** | 범위(ge, le), 필수 여부 자동 확인 |
| **에러 메시지** | 오류 발생 시 명확한 메시지 제공 |
| **문서화** | 자동으로 API 문서 생성 |

---

## 📄 파일 3: `server/app/hand_router.py`

### 목적
손 추적 API 라우터 (WebSocket + 액션 로직)

### 주요 기능

#### 1. Health Check 엔드포인트
```python
@router.get("/health")
async def health_check():
    return {"ok": True, "message": "Server is running"}
```
- 용도: 서버 상태 확인
- URL: `GET http://127.0.0.1:8888/health`
- 반환: `{"ok": true, "message": "..."}`

#### 2. WebSocket 핸들러
```python
@router.websocket("/ws/hand")
async def websocket_hand(websocket: WebSocket):
    # 1. 연결 수용
    await websocket.accept()
    
    # 2. 무한 루프 (데이터 수신)
    while True:
        raw_data = await websocket.receive_json()
        # 3. Pydantic 검증
        data = HandPayload(**raw_data)
        # 4. 액션 판정
        action = decide_action(data)
        # 5. 응답 전송
        response = ServerResponse(...)
        await websocket.send_json(response.model_dump())
```

**WebSocket 흐름:**
```
클라이언트                서버
    │                      │
    ├─ 연결 요청 ──────────→│
    │                      ├─ 연결 수용
    │                      │
    ├─ JSON 전송 ──────────→│
    │                      ├─ 검증 (Pydantic)
    │                      ├─ 액션 판정
    │                      │
    │←──── 응답 JSON ───────┤
    │                      │
    ├─ JSON 전송 ──────────→│  (계속 반복)
    │                      │
    ├─ 연결 종료 ──────────→│
    │                      ├─ 연결 정리
```

#### 3. `decide_action()` 함수

손 정보를 받고 액션을 결정하는 로직:

```python
def decide_action(data: HandPayload) -> str:
    if not data.hand_detected:
        return "idle"  # 손 미감지
    
    if data.gesture == "pinch":
        # 그전엔 open이었는가?
        if last_gesture != "pinch":
            return "grab_start"  # 새로 집기 시작
        else:
            return "grab_hold"   # 계속 집고 있음
    
    elif data.gesture == "open":
        # 그전엔 pinch였는가?
        if last_gesture == "pinch":
            return "drop"        # 손을 폈을 때
        else:
            return "hover"       # 손 열린 상태 유지
    
    return "idle"
```

**상태 전이 다이어그램:**
```
idle ────→ hover
         (손 열림)
 ↑           │
 │    pinch  │
 │           ↓
 └─ grab_hold
       ↑
       │ 검지
       │ 모음
    drop ← open
```

### 예외 처리

| 예외 | 처리 방법 |
|------|---------|
| `WebSocketDisconnect` | 클라이언트 연결 끊김 인식 |
| `ValueError` | Pydantic 검증 실패 → 에러 응답 |
| 기타 Exception | 로그 기록 및 에러 응답 |

---

## 📄 파일 4: `client/mediapipe_client.py`

### 목적
웹캠에서 손 추적, 제스처 판정, 서버와 통신

### 주요 함수

#### 1. `calculate_distance(point1, point2)`
두 점 사이의 거리 계산:
```python
distance = calculate_distance((0.5, 0.5), (0.3, 0.3))
# 리턴: 약 0.283
```

#### 2. `detect_gesture(hand_landmarks)`
손의 21개 점을 분석해 제스처 판정:
```python
gesture = detect_gesture(hand_landmarks)
# 리턴: "pinch" 또는 "open"
```

**판정 로직:**
- 엄지(#4)와 검지(#8) 사이 거리 계산
- 거리 < 0.05 → "pinch"
- 거리 ≥ 0.05 → "open"

#### 3. `extract_payload(result, frame_time)`
MediaPipe 결과를 JSON으로 변환:
```python
payload = extract_payload(result, time.time())
# 리턴:
# {
#     "hand_detected": True,
#     "gesture": "open",
#     "index_tip": {"x": 0.5, "y": 0.3},
#     "thumb_tip": {"x": 0.51, "y": 0.32},
#     "landmarks": [...],  # 21개 좌표
#     "timestamp": 1234567890.123
# }
```

#### 4. `draw_frame_info(frame, result, payload, server_response)`
웹캠 프레임에 정보 그리기:
```python
frame = draw_frame_info(frame, result, payload, server_response)
# 원본 frame에 다음 정보 추가:
# - 손 랜드마크 (초록 점)
# - 손 연결선 (파란 선)
# - 엄지-검지 거리 (청록 선)
# - 텍스트 (제스처, 액션)
```

#### 5. `async handle_websocket(payload)`
WebSocket을 통해 서버와 통신 (비동기):
```python
server_response = await handle_websocket(payload)
# 리턴:
# {
#     "ok": True,
#     "message": "액션 판정 완료 (open)",
#     "action": "hover"
# }
```

#### 6. `async main()`
메인 실행 루프:
```
1. 웹캠 열기
2. MediaPipe Hands 초기화
3. 프레임 읽기 루프:
   - RGB 변환
   - MediaPipe 처리
   - 페이로드 추출
   - 서버 통신 (비동기)
   - 화면 표시
4. 'q' 키 입력 확인 → 종료
5. 리소스 정리
```

### MediaPipe Hands 랜드마크

손의 21개 점:
```
손가락 끝 (5개)
    ↓
0-1-2-3
│     │ 4 (엄지)
│     │
└─────┤ 5-6-7-8 (검지)
      │
      ├─────9-10-11-12 (중지)
      │
      ├─13-14-15-16 (약지)
      │
      └─17-18-19-20 (소지)
```

### 비동기 프로그래밍

클라이언트는 비동기 방식을 사용해 프레임 처리 중에도 네트워크 대기:

```python
# 동기식 (느림): 네트워크 응답을 기다리며 프레임 중단
response = websocket.send_and_recv(payload)  # 블로킹

# 비동기식 (빠름): 다른 작업하며 네트워크 완료 기다림
response = await handle_websocket(payload)   # Non-blocking
```

---

## 📊 데이터 흐름 예시

### 예시 시나리오: 손을 펼쳤다 모았다

#### 프레임 1: 손 펼친 상태
```
클라이언트:
  ├─ 웹캠에서 프레임 읽기
  ├─ MediaPipe 처리 → 21개 랜드마크
  ├─ 엄지-검지 거리 0.08 계산
  ├─ gesture = "open" (거리 > 0.05)
  └─ 페이로드 생성

  {
    "hand_detected": true,
    "gesture": "open",
    "index_tip": {"x": 0.5, "y": 0.3},
    "thumb_tip": {"x": 0.51, "y": 0.32},
    ...
  }

  ├─ WebSocket으로 서버 전송
  │
  서버:
  ├─ Pydantic 검증 (HandPayload)
  ├─ decide_action(data) 호출
  ├─ 제스처 = "open", last_gesture = "none"
  ├─ action = "hover" (손 열린 상태)
  ├─ 응답 생성
  │
  └─ {
       "ok": true,
       "message": "액션 판정 완료 (open)",
       "action": "hover"
     }

   클라이언트:
  ├─ 응답 수신
  ├─ 프레임에 그리기
  │  - Gesture: open
  │  - Action: hover
  └─ 화면에 표시
```

#### 프레임 2-5: 손 모으는 중
```
거리: 0.08 → 0.06 → 0.04 → 0.02
gesture: open → open → pinch → pinch
action: hover → hover → grab_start → grab_hold
```

#### 프레임 6: 손 다시 펼침
```
거리: 0.02 → 0.08
gesture: pinch → open
action: drop → hover
```

---

## 🔄 통신 프로토콜

### WebSocket 메시지 형식

**클라이언트 → 서버:**
```json
{
  "hand_detected": true,
  "gesture": "open",
  "index_tip": {"x": 0.5, "y": 0.3},
  "thumb_tip": {"x": 0.51, "y": 0.32},
  "landmarks": [
    {"x": 0.0, "y": 0.0},
    {"x": 0.1, "y": 0.05},
    ...
  ],
  "timestamp": 1709000000.123
}
```

**서버 → 클라이언트:**
```json
{
  "ok": true,
  "message": "액션 판정 완료 (open)",
  "action": "hover"
}
```

---

완료! 각 파일의 역할과 상호작용을 이해했으면 코드를 수정하고 확장해볼 수 있습니다.
