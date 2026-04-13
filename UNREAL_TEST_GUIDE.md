# Unreal Engine 테스트 모드 사용 가이드

## 🧪 테스트 모드 활성화

Unreal 클라이언트에서 실제 카메라 없이 테스트 데이터를 받을 수 있습니다.

### **블루프린트 설정**

1. **액터 배치:**
   - 월드에 `AHandTrackingClient` 액터 배치

2. **Details 패널 설정:**

| 속성 | 값 | 설명 |
|------|-----|------|
| Server IP | `127.0.0.1` (또는 서버 IP) | 서버 주소 |
| Server Port | `8888` | 서버 포트 |
| Connection Type | `REST` | 연결 방식 |
| **Use Test Mode** | ✓ 체크 | **테스트 모드 활성화** |
| **Test Action** | `idle` | 테스트 액션 |

3. **테스트 액션 옵션:**
   - `idle` - 손 미감지
   - `hover` - 손 열림
   - `grab_start` - 집기 시작
   - `grab_hold` - 집기 유지
   - `drop` - 놓기

---

## 🎮 블루프린트 테스트

### **방법 1: 단일 테스트 (한 번만)**

```
Event BeginPlay
├─ Hand Tracking Client
└─ Get Test Data (Input: "grab_start")
```

**효과:**
- 서버의 `/test/data/grab_start` 엔드포인트에서 한 번 JSON을 받음
- `On Hand Detected` 이벤트 발생

### **방법 2: 연속 테스트 (자동 폴링)**

```
Event BeginPlay
├─ Set Test Mode (Input: true)
├─ Test Action = "grab_start"
└─ [나머지 로직...]
```

**효과:**
- Tick마다 테스트 데이터 자동 수신
- `/test/data/{TestAction}` 엔드포인트 자동 호출
- `On Hand Detected` 이벤트 계속 발생

---

## 📊 테스트 흐름

### **시나리오 1: grab_start 테스트**

블루프린트:
```
Event BeginPlay
├─ Print String ("테스트 시작")
├─ Get Test Data ("grab_start")
└─ Wait (0.5 second)

Event On Hand Detected
├─ If (Action == "grab_start")
│  ├─ Print String ("손으로 집기 시작!")
│  └─ Play Sound (GrabSound)
└─ ...
```

예상 결과:
1. 플레이 시작
2. "테스트 시작" 출력
3. 서버에서 JSON 수신
4. `On Hand Detected` 이벤트 발생
5. "손으로 집기 시작!" 출력
6. 사운드 재생

### **시나리오 2: 연속 테스트 루프**

블루프린트:
```
Event BeginPlay
├─ Set Test Mode (true)
├─ Test Action = "idle"
└─ Print String ("테스트 모드 활성화")

Event On Hand Detected
├─ Print String (Action) // idle, hover, grab_start, ...
└─ Update UI
```

효과:
- 매 프레임마다 테스트 데이터 수신
- 손 상태 자동 업데이트
- 게임 로직 테스트 가능

---

## 🔄 테스트 액션 변경

블루프린트에서 액션 변경:

```
Event Input // 키 입력 시
├─ If (Pressed Key == "1")
│  └─ Test Action = "idle"
├─ If (Pressed Key == "2")
│  └─ Test Action = "hover"
├─ If (Pressed Key == "3")
│  └─ Test Action = "grab_start"
├─ If (Pressed Key == "4")
│  └─ Test Action = "grab_hold"
└─ If (Pressed Key == "5")
   └─ Test Action = "drop"
```

**사용법:**
- 1키 → idle (손 미감지)
- 2키 → hover (손 열림)
- 3키 → grab_start (집기 시작)
- 4키 → grab_hold (집기 유지)
- 5키 → drop (놓기)

---

## C++ 테스트

### **방법 1: 한 번만 테스트**

