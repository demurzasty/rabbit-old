#include "window_win32.hpp"

#include <cassert>

using namespace rb;

static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

window_win32::window_win32(config& config) {
	WNDCLASS wc = {};
	wc.lpfnWndProc = &window_proc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = "RabBit";
	RegisterClass(&wc);

	HDC screenDC = GetDC(NULL);
	const int left = (GetDeviceCaps(screenDC, HORZRES) - config.window_size.x) / 2;
	const int top = (GetDeviceCaps(screenDC, VERTRES) - config.window_size.y) / 2;
	ReleaseDC(NULL, screenDC);

	RECT rect;
	rect.left = rect.top = 0;
	rect.right = config.window_size.x;
	rect.bottom = config.window_size.y;

	const DWORD style = WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_SYSMENU | WS_VISIBLE;

	auto width = config.window_size.x;
	auto height = config.window_size.y;

	if (AdjustWindowRect(&rect, style, FALSE)) {
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	_hwnd = CreateWindow("RabBit", config.window_title.c_str(), style, left, top, width, height, NULL, NULL, GetModuleHandle(NULL), this);
	assert(_hwnd);
}

window_win32::~window_win32() {
	DestroyWindow(_hwnd);
	UnregisterClass("RabBit", GetModuleHandle(NULL));
}

bool window_win32::is_open() const {
	return _open;
}

window_handle window_win32::native_handle() const {
	return _hwnd;
}

void window_win32::poll_events() {
	MSG message;
	while (PeekMessage(&message, _hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

vec2i window_win32::size() const {
	RECT rect;
	GetClientRect(_hwnd, &rect);
	return vec2i(rect.right - rect.left, rect.bottom - rect.top);
}

void window_win32::maximize() {
	ShowWindow(_hwnd, SW_MAXIMIZE);
}

void window_win32::set_resizable(bool resizable) const {
}

bool window_win32::is_resizable() const {
	return false;
}

bool window_win32::is_focused() const {
	return GetFocus() == _hwnd;
}

void window_win32::set_title(const std::string& title) {
	SetWindowText(_hwnd, title.c_str());
}

std::string window_win32::title() const {
	char buffer[128];
	GetWindowText(_hwnd, buffer, sizeof(buffer));
	return buffer;
}

void window_win32::close() {
	_open = false;
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_CREATE) {
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)(lParam))->lpCreateParams);
	}

	window_win32* window = hwnd ? reinterpret_cast<window_win32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)) : NULL;
	if (window) {
		if (msg == WM_CLOSE) {
			window->close();
		}
	}

	if (msg == WM_CLOSE) {
		return 0;
	}

	if ((msg == WM_SYSCOMMAND) && (wParam == SC_KEYMENU)) {
		return 0;
	}

	if (!hwnd) {
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
