// ============================================================================
// Unreal Engine - Hand Tracking Client (C++)
// ============================================================================
// 
// 역할: 서버에서 받은 JSON 손 정보를 파싱하고 게임 로직 구현
// 
// 설정:
// 1. .Build.cs에 다음 추가:
//    PublicDependencyModuleNames.AddRange(new string[] { 
//        "Core", "CoreUObject", "Engine", "InputCore",
//        "HTTP", "Json", "JsonUtilities"
//    });
//
// 2. 액터에 이 클래스를 사용
//
// 사용 방법:
// 1. 서버 IP 주소 설정 (예: 192.168.0.100)
// 2. GetHandData() 또는 ConnectWebSocket() 호출
// 3. 손 정보에 따라 게임 로직 실행
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "Json.h"
#include "Interfaces/IHttpRequest.h"
#include "HandTrackingClient.generated.h"

// ============================================================================
// 손 제스처 이벤트 델리게이트
// ============================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_Four(
    FHandDetectionDelegate,
    bool, bHandDetected,
    FString, Gesture,
    FString, Action,
    FVector2D, HandPosition
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_Two(
    FActionEventDelegate,
    FString, Action,
    FVector2D, Position
);

// ============================================================================
// 손 정보 데이터 구조
// ============================================================================
USTRUCT(BlueprintType)
struct FHandData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bHandDetected = false;

    UPROPERTY(BlueprintReadOnly)
    FString Gesture = FString(TEXT("none"));

    UPROPERTY(BlueprintReadOnly)
    FString Action = FString(TEXT("idle"));

    UPROPERTY(BlueprintReadOnly)
    FVector2D HandPosition = FVector2D(0.5f, 0.5f);

    UPROPERTY(BlueprintReadOnly)
    FVector2D IndexPosition = FVector2D(0.5f, 0.5f);

    UPROPERTY(BlueprintReadOnly)
    FVector2D ThumbPosition = FVector2D(0.5f, 0.5f);

    UPROPERTY(BlueprintReadOnly)
    float HandConfidence = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    double Timestamp = 0.0;
};

// ============================================================================
// 고정 이 클래스
// ============================================================================
UCLASS()
class YOURPROJECT_API AHandTrackingClient : public AActor
{
    GENERATED_BODY()

public:
    AHandTrackingClient();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    // ========================================================================
    // 설정
    // ========================================================================
    
    /** 서버 IP 주소 (예: "192.168.0.100") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Tracking")
    FString ServerIP = FString(TEXT("127.0.0.1"));

    /** 서버 포트 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Tracking")
    int32 ServerPort = 8888;

    /** 사용할 연결 방식: "REST" 또는 "WebSocket" */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Tracking")
    FString ConnectionType = FString(TEXT("REST"));

    /** REST API 폴링 간격 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Tracking")
    float PollingInterval = 0.033f;

    /** 테스트 모드 활성화 (실제 카메라 대신 테스트 데이터 사용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Tracking|Test")
    bool bUseTestMode = false;

    /** 테스트 액션 (idle, hover, grab_start, grab_hold, drop) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Tracking|Test")
    FString TestAction = FString(TEXT("idle"));

    // ========================================================================
    // 공개 함수
    // ========================================================================

    /** REST API로 손 정보 요청 (주기적으로 호출) */
    UFUNCTION(BlueprintCallable, Category = "Hand Tracking")
    void GetHandData();

    /** 테스트 액션 요청 (서버의 테스트 엔드포인트 사용) */
    UFUNCTION(BlueprintCallable, Category = "Hand Tracking|Test")
    void GetTestData(const FString& InTestAction);

    /** 테스트 모드 토글 */
    UFUNCTION(BlueprintCallable, Category = "Hand Tracking|Test")
    void SetTestMode(bool bEnabled);

    /** WebSocket으로 서버 연결 */
    UFUNCTION(BlueprintCallable, Category = "Hand Tracking")
    void ConnectWebSocket();

    /** 연결 종료 */
    UFUNCTION(BlueprintCallable, Category = "Hand Tracking")
    void Disconnect();

    // ========================================================================
    // 델리게이트 (이벤트)
    // ========================================================================

    /** 손 감지 상태 변경 (손 감지됨, 제스처, 액션, 위치) */
    UPROPERTY(BlueprintAssignable, Category = "Hand Tracking")
    FHandDetectionDelegate OnHandDetected;

    /** 액션 실행 (액션, 위치) */
    UPROPERTY(BlueprintAssignable, Category = "Hand Tracking")
    FActionEventDelegate OnActionTriggered;

    // ========================================================================
    // 참고 데이터
    // ========================================================================

    /** 최종 손 정보 */
    UPROPERTY(BlueprintReadOnly, Category = "Hand Tracking")
    FHandData LastHandData;

private:
    // ========================================================================
    // 비공개 함수
    // ========================================================================

    /** HTTP 응답 콜백 */
    void OnHandDataReceived(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bWasSuccessful
    );

    /** JSON 파싱 */
    bool ParseHandDataJSON(const FString& JsonString, FHandData& OutData);

    /** 액션 처리 */
    void ProcessAction(const FString& Action, const FVector2D& Position);

    // ========================================================================
    // 비공개 멤버 변수
    // ========================================================================

    FHttpModule* HttpModule;
    float TimeSinceLastPoll;
};
