// no thread-safe and non-modal support, sorry
#pragma once
#include <Windows.h>
#include "resource.h"

HRESULT latency_modify_dialog(HINSTANCE inst, HWND parent, int *latency, double *ratio);