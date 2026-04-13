#include "HandTrackingSubsystem.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Json.h"

void UHandTrackingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // WebSocket 모듈 로드
    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    Connect();
}

void UHandTrackingSubsystem::Deinitialize()
{
    Disconnect();
    Super::Deinitialize();
}

void UHandTrackingSubsystem::Connect()
{
    FString ServerURL = TEXT("ws://172.16.30.66:8888/ws/hand"); // MediaPipe 서버 주소

    WebSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL);

    WebSocket->OnConnected().AddLambda([]()
    {
        UE_LOG(LogTemp, Log, TEXT("WebSocket Connected"));
    });

    WebSocket->OnConnectionError().AddLambda([](const FString& Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Connection Error: %s"), *Error);
    });

    WebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean)
    {
        UE_LOG(LogTemp, Warning, TEXT("WebSocket Closed: %s"), *Reason);
    });

    WebSocket->OnMessage().AddUObject(this, &UHandTrackingSubsystem::HandleMessage);

    WebSocket->Connect();
}

void UHandTrackingSubsystem::Disconnect()
{
    if (WebSocket.IsValid())
    {
        WebSocket->Close();
        WebSocket = nullptr;
    }
}

void UHandTrackingSubsystem::HandleMessage(const FString& Message)
{
    // 메시지 파싱
    FHandData Data = ParseMessage(Message);

    // 브로드캐스트
    OnHandDataReceived.Broadcast(Data);
}

FHandData UHandTrackingSubsystem::ParseMessage(const FString& Message)
{
    FHandData Result;

    // JSON 파싱 가정
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        Result.Sign = JsonObject->GetStringField("Sign");
        Result.X = JsonObject->GetNumberField("X");
        Result.Y = JsonObject->GetNumberField("Y");
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to parse message: %s"), *Message);
    }

    return Result;
}