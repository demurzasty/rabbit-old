#include "window_win32.hpp"

#include <rabbit/core/config.hpp>

using namespace rb;

static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void setProcessDpiAware()
{
	// Try SetProcessDpiAwareness first
	HINSTANCE shCoreDll = LoadLibrary("Shcore.dll");

	if (shCoreDll)
	{
		enum ProcessDpiAwareness
		{
			ProcessDpiUnaware = 0,
			ProcessSystemDpiAware = 1,
			ProcessPerMonitorDpiAware = 2
		};

		typedef HRESULT(WINAPI* SetProcessDpiAwarenessFuncType)(ProcessDpiAwareness);
		SetProcessDpiAwarenessFuncType SetProcessDpiAwarenessFunc = reinterpret_cast<SetProcessDpiAwarenessFuncType>(GetProcAddress(shCoreDll, "SetProcessDpiAwareness"));

		if (SetProcessDpiAwarenessFunc)
		{
			// We only check for E_INVALIDARG because we would get
			// E_ACCESSDENIED if the DPI was already set previously
			// and S_OK means the call was successful
			if (SetProcessDpiAwarenessFunc(ProcessSystemDpiAware) == E_INVALIDARG)
			{
				// sf::err() << "Failed to set process DPI awareness" << std::endl;
			}
			else
			{
				FreeLibrary(shCoreDll);
				return;
			}
		}

		FreeLibrary(shCoreDll);
	}

	// Fall back to SetProcessDPIAware if SetProcessDpiAwareness
	// is not available on this system
	HINSTANCE user32Dll = LoadLibrary("user32.dll");

	if (user32Dll)
	{
		typedef BOOL(WINAPI* SetProcessDPIAwareFuncType)(void);
		SetProcessDPIAwareFuncType SetProcessDPIAwareFunc = reinterpret_cast<SetProcessDPIAwareFuncType>(GetProcAddress(user32Dll, "SetProcessDPIAware"));

		if (SetProcessDPIAwareFunc)
		{
			if (!SetProcessDPIAwareFunc()) {
				// sf::err() << "Failed to set process DPI awareness" << std::endl;
			}
		}

		FreeLibrary(user32Dll);
	}
}

window_win32::window_win32() {
	setProcessDpiAware();

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = &window_proc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = "RabBit";
	RegisterClass(&wc);

	HDC screenDC = GetDC(NULL);
	auto screen_width = (unsigned int)GetDeviceCaps(screenDC, HORZRES);
	auto screen_height = (unsigned int)GetDeviceCaps(screenDC, VERTRES);

	if (settings::fullscreen) {
		settings::window_size = { screen_width, screen_height };
	}

	auto left = (screen_width - settings::window_size.x) / 2;
	auto top = (screen_height - settings::window_size.y) / 2;
	ReleaseDC(NULL, screenDC);

	RECT rect;
	rect.left = rect.top = 0;
	rect.right = settings::window_size.x;
	rect.bottom = settings::window_size.y;

	const DWORD style = settings::fullscreen ?
		WS_VISIBLE | WS_POPUP : 
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE;

	auto width = settings::window_size.x;
	auto height = settings::window_size.y;

	if (!settings::fullscreen) {
		if (AdjustWindowRect(&rect, style, FALSE)) {
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;
		}
	} else {
		left = 0;
		top = 0;
		width = screen_width;
		height = screen_height;
	}

	_hwnd = CreateWindow("RabBit", settings::window_title.c_str(), style,
		left, top, width, height, NULL, NULL, GetModuleHandle(NULL), this);
	RB_ASSERT(_hwnd, "Cannot create window");
}

window_win32::~window_win32() {
	if (settings::fullscreen) {
		ChangeDisplaySettings(nullptr, 0);
	}

	DestroyWindow(_hwnd);
	UnregisterClass("RabBit", GetModuleHandle(NULL));
}

bool window_win32::is_open() const {
	return _open;
}

bool window_win32::is_minimized() const {
	return IsIconic(_hwnd) == TRUE;
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

vec2u window_win32::size() const {
	RECT rect;
	GetClientRect(_hwnd, &rect);
	return {
		static_cast<unsigned int>(rect.right - rect.left),
		static_cast<unsigned int>(rect.bottom - rect.top)
	};
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
