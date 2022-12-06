#include "main.h"
#include "app.h"

constexpr UINT frame_timer_id = 1;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_CREATE:
		init(hwnd);
		return 0;
	case WM_DESTROY:
		destroy();
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		onPaint(hwnd);
		ValidateRect(hwnd, nullptr);
		return 0;
	case WM_MOUSEMOVE:
		setMouse(GetAsyncKeyState(VK_LBUTTON) < 0);
		onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONDOWN:
		setMouse(true);
		return 0;
	case WM_LBUTTONUP:
		setMouse(false);
		return 0;
	case WM_TIMER:
		switch (wParam) {
		case frame_timer_id:
			tick();
			InvalidateRect(hwnd, nullptr, true);
			return 0;
		}
	case WM_SIZE:
		destroyRenderTarget();
		return 0;
	}
	return DefWindowProc(hwnd, Msg, wParam, lParam);
}

void setupDefaultWcex(WNDCLASSEX& wcex, HINSTANCE& instance) {
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(instance, IDC_ARROW);
	wcex.hbrBackground = nullptr,
		wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = TEXT("Template Window Class");
	wcex.hIconSm = nullptr;
}

void setupDefaoultHwnd(HWND& hwnd, WNDCLASSEX& wcex) {
	hwnd = CreateWindowEx(
		0, // Optional window styles.
		wcex.lpszClassName, // Window class
		TEXT("Window"), // Window text
		WS_OVERLAPPEDWINDOW, // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		nullptr, // Parent window
		nullptr, // Menu
		wcex.hInstance, // Instance handle
		nullptr // Additional application data
	);
}

INT WINAPI wWinMain(_In_ [[maybe_unused]] HINSTANCE instance,
	_In_opt_ [[maybe_unused]] HINSTANCE prev_instance,
	_In_ [[maybe_unused]] PWSTR cmd_line,
	_In_ [[maybe_unused]] INT cmd_show) {

	WNDCLASSEX wcex;
	setupDefaultWcex(wcex, instance);
	RegisterClassEx(&wcex);

	HWND hwnd;
	setupDefaoultHwnd(hwnd, wcex);

	if (hwnd == nullptr) {
		return 1;
	}

	auto timer_result = SetTimer(hwnd, frame_timer_id, 20, nullptr);
	if (timer_result == 0) {
		return 1;
	}

	ShowWindow(hwnd, cmd_show);

	try {
		MSG msg = {};
		while (BOOL rv = GetMessage(&msg, nullptr, 0, 0) != 0) {
			if (rv < 0) {
				DestroyWindow(hwnd);
				return 1;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	catch (...) {
		DestroyWindow(hwnd);
	}

	KillTimer(hwnd, frame_timer_id);
	return 0;
}
