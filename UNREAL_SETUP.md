# Unreal Engine - 손 추적 클라이언트 설정 가이드

## 📋 목차
1. [C++ 설정](#c-설정)
2. [블루프린트 설정](#블루프린트-설정)
3. [Python 테스트](#python-테스트)
4. [JSON 응답 형식](#json-응답-형식)
5. [액션 처리](#액션-처리)

---

## C++ 설정

### 1. 프로젝트 파일 설정

#### .Build.cs 수정
```csharp
// YourProject.Build.cs
public class YourProject : ModuleRules
{
    public YourProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore",
            "HTTP",           // ← 추가
            "Json",           // ← 추가
            "JsonUtilities"   // ← 추가
        });
        
        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
```

### 2. 헤더/소스 파일 복사

- [HandTrackingClient.h](HandTrackingClient.h) → `Source/YourProject/Public/`
- [HandTrackingClient.cpp](HandTrackingClient.cpp) → `Source/YourProject/Private/`

### 3. 액터에 컴포넌트 추가 (옵션)

```cpp
// YourCharacter.h
#include "HandTrackingClient.h"

class AYourCharacter : public ACharacter
{
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    AHandTrackingClient* HandTracker;
};

// YourCharacter.cpp 생성자
AYourCharacter::AYourCharacter()
{
    HandTracker = GetWorld()->SpawnActor<AHandTrackingClient>();
}
```

---

## 블루프린트 설정

### 1. 액터 배치

**월드에 AHandTrackingClient 배치:**

1. Place Actor 패널에서 "HandTrackingClient" 검색
2. 월드에 드래그 앤 드롭

### 2. 세부 정보 설정

Details 패널에서:

| 속성 | 값 | 설명 |
|------|-----|------|
| Server IP | `192.168.x.x` | 서버 PC의 IP 주소 |
| Server Port | `8888` | 서버 포트 |
| Connection Type | `REST` | 연결 방식 (REST 또는 WebSocket) |
| Polling Interval | `0.033` | 폴링 간격 (초) |

### 3. 이벤트 바인딩

#### On Hand Detected 이벤트
```
손 감지 상태 변경 시 자동으로 호출됨
입력 매개변수:
  - bHandDetected (Boolean): 손이 감지되었는가
  - Gesture (String): 제스처 ("open", "pinch", "none")
  - Action (String): 액션 ("idle", "hover", "grab_start", "grab_hold", "drop")
  - HandPosition (Vector2D): 손의 정규화된 위치 (0.0~1.0)
```

**블루프린트 예시:**
```
Event On Hand Detected (Custom Event)
  ├─ Do Something
  │  ├─ If (Action == "grab_start")
  │  │  └─ Play Sound
  │  ├─ If (Action == "hover")
  │  │  └─ Show UI
  │  └─ If (Action == "drop")
  │     └─ End Action
```

#### On Action Triggered 이벤트
```
액션 실행 시 호출됨
입력 매개변수:
  - Action (String): 실행할 액션
  - Position (Vector2D): 손의 위치
```

### 4. 블루프린트 노드 사용

#### Get Hand Data (REST API)
```
MyHandTracker → Get Hand Data
```

#### Connect WebSocket
```
MyHandTracker → Connect WebSocket
```

#### Disconnect
```
MyHandTracker → Disconnect
```

---

## Python 테스트

### REST API 테스트

```python
import requests
import json

SERVER_URL = "http://192.168.0.100:8888"

response = requests.get(f"{SERVER_URL}/data")
data = response.json()

print(json.dumps(data, indent=2))
```

### WebSocket 테스트

```python
import asyncio
import json
import websockets

async def test():
    async with websockets.connect("ws://192.168.0.100:8888/ws/hand/stream") as ws:
        async for message in ws:
            data = json.loads(message)
            print(json.dumps(data, indent=2))

asyncio.run(test())
```

---

## JSON 응답 형식

### 손 감지됨 (grab_start)

```json
{
  "ok": true,
  "hand_detected": true,
  "action": "grab_start",
  "action_data": {
    "hand_position": {
      "x": 0.5,
      "y": 0.4
    },
    "hand_confidence": 0.95,
    "gesture": "pinch",
    "is_left_hand": false
  },
  "gesture": "pinch",
  "index_position": {
    "x": 0.51,
    "y": 0.41
  },
  "thumb_position": {
    "x": 0.5,
    "y": 0.42
  },
  "timestamp": 1704067200.123,
  "message": null
}
```

### 손 미감지 (idle)

```json
{
  "ok": true,
  "hand_detected": false,
  "action": "idle",
  "action_data": {
    "hand_position": null,
    "hand_confidence": 0.0,
    "gesture": "none",
    "is_left_hand": false
  },
  "gesture": "none",
  "index_position": null,
  "thumb_position": null,
  "timestamp": 1704067200.123,
  "message": null
}
```

### JSON 필드 설명

| 필드 | 타입 | 범위 | 설명 |
|------|------|------|------|
| `ok` | bool | - | 요청 성공 여부 |
| `hand_detected` | bool | - | 손이 감지되었는가 |
| `action` | string | idle, hover, grab_start, grab_hold, drop | 실행할 액션 |
| `gesture` | string | open, pinch, none | 현재 제스처 |
| `hand_position.x` | float | 0.0~1.0 | 손 중심 X 위치 (정규화) |
| `hand_position.y` | float | 0.0~1.0 | 손 중심 Y 위치 (정규화) |
| `hand_confidence` | float | 0.0~1.0 | 손 감지 신뢰도 |
| `index_position` | object | - | 검지 끝 좌표 (정규화) |
| `thumb_position` | object | - | 엄지 끝 좌표 (정규화) |
| `timestamp` | float | - | Unix 타임스탐프 |

---

## 액션 처리

### 액션별 구현 예시

#### 1. Idle (손 미감지)
```cpp
// C++
if (LastHandData.Action == TEXT("idle"))
{
    // 손 미감지 - 아무것도 하지 않음
    HideCursor();
}

// 블루프린트
If (Action == "idle")
  └─ Hide Widget
```

#### 2. Hover (손 열린 상태)
```cpp
// C++
if (LastHandData.Action == TEXT("hover"))
{
    // 손 위치를 커서로 표시
    FVector2D ScreenPosition = LastHandData.HandPosition * GetViewportSize();
    UpdateCursorPosition(ScreenPosition);
}

// 블루프린트
If (Action == "hover")
  ├─ Convert Normalized to Pixel
  └─ Update Cursor
```

#### 3. Grab Start (집기 시작)
```cpp
// C++
if (LastHandData.Action == TEXT("grab_start"))
{
    // 객체 선택 시작
    AActor* SelectedActor = GetActorAtPosition(LastHandData.HandPosition);
    BeginGrabbing(SelectedActor);
}

// 블루프린트
If (Action == "grab_start")
  ├─ Line Trace by Channel
  ├─ Get Hit Actor
  └─ Begin Physics Handle
```

#### 4. Grab Hold (집기 유지)
```cpp
// C++
if (LastHandData.Action == TEXT("grab_hold"))
{
    // 손 위치에 따라 객체 이동
    UpdateGrabbedActorPosition(LastHandData.HandPosition);
}

// 블루프린트
If (Action == "grab_hold")
  ├─ Get Grabbed Actor
  └─ Update Physics Handle Target
```

#### 5. Drop (놓기)
```cpp
// C++
if (LastHandData.Action == TEXT("drop"))
{
    // 객체 놓기
    EndGrabbing();
}

// 블루프린트
If (Action == "drop")
  ├─ Release Physics Handle
  └─ Add Physics Impulse
```

---

## 트러블슈팅

### 1. "액터를 찾을 수 없음" 오류

**해결방법:**
1. Unreal 프로젝트 재빌드
```bash
cd YourProject
./Binaries/Win64/UE4Editor.exe YourProject.uproject -game
```

2. Visual Studio 프로젝트 재생성
```bash
右클릭 .uproject → Generate Visual Studio Project Files
```

### 2. HTTP 요청 실패

**확인 사항:**
- [ ] 서버가 실행 중인가 (python main.py)
- [ ] 서버 IP 주소가 정확한가 (ipconfig)
- [ ] 포트 8888이 열려있는가 (netstat -an | grep 8888)
- [ ] 방화벽이 차단하는가

### 3. 손 정보가 업데이트되지 않음

**확인 사항:**
- [ ] Tick 활성화되었나 (PrimaryActorTick.bCanEverTick = true)
- [ ] GetHandData() 호출 중인가
- [ ] 폴링 간격이 너무 길지 않은가 (기본값: 0.033초)

### 4. JSON 파싱 실패

**로그 확인:**
```cpp
if (!ParseHandDataJSON(JsonString, LastHandData))
{
    UE_LOG(LogTemp, Error, TEXT("JSON: %s"), *JsonString);
}
```

서버에서 받은 JSON 형식이 올바른지 확인

---

## 성능 팁

### 1. 폴링 간격 최적화

```cpp
// 빠른 반응 필요 (게임플레이)
PollingInterval = 0.016f;  // ~60fps

// 일반적인 상황
PollingInterval = 0.033f;  // ~30fps

// 낮은 사양
PollingInterval = 0.066f;  // ~15fps
```

### 2. 이벤트 최소화

```cpp
// 모든 프레임마다 이벤트 발생 ❌
OnHandDetected.Broadcast(...);

// 액션 변경 시만 이벤트 발생 ✓
if (LastHandData.Action != PreviousAction)
{
    OnHandDetected.Broadcast(...);
    PreviousAction = LastHandData.Action;
}
```

### 3. 네트워크 최적화

- 서버와 클라이언트를 같은 네트워크에 연결
- 유선 연결 사용 (WiFi보다 안정적)
- 방화벽 최소화

---

## 문서 참고

- [Unreal HTTP API 문서](https://docs.unrealengine.com/5.0/en-US/internet-connection-manager-in-unreal-engine/)
- [Unreal JSON 문서](https://docs.unrealengine.com/5.0/en-US/JSON-in-unreal-engine/)
- [서버 API 문档](../SERVER_SETUP.md)
