#include "capture_window_dxgi.h"

DXGICaptureScreen::DXGICaptureScreen()
{
	m_initialized = FALSE;

	m_d3d11_device = NULL;
	m_d3d11_device_context = NULL;
	m_dxgi_duplication = NULL;
	m_dxgi_texture2d = NULL;
	m_dxgi_surface = NULL;
	m_data_buffer = NULL;

	m_thread_attached = FALSE;

	ZeroMemory(&m_dxgi_out_desc, sizeof(DXGI_OUTPUT_DESC));
}

DXGICaptureScreen::~DXGICaptureScreen()
{
	un_init();
}

BOOL DXGICaptureScreen::init()
{
	if (m_initialized)
	{
		return TRUE;
	}

	un_init();

	HRESULT hr = S_OK;

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	UINT flags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	for (UINT i = 0; i < numDriverTypes; ++i)
	{
		hr = D3D11CreateDevice(NULL, driverTypes[i], NULL, flags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &m_d3d11_device, &featureLevel, &m_d3d11_device_context);
		if (SUCCEEDED(hr))
		{
			break;
		}
	}

	if (FAILED(hr))
	{
		return FALSE;
	}

	IDXGIDevice *dxgiDevice = NULL;
	hr = m_d3d11_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (FAILED(hr))
	{
		return FALSE;
	}

	IDXGIAdapter *dxgiAdapter = NULL;
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)(&dxgiAdapter));
	dxgiDevice->Release();
	dxgiDevice = NULL;
	if (FAILED(hr))
	{
		return FALSE;
	}

	INT nOutput = 0;
	IDXGIOutput *dxgiOutput = NULL;
	hr = dxgiAdapter->EnumOutputs(nOutput, &dxgiOutput);
	dxgiAdapter->Release();
	dxgiAdapter = NULL;
	if (FAILED(hr))
	{
		return FALSE;
	}

	dxgiOutput->GetDesc(&m_dxgi_out_desc);

	IDXGIOutput1 *dxgiOutput1 = NULL;
	hr = dxgiOutput->QueryInterface(__uuidof(dxgiOutput1), (void**)(&dxgiOutput1));
	dxgiOutput->Release();
	dxgiOutput = NULL;
	if (FAILED(hr))
	{
		return FALSE;
	}

	hr = dxgiOutput1->DuplicateOutput(m_d3d11_device, &m_dxgi_duplication);
	dxgiOutput1->Release();
	dxgiOutput1 = NULL;
	if (FAILED(hr))
	{
		return FALSE;
	}

	DEVMODE devmode;
	memset(&devmode, 0, sizeof(DEVMODE));
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	BOOL ret = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	if (!ret)
	{
		un_init();

		return FALSE;
	}

	m_width = devmode.dmPelsWidth;
	m_height = devmode.dmPelsHeight;
	DWORD bitcount = devmode.dmBitsPerPel;
	if (bitcount != 16 && bitcount != 24 && bitcount != 32)
	{
		//if bitcount equals 8, the screen display device is too old.
		un_init();
		return FALSE;
	}

	m_line_bytes = m_width * bitcount / 8;
	m_line_stride = (m_line_bytes + 3) / 4 * 4;

	m_initialized = TRUE;
	return TRUE;
}

BOOL DXGICaptureScreen::un_init()
{
	m_initialized = FALSE;
	m_data_buffer = NULL;

	if (m_dxgi_surface)
	{
		m_dxgi_surface->Unmap();
		m_dxgi_surface->Release();
		m_dxgi_surface = NULL;
	}

	if (m_dxgi_texture2d)
	{
		m_dxgi_texture2d->Release();
		m_dxgi_texture2d = NULL;
	}

	if (m_dxgi_duplication)
	{
		m_dxgi_duplication->Release();
		m_dxgi_duplication = NULL;
	}

	if (m_d3d11_device)
	{
		m_d3d11_device->Release();
		m_d3d11_device = NULL;
	}

	if (m_d3d11_device_context)
	{
		m_d3d11_device_context->Release();
		m_d3d11_device_context = NULL;
	}

	ZeroMemory(&m_dxgi_out_desc, sizeof(DXGI_OUTPUT_DESC));

	return TRUE;
}

BOOL DXGICaptureScreen::attach_to_this_thread()
{
	if (m_thread_attached)
	{
		return TRUE;
	}

	HDESK currentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
	if (!currentDesktop)
	{
		return FALSE;
	}

	BOOL desktopAttached = SetThreadDesktop(currentDesktop);
	CloseDesktop(currentDesktop);

	m_thread_attached = TRUE;

	return desktopAttached;
}

BOOL DXGICaptureScreen::render_loop()
{
	if (!m_initialized || !attach_to_this_thread())
	{
		return FALSE;
	}

	HRESULT hr;

	IDXGIResource *desktopResource = NULL;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	hr = m_dxgi_duplication->AcquireNextFrame(100, &frameInfo, &desktopResource);
	if (hr == DXGI_ERROR_ACCESS_LOST)
	{
		un_init();
		init();
		return FALSE;
	}
	else if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		return FALSE;
	}
	else if (FAILED(hr))
	{
		return FALSE;
	}

	ID3D11Texture2D *desktopImage = NULL;
	hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void **)(&desktopImage));
	desktopResource->Release();
	desktopResource = NULL;
	if (FAILED(hr))
	{
		return FALSE;
	}

	if (!m_dxgi_texture2d)
	{
		D3D11_TEXTURE2D_DESC frameDescriptor;
		desktopImage->GetDesc(&frameDescriptor);

		frameDescriptor.Usage = D3D11_USAGE_STAGING;
		frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		frameDescriptor.BindFlags = 0;
		frameDescriptor.MiscFlags = 0;
		frameDescriptor.MipLevels = 1;
		frameDescriptor.ArraySize = 1;
		frameDescriptor.SampleDesc.Count = 1;
		hr = m_d3d11_device->CreateTexture2D(&frameDescriptor, NULL, &m_dxgi_texture2d);
		if (FAILED(hr))
		{
			desktopImage->Release();
			desktopImage = NULL;
			m_dxgi_duplication->ReleaseFrame();

			return FALSE;
		}

		hr = m_dxgi_texture2d->QueryInterface(__uuidof(IDXGISurface), (void **)(&m_dxgi_surface));
		if (FAILED(hr))
		{
			desktopImage->Release();
			desktopImage = NULL;
			m_dxgi_duplication->ReleaseFrame();

			m_dxgi_texture2d->Release();
			m_dxgi_texture2d = NULL;

			return FALSE;
		}

		DXGI_MAPPED_RECT mappedRect;
		hr = m_dxgi_surface->Map(&mappedRect, DXGI_MAP_READ);
		if (SUCCEEDED(hr))
		{
			m_data_buffer = mappedRect.pBits;
		}
		else
		{
			desktopImage->Release();
			desktopImage = NULL;
			m_dxgi_duplication->ReleaseFrame();

			m_dxgi_texture2d->Release();
			m_dxgi_texture2d = NULL;

			m_dxgi_surface->Release();
			m_dxgi_surface = NULL;

			return FALSE;
		}
	}

	m_d3d11_device_context->CopyResource(m_dxgi_texture2d, desktopImage);

	desktopImage->Release();
	desktopImage = NULL;
	m_dxgi_duplication->ReleaseFrame();

	unsigned long len = m_line_stride * m_height;

	m_draw_func(m_data_buffer, len, m_width, m_height);
	return TRUE;
}

void DXGICaptureScreen::set_draw_callback(OnScreenDraw func)
{
	this->m_draw_func = func;
}