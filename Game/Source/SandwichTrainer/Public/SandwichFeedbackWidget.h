#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HandTrackingSubsystem.h"
#include "SandwichFeedbackWidget.generated.h"

UCLASS()
class SANDWICHTRAINER_API USandwichFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Sandwich Feedback")
	void ApplyFeedbackMessage(const FFeedbackMessage& FeedbackMessage);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Sandwich Feedback")
	void ShowFeedback(const FString& FeedbackText, const TArray<FString>& Tips);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Sandwich Feedback")
	void HideFeedback();

	UFUNCTION(BlueprintPure, Category="Sandwich Feedback")
	const FString& GetLastSessionId() const { return LastSessionId; }

	UFUNCTION(BlueprintPure, Category="Sandwich Feedback")
	const FString& GetLastFeedbackText() const { return LastFeedbackText; }

	UFUNCTION(BlueprintPure, Category="Sandwich Feedback")
	const TArray<FString>& GetLastTips() const { return LastTips; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="Sandwich Feedback")
	FString LastSessionId;

	UPROPERTY(BlueprintReadOnly, Category="Sandwich Feedback")
	FString LastFeedbackText;

	UPROPERTY(BlueprintReadOnly, Category="Sandwich Feedback")
	TArray<FString> LastTips;
};