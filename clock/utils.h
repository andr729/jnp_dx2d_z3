#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1_3.h>

template <class T>
void SafeRelease(T** ppT){
	if (*ppT) {
		(*ppT)->Release();
		*ppT = nullptr;
	}
}

#define THROW_IF_FAILED(hr, str) \
	if (FAILED((hr))) {		     \
		throw std::logic_error((str));  \
	}


