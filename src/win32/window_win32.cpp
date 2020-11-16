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

#if RB_EDITOR
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
#endif

	window_win32* window = hwnd ? reinterpret_cast<window_win32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)) : NULL;
	if (window) {
		switch (msg) {
			case WM_CLOSE:
				window->close();
				break;
			case WM_SETCURSOR:
				if (LOWORD(lParam) == HTCLIENT) {
					SetCursor(LoadCursor(NULL, IDC_ARROW));
				}
				break;
			case WM_SIZE:
				//if (service_collection::has<graphics_device>()) {
				//	service_collection::get<graphics_device>().set_backbuffer_size({ LOWORD(lParam), HIWORD(lParam) });
				//}
				//if (has_plugin<graphics>()) {
				//	get_plugin<graphics, graphics_directx>().set_backbuffer_size({ LOWORD(lParam), HIWORD(lParam) });
				//}
				break;
			case WM_LBUTTONUP:
			case WM_LBUTTONDOWN:
				// detail::set_mouse_button(mouse_button::left, msg == WM_LBUTTONDOWN);
				break;
			case WM_MBUTTONUP:
			case WM_MBUTTONDOWN:
				// detail::set_mouse_button(mouse_button::middle, msg == WM_MBUTTONDOWN);
				break;
			case WM_RBUTTONUP:
			case WM_RBUTTONDOWN:
				// detail::set_mouse_button(mouse_button::right, msg == WM_RBUTTONDOWN);
				break;
			case WM_MOUSEMOVE:
			{
				const auto x = static_cast<std::int16_t>(LOWORD(lParam));
				const auto y = static_cast<std::int16_t>(HIWORD(lParam));

				// detail::set_mouse_position(vec2i(x, y));
				break;
			}
			case WM_MOUSEWHEEL:
			{
				const auto delta = static_cast<std::int16_t>(HIWORD(wParam));
				// detail::set_mouse_wheel(delta / 120.0f);
				break;
			}
			case WM_CHAR:
			{
				const auto codepoint = static_cast<std::uint32_t>(wParam);

				char text[5] = { 0 };
				if (codepoint <= 0x7F) {
					text[0] = (char)codepoint;
					text[1] = '\0';
				} else if (codepoint <= 0x7FF) {
					text[0] = 0xC0 | (char)((codepoint >> 6) & 0x1F);
					text[1] = 0x80 | (char)(codepoint & 0x3F);
					text[2] = '\0';
				} else if (codepoint <= 0xFFFF) {
					text[0] = 0xE0 | (char)((codepoint >> 12) & 0x0F);
					text[1] = 0x80 | (char)((codepoint >> 6) & 0x3F);
					text[2] = 0x80 | (char)(codepoint & 0x3F);
					text[3] = '\0';
				} else if (codepoint <= 0x10FFFF) {
					text[0] = 0xF0 | (char)((codepoint >> 18) & 0x0F);
					text[1] = 0x80 | (char)((codepoint >> 12) & 0x3F);
					text[2] = 0x80 | (char)((codepoint >> 6) & 0x3F);
					text[3] = 0x80 | (char)(codepoint & 0x3F);
					text[4] = '\0';
				}

				if (codepoint >= 32 && text[0] != '\0') {
					// detail::add_input_text(text);
				}
				break;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if ((HIWORD(lParam) & KF_REPEAT) == 0) {
					// detail::set_key(translate_key(wParam, lParam), true);
				}
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				// detail::set_key(translate_key(wParam, lParam), false);
				break;
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
