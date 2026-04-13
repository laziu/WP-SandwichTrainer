#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IWebSocket.h"
#include "HandTrackingSubsystem.generated.h"

USTRUCT()
struct FHandData
{
	GENERATED_BODY()

	FString Sign;
	float X;
	float Y;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHandData, const FHandData&);

UCLASS()
class SANDWICHTRAINER_API UHandTrackingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void Connect();
	void Disconnect();

	FOnHandData OnHandDataReceived;

private:
	TSharedPtr<IWebSocket> WebSocket;

	void HandleMessage(const FString& Message);

	FHandData ParseMessage(const FString& Message);
};
