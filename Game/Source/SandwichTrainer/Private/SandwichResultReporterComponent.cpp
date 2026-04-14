#include "SandwichResultReporterComponent.h"

#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"

USandwichResultReporterComponent::USandwichResultReporterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USandwichResultReporterComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USandwichResultReporterComponent::StartSession(const FString& InSessionId, const FString& InRecipeId)
{
	ResetSession();

	bSessionActive = true;
	CurrentResult.Type = TEXT("sandwich_result");
	CurrentResult.SessionId = InSessionId;
	CurrentResult.RecipeId = InRecipeId;

	UE_LOG(LogTemp, Log, TEXT("Sandwich session started. SessionId=%s, RecipeId=%s"),
		*CurrentResult.SessionId,
		*CurrentResult.RecipeId);
}

void USandwichResultReporterComponent::ResetSession()
{
	bSessionActive = false;
	CurrentResult = FSandwichResultMessage();
	CurrentResult.Type = TEXT("sandwich_result");

	UE_LOG(LogTemp, Log, TEXT("Sandwich session reset."));
}

void USandwichResultReporterComponent::RecordIngredientUsed(const FString& IngredientId)
{
	if (!bSessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("RecordIngredientUsed called without active session."));
		return;
	}

	if (IngredientId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("RecordIngredientUsed called with empty IngredientId."));
		return;
	}

	if (!ContainsIngredient(IngredientId))
	{
		CurrentResult.IngredientsUsed.Add(IngredientId);
		UE_LOG(LogTemp, Log, TEXT("Ingredient used recorded: %s"), *IngredientId);
	}
}

void USandwichResultReporterComponent::RecordIngredientPlaced(const FString& IngredientId)
{
	if (!bSessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("RecordIngredientPlaced called without active session."));
		return;
	}

	if (IngredientId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("RecordIngredientPlaced called with empty IngredientId."));
		return;
	}

	CurrentResult.IngredientsOrder.Add(IngredientId);
	UE_LOG(LogTemp, Log, TEXT("Ingredient placed recorded: %s"), *IngredientId);
}

void USandwichResultReporterComponent::AddError(const FString& ErrorType, const FString& Detail)
{
	if (!bSessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddError called without active session."));
		return;
	}

	if (ErrorType.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AddError called with empty ErrorType."));
		return;
	}

	FSandwichError NewError;
	NewError.Type = ErrorType;
	NewError.Detail = Detail;

	CurrentResult.Errors.Add(NewError);

	UE_LOG(LogTemp, Warning, TEXT("Sandwich error recorded. Type=%s, Detail=%s"),
		*ErrorType,
		*Detail);
}

void USandwichResultReporterComponent::SetScore(float InScore)
{
	if (!bSessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetScore called without active session."));
		return;
	}

	CurrentResult.Score = InScore;
}

void USandwichResultReporterComponent::SetElapsedTime(float InElapsedTimeSeconds)
{
	if (!bSessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetElapsedTime called without active session."));
		return;
	}

	CurrentResult.TimeSeconds = InElapsedTimeSeconds;
}

bool USandwichResultReporterComponent::SubmitResult()
{
	if (!bSessionActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitResult called without active session."));
		return false;
	}

	if (CurrentResult.SessionId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitResult failed: SessionId is empty."));
		return false;
	}

	if (CurrentResult.RecipeId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitResult failed: RecipeId is empty."));
		return false;
	}

	UHandTrackingSubsystem* HandTrackingSubsystem = nullptr;
	if (!TryGetHandTrackingSubsystem(HandTrackingSubsystem) || HandTrackingSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitResult failed: HandTrackingSubsystem not found."));
		return false;
	}

	const bool bSent = HandTrackingSubsystem->SendSandwichResult(CurrentResult);
	if (bSent)
	{
		UE_LOG(LogTemp, Log, TEXT("Sandwich result submitted successfully. SessionId=%s"), *CurrentResult.SessionId);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Sandwich result submission failed. SessionId=%s"), *CurrentResult.SessionId);
	}

	return bSent;
}

bool USandwichResultReporterComponent::TryGetHandTrackingSubsystem(UHandTrackingSubsystem*& OutSubsystem) const
{
	OutSubsystem = nullptr;

	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return false;
	}

	UGameInstance* GameInstance = OwnerActor->GetGameInstance();
	if (GameInstance == nullptr)
	{
		return false;
	}

	OutSubsystem = GameInstance->GetSubsystem<UHandTrackingSubsystem>();
	return OutSubsystem != nullptr;
}

bool USandwichResultReporterComponent::ContainsIngredient(const FString& IngredientId) const
{
	return CurrentResult.IngredientsUsed.Contains(IngredientId);
}