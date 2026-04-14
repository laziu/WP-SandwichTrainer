#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandTrackingSubsystem.h"
#include "SandwichFeedbackWidget.h"
#include "SandwichResultReporterComponent.h"
#include "SandwichDebugTestActor.generated.h"

UCLASS()
class SANDWICHTRAINER_API ASandwichDebugTestActor : public AActor
{
	GENERATED_BODY()
	
public:
	ASandwichDebugTestActor();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sandwich Test")
	TObjectPtr<USandwichResultReporterComponent> ResultReporterComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sandwich Test|UI")
	TSubclassOf<USandwichFeedbackWidget> FeedbackWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="Sandwich Test|UI")
	TObjectPtr<USandwichFeedbackWidget> FeedbackWidget;

	UFUNCTION(BlueprintCallable, Category="Sandwich Test")
	void SendTestResult();

	UFUNCTION(BlueprintCallable, Category="Sandwich Test")
	void ShowTestFeedback();

	UFUNCTION()
	void HandleFeedbackReceived(const FFeedbackMessage& FeedbackMessage);

private:
	UHandTrackingSubsystem* GetHandTrackingSubsystem() const;
	void CreateFeedbackWidget();
};