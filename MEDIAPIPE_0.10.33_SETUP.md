# MediaPipe 0.10.33 설치 및 설정 가이드

## 개요
코드가 **mediapipe 0.10.33** 버전에 맞게 업데이트되었습니다.
이 버전은 새로운 **MediaPipe Tasks API**를 사용합니다.

## 1. 프로젝트 재설치

### Python 환경 정리 및 재설치

```bash
cd C:\potenup3\WP-SandwichTrainer

# 기존 환경 제거 (선택사항)
deactivate
rmdir /s /q .venv

# 새로운 가상환경 생성
python -m venv .venv

# 가상환경 활성화
.venv\Scripts\activate

# 의존성 설치
pip install -r requirements.txt
```

### requirements.txt 확인
- mediapipe==0.10.33 (최신 Tasks API가 포함됨)

## 2. MediaPipe 모델 파일 다운로드

### 필수: hand_landmarker.task 모델 파일

mediapipe 0.10.33은 `.task` 모델 파일을 필요로 합니다.

**다운로드 방법:**

```bash
# models 디렉토리 생성
mkdir models

# 모델 파일 다운로드 (Google MediaPipe 공식 모델)
# URL: https://storage.googleapis.com/mediapipe-models/hand_landmarker.task

# PowerShell 사용 (Windows)
$url = "https://storage.googleapis.com/mediapipe-models/hand_landmarker.task"
$output = "models\hand_landmarker.task"
Invoke-WebRequest -Uri $url -OutFile $output

# 또는 curl 사용
curl -L "https://storage.googleapis.com/mediapipe-models/hand_landmarker.task" -o models\hand_landmarker.task
```

**디렉토리 구조:**
```
WP-SandwichTrainer/
├── models/
│   └── hand_landmarker.task          # ← 이 파일이 필요
├── server/
│   ├── main.py
│   └── app/
│       ├── camera_processor.py
│       └── ...
├── client/
│   ├── mediapipe_client.py
│   └── ...
└── requirements.txt
```

## 3. 주요 변경사항

### server/app/camera_processor.py
- ❌ 제거: `mp.solutions.hands` (구형 API)
- ✅ 추가: `HandLandmarker` (Tasks API)
- 사용: `vision.HandLandmarker.create_from_options(options)`

### client/mediapipe_client.py
- ❌ 제거: `mp_hands = mp.solutions.hands`
- ✅ 추가: `from mediapipe.tasks import python`
- 사용: Tasks API의 `HandLandmarker`

## 4. 실행 방법

### 서버 실행
```bash
cd server
uvicorn main:app --reload --port 8888
```

### 클라이언트 실행 (동일 PC)
```bash
cd client
python mediapipe_client.py
```

## 5. 성공 신호

**서버 로그에서 확인:**
```
✓ 카메라 0 초기화됨
✓ MediaPipe HandLandmarker (v0.10.33) 초기화됨
✓ CameraProcessor 시작됨
```

**클라이언트 로그에서 확인:**
```
MediaPipe 0.10.33 Tasks API 사용 중
✓ MediaPipe HandLandmarker (v0.10.33) 초기화 완료
```

## 6. 모델 파일이 없을 경우

- **경고**: "모델 파일이 없습니다"
- **대체**: 더미 모드로 실행 (손 인식 미작동)
- **동작**: 서버는 계속 실행되지만 항상 "hand_detected: false" 반환

## 7. 문제 해결

### 오류: "module 'mediapipe' has no attribute 'solutions'"
```
해결: requirements.txt 재확인
pip uninstall mediapipe -y
pip install mediapipe==0.10.33
```

### 오류: "Model not found at path/to/hand_landmarker.task"
```
해결: models/hand_landmarker.task 파일 존재 확인
ls models/
```

### 오류: "protobuf" 관련 에러
```
해결: 프로토버프 재설치
pip install --upgrade protobuf
```

## 8. mediapipe 0.10.33의 주요 특징

| 특징 | 0.9.x (구) | 0.10.x (신) |
|------|----------|-----------|
| API 방식 | `mp.solutions.hands` | `vision.HandLandmarker` |
| 모델 파일 | 자동 다운로드 | `.task` 파일 필요 |
| 초기화 | `Hands()` 컨텍스트 | `create_from_options()` |
| 처리 | `hands.process(frame)` | `landmarker.detect(image)` |
| 성능 | 기본 | ⬆️ 개선됨 |

## 9. 추가 정보

- 공식 문서: https://developers.google.com/mediapipe/solutions/vision/hand_landmarker/python
- 모델 다운로드: https://storage.googleapis.com/mediapipe-models/

---

**최종 확인 날짜**: 2026-04-13
**버전**: mediapipe==0.10.33
**상태**: ✅ Tasks API 완전 전환