```cpp
void AMyCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    AHandTrackingClient* HandTracker = 
        GetWorld()->SpawnActor<AHandTrackingClient>();
    
    // 테스트 데이터 요청
    HandTracker->GetTestData(TEXT("grab_start"));
}
```

### **방법 2: 테스트 모드 활성화**

```cpp
void AMyCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    AHandTrackingClient* HandTracker = 
        GetWorld()->SpawnActor<AHandTrackingClient>();
    
    // 테스트 모드 활성화
    HandTracker->SetTestMode(true);
    HandTracker->TestAction = TEXT("hover");
}
```

### **방법 3: 키 입력으로 테스트**

```cpp
void AMyCharacter::SetupPlayerInputComponent(class UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    
    Input->BindAction("Test1", IE_Pressed, this, &AMyCharacter::TestIdle);
    Input->BindAction("Test2", IE_Pressed, this, &AMyCharacter::TestHover);
    Input->BindAction("Test3", IE_Pressed, this, &AMyCharacter::TestGrabStart);
}

void AMyCharacter::TestGrabStart()
{
    if (HandTracker)
    {
        HandTracker->GetTestData(TEXT("grab_start"));
    }
}
```

---

## ✅ 테스트 체크리스트

- [ ] 서버 실행 중 (python main.py)
- [ ] Unreal 에디터 플레이 중
- [ ] `Use Test Mode` 체크됨 또는 `SetTestMode(true)` 호출
- [ ] `Test Action` 설정 또는 `GetTestData()` 호출
- [ ] `On Hand Detected` 이벤트 바인딩됨
- [ ] 출력 로그 확인

---

## 📋 테스트 결과 예시

### **로그 출력**

```
LogTemp Warning: ✓ 테스트 요청: http://127.0.0.1:8888/test/data/grab_start
LogTemp Warning: Frame #30: hand=True, gesture=pinch, action=grab_start, 신뢰도: 0.90
LogTemp Warning: 손으로 집기 시작: (0.52, 0.41)
```

### **JSON 응답**

```json
{
  "ok": true,
  "hand_detected": true,
  "action": "grab_start",
  "gesture": "pinch",
  "action_data": {
    "hand_position": {"x": 0.5, "y": 0.4},
    "hand_confidence": 0.9,
    "gesture": "pinch",
    "is_left_hand": false
  },
  "index_position": {"x": 0.52, "y": 0.41},
  "thumb_position": {"x": 0.50, "y": 0.42},
  "timestamp": 1712973216.123
}
```

---

## 🔧 트러블슈팅

### 문제: 테스트 데이터가 안 받아와짐

**확인:**
1. 서버 실행 중인가? `python server/main.py`
2. 포트 8888 맞나?
3. 방화벽 차단 아닌가?

**해결:**
```bash
# 서버 테스트
curl http://127.0.0.1:8888/test/data/grab_start
```

### 문제: 이벤트가 발생하지 않음

**확인:**
1. `On Hand Detected` 이벤트 바인딩됨?
2. 이벤트 바디에 로직 있나?

**해결:**
```cpp
// 로그로 확인
UE_LOG(LogTemp, Warning, TEXT("이벤트 발생: %s"), *Action);
```

### 문제: 테스트 모드 활성화 안 됨

**확인:**
1. `bUseTestMode = true` 설정됨?
2. `GetHandData()` 호출됨?

**해결:**
```cpp
// C++에서 명시적 설정
HandTracker->bUseTestMode = true;
HandTracker->TestAction = TEXT("grab_start");
```

---

## 🎯 다음 단계

테스트 완료 후:

1. **테스트 모드 비활성화:**
   ```cpp
   HandTracker->SetTestMode(false);
   ```

2. **실제 카메라로 전환:**
   - 서버에서 카메라 활성화 확인
   - 클라이언트 테스트 모드 비활성화
   - 손 동작 인식 테스트

3. **게임 로직 통합:**
   - 각 액션별 게임 이벤트 연결
   - UI 업데이트
   - 오디오 효과 추가
