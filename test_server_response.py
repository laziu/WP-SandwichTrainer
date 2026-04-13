#!/usr/bin/env python3
# ============================================================================
# 서버 응답 테스트 및 시뮬레이션
# ============================================================================
# 
# 역할: 서버의 JSON 응답 형식을 테스트하고 Unreal이 파싱할 수 있는지 확인
#
# 사용법:
#   python test_server_response.py
#
# ============================================================================

import json
import time
from typing import Dict, Any
from datetime import datetime

# ============================================================================
# 테스트 데이터
# ============================================================================

def create_sample_response(action: str, gesture: str = "none") -> Dict[str, Any]:
    """
    Unreal Engine이 받을 것과 동일한 형식의 JSON 응답을 생성합니다.
    
    Args:
        action: 액션 유형 ("idle", "hover", "grab_start", "grab_hold", "drop")
        gesture: 제스처 ("open", "pinch", "none")
    
    Returns:
        dict: Unreal 클라이언트용 응답
    """
    hand_detected = action != "idle"
    
    response = {
        "ok": True,
        "hand_detected": hand_detected,
        "action": action,
        "action_data": {
            "hand_position": {
                "x": 0.5 if hand_detected else None,
                "y": 0.4 if hand_detected else None,
            },
            "hand_confidence": 0.95 if hand_detected else 0.0,
            "gesture": gesture,
            "is_left_hand": False
        },
        "gesture": gesture,
        "index_position": {
            "x": 0.51 if hand_detected else None,
            "y": 0.41 if hand_detected else None,
        },
        "thumb_position": {
            "x": 0.50 if hand_detected else None,
            "y": 0.42 if hand_detected else None,
        },
        "timestamp": time.time(),
        "message": None
    }
    
    return response


# ============================================================================
# 테스트함수
# ============================================================================

def test_response_format():
    """응답 형식이 올바른지 검증합니다."""
    
    print("=" * 70)
    print("Unreal Engine JSON 응답 형식 테스트")
    print("=" * 70)
    print()
    
    # 테스트 케이스
    test_cases = [
        ("idle", "none", "손 미감지"),
        ("hover", "open", "손 열린 상태 (호버)"),
        ("grab_start", "pinch", "손가락 모음 시작"),
        ("grab_hold", "pinch", "손가락 모음 유지"),
        ("drop", "open", "손가락 펼침 (놓기)"),
    ]
    
    for action, gesture, description in test_cases:
        print("-" * 70)
        print(f"테스트: {description}")
        print(f"액션: {action} | 제스처: {gesture}")
        print("-" * 70)
        
        response = create_sample_response(action, gesture)
        
        # JSON 형식으로 출력
        json_str = json.dumps(response, indent=2, ensure_ascii=False)
        print(f"JSON 응답:\n{json_str}")
        
        # 필드 검증
        print("\n필드 검증:")
        assert response["ok"] == True, "ok 필드 오류"
        print("  ✓ ok: True")
        
        assert response["action"] == action, "action 필드 오류"
        print(f"  ✓ action: {action}")
        
        assert response["gesture"] == gesture, "gesture 필드 오류"
        print(f"  ✓ gesture: {gesture}")
        
        assert "action_data" in response, "action_data 필드 누락"
        print("  ✓ action_data 필드 존재")
        
        assert "hand_position" in response["action_data"], "hand_position 필드 누락"
        print("  ✓ hand_position 필드 존재")
        
        assert "hand_confidence" in response["action_data"], "hand_confidence 필드 누락"
        print("  ✓ hand_confidence 필드 존재")
        
        assert "timestamp" in response, "timestamp 필드 누락"
        print("  ✓ timestamp 필드 존재")
        
        print()


def test_unreal_parsing():
    """Unreal이 파싱할 수 있는지 시뮬레이션합니다."""
    
    print("=" * 70)
    print("Unreal Engine 파싱 시뮬레이션")
    print("=" * 70)
    print()
    
    # 시뮬레이션: 애니메이션 수행
    actions = [
        ("idle", "none"),
        ("hover", "open"),
        ("hover", "open"),
        ("grab_start", "pinch"),
        ("grab_hold", "pinch"),
        ("grab_hold", "pinch"),
        ("drop", "open"),
        ("idle", "none"),
    ]
    
    print("시뮬레이션: 손 제스처 시퀀스")
    print("(1초마다 액션 변경)")
    print()
    
    for i, (action, gesture) in enumerate(actions, 1):
        response = create_sample_response(action, gesture)
        
        # Unreal에서 파싱하는 것 처럼 수행
        hand_detected = response["hand_detected"]
        action_type = response["action"]
        hand_pos = response["action_data"]["hand_position"]
        confidence = response["action_data"]["hand_confidence"]
        
        print(f"[Frame {i}] action={action_type:12} | gesture={gesture:6} | "
              f"detected={str(hand_detected):5} | confidence={confidence:.2f}")
        
        # 액션별 게임 로직
        if action_type == "idle":
            print("         → 대기 상태")
        elif action_type == "hover":
            print(f"         → 커서 위치: ({hand_pos['x']:.2f}, {hand_pos['y']:.2f})")
        elif action_type == "grab_start":
            print(f"         → 집기 시작 @ ({hand_pos['x']:.2f}, {hand_pos['y']:.2f})")
        elif action_type == "grab_hold":
            print(f"         → 객체 드래그 중...")
        elif action_type == "drop":
            print(f"         → 객체 놓기")
        
        print()
        time.sleep(0.5)  # 시뮬레이션 딜레이


def test_rest_api_call():
    """실제 서버 호출 테스트"""
    
    print("=" * 70)
    print("실제 서버 호출 테스트 (선택사항)")
    print("=" * 70)
    print()
    
    try:
        import requests
        
        server_url = "http://127.0.0.1:8888/data"
        
        print(f"서버 연결 중: {server_url}")
        response = requests.get(server_url, timeout=5)
        
        if response.status_code == 200:
            print("✓ 연결 성공!")
            data = response.json()
            print("\n받은 JSON:")
            print(json.dumps(data, indent=2, ensure_ascii=False))
        else:
            print(f"✗ 서버 오류: {response.status_code}")
    
    except requests.exceptions.ConnectionError:
        print("✗ 서버에 연결할 수 없습니다.")
        print("  서버가 실행 중인지 확인하세요: python main.py")
    
    except Exception as e:
        print(f"✗ 오류: {e}")
    
    print()


# ============================================================================
# 메인
# ============================================================================

if __name__ == "__main__":
    # 1. 응답 형식 테스트
    test_response_format()
    
    # 2. Unreal 파싱 시뮬레이션
    test_unreal_parsing()
    
    # 3. 실제 서버 호출 테스트
    test_rest_api_call()
    
    print("=" * 70)
    print("테스트 완료!")
    print("=" * 70)
