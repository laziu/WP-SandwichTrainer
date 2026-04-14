#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HandTrackingSubsystem.h"
#include "SandwichResultReporterComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SANDWICHTRAINER_API USandwichResultReporterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USandwichResultReporterComponent();

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	void StartSession(const FString& InSessionId, const FString& InRecipeId);

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	void ResetSession();

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	void RecordIngredientUsed(const FString& IngredientId);

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	void RecordIngredientPlaced(const FString& IngredientId);

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	void AddError(const FString& ErrorType, const FString& Detail);

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	void SetScore(float InScore);

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	void SetElapsedTime(float InElapsedTimeSeconds);

	UFUNCTION(BlueprintCallable, Category="Sandwich Result")
	bool SubmitResult();

	UFUNCTION(BlueprintPure, Category="Sandwich Result")
	bool IsSessionActive() const { return bSessionActive; }

	UFUNCTION(BlueprintPure, Category="Sandwich Result")
	const FSandwichResultMessage& GetCurrentResult() const { return CurrentResult; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category="Sandwich Result")
	bool bSessionActive = false;

	UPROPERTY(VisibleAnywhere, Category="Sandwich Result")
	FSandwichResultMessage CurrentResult;

	bool TryGetHandTrackingSubsystem(UHandTrackingSubsystem*& OutSubsystem) const;
	bool ContainsIngredient(const FString& IngredientId) const;
};