#include <d2d1_3.h>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <stdexcept>
#include <chrono>
#include <wincodec.h>
#include "app.h"
#include "utils.h"
#include "matrix.h"

// This line is needed on my local computer for some reason
// I suspose I have errors in linker configuration
#pragma comment(lib, "d2d1")

using D2D1::BitmapProperties;
using D2D1::PixelFormat;
using D2D1::Point2F;
using D2D1::RectF;
using D2D1::Matrix3x2F;
using std::sin;
using std::array;


HRESULT LoadBitmapFromFile(
	ID2D1RenderTarget* pRenderTarget,
	IWICImagingFactory* pIWICFactory,
	PCWSTR uri,
	UINT destinationWidth,
	UINT destinationHeight,
	ID2D1Bitmap** ppBitmap);

namespace {
	ID2D1Factory7* d2d_factory = nullptr;
	IWICImagingFactory* img_factory;
	ID2D1HwndRenderTarget* d2d_render_target = nullptr;

	// Stałe kolorów
	constexpr D2D1_COLOR_F background_color =
	{ .r = 0.57f, .g = 0.28f, .b = 0.74f, .a = 1.0f };


	ID2D1Bitmap* bitmap_img = nullptr;
	ID2D1Bitmap* digits_img = nullptr;

	Matrix3 base_transformation;

	FLOAT window_size_x;
	FLOAT window_size_y;

	FLOAT mouse_x = 0;
	FLOAT mouse_y = 0;

	bool mouse_left_down;

	FLOAT base_x_offset = 0;
	FLOAT base_y_offset = 300;

	UINT64 tick_count = 0;

	UINT64 base_seconds = 0;

}

UINT64 get_total_seconds() {
	return base_seconds + (tick_count / 10);
}

UINT64 get_hour() {
	return (get_total_seconds() / 60 / 60) % 24;
}

UINT64 get_minutes() {
	return (get_total_seconds() / 60) % 60;
}

bool dots() {
	return get_total_seconds() % 2 == 0;
}

void tick() {
	tick_count++;
}


void init(HWND hwnd) {
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;
	using std::chrono::seconds;
	using std::chrono::system_clock;
	auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	srand(int(millisec_since_epoch));
	rand();
	base_seconds = (rand() % 24) * 60 * 60;
	base_seconds += (rand() % 60) * 60;

	auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);
	THROW_IF_FAILED(hr, "D2D1CreateFactoryfailed");
	if (d2d_factory == nullptr) {
		exit(1);
	}

	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	THROW_IF_FAILED(hr, "CoInitializeEx failed");

	// Create WIC factory
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&img_factory)
	);
	THROW_IF_FAILED(hr, "Creating WIC factory failed");

	recreateRenderTarget(hwnd);
}

void recreateRenderTarget(HWND hwnd) {
	HRESULT hr;
	RECT rc;
	GetClientRect(hwnd, &rc);

	window_size_x = static_cast<FLOAT>(rc.right - rc.left);
	window_size_y = static_cast<FLOAT>(rc.bottom - rc.top);

	hr = d2d_factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(hwnd,
			D2D1::SizeU(
				static_cast<UINT32>(rc.right) -
				static_cast<UINT32>(rc.left),
				static_cast<UINT32>(rc.bottom) -
				static_cast<UINT32>(rc.top))),
		&d2d_render_target);

	THROW_IF_FAILED(hr, "CreateHwndRenderTarget failed");

	if (d2d_render_target == nullptr) {
		exit(1);
	}


	hr = LoadBitmapFromFile(
		d2d_render_target,
		img_factory,
		L"Watch.png",
		100, 100,
		&bitmap_img
	);
	THROW_IF_FAILED(hr, "failed loading bitmap");

	hr = LoadBitmapFromFile(
		d2d_render_target,
		img_factory,
		L"Digits.png",
		100, 100,
		&digits_img
	);
	THROW_IF_FAILED(hr, "failed loading bitmap");

}

void destroyRenderTarget() {
	if (d2d_render_target) {
		d2d_render_target->Release();
		d2d_render_target = nullptr;
	}
}

void destroy() {
	if (d2d_render_target) d2d_render_target->Release();
	if (d2d_factory) d2d_factory->Release();
}

void onMouseMove(FLOAT x, FLOAT y)  {
	mouse_x = x - window_size_x / 2 - base_x_offset;
	mouse_y = y - window_size_y / 2 - base_y_offset;
}

void setMouse(bool is_left_down) {
	mouse_left_down = is_left_down;
}

void draw_digit(ID2D1HwndRenderTarget* d2d_render_target, int pos, unsigned int digit) {
	if (digit >= 10) {
		throw std::logic_error("Bad digit passed to draw_digit");
	}

	FLOAT x_offset = pos * 108;
	FLOAT x_off_close = 20 * (pos < 0) - 20 * (pos > 0);

	d2d_render_target->DrawBitmap(
		digits_img,
		RectF(-108/2 + x_off_close + x_offset, -192 / 2, 108/2 + x_off_close + x_offset, 192/2),
		0.6f,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
		RectF(108 * digit, 0, 108 * digit + 108, 192));
}

void onPaint(HWND hwnd) {
	if (!d2d_render_target) recreateRenderTarget(hwnd);

	Matrix3 transform = Matrix3x2F::Identity();

	d2d_render_target->BeginDraw();
	d2d_render_target->Clear(background_color);

	transform *= Matrix3x2F::Rotation(-10, Point2F(0, 0));
	transform *= Matrix3x2F::Translation(window_size_x/2, window_size_y/2);

	d2d_render_target->SetTransform(transform.getInner());

	d2d_render_target->DrawBitmap(
		bitmap_img,
		RectF(-770 / 2, -400 / 2, 770/2, 400/2),
		0.7f,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

	draw_digit(d2d_render_target, -1, get_hour() % 10);
	draw_digit(d2d_render_target, -2, get_hour() / 10);

	draw_digit(d2d_render_target, 1, get_minutes() / 10);
	draw_digit(d2d_render_target, 2, get_minutes() % 10);

	if (dots()) {
		d2d_render_target->DrawBitmap(
			digits_img,
			RectF(-50, -192/2, 50, 192/2),
			0.6f,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
			RectF(108*10, 0, 108*10+100, 192));
	}

	if (d2d_render_target->EndDraw() == D2DERR_RECREATE_TARGET) {
		destroyRenderTarget();
		onPaint(hwnd);
	}
}

HRESULT LoadBitmapFromFile(
		ID2D1RenderTarget* pRenderTarget,
		IWICImagingFactory* pIWICFactory,
		PCWSTR uri,
		UINT destinationWidth,
		UINT destinationHeight,
		ID2D1Bitmap** ppBitmap) {

	IWICBitmapDecoder* pDecoder = nullptr;
	IWICBitmapFrameDecode* pSource = nullptr;
	IWICStream* pStream = nullptr;
	IWICFormatConverter* pConverter = nullptr;
	IWICBitmapScaler* pScaler = nullptr;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
		uri,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);
	if (SUCCEEDED(hr)) {
		// Create the initial frame.
		hr = pDecoder->GetFrame(0, &pSource);
	}
	if (SUCCEEDED(hr)) {
		// Convert the image format to 32bppPBGRA
		// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
		hr = pIWICFactory->CreateFormatConverter(&pConverter);

	}

	if (SUCCEEDED(hr)) {
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0.f,
			WICBitmapPaletteTypeMedianCut
		);
	}

	if (SUCCEEDED(hr)) {

		// Create a Direct2D bitmap from the WIC bitmap.
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			nullptr,
			ppBitmap
		);
	}

	SafeRelease(&pDecoder);
	SafeRelease(&pSource);
	SafeRelease(&pStream);
	SafeRelease(&pConverter);
	SafeRelease(&pScaler);

	return hr;
}