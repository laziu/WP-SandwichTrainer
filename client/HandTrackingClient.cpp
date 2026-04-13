// ============================================================================
// Unreal Engine - Hand Tracking Client (C++ 구현)
// ============================================================================

#include "HandTrackingClient.h"
#include "Http.h"
#include "Kismet/GameplayStatics.h"

AHandTrackingClient::AHandTrackingClient()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.016f;  // 60fps
}

void AHandTrackingClient::BeginPlay()
{
    Super::BeginPlay();

    HttpModule = &FHttpModule::Get();

    if (ConnectionType == TEXT("WebSocket"))
    {
        ConnectWebSocket();
    }
}

void AHandTrackingClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Disconnect();
    Super::EndPlay(EndPlayReason);
}

void AHandTrackingClient::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // REST API 폴링
    if (ConnectionType == TEXT("REST"))
    {
        TimeSinceLastPoll += DeltaSeconds;

        if (TimeSinceLastPoll >= PollingInterval)
        {
            GetHandData();
            TimeSinceLastPoll = 0.0f;
        }
    }
}

void AHandTrackingClient::GetHandData()
{
    // 테스트 모드 활성화 시
    if (bUseTestMode)
    {
        GetTestData(TestAction);
        return;
    }

    if (!HttpModule)
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP 모듈 없음"));
        return;
    }

    // ========================================================================
    // 1. 요청 URL 구성
    // ========================================================================
    FString URL = FString::Printf(
        TEXT("http://%s:%d/data"),
        *ServerIP,
        ServerPort
    );

    // ========================================================================
    // 2. HTTP GET 요청 생성
    // ========================================================================
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        HttpModule->CreateRequest();

    Request->SetURL(URL);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // ========================================================================
    // 3. 응답 콜백 등록
    // ========================================================================
    Request->OnProcessRequestComplete().BindUObject(
        this,
        &AHandTrackingClient::OnHandDataReceived
    );

    // ========================================================================
    // 4. 요청 전송
    // ========================================================================
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP 요청 전송 실패"));
    }
}

void AHandTrackingClient::OnHandDataReceived(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bWasSuccessful
)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("서버 응답 실패"));
        return;
    }

    // ========================================================================
    // 1. JSON 응답 파싱
    // ========================================================================
    FString JsonString = Response->GetContentAsString();

    if (!ParseHandDataJSON(JsonString, LastHandData))
    {
        UE_LOG(LogTemp, Error, TEXT("JSON 파싱 실패: %s"), *JsonString);
        return;
    }

    // ========================================================================
    // 2. 손 감지 이벤트 발생
    // ========================================================================
    OnHandDetected.Broadcast(
        LastHandData.bHandDetected,
        LastHandData.Gesture,
        LastHandData.Action,
        LastHandData.HandPosition
    );

    // ========================================================================
    // 3. 액션 처리
    // ========================================================================
    ProcessAction(LastHandData.Action, LastHandData.HandPosition);

    // ========================================================================
    // 4. 디버그 로그
    // ========================================================================
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("손 감지: %d, 제스처: %s, 액션: %s, 신뢰도: %.2f"),
        LastHandData.bHandDetected,
        *LastHandData.Gesture,
        *LastHandData.Action,
        LastHandData.HandConfidence
    );
}

bool AHandTrackingClient::ParseHandDataJSON(
    const FString& JsonString,
    FHandData& OutData
)
{
    // JSON 파싱
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // ========================================================================
    // JSON 필드 추출
    // ========================================================================

    OutData.bHandDetected = JsonObject->GetBoolField(TEXT("hand_detected"));
    OutData.Gesture = JsonObject->GetStringField(TEXT("gesture"));
    OutData.Action = JsonObject->GetStringField(TEXT("action"));
    OutData.HandConfidence = 
        JsonObject->GetNumberField(TEXT("action_data"))? 
        JsonObject->GetObjectField(TEXT("action_data"))->
            GetNumberField(TEXT("hand_confidence")) : 0.0f;
    OutData.Timestamp = JsonObject->GetNumberField(TEXT("timestamp"));

    // ========================================================================
    // 손 위치 (정규화된 좌표)
    // ========================================================================

    if (JsonObject->HasField(TEXT("action_data")))
    {
        TSharedPtr<FJsonObject> ActionData =
            JsonObject->GetObjectField(TEXT("action_data"));

        if (ActionData->HasField(TEXT("hand_position")))
        {
            TSharedPtr<FJsonObject> HandPos =
                ActionData->GetObjectField(TEXT("hand_position"));

            OutData.HandPosition.X = HandPos->GetNumberField(TEXT("x"));
            OutData.HandPosition.Y = HandPos->GetNumberField(TEXT("y"));
        }
    }

    // ========================================================================
    // 검지 위치
    // ========================================================================

    if (JsonObject->HasField(TEXT("index_position")))
    {
        TSharedPtr<FJsonObject> IndexPos =
            JsonObject->GetObjectField(TEXT("index_position"));

        OutData.IndexPosition.X = IndexPos->GetNumberField(TEXT("x"));
        OutData.IndexPosition.Y = IndexPos->GetNumberField(TEXT("y"));
    }

    // ========================================================================
    // 엄지 위치
    // ========================================================================

    if (JsonObject->HasField(TEXT("thumb_position")))
    {
        TSharedPtr<FJsonObject> ThumbPos =
            JsonObject->GetObjectField(TEXT("thumb_position"));

        OutData.ThumbPosition.X = ThumbPos->GetNumberField(TEXT("x"));
        OutData.ThumbPosition.Y = ThumbPos->GetNumberField(TEXT("y"));
    }

    return true;
}

void AHandTrackingClient::ProcessAction(
    const FString& Action,
    const FVector2D& Position
)
{
    if (Action == TEXT("idle"))
    {
        // 대기 상태 - 손 미감지
    }
    else if (Action == TEXT("hover"))
    {
        // 손 열린 상태 - 마우스 호버 상태
        // 예: 커서 이동
    }
    else if (Action == TEXT("grab_start"))
    {
        // 집기 시작
        // 예: 객체 선택 시작
        OnActionTriggered.Broadcast(TEXT("grab_start"), Position);

        UE_LOG(LogTemp, Warning, TEXT("손으로 집기 시작: (%.2f, %.2f)"),
            Position.X, Position.Y);
    }
    else if (Action == TEXT("grab_hold"))
    {
        // 집기 유지
        // 예: 객체 드래그
    }
    else if (Action == TEXT("drop"))
    {
        // 놓기
        // 예: 객체 놓기
        OnActionTriggered.Broadcast(TEXT("drop"), Position);

        UE_LOG(LogTemp, Warning, TEXT("손으로 놓기: (%.2f, %.2f)"),
            Position.X, Position.Y);
    }
}

void AHandTrackingClient::ConnectWebSocket()
{
    UE_LOG(LogTemp, Warning, TEXT("WebSocket 연결 아직 미구현 (REST API 사용 중)"));
    // WebSocket 구현은 복잡하므로 REST API 권장
}

void AHandTrackingClient::Disconnect()
{
    // 정리 로직
    UE_LOG(LogTemp, Warning, TEXT("연결 종료"));
}

// ============================================================================
// 테스트 함수 구현
// ============================================================================

void AHandTrackingClient::GetTestData(const FString& InTestAction)
{
    /**
    테스트 엔드포인트에서 더미 손 정보를 받습니다.
    
    사용법:
        GetTestData(TEXT("grab_start"));
    
    가능한 액션:
        - idle: 손 미감지
        - hover: 손 열림
        - grab_start: 집기 시작
        - grab_hold: 집기 유지
        - drop: 놓기
    */
    
    if (!HttpModule)
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP 모듈 없음"));
        return;
    }
    
    // 테스트 URL 구성
    FString URL = FString::Printf(
        TEXT("http://%s:%d/test/data/%s"),
        *ServerIP,
        ServerPort,
        *InTestAction
    );
    
    UE_LOG(LogTemp, Warning, TEXT("테스트 요청: %s"), *URL);
    
    // HTTP GET 요청 생성
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        HttpModule->CreateRequest();
    
    Request->SetURL(URL);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // 응답 콜백 등록
    Request->OnProcessRequestComplete().BindUObject(
        this,
        &AHandTrackingClient::OnHandDataReceived
    );
    
    // 요청 전송
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("테스트 요청 전송 실패"));
    }
}

void AHandTrackingClient::SetTestMode(bool bEnabled)
{
    /**
    테스트 모드를 활성화/비활성화합니다.
    
    테스트 모드 활성화 시 GetHandData()를 호출해도 테스트 엔드포인트를 사용합니다.
    */
    
    bUseTestMode = bEnabled;
    
    if (bEnabled)
    {
        UE_LOG(LogTemp, Warning, TEXT("✓ 테스트 모드 활성화 (액션: %s)"),
            *TestAction);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("✓ 테스트 모드 비활성화"));
    }
}
