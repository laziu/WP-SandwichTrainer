#include "SandwichDebugTestActor.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"

ASandwichDebugTestActor::ASandwichDebugTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ResultReporterComponent = CreateDefaultSubobject<USandwichResultReporterComponent>(TEXT("ResultReporterComponent"));
}

void ASandwichDebugTestActor::BeginPlay()
{
	Super::BeginPlay();

	CreateFeedbackWidget();

	if (UHandTrackingSubsystem* HandTrackingSubsystem = GetHandTrackingSubsystem())
	{
		HandTrackingSubsystem->OnFeedbackReceived.AddDynamic(this, &ASandwichDebugTestActor::HandleFeedbackReceived);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SandwichDebugTestActor: HandTrackingSubsystem not found."));
	}
}

void ASandwichDebugTestActor::CreateFeedbackWidget()
{
	if (FeedbackWidget != nullptr)
	{
		return;
	}

	if (!FeedbackWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SandwichDebugTestActor: FeedbackWidgetClass is not set."));
		return;
	}

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		FeedbackWidget = CreateWidget<USandwichFeedbackWidget>(PC, FeedbackWidgetClass);
		if (FeedbackWidget)
		{
			FeedbackWidget->AddToViewport();
			FeedbackWidget->HideFeedback();
		}
	}
}

UHandTrackingSubsystem* ASandwichDebugTestActor::GetHandTrackingSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UHandTrackingSubsystem>();
	}

	return nullptr;
}

void ASandwichDebugTestActor::SendTestResult()
{
	if (!ResultReporterComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("SandwichDebugTestActor: ResultReporterComponent is null."));
		return;
	}

	ResultReporterComponent->StartSession(TEXT("test-bmt-001"), TEXT("italian_bmt"));

	ResultReporterComponent->RecordIngredientUsed(TEXT("white_bread"));
	ResultReporterComponent->RecordIngredientUsed(TEXT("pepperoni"));
	ResultReporterComponent->RecordIngredientUsed(TEXT("salami"));
	ResultReporterComponent->RecordIngredientUsed(TEXT("ham"));
	ResultReporterComponent->RecordIngredientUsed(TEXT("lettuce"));
	ResultReporterComponent->RecordIngredientUsed(TEXT("tomato"));
	ResultReporterComponent->RecordIngredientUsed(TEXT("ranch"));

	ResultReporterComponent->RecordIngredientPlaced(TEXT("bread_bottom"));
	ResultReporterComponent->RecordIngredientPlaced(TEXT("pepperoni"));
	ResultReporterComponent->RecordIngredientPlaced(TEXT("salami"));
	ResultReporterComponent->RecordIngredientPlaced(TEXT("ham"));
	ResultReporterComponent->RecordIngredientPlaced(TEXT("lettuce"));
	ResultReporterComponent->RecordIngredientPlaced(TEXT("tomato"));
	ResultReporterComponent->RecordIngredientPlaced(TEXT("ranch"));
	ResultReporterComponent->RecordIngredientPlaced(TEXT("bread_top"));

	ResultReporterComponent->SetScore(95.f);
	ResultReporterComponent->SetElapsedTime(38.f);

	const bool bSent = ResultReporterComponent->SubmitResult();
	UE_LOG(LogTemp, Warning, TEXT("SandwichDebugTestActor: SendTestResult finished. Sent=%s"), bSent ? TEXT("true") : TEXT("false"));
}

void ASandwichDebugTestActor::ShowTestFeedback()
{
	if (!FeedbackWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("SandwichDebugTestActor: FeedbackWidget is null."));
		return;
	}

	FFeedbackMessage FeedbackMessage;
	FeedbackMessage.Type = TEXT("feedback");
	FeedbackMessage.SessionId = TEXT("local-test-session");
	FeedbackMessage.FeedbackText = TEXT("이야~ 거의 다 왔는데! 치즈를 깜빡했네~ 그리고 토마토는 양상추 다음에 올리면 더 안정적이야!");
	FeedbackMessage.Tips = {
		TEXT("치즈를 잊지 말자!"),
		TEXT("양상추 → 토마토 순서 기억!"),
		TEXT("마지막 빵까지 올렸는지 확인!")
	};

	FeedbackWidget->ApplyFeedbackMessage(FeedbackMessage);
}

void ASandwichDebugTestActor::HandleFeedbackReceived(const FFeedbackMessage& FeedbackMessage)
{
	if (!FeedbackWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("SandwichDebugTestActor: FeedbackWidget is null when feedback received."));
		return;
	}

	FeedbackWidget->ApplyFeedbackMessage(FeedbackMessage);
}