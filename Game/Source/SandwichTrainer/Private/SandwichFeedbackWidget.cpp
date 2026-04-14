#include "SandwichFeedbackWidget.h"

void USandwichFeedbackWidget::ApplyFeedbackMessage(const FFeedbackMessage& FeedbackMessage)
{
	LastSessionId = FeedbackMessage.SessionId;
	LastFeedbackText = FeedbackMessage.FeedbackText;
	LastTips = FeedbackMessage.Tips;

	ShowFeedback(LastFeedbackText, LastTips);
}