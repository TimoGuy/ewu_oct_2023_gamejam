#pragma once


namespace input
{
    extern bool
		onLMBPress,
		onLMBRelease,
		LMBPressed,
		onRMBPress,
		onRMBRelease,
		RMBPressed;

	extern ivec2 mouseDelta;
	extern ivec2  mouseScrollDelta;

	extern bool
		onKeyUpPress,
		keyUpPressed,
		onKeyDownPress,
		keyDownPressed,
		keyLeftPressed,
		keyRightPressed,
		keyWorldUpPressed,
		keyWorldDownPressed,
		keyShiftPressed,
		keyDelPressed,
		keyCtrlPressed,
		keyQPressed,
		keyWPressed,
		keyEPressed,
		keyRPressed,
		keyDPressed,
		keyCPressed,
		keyXPressed,
		keyVPressed,
		keyBPressed,
		keyEscPressed,
		keyEnterPressed,
		keyTargetPressed,
		onKeyJumpPress,
		keyJumpPressed,
		onKeyInteractPress,
		onKeyF11Press,
		onKeyF10Press,
		onKeyF9Press,
		onKeyF8Press,
		onKeyLSBPress,
		onKeyRSBPress,
		onKeyF1Press;

	void processInput(bool* isRunning, bool* isWindowMinimized);
}
