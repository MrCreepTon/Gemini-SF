#pragma once

#include <stdio.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <assert.h>
#include <process.h>
#include <fstream>
#include <mutex>

#include "SAMPFUNCS_API.h"
#include "game_api\game_api.h"

#include "json.hpp"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_stdlib.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "util/EncodingUtil.h"
#include "util/MathUtil.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);