import asyncio
import json
import time
from typing import Optional, Dict, Any

import cv2
import mediapipe as mp
import websockets


WS_URL = "ws://127.0.0.1:8888/ws/hand"

mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils


def calc_gesture(hand_landmarks) -> str:
    """
    가장 단순한 제스처 판정:
    엄지 끝(4)과 검지 끝(8) 거리로 pinch 판정
    """
    thumb_tip = hand_landmarks.landmark[4]
    index_tip = hand_landmarks.landmark[8]

    dx = thumb_tip.x - index_tip.x
    dy = thumb_tip.y - index_tip.y
    dist = (dx * dx + dy * dy) ** 0.5

    if dist < 0.05:
        return "pinch"
    return "open"


def extract_payload(result) -> Dict[str, Any]:
    if not result.multi_hand_landmarks:
        return {
            "hand_detected": False,
            "gesture": "none",
            "index_tip": None,
            "thumb_tip": None,
            "landmarks": None,
            "timestamp": time.time(),
        }

    hand = result.multi_hand_landmarks[0]
    gesture = calc_gesture(hand)

    index_tip = hand.landmark[8]
    thumb_tip = hand.landmark[4]

    landmarks = [{"x": lm.x, "y": lm.y} for lm in hand.landmark]

    return {
        "hand_detected": True,
        "gesture": gesture,
        "index_tip": {"x": index_tip.x, "y": index_tip.y},
        "thumb_tip": {"x": thumb_tip.x, "y": thumb_tip.y},
        "landmarks": landmarks,
        "timestamp": time.time(),
    }


async def run_client():
    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        raise RuntimeError("웹캠을 열 수 없습니다.")

    async with websockets.connect(WS_URL) as ws:
        with mp_hands.Hands(
            static_image_mode=False,
            max_num_hands=1,
            min_detection_confidence=0.5,
            min_tracking_confidence=0.5,
        ) as hands:
            while True:
                ok, frame = cap.read()
                if not ok:
                    break

                frame = cv2.flip(frame, 1)
                rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                result = hands.process(rgb)

                payload = extract_payload(result)

                await ws.send(json.dumps(payload))
                response = json.loads(await ws.recv())

                # 화면 표시
                if result.multi_hand_landmarks:
                    for hand_landmarks in result.multi_hand_landmarks:
                        mp_draw.draw_landmarks(
                            frame,
                            hand_landmarks,
                            mp_hands.HAND_CONNECTIONS
                        )

                gesture_text = payload["gesture"]
                action_text = response.get("action", "idle")

                cv2.putText(
                    frame,
                    f"gesture={gesture_text} action={action_text}",
                    (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.8,
                    (0, 255, 0),
                    2,
                )

                cv2.imshow("MediaPipe Client", frame)

                key = cv2.waitKey(1) & 0xFF
                if key == 27:  # ESC
                    break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    asyncio.run(run_client())