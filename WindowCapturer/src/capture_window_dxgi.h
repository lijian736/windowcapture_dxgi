#ifndef _H_DXGI_CAPTURE_SCREEN_H_
#define _H_DXGI_CAPTURE_SCREEN_H_

#include <vector>
#include <string>
#include <functional>
#include <windows.h>

#include <d3d9.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

//the callback function
typedef std::function< void(unsigned char* data, unsigned long len, unsigned int width, unsigned int height) > OnScreenDraw;

//the DXGI capture class
class DXGICaptureScreen
{
public:
	DXGICaptureScreen();
	virtual ~DXGICaptureScreen();

	BOOL init();
	BOOL un_init();

	BOOL render_loop();
	void set_draw_callback(OnScreenDraw func);

private:
	BOOL attach_to_this_thread();

private:
	BOOL m_initialized;

	ID3D11Device           *m_d3d11_device;
	ID3D11DeviceContext    *m_d3d11_device_context;

	IDXGIOutputDuplication *m_dxgi_duplication;
	DXGI_OUTPUT_DESC        m_dxgi_out_desc;
	ID3D11Texture2D        *m_dxgi_texture2d;
	IDXGISurface           *m_dxgi_surface;

	BYTE* m_data_buffer;

	DWORD m_width;
	DWORD m_height;
	DWORD m_line_bytes;
	DWORD m_line_stride;

	BOOL m_thread_attached;

	OnScreenDraw m_draw_func;
};

#endif