#include "HandTrackingSubsystem.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Json.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const FString HandTrackingServerURL = TEXT("ws://172.16.15.106:8888/ws/hand");
	const FString FeedbackServerURL = TEXT("ws://172.16.15.75:8000/ws");
}

void UHandTrackingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebSockets")))
	{
		FModuleManager::Get().LoadModule(TEXT("WebSockets"));
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
	ConnectHandTrackingSocket();
	ConnectFeedbackSocket();
}

void UHandTrackingSubsystem::Disconnect()
{
	if (HandTrackingWebSocket.IsValid())
	{
		if (HandTrackingWebSocket->IsConnected())
		{
			HandTrackingWebSocket->Close();
		}
		HandTrackingWebSocket.Reset();
	}

	if (FeedbackWebSocket.IsValid())
	{
		if (FeedbackWebSocket->IsConnected())
		{
			FeedbackWebSocket->Close();
		}
		FeedbackWebSocket.Reset();
	}
}

void UHandTrackingSubsystem::ConnectHandTrackingSocket()
{
	if (HandTrackingWebSocket.IsValid())
	{
		if (HandTrackingWebSocket->IsConnected())
		{
			UE_LOG(LogTemp, Log, TEXT("HandTracking WebSocket already connected."));
			return;
		}

		HandTrackingWebSocket->Close();
		HandTrackingWebSocket.Reset();
	}

	HandTrackingWebSocket = FWebSocketsModule::Get().CreateWebSocket(HandTrackingServerURL);

	HandTrackingWebSocket->OnConnected().AddLambda([]()
	{
		UE_LOG(LogTemp, Log, TEXT("HandTracking WebSocket Connected: ws://172.16.30.66:8888/ws/hand"));
	});

	HandTrackingWebSocket->OnConnectionError().AddLambda([](const FString& Error)
	{
		UE_LOG(LogTemp, Error, TEXT("HandTracking Connection Error: %s"), *Error);
	});

	HandTrackingWebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandTracking WebSocket Closed. StatusCode=%d, Reason=%s, Clean=%s"),
			StatusCode,
			*Reason,
			bWasClean ? TEXT("true") : TEXT("false"));
	});

	HandTrackingWebSocket->OnMessage().AddUObject(this, &UHandTrackingSubsystem::HandleHandTrackingMessage);

	HandTrackingWebSocket->Connect();
}

void UHandTrackingSubsystem::ConnectFeedbackSocket()
{
	if (FeedbackWebSocket.IsValid())
	{
		if (FeedbackWebSocket->IsConnected())
		{
			UE_LOG(LogTemp, Log, TEXT("Feedback WebSocket already connected."));
			return;
		}

		FeedbackWebSocket->Close();
		FeedbackWebSocket.Reset();
	}

	FeedbackWebSocket = FWebSocketsModule::Get().CreateWebSocket(FeedbackServerURL);

	FeedbackWebSocket->OnConnected().AddLambda([]()
	{
		UE_LOG(LogTemp, Log, TEXT("Feedback WebSocket Connected: ws://172.16.30.160:8000/ws"));
	});

	FeedbackWebSocket->OnConnectionError().AddLambda([](const FString& Error)
	{
		UE_LOG(LogTemp, Error, TEXT("Feedback Connection Error: %s"), *Error);
	});

	FeedbackWebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		UE_LOG(LogTemp, Warning, TEXT("Feedback WebSocket Closed. StatusCode=%d, Reason=%s, Clean=%s"),
			StatusCode,
			*Reason,
			bWasClean ? TEXT("true") : TEXT("false"));
	});

	FeedbackWebSocket->OnMessage().AddUObject(this, &UHandTrackingSubsystem::HandleFeedbackMessageRaw);

	FeedbackWebSocket->Connect();
}

bool UHandTrackingSubsystem::IsHandTrackingConnected() const
{
	return HandTrackingWebSocket.IsValid() && HandTrackingWebSocket->IsConnected();
}

bool UHandTrackingSubsystem::IsFeedbackConnected() const
{
	return FeedbackWebSocket.IsValid() && FeedbackWebSocket->IsConnected();
}

bool UHandTrackingSubsystem::SendSandwichResult(const FSandwichResultMessage& Message)
{
	if (!FeedbackWebSocket.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send sandwich result: Feedback WebSocket is invalid."));
		return false;
	}

	if (!FeedbackWebSocket->IsConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send sandwich result: Feedback WebSocket is not connected."));
		return false;
	}

	const FString Payload = BuildSandwichResultJson(Message);
	FeedbackWebSocket->Send(Payload);

	UE_LOG(LogTemp, Log, TEXT("Sent sandwich_result payload: %s"), *Payload);
	return true;
}

FString UHandTrackingSubsystem::BuildSandwichResultJson(const FSandwichResultMessage& Message) const
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();

	RootObject->SetStringField(TEXT("type"), Message.Type);
	RootObject->SetStringField(TEXT("session_id"), Message.SessionId);
	RootObject->SetStringField(TEXT("recipe_id"), Message.RecipeId);
	RootObject->SetNumberField(TEXT("score"), Message.Score);
	RootObject->SetNumberField(TEXT("time_seconds"), Message.TimeSeconds);

	TArray<TSharedPtr<FJsonValue>> IngredientsUsedArray;
	for (const FString& Ingredient : Message.IngredientsUsed)
	{
		IngredientsUsedArray.Add(MakeShared<FJsonValueString>(Ingredient));
	}
	RootObject->SetArrayField(TEXT("ingredients_used"), IngredientsUsedArray);

	TArray<TSharedPtr<FJsonValue>> IngredientsOrderArray;
	for (const FString& Ingredient : Message.IngredientsOrder)
	{
		IngredientsOrderArray.Add(MakeShared<FJsonValueString>(Ingredient));
	}
	RootObject->SetArrayField(TEXT("ingredients_order"), IngredientsOrderArray);

	TArray<TSharedPtr<FJsonValue>> ErrorsArray;
	for (const FSandwichError& Error : Message.Errors)
	{
		TSharedRef<FJsonObject> ErrorObject = MakeShared<FJsonObject>();
		ErrorObject->SetStringField(TEXT("type"), Error.Type);
		ErrorObject->SetStringField(TEXT("detail"), Error.Detail);
		ErrorsArray.Add(MakeShared<FJsonValueObject>(ErrorObject));
	}
	RootObject->SetArrayField(TEXT("errors"), ErrorsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject, Writer);

	return OutputString;
}

void UHandTrackingSubsystem::HandleHandTrackingMessage(const FString& Message)
{
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse hand tracking message: %s"), *Message);
		return;
	}

	const FHandData HandData = ParseHandMessage(JsonObject);
	OnHandDataReceived.Broadcast(HandData);
}

void UHandTrackingSubsystem::HandleFeedbackMessageRaw(const FString& Message)
{
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse feedback message: %s"), *Message);
		return;
	}

	FFeedbackMessage FeedbackMessage;
	if (ParseFeedbackMessage(JsonObject, FeedbackMessage))
	{
		OnFeedbackReceived.Broadcast(FeedbackMessage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid feedback payload: %s"), *Message);
	}
}

FHandData UHandTrackingSubsystem::ParseHandMessage(const TSharedPtr<FJsonObject>& JsonObject) const
{
	FHandData Result;

	if (!JsonObject.IsValid())
	{
		return Result;
	}

	JsonObject->TryGetStringField(TEXT("gesture"), Result.Sign);

	double XValue = 0.0;
	double YValue = 0.0;

	if (JsonObject->TryGetNumberField(TEXT("x"), XValue))
	{
		Result.X = static_cast<float>(XValue);
	}

	if (JsonObject->TryGetNumberField(TEXT("y"), YValue))
	{
		Result.Y = static_cast<float>(YValue);
	}

	return Result;
}

bool UHandTrackingSubsystem::ParseFeedbackMessage(const TSharedPtr<FJsonObject>& JsonObject, FFeedbackMessage& OutMessage) const
{
	if (!JsonObject.IsValid())
	{
		return false;
	}

	JsonObject->TryGetStringField(TEXT("type"), OutMessage.Type);
	JsonObject->TryGetStringField(TEXT("session_id"), OutMessage.SessionId);
	JsonObject->TryGetStringField(TEXT("feedback_text"), OutMessage.FeedbackText);

	const TArray<TSharedPtr<FJsonValue>>* TipsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("tips"), TipsArray) && TipsArray != nullptr)
	{
		for (const TSharedPtr<FJsonValue>& TipValue : *TipsArray)
		{
			FString TipText;
			if (TipValue.IsValid() && TipValue->TryGetString(TipText))
			{
				OutMessage.Tips.Add(TipText);
			}
		}
	}

	return OutMessage.Type.Equals(TEXT("feedback"), ESearchCase::IgnoreCase)
		&& !OutMessage.SessionId.IsEmpty();
}
