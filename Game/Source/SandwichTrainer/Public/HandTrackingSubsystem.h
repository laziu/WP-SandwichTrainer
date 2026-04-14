#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IWebSocket.h"
#include "HandTrackingSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FHandData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Sign;

	UPROPERTY(BlueprintReadOnly)
	float X = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Y = 0.f;
};

USTRUCT(BlueprintType)
struct FSandwichError
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Detail;
};

USTRUCT(BlueprintType)
struct FSandwichResultMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type = TEXT("sandwich_result");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RecipeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Score = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> IngredientsUsed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> IngredientsOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSandwichError> Errors;
};

USTRUCT(BlueprintType)
struct FFeedbackMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Type;

	UPROPERTY(BlueprintReadOnly)
	FString SessionId;

	UPROPERTY(BlueprintReadOnly)
	FString FeedbackText;

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Tips;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHandData, const FHandData&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFeedbackReceived, const FFeedbackMessage&, FeedbackMessage);

UCLASS()
class SANDWICHTRAINER_API UHandTrackingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void Connect();

	UFUNCTION(BlueprintCallable)
	void Disconnect();

	UFUNCTION(BlueprintCallable)
	bool SendSandwichResult(const FSandwichResultMessage& Message);

	UFUNCTION(BlueprintPure)
	bool IsHandTrackingConnected() const;

	UFUNCTION(BlueprintPure)
	bool IsFeedbackConnected() const;

	FOnHandData OnHandDataReceived;

	UPROPERTY(BlueprintAssignable)
	FOnFeedbackReceived OnFeedbackReceived;

private:
	TSharedPtr<IWebSocket> HandTrackingWebSocket;
	TSharedPtr<IWebSocket> FeedbackWebSocket;

	void ConnectHandTrackingSocket();
	void ConnectFeedbackSocket();

	void HandleHandTrackingMessage(const FString& Message);
	void HandleFeedbackMessageRaw(const FString& Message);

	FHandData ParseHandMessage(const TSharedPtr<FJsonObject>& JsonObject) const;
	bool ParseFeedbackMessage(const TSharedPtr<FJsonObject>& JsonObject, FFeedbackMessage& OutMessage) const;

	FString BuildSandwichResultJson(const FSandwichResultMessage& Message) const;
};