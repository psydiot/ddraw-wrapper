/**
* Copyright (C) 2019 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*
* Code taken from: https://github.com/strangebytes/diablo-ddrawwrapper
*/
#include "ddraw.h"
#include "ddrawExternal.h"
#include "..\Utils\Utils.h"
#include "IDirectDrawTypes.h"
#include "IDirectDrawEnumCallback.h"
#include "IDirectDrawSurfaceX.h"
#include "IDirectDrawClipper.h"
#include "Versions/IDirectDrawSurface7.h"
#include "Versions/IDirectDrawSurface4.h"
#include "Versions/IDirectDrawSurface3.h"
#include "Versions/IDirectDrawSurface2.h"
#include "Versions/IDirectDrawSurface.h"
#include "IDirectDrawPalette.h"
// ddraw interface counter
DWORD ddrawRefCount = 0;

// Store a list of ddraw devices
std::vector<m_IDirectDrawX*> DDrawVector;

// Convert to Direct3D9
HWND MainhWnd;
HDC MainhDC;
bool IsInScene;
bool AllowModeX;
bool MultiThreaded;
bool FUPPreserve;
bool NoWindowChanges;
bool isWindowed;					// Window mode enabled

// Exclusive mode
bool ExclusiveMode;
HWND ExclusiveHwnd;

// Application display mode
bool ResetDisplayMode;
DWORD displayModeWidth;
DWORD displayModeHeight;
DWORD displayModeBPP;
DWORD displayModeRefreshRate;

// Display resolution
bool SetDefaultDisplayMode;			// Set native resolution
DWORD displayWidth;
DWORD displayHeight;
DWORD displayRefreshRate;			// Refresh rate for fullscreen

// High resolution counter
bool FrequencyFlag = false;
LARGE_INTEGER clockFrequency, clickTime, lastPresentTime = { 0, 0 };
LONGLONG lastFrameTime = 0;
DWORD FrameCounter = 0;
DWORD monitorRefreshRate = 0;
DWORD monitorHeight = 0;

// Direct3D9 Objects
LPDIRECT3D9 d3d9Object = nullptr;
LPDIRECT3DDEVICE9 d3d9Device = nullptr;
D3DPRESENT_PARAMETERS presParams;

constexpr DWORD MaxVidMemory = 0x8000000;

struct handle_data
{
	DWORD process_id = 0;
	HWND best_handle = nullptr;
};

std::unordered_map<HWND, m_IDirectDraw7*> g_hookmap;
m_IDirectDrawX* ddrawParent = nullptr;
std::unique_ptr<m_IDirect3D9Ex> UniqueProxyInterface = nullptr;
void* m_IDirect3D9Ex::GetWrapperInterfaceX(DWORD DirectXVersion)
{
	// Check for device
	if (!ddrawParent)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
		return nullptr;
	}

	switch (DirectXVersion)
	{
	case 1:
		if (!UniqueProxyInterface.get())
		{
			GUID guid;
			UniqueProxyInterface = std::make_unique<m_IDirect3D9Ex>(this,guid);
		}
		return UniqueProxyInterface.get();
	
	default:
		LOG_LIMIT(100, __FUNCTION__ << " Error: wrapper interface version not found: " << DirectXVersion);
		return nullptr;
	}
}

void m_IDirectDrawX::SetDdrawParent(m_IDirectDrawX* ddraw) { ddrawParent = ddraw; }

HRESULT m_IDirectDrawX::QueryInterface(REFIID riid, LPVOID FAR* ppvObj, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if ((riid == IID_IDirectDraw || riid == IID_IDirectDraw2 || riid == IID_IDirectDraw3 || riid == IID_IDirectDraw4 || riid == IID_IDirectDraw7 || riid == IID_IUnknown) && ppvObj)
		{
			DWORD DxVersion = (riid == IID_IUnknown) ? DirectXVersion : GetIIDVersion(riid);

			*ppvObj = GetWrapperInterfaceX(DxVersion);

			::AddRef(*ppvObj);

			return DD_OK;
		}
		if ((riid == IID_IDirect3D9 || riid == IID_IDirect3D9 || riid == IID_IDirect3D9 || riid == IID_IDirect3D9) && ppvObj)
		{
			DWORD DxVersion = GetIIDVersion(riid);

			SetCriticalSection();
			if (D3DInterface)
			{
				*ppvObj = D3DInterface->GetWrapperInterfaceX(DxVersion);

				::AddRef(*ppvObj);
			}
			else
			{

				//m_IDirect3D9Ex* p_IDirect3DX = new m_IDirect3D9Ex(this, DxVersion);
				m_IDirect3D9Ex* p_IDirect3DX=new m_IDirect3D9Ex((IDirect3D9Ex*)this, IID_IDirect3D9);
				*ppvObj = p_IDirect3DX->GetWrapperInterfaceX(DxVersion);

				D3DInterface = p_IDirect3DX;
			}
			ReleaseCriticalSection();

			return DD_OK;
		}
	

	HRESULT hr = ProxyQueryInterface(ProxyInterface, riid, ppvObj, GetWrapperType(DirectXVersion), WrapperInterface);

	if (SUCCEEDED(hr) && ppvObj)
	{
		if (riid == IID_IDirect3D9 || riid == IID_IDirect3D9 || riid == IID_IDirect3D9 || riid == IID_IDirect3D9)
		{
			//m_IDirect3D9Ex* lpD3DirectX = ((m_IDirect3D9Ex*)* ppvObj)->GetWrapperInterfaceX;
			m_IDirect3D9Ex*  lpD3DirectX=(m_IDirect3D9Ex*)((m_IDirect3D9Ex*)* ppvObj)->GetWrapperInterfaceX(DirectXVersion);
			if (lpD3DirectX)
			{
				//lpD3DirectX->SetDdrawParent(this);
				ddrawParent = this;
			}
		}
	}

	return hr;
}

void* m_IDirectDrawX::GetWrapperInterfaceX(DWORD DirectXVersion)
{
	switch (DirectXVersion)
	{
	case 1:
		if (!UniqueProxyInterface.get())
		{
			UniqueProxyInterface = std::make_unique<m_IDirectDraw>(this);
		}
		return UniqueProxyInterface.get();
	case 2:
		if (!UniqueProxyInterface2.get())
		{
			UniqueProxyInterface2 = std::make_unique<m_IDirectDraw2>(this);
		}
		return UniqueProxyInterface2.get();
	case 3:
		if (!UniqueProxyInterface3.get())
		{
			UniqueProxyInterface3 = std::make_unique<m_IDirectDraw3>(this);
		}
		return UniqueProxyInterface3.get();
	case 4:
		if (!UniqueProxyInterface4.get())
		{
			UniqueProxyInterface4 = std::make_unique<m_IDirectDraw4>(this);
		}
		return UniqueProxyInterface4.get();
	case 7:
		if (!UniqueProxyInterface7.get())
		{
			UniqueProxyInterface7 = std::make_unique<m_IDirectDraw7>(this);
		}
		return UniqueProxyInterface7.get();
	default:
		LOG_LIMIT(100, __FUNCTION__ << " Error: wrapper interface version not found: " << DirectXVersion);
		return nullptr;
	}
}

ULONG m_IDirectDrawX::AddRef()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	

		return InterlockedIncrement(&RefCount);


	//return ProxyInterface->AddRef();
}

ULONG m_IDirectDrawX::Release()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	ULONG ref;

	
		ref = InterlockedDecrement(&RefCount);
	
	

	if (ref == 0)
	{
		if (WrapperInterface)
		{
			WrapperInterface->DeleteMe();
		}
		else
		{
			delete this;
		}
	}

	return ref;
}

/***************************/
/*** IDirectDraw methods ***/
/***************************/

HRESULT m_IDirectDrawX::Compact()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		// This method is not currently implemented even in ddraw.
		return DD_OK;
	

	//return ProxyInterface->Compact();
}

HRESULT m_IDirectDrawX::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR * lplpDDClipper, IUnknown FAR * pUnkOuter)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lplpDDClipper)
		{
			return DDERR_INVALIDPARAMS;
		}

		*lplpDDClipper = new m_IDirectDrawClipper(dwFlags);

		return DD_OK;
	

	HRESULT hr = ProxyInterface->CreateClipper(dwFlags, lplpDDClipper, pUnkOuter);

	if (SUCCEEDED(hr) && lplpDDClipper)
	{
		*lplpDDClipper = ProxyAddressLookupTable.FindAddress<m_IDirectDrawClipper>(*lplpDDClipper);
	}

	return hr;
}

HRESULT m_IDirectDrawX::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR * lplpDDPalette, IUnknown FAR * pUnkOuter)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lplpDDPalette || !lpDDColorArray)
		{
			return DDERR_INVALIDPARAMS;
		}

		m_IDirectDrawPalette* PaletteX = new m_IDirectDrawPalette(this, dwFlags, lpDDColorArray);

		AddPaletteToVector(PaletteX);

		*lplpDDPalette = PaletteX;

		return DD_OK;
	

	HRESULT hr = ProxyInterface->CreatePalette(dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);

	if (SUCCEEDED(hr) && lplpDDPalette)
	{
		*lplpDDPalette = ProxyAddressLookupTable.FindAddress<m_IDirectDrawPalette>(*lplpDDPalette);
	}

	return hr;
}

HRESULT m_IDirectDrawX::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE7 FAR * lplpDDSurface, IUnknown FAR * pUnkOuter, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Game using old DirectX, Convert to LPDDSURFACEDESC2
	if (ProxyDirectXVersion > 3)
	{
		if (!lplpDDSurface || !lpDDSurfaceDesc)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ConvertSurfaceDesc(Desc2, *lpDDSurfaceDesc);

		return CreateSurface2(&Desc2, lplpDDSurface, pUnkOuter, DirectXVersion);
	}

	HRESULT hr = GetProxyInterfaceV3()->CreateSurface(lpDDSurfaceDesc, (LPDIRECTDRAWSURFACE*)lplpDDSurface, pUnkOuter);

	if (SUCCEEDED(hr) && lplpDDSurface)
	{
		*lplpDDSurface =(LPDIRECTDRAWSURFACE7) ProxyAddressLookupTable.FindAddress<m_IDirectDrawSurface7>(*lplpDDSurface);
	}

	return hr;
}

HRESULT m_IDirectDrawX::CreateSurface2(LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR * lplpDDSurface, IUnknown FAR * pUnkOuter, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lplpDDSurface || !lpDDSurfaceDesc2)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for existing primary surface
		if ((lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) && GetPrimarySurface())
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: primary surface already exists!");
			return DDERR_PRIMARYSURFACEALREADYEXISTS;
		}

		// Check for invalid surface flip flags
		if ((lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_FLIP) &&
			(!(lpDDSurfaceDesc2->dwFlags & DDSD_BACKBUFFERCOUNT) || !(lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_COMPLEX)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: invalid flip surface flags!");
			return DDERR_INVALIDPARAMS;
		}

		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ConvertSurfaceDesc(Desc2, *lpDDSurfaceDesc2);
		Desc2.ddsCaps.dwCaps4 = DDSCAPS4_CREATESURFACE |											// Indicates surface was created using CreateSurface()
			((Desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) ? DDSCAPS4_PRIMARYSURFACE : NULL);		// Indicates surface is a primary surface or a backbuffer of a primary surface
		Desc2.dwReserved = 0;

		if (Desc2.ddsCaps.dwCaps & DDSCAPS_FLIP)
		{
			Desc2.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
		}

		if (Desc2.dwFlags & DDSD_BACKBUFFERCOUNT)
		{
			if (!Desc2.dwBackBufferCount)
			{
				Desc2.dwBackBufferCount = 1;
			}
		}
		else
		{
			Desc2.dwBackBufferCount = 0;
		}

		m_IDirectDrawSurfaceX* p_IDirectDrawSurfaceX = new m_IDirectDrawSurfaceX(&d3d9Device, this, DirectXVersion, &Desc2, displayWidth, displayHeight);

		*lplpDDSurface = (LPDIRECTDRAWSURFACE7)p_IDirectDrawSurfaceX->GetWrapperInterfaceX(DirectXVersion);

		if (Desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			PrimarySurface = p_IDirectDrawSurfaceX;
		}

		return DD_OK;
	

	// BackBufferCount must be at least 1
	if (ProxyDirectXVersion != DirectXVersion && lpDDSurfaceDesc2)
	{
		if (lpDDSurfaceDesc2->dwFlags & DDSD_BACKBUFFERCOUNT)
		{
			if (!lpDDSurfaceDesc2->dwBackBufferCount)
			{
				lpDDSurfaceDesc2->dwBackBufferCount = 1;
			}
		}
		else
		{
			lpDDSurfaceDesc2->dwBackBufferCount = 0;
		}
	}

	HRESULT hr = ProxyInterface->CreateSurface(lpDDSurfaceDesc2, lplpDDSurface, pUnkOuter);

	if (SUCCEEDED(hr) && lplpDDSurface)
	{
		*lplpDDSurface =(LPDIRECTDRAWSURFACE7) ProxyAddressLookupTable.FindAddress<m_IDirectDrawSurface7>(*lplpDDSurface);

		if ( *lplpDDSurface)
		{
			m_IDirectDrawSurfaceX* lpDDSurfaceX = ((m_IDirectDrawSurface7*)* lplpDDSurface)->GetWrapperInterface();

			if (lpDDSurfaceX)
			{
				lpDDSurfaceX->SetDdrawParent(this);
			}
		}
	}

	return hr;
}

HRESULT m_IDirectDrawX::DuplicateSurface(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7 FAR * lplpDupDDSurface, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		m_IDirectDrawSurfaceX* lpDDSurfaceX = (m_IDirectDrawSurfaceX*)lpDDSurface;
		if (!DoesSurfaceExist(lpDDSurfaceX))
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		lpDDSurfaceX->GetSurfaceDesc2(&Desc2);
		Desc2.ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;		// Remove Primary surface flag

		m_IDirectDrawSurfaceX* p_IDirectDrawSurfaceX = new m_IDirectDrawSurfaceX(&d3d9Device, this, DirectXVersion, &Desc2, displayWidth, displayHeight);

		*lplpDupDDSurface = (LPDIRECTDRAWSURFACE7)p_IDirectDrawSurfaceX->GetWrapperInterfaceX(DirectXVersion);

		return DD_OK;
	

	if (lpDDSurface)
	{
		lpDDSurface = static_cast<m_IDirectDrawSurface7*>(lpDDSurface)->GetProxyInterface();
	}

	HRESULT hr = ProxyInterface->DuplicateSurface(lpDDSurface, lplpDupDDSurface);

	if (SUCCEEDED(hr) && lplpDupDDSurface && lpDDSurface)
	{
		*lplpDupDDSurface = (LPDIRECTDRAWSURFACE7)ProxyAddressLookupTable.FindAddress<m_IDirectDrawSurface7>(*lplpDupDDSurface);
	}

	return hr;
}

HRESULT m_IDirectDrawX::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Game using old DirectX, Convert to LPDDSURFACEDESC2
	if (ProxyDirectXVersion > 3)
	{
		if (!lpEnumModesCallback)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		if (lpDDSurfaceDesc)
		{
			ConvertSurfaceDesc(Desc2, *lpDDSurfaceDesc);
		}

		ENUMDISPLAYMODES CallbackContext;
		CallbackContext.lpContext = lpContext;
		CallbackContext.lpCallback = lpEnumModesCallback;

		return EnumDisplayModes2(dwFlags, (lpDDSurfaceDesc) ? &Desc2 : nullptr, &CallbackContext, m_IDirectDrawEnumDisplayModes::ConvertCallback);
	}

	return GetProxyInterfaceV3()->EnumDisplayModes(dwFlags, lpDDSurfaceDesc, lpContext, lpEnumModesCallback);
}

HRESULT m_IDirectDrawX::EnumDisplayModes2(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback2)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lpEnumModesCallback2)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false)))
		{
			return DDERR_GENERIC;
		}

		// Save width, height and refresh rate
		DWORD EnumWidth = 0;
		DWORD EnumHeight = 0;
		DWORD EnumRefreshRate = 0;
		if (lpDDSurfaceDesc2)
		{
			EnumWidth = (lpDDSurfaceDesc2->dwFlags & DDSD_WIDTH) ? lpDDSurfaceDesc2->dwWidth : 0;
			EnumHeight = (lpDDSurfaceDesc2->dwFlags & DDSD_HEIGHT) ? lpDDSurfaceDesc2->dwHeight : 0;
			EnumRefreshRate = (lpDDSurfaceDesc2->dwFlags & DDSD_REFRESHRATE) ? lpDDSurfaceDesc2->dwRefreshRate : 0;
		}
		if (!(dwFlags & DDEDM_REFRESHRATES) && !EnumRefreshRate)
		{
			EnumRefreshRate = Utils::GetRefreshRate(MainhWnd);
		}

		// Get display modes to enum
		DWORD DisplayBitCount = (displayModeBPP) ? displayModeBPP : 0;
		if (lpDDSurfaceDesc2 && (lpDDSurfaceDesc2->dwFlags & DDSD_PIXELFORMAT))
		{
			DisplayBitCount = GetBitCount(lpDDSurfaceDesc2->ddpfPixelFormat);
		}
		bool DisplayAllModes = (DisplayBitCount != 8 && DisplayBitCount != 16 && DisplayBitCount != 24 && DisplayBitCount != 32);

		// Setup surface desc
		DDSURFACEDESC2 Desc2 = { NULL };
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

		// Setup display mode
		D3DDISPLAYMODE d3ddispmode;

		// Enumerate modes for format XRGB
		UINT modeCount = d3d9Object->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);

		// Loop through all modes
		DWORD Loop = 0;
		for (UINT i = 0; i < modeCount; i++)
		{
			// Get display modes
			ZeroMemory(&d3ddispmode, sizeof(D3DDISPLAYMODE));
			if (FAILED(d3d9Object->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &d3ddispmode)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: EnumAdapterModes failed");
				break;
			}

			// Loop through each bit count
			for (DWORD bpMode : {8, 16, 24, 32})
			{
				// Set display bit count
				if (DisplayAllModes)
				{
					DisplayBitCount = bpMode;
				}

				// Check refresh mode
				if ((!EnumWidth || d3ddispmode.Width == EnumWidth) &&
					(!EnumHeight || d3ddispmode.Height == EnumHeight) &&
					(!EnumRefreshRate || d3ddispmode.RefreshRate == EnumRefreshRate))
				{
					if (++Loop > Config.DdrawLimitDisplayModeCount && Config.DdrawLimitDisplayModeCount)
					{
						return DD_OK;
					}

					// Set surface desc options
					Desc2.dwSize = sizeof(DDSURFACEDESC2);
					Desc2.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_REFRESHRATE | DDSD_PITCH | DDSD_PIXELFORMAT;
					Desc2.dwWidth = d3ddispmode.Width;
					Desc2.dwHeight = d3ddispmode.Height;
					Desc2.dwRefreshRate = d3ddispmode.RefreshRate;

					// Set adapter pixel format
					Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					switch (DisplayBitCount)
					{
					case 8:
						SetPixelDisplayFormat(D3DFMT_P8, Desc2.ddpfPixelFormat);
						break;
					case 16:
						SetPixelDisplayFormat(D3DFMT_R5G6B5, Desc2.ddpfPixelFormat);
						break;
					case 24:
						SetPixelDisplayFormat(D3DFMT_R8G8B8, Desc2.ddpfPixelFormat);
						break;
					case 32:
						SetPixelDisplayFormat(D3DFMT_X8R8G8B8, Desc2.ddpfPixelFormat);
						break;
					}
					Desc2.lPitch = (Desc2.ddpfPixelFormat.dwRGBBitCount / 8) * Desc2.dwHeight;

					if (lpEnumModesCallback2(&Desc2, lpContext) == DDENUMRET_CANCEL)
					{
						return DD_OK;
					}
				}

				// Break if not displaying all modes
				if (!DisplayAllModes)
				{
					break;
				}
			}
		}

		return DD_OK;
	

	return ProxyInterface->EnumDisplayModes(dwFlags, lpDDSurfaceDesc2, lpContext, lpEnumModesCallback2);
}

HRESULT m_IDirectDrawX::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!lpEnumSurfacesCallback)
	{
		return DDERR_INVALIDPARAMS;
	}

	// Game using old DirectX, Convert to LPDDSURFACEDESC2
	if (ProxyDirectXVersion > 3)
	{
		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		if (lpDDSurfaceDesc)
		{
			ConvertSurfaceDesc(Desc2, *lpDDSurfaceDesc);
		}

		return EnumSurfaces2(dwFlags, (lpDDSurfaceDesc) ? &Desc2 : nullptr, lpContext, (LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback, DirectXVersion);
	}

	ENUMSURFACE CallbackContext;
	CallbackContext.lpContext = lpContext;
	CallbackContext.lpCallback = lpEnumSurfacesCallback;
	CallbackContext.DirectXVersion = DirectXVersion;

	return GetProxyInterfaceV3()->EnumSurfaces(dwFlags, lpDDSurfaceDesc, &CallbackContext, m_IDirectDrawEnumSurface::ConvertCallback);
}

HRESULT m_IDirectDrawX::EnumSurfaces2(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback7, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!lpEnumSurfacesCallback7)
	{
		return DDERR_INVALIDPARAMS;
	}

	
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	

	ENUMSURFACE CallbackContext;
	CallbackContext.lpContext = lpContext;
	CallbackContext.lpCallback7 = lpEnumSurfacesCallback7;
	CallbackContext.DirectXVersion = DirectXVersion;
	CallbackContext.ConvertSurfaceDescTo2 = (ProxyDirectXVersion > 3 && DirectXVersion < 4);

	return ProxyInterface->EnumSurfaces(dwFlags, lpDDSurfaceDesc2, &CallbackContext, m_IDirectDrawEnumSurface::ConvertCallback2);
}

HRESULT m_IDirectDrawX::FlipToGDISurface()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (d3d9Device && FAILED(CreateD3D9Device()))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: creating Direct3D9 Device");
			return DDERR_GENERIC;
		}

		ResetDisplayMode = true;

		return DD_OK;
	

	return ProxyInterface->FlipToGDISurface();
}

HRESULT m_IDirectDrawX::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	DDCAPS DriverCaps, HELCaps;
	DriverCaps.dwSize = sizeof(DDCAPS);
	HELCaps.dwSize = sizeof(DDCAPS);

	HRESULT hr = DDERR_GENERIC;

	
		if (!lpDDDriverCaps && !lpDDHELCaps)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false)))
		{
			return DDERR_GENERIC;
		}

		// Get video memory
		DWORD dwVidTotal = MaxVidMemory;
		DWORD dwVidFree = MaxVidMemory - 0x100000;
		GetAvailableVidMem2(nullptr, &dwVidTotal, &dwVidFree);

		// Get caps
		D3DCAPS9 Caps9;
		if (lpDDDriverCaps)
		{
			hr = d3d9Object->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, &Caps9);
			ConvertCaps(DriverCaps, Caps9);
			DriverCaps.dwVidMemTotal = dwVidTotal;
			DriverCaps.dwVidMemFree = dwVidFree;
		}
		if (lpDDHELCaps)
		{
			hr = d3d9Object->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &Caps9);
			ConvertCaps(HELCaps, Caps9);
			HELCaps.dwVidMemTotal = dwVidTotal;
			HELCaps.dwVidMemFree = dwVidFree;
		}
	

	if (SUCCEEDED(hr))
	{
		if (lpDDDriverCaps)
		{
			ConvertCaps(*lpDDDriverCaps, DriverCaps);
		}
		if (lpDDHELCaps)
		{
			ConvertCaps(*lpDDHELCaps, HELCaps);
		}
	}
	else
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to GetCaps!");
	}

	return hr;
}

HRESULT m_IDirectDrawX::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Game using old DirectX, Convert to LPDDSURFACEDESC2
	if (ProxyDirectXVersion > 3)
	{
		if (!lpDDSurfaceDesc)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ConvertSurfaceDesc(Desc2, *lpDDSurfaceDesc);

		HRESULT hr = GetDisplayMode2(&Desc2);

		// Convert back to LPDDSURFACEDESC
		if (SUCCEEDED(hr))
		{
			ConvertSurfaceDesc(*lpDDSurfaceDesc, Desc2);
		}

		return hr;
	}

	return GetProxyInterfaceV3()->GetDisplayMode(lpDDSurfaceDesc);
}

HRESULT m_IDirectDrawX::GetDisplayMode2(LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lpDDSurfaceDesc2)
		{
			// Just return OK
			return DD_OK;
		}

		// Set Surface Desc
		lpDDSurfaceDesc2->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_REFRESHRATE | DDSD_PIXELFORMAT;
		DWORD displayModeBits = displayModeBPP;
		if (displayModeBits)
		{
			lpDDSurfaceDesc2->dwWidth = displayModeWidth;
			lpDDSurfaceDesc2->dwHeight = displayModeHeight;
			lpDDSurfaceDesc2->dwRefreshRate = displayModeRefreshRate;
		}
		else
		{
			HDC hdc = ::GetDC(nullptr);
			lpDDSurfaceDesc2->dwWidth = GetSystemMetrics(SM_CXSCREEN);
			lpDDSurfaceDesc2->dwHeight = GetSystemMetrics(SM_CYSCREEN);
			lpDDSurfaceDesc2->dwRefreshRate = Utils::GetRefreshRate(MainhWnd);
			displayModeBits = GetDeviceCaps(hdc, BITSPIXEL);
			ReleaseDC(nullptr, hdc);
		}

		// Force color mode
		displayModeBits = (Config.DdrawOverrideBitMode) ? Config.DdrawOverrideBitMode : displayModeBits;

		// Set Pixel Format
		switch (displayModeBits)
		{
		case 8:
			SetPixelDisplayFormat(D3DFMT_P8, lpDDSurfaceDesc2->ddpfPixelFormat);
			break;
		case 16:
			SetPixelDisplayFormat(D3DFMT_R5G6B5, lpDDSurfaceDesc2->ddpfPixelFormat);
			break;
		case 24:
			SetPixelDisplayFormat(D3DFMT_R8G8B8, lpDDSurfaceDesc2->ddpfPixelFormat);
			break;
		case 32:
			SetPixelDisplayFormat(D3DFMT_X8R8G8B8, lpDDSurfaceDesc2->ddpfPixelFormat);
			break;
		default:
			LOG_LIMIT(100, __FUNCTION__ << " Not implemented bit count " << displayModeBits);
			return DDERR_UNSUPPORTED;
		}

		return DD_OK;
	

	return ProxyInterface->GetDisplayMode(lpDDSurfaceDesc2);
}

HRESULT m_IDirectDrawX::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	

	return ProxyInterface->GetFourCCCodes(lpNumCodes, lpCodes);
}

HRESULT m_IDirectDrawX::GetGDISurface(LPDIRECTDRAWSURFACE7 FAR * lplpGDIDDSSurface, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	

	HRESULT hr = ProxyInterface->GetGDISurface(lplpGDIDDSSurface);

	if (SUCCEEDED(hr) && lplpGDIDDSSurface)
	{
		*lplpGDIDDSSurface = (LPDIRECTDRAWSURFACE7)ProxyAddressLookupTable.FindAddress<m_IDirectDrawSurface7>(*lplpGDIDDSSurface);
	}

	return hr;
}

HRESULT m_IDirectDrawX::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lpdwFrequency)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true)))
		{
			return DDERR_GENERIC;
		}

		D3DDISPLAYMODE Mode;
		if (FAILED(d3d9Device->GetDisplayMode(0, &Mode)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to get display mode!");
			return DDERR_GENERIC;
		}

		*lpdwFrequency = Mode.RefreshRate;

		return DD_OK;
	

	return ProxyInterface->GetMonitorFrequency(lpdwFrequency);
}

HRESULT m_IDirectDrawX::GetScanLine(LPDWORD lpdwScanLine)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lpdwScanLine)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true)))
		{
			return DDERR_GENERIC;
		}

		D3DRASTER_STATUS RasterStatus;
		if (FAILED(d3d9Device->GetRasterStatus(0, &RasterStatus)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to get raster status!");
			return DDERR_GENERIC;
		}

		*lpdwScanLine = RasterStatus.ScanLine;

		return DD_OK;
	

	return ProxyInterface->GetScanLine(lpdwScanLine);
}

HRESULT m_IDirectDrawX::GetVerticalBlankStatus(LPBOOL lpbIsInVB)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lpbIsInVB)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true)))
		{
			return DDERR_GENERIC;
		}

		D3DRASTER_STATUS RasterStatus;
		if (FAILED(d3d9Device->GetRasterStatus(0, &RasterStatus)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to get raster status!");
			return DDERR_GENERIC;
		}

		*lpbIsInVB = RasterStatus.InVBlank;

		return DD_OK;
	

	return ProxyInterface->GetVerticalBlankStatus(lpbIsInVB);
}

HRESULT m_IDirectDrawX::Initialize(GUID FAR * lpGUID)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		// Not needed with d3d9
		return DD_OK;
	



	return ProxyInterface->Initialize(lpGUID);
}

// Resets the mode of the display device hardware for the primary surface to what it was before the IDirectDraw7::SetDisplayMode method was called.
HRESULT m_IDirectDrawX::RestoreDisplayMode()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		// Exclusive-level access is required to use this method.
		if (!ExclusiveMode)
		{
			return DDERR_NOEXCLUSIVEMODE;
		}

		// Reset mode
		displayModeWidth = 0;
		displayModeHeight = 0;
		displayModeBPP = 0;
		displayModeRefreshRate = 0;

		// Reset primary surface display settings
		for (m_IDirectDrawSurfaceX* pSurface : SurfaceVector)
		{
			pSurface->RestoreSurfaceDisplay();
		}

		return DD_OK;
	

	return ProxyInterface->RestoreDisplayMode();
}

// Fixes a bug in ddraw in Windows 8 and 10 where the exclusive flag remains even after the window (hWnd) closes
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (nCode == HCBT_DESTROYWND && !Config.Exiting)
	{
		Logging::LogDebug() << __FUNCTION__;

		HWND hWnd = (HWND)wParam;
		auto it = g_hookmap.find(hWnd);
		if (it != std::end(g_hookmap))
		{
			m_IDirectDraw7* lpDDraw = it->second;
			if (lpDDraw && ProxyAddressLookupTable.IsValidAddress(lpDDraw))
			{
				lpDDraw->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
			}
			g_hookmap.clear();
		}
	}

	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// Enums all windows and returns the handle to the active window
BOOL CALLBACK EnumProcWindowCallback(HWND hwnd, LPARAM lParam)
{
	// Get variables from call back
	handle_data& data = *(handle_data*)lParam;

	// Skip windows that are from a different process ID
	DWORD process_id;
	GetWindowThreadProcessId(hwnd, &process_id);
	if (data.process_id != process_id)
	{
		return TRUE;
	}

	// Skip compatibility class windows
	char class_name[80] = { 0 };
	GetClassName(hwnd,(LPWSTR) class_name, sizeof(class_name));
	if (strcmp(class_name, "CompatWindowDesktopReplacement") == 0)			// Compatibility class windows
	{
		return TRUE;
	}

	// Match found returning value
	data.best_handle = hwnd;
	return FALSE;
}

HRESULT m_IDirectDrawX::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		// Check for valid parameters
		if (((dwFlags & DDSCL_NORMAL) && (dwFlags & (DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN))) ||						// Both Normal and Exclusive mode flags cannot be set
			((dwFlags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)) != (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) && !(dwFlags & DDSCL_NORMAL)) ||	// Both Exclusive and Fullscreen flags must be set together
			((dwFlags & DDSCL_ALLOWMODEX) && !(dwFlags & DDSCL_EXCLUSIVE)) ||															// If AllowModeX is set then Exclusive mode must be set
			(((dwFlags & DDSCL_EXCLUSIVE) || (hWnd && hWnd != ExclusiveHwnd)) && !IsWindow(hWnd)))										// When using Exclusive mode the hwnd must be valid
		{
			return DDERR_INVALIDPARAMS;
		}

		// Set windowed mode
		if (dwFlags & DDSCL_NORMAL)
		{
			// Check for exclusive mode
			if (ExclusiveMode && hWnd && ExclusiveHwnd == hWnd)
			{
				ExclusiveMode = false;
				ExclusiveHwnd = nullptr;
			}
			isWindowed = true;
		}
		else if (dwFlags & DDSCL_FULLSCREEN)
		{
			if (ExclusiveMode && ExclusiveHwnd != hWnd && IsWindow(ExclusiveHwnd))
			{
				return DDERR_EXCLUSIVEMODEALREADYSET;
			}
			isWindowed = false;
			ExclusiveMode = true;
			ExclusiveHwnd = hWnd;
		}

		// Set device flags
		AllowModeX = ((dwFlags & DDSCL_ALLOWMODEX) != 0);
		MultiThreaded = ((dwFlags & DDSCL_MULTITHREADED) != 0);
		FUPPreserve = ((dwFlags & (DDSCL_FPUPRESERVE | DDSCL_FPUSETUP)) != 0);
		NoWindowChanges = ((dwFlags & DDSCL_NOWINDOWCHANGES) != 0);

		// Set display window
		HWND t_hWnd = (IsWindow(ExclusiveHwnd)) ? ExclusiveHwnd : (IsWindow(hWnd)) ? hWnd : nullptr;
		if (!t_hWnd)
		{
			// Set variables
			handle_data data;
			data.process_id = GetCurrentProcessId();
			data.best_handle = nullptr;

			// Gets all window layers and looks for a main window that is fullscreen
			EnumWindows(EnumProcWindowCallback, (LPARAM)& data);

			// Get top window
			HWND m_hWnd = GetTopWindow(data.best_handle);
			if (m_hWnd)
			{
				data.best_handle = m_hWnd;
			}

			// Cannot find window handle
			if (!data.best_handle)
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: could not get window handle");
			}

			// Return the best handle
			t_hWnd = data.best_handle;
		}

		if (MainhWnd && MainhDC && (MainhWnd != t_hWnd))
		{
			ReleaseDC(MainhWnd, MainhDC);
			MainhDC = nullptr;
		}

		MainhWnd = t_hWnd;

		if (MainhWnd && !MainhDC)
		{
			MainhDC = ::GetDC(MainhWnd);
		}

		return DD_OK;
	

	HRESULT hr = ProxyInterface->SetCooperativeLevel(hWnd, dwFlags);

	// Release previouse Exclusive flag
	// Hook window message to get notified when the window is about to exit to remove the exclusive flag
	if (SUCCEEDED(hr) && (dwFlags & DDSCL_EXCLUSIVE) && IsWindow(hWnd) && hWnd != chWnd)
	{
		g_hookmap.clear();

		if (g_hook)
		{
			UnhookWindowsHookEx(g_hook);
			g_hook = nullptr;
		}

		g_hookmap[hWnd] = WrapperInterface;
		g_hook = SetWindowsHookEx(WH_CBT, CBTProc, nullptr, GetWindowThreadProcessId(hWnd, nullptr));

		chWnd = hWnd;
	}

	// Remove hWnd ExclusiveMode
	if (SUCCEEDED(hr) && (dwFlags & DDSCL_NORMAL) && IsWindow(hWnd) && hWnd == chWnd)
	{
		g_hookmap.clear();
		chWnd = nullptr;
	}

	// Remove window border on fullscreen windows 
	// Fixes a bug in ddraw in Windows 8 and 10 where the window border is visible in fullscreen mode
	if (SUCCEEDED(hr) && (dwFlags & DDSCL_FULLSCREEN) && IsWindow(hWnd))
	{
		LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
		if (lStyle & WS_CAPTION)
		{
			Logging::LogDebug() << __FUNCTION__ << " Removing window WS_CAPTION!";

			SetWindowLong(hWnd, GWL_STYLE, lStyle & ~WS_CAPTION);
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING | SWP_FRAMECHANGED);
		}
	}

	return hr;
}

HRESULT m_IDirectDrawX::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		bool ChangeMode = false;

		// Set display mode to dwWidth x dwHeight with dwBPP color depth
		DWORD NewWidth = dwWidth;
		DWORD NewHeight = dwHeight;
		DWORD NewBPP = (Config.DdrawOverrideBitMode) ? Config.DdrawOverrideBitMode : dwBPP;
		DWORD NewRefreshRate = dwRefreshRate;

		if (ResetDisplayMode || displayModeWidth != NewWidth || displayModeHeight != NewHeight || displayModeBPP != NewBPP || displayModeRefreshRate != NewRefreshRate)
		{
			ChangeMode = true;
			displayModeWidth = NewWidth;
			displayModeHeight = NewHeight;
			displayModeBPP = NewBPP;
			displayModeRefreshRate = NewRefreshRate;

			// Display resolution
			if (SetDefaultDisplayMode)
			{
				displayWidth = (Config.DdrawUseNativeResolution || Config.DdrawOverrideWidth) ? displayWidth : displayModeWidth;
				displayHeight = (Config.DdrawUseNativeResolution || Config.DdrawOverrideHeight) ? displayHeight : displayModeHeight;
				displayRefreshRate = (Config.DdrawOverrideRefreshRate) ? displayRefreshRate : displayModeRefreshRate;
			}
		}

		// Update the d3d9 device to use new display mode
		if ((ChangeMode || !d3d9Device) && FAILED(CreateD3D9Device()))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: creating Direct3D9 Device");
			return DDERR_GENERIC;
		}

		ResetDisplayMode = false;

		return DD_OK;
	

	// Force color mode
	dwBPP = (Config.DdrawOverrideBitMode) ? Config.DdrawOverrideBitMode : dwBPP;

	if (ProxyDirectXVersion == 1)
	{
		return GetProxyInterfaceV1()->SetDisplayMode(dwWidth, dwHeight, dwBPP);
	}

	return ProxyInterface->SetDisplayMode(dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);
}

HRESULT m_IDirectDrawX::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		// Check flags
		switch (dwFlags)
		{
		case DDWAITVB_BLOCKBEGIN:
			// Return when vertical blank begins
		case DDWAITVB_BLOCKEND:
			// Return when the vertical blank interval ends and the display begins
			break;
		case DDWAITVB_BLOCKBEGINEVENT:
			// Triggers an event when the vertical blank begins. This value is not supported.
			return DDERR_UNSUPPORTED;
		default:
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true)))
		{
			return DDERR_GENERIC;
		}

		// Do some simple wait
		D3DRASTER_STATUS RasterStatus;
		if (SUCCEEDED(d3d9Device->GetRasterStatus(0, &RasterStatus)) &&
			(!RasterStatus.InVBlank || dwFlags != DDWAITVB_BLOCKBEGIN) &&
			monitorHeight && monitorRefreshRate)
		{
			float percentageLeft = 1.0f - (float)((RasterStatus.InVBlank) ? monitorHeight : RasterStatus.ScanLine) / (float)monitorHeight;
			float blinkTime = trunc(1000.0f / monitorRefreshRate);
			DWORD WaitTime = min(100, (DWORD)trunc(blinkTime * percentageLeft) + ((dwFlags == DDWAITVB_BLOCKBEGIN) ? 0 : 2));
			Sleep(WaitTime);
		}

		// Vertical blank supported by vsync so just return
		return DD_OK;
	

	return ProxyInterface->WaitForVerticalBlank(dwFlags, hEvent);
}

/*********************************/
/*** Added in the v2 interface ***/
/*********************************/

HRESULT m_IDirectDrawX::GetAvailableVidMem(LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Game using old DirectX, Convert DDSCAPS to DDSCAPS2
	if (ProxyDirectXVersion > 3)
	{
		DDSCAPS2 Caps2;
		if (lpDDSCaps)
		{
			ConvertCaps(Caps2, *lpDDSCaps);
		}

		return GetAvailableVidMem2((lpDDSCaps) ? &Caps2 : nullptr, lpdwTotal, lpdwFree);
	}

	HRESULT hr = GetProxyInterfaceV3()->GetAvailableVidMem(lpDDSCaps, lpdwTotal, lpdwFree);

	// Set available memory
	AdjustVidMemory(lpdwTotal, lpdwFree);

	return hr;
}

HRESULT m_IDirectDrawX::GetAvailableVidMem2(LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	HRESULT hr = DD_OK;

	
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true)))
		{
			return DDERR_GENERIC;
		}

		DWORD TotalMemory = d3d9Device->GetAvailableTextureMem();

		if (lpdwTotal)
		{
			*lpdwTotal = TotalMemory;
		}
		if (lpdwFree)
		{
			*lpdwFree = (TotalMemory > 0x100000) ? TotalMemory - 0x100000 : TotalMemory;
		}
	
	

	// Ajdust available memory
	AdjustVidMemory(lpdwTotal, lpdwFree);

	return hr;
}

/*********************************/
/*** Added in the V4 Interface ***/
/*********************************/

HRESULT m_IDirectDrawX::GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7 * lpDDS, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	

	HRESULT hr = ProxyInterface->GetSurfaceFromDC(hdc, lpDDS);

	if (SUCCEEDED(hr) && lpDDS)
	{
		*lpDDS = (LPDIRECTDRAWSURFACE7)ProxyAddressLookupTable.FindAddress<m_IDirectDrawSurface7>(*lpDDS);
	}

	return hr;
}

HRESULT m_IDirectDrawX::RestoreAllSurfaces()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		for (m_IDirectDrawSurfaceX* it : SurfaceVector)
		{
			HRESULT hr = it->Restore();
			if (FAILED(hr))
			{
				return hr;
			}
		}

		return DD_OK;
	
	return ProxyInterface->RestoreAllSurfaces();
}

HRESULT m_IDirectDrawX::TestCooperativeLevel()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!ExclusiveMode)
		{
			return DDERR_NOEXCLUSIVEMODE;
		}

		if (!d3d9Device)
		{
			// Just return OK until device is setup
			return DD_OK;
		}

		switch (d3d9Device->TestCooperativeLevel())
		{
		case D3DERR_DRIVERINTERNALERROR:
		case D3DERR_INVALIDCALL:
			return DDERR_WRONGMODE;
		case D3DERR_DEVICELOST:
			return DDERR_NOEXCLUSIVEMODE;
		case D3DERR_DEVICENOTRESET:
		default:
			return DD_OK;
		}
	

	return ProxyInterface->TestCooperativeLevel();
}

HRESULT m_IDirectDrawX::GetDeviceIdentifier(LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (ProxyDirectXVersion > 4)
	{
		if (!lpdddi)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDDEVICEIDENTIFIER2 Id2;

		HRESULT hr = GetDeviceIdentifier2(&Id2, dwFlags);

		if (SUCCEEDED(hr))
		{
			ConvertDeviceIdentifier(*lpdddi, Id2);
		}

		return hr;
	}

	return GetProxyInterfaceV4()->GetDeviceIdentifier(lpdddi, dwFlags);
}

HRESULT m_IDirectDrawX::GetDeviceIdentifier2(LPDDDEVICEIDENTIFIER2 lpdddi2, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		if (!lpdddi2)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false)))
		{
			return DDERR_GENERIC;
		}

		D3DADAPTER_IDENTIFIER9 Identifier9;
		HRESULT hr = d3d9Object->GetAdapterIdentifier(D3DADAPTER_DEFAULT, D3DENUM_WHQL_LEVEL, &Identifier9);

		if (FAILED(hr))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to get Adapter Identifier");
			return hr;
		}

		ConvertDeviceIdentifier(*lpdddi2, Identifier9);

		return DD_OK;
	

	return ProxyInterface->GetDeviceIdentifier(lpdddi2, dwFlags);
}

HRESULT m_IDirectDrawX::StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	

	return ProxyInterface->StartModeTest(lpModesToTest, dwNumEntries, dwFlags);
}

HRESULT m_IDirectDrawX::EvaluateMode(DWORD dwFlags, DWORD * pSecondsUntilTimeout)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	

	return ProxyInterface->EvaluateMode(dwFlags, pSecondsUntilTimeout);
}

/************************/
/*** Helper functions ***/
/************************/

LPDIRECT3D9 m_IDirectDrawX::GetDirect3D9Object()
{
	return d3d9Object;
}

LPDIRECT3DDEVICE9* m_IDirectDrawX::GetDirect3D9Device()
{
	return &d3d9Device;
}

void m_IDirectDrawX::InitDdrawSettings()
{
	DWORD ref = InterlockedIncrement(&ddrawRefCount);

	SetCriticalSection();

	// Check interface to create d3d9 object
	CheckInterface(__FUNCTION__, false);

	DDrawVector.push_back(this);

	if (ref == 1)
	{
		SetDdrawDefaults();
		m_IDirectDrawSurfaceX::StartSharedEmulatedMemory();
	}

	ReleaseCriticalSection();
}

void m_IDirectDrawX::SetDdrawDefaults()
{
	// Convert to Direct3D9
	IsInScene = false;
	AllowModeX = false;
	MultiThreaded = false;
	FUPPreserve = false;
	NoWindowChanges = false;
	isWindowed = true;

	// Exclusive mode
	ExclusiveMode = false;
	ExclusiveHwnd = nullptr;

	// Application display mode
	if (MainhWnd && MainhDC)
	{
		ReleaseDC(MainhWnd, MainhDC);
	}
	MainhWnd = nullptr;
	MainhDC = nullptr;
	displayModeWidth = 0;
	displayModeHeight = 0;
	displayModeBPP = 0;
	displayModeRefreshRate = 0;
	ResetDisplayMode = false;

	// Display resolution
	displayWidth = (Config.DdrawUseNativeResolution) ? GetSystemMetrics(SM_CXSCREEN) : (Config.DdrawOverrideWidth) ? Config.DdrawOverrideWidth : 0;
	displayHeight = (Config.DdrawUseNativeResolution) ? GetSystemMetrics(SM_CYSCREEN) : (Config.DdrawOverrideHeight) ? Config.DdrawOverrideHeight : 0;
	displayRefreshRate = (Config.DdrawOverrideRefreshRate) ? Config.DdrawOverrideRefreshRate : 0;

	SetDefaultDisplayMode = (!displayWidth || !displayHeight || !displayRefreshRate);

	// Other settings
	monitorRefreshRate = 0;
	monitorHeight = 0;
	FrequencyFlag = (QueryPerformanceFrequency(&clockFrequency) != 0);
	ZeroMemory(&presParams, sizeof(presParams));
}

void m_IDirectDrawX::ReleaseDdraw()
{
	DWORD ref = InterlockedDecrement(&ddrawRefCount);

	SetCriticalSection();

	// Remove ddraw device from vector
	auto it = std::find_if(DDrawVector.begin(), DDrawVector.end(),
		[=](auto pDDraw) -> bool { return pDDraw == this; });

	if (it != std::end(DDrawVector))
	{
		DDrawVector.erase(it);
	}

	// Release Direct3DDevice interfaces
	if (D3DDeviceInterface)
	{
		ddrawParent = nullptr;
		D3DDeviceInterface = nullptr;
	}

	// Release Direct3D interfaces
	if (D3DInterface)
	{
		ddrawParent = nullptr;
		D3DInterface = nullptr;
	}

	// Release surfaces
	for (m_IDirectDrawSurfaceX* pSurface : SurfaceVector)
	{
		pSurface->ReleaseD9Surface();
		pSurface->ClearDdraw();
	}
	SurfaceVector.clear();

	// Release palettes
	for (m_IDirectDrawPalette* pPalette : PaletteVector)
	{
		pPalette->ClearDdraw();
	}
	PaletteVector.clear();

	if (ref == 0)
	{
		// Release shared d3d9device
		ReleaseD3d9Device();

		// Release shared d3d9object
		d3d9Object->Release();
		d3d9Object = nullptr;

		// Release DC
		if (MainhWnd && MainhDC)
		{
			ReleaseDC(MainhWnd, MainhDC);
			MainhDC = nullptr;
		}

		// Clean up shared memory
		m_IDirectDrawSurfaceX::CleanupSharedEmulatedMemory();
	}

	ReleaseCriticalSection();
}

HWND m_IDirectDrawX::GetHwnd()
{
	return MainhWnd;
}

HDC m_IDirectDrawX::GetDC()
{
	return MainhDC;
}

bool m_IDirectDrawX::IsExclusiveMode()
{
	return ExclusiveMode;
}

HRESULT m_IDirectDrawX::CheckInterface(char* FunctionName, bool CheckD3DDevice)
{
	// Check for device
	if (!d3d9Object)
	{
		// Declare Direct3DCreate9
		static PFN_Direct3DCreate9 Direct3DCreate9 = reinterpret_cast<PFN_Direct3DCreate9>(Direct3DCreate9_out);

		if (!Direct3DCreate9)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to get 'Direct3DCreate9' ProcAddress of d3d9.dll!");
			return DDERR_GENERIC;
		}

		d3d9Object = Direct3DCreate9(D3D_SDK_VERSION);

		// Error creating Direct3D9
		if (!d3d9Object)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: d3d9 object not setup!");
			return DDERR_GENERIC;
		}
	}

	// Check for device, if not then create it
	if (CheckD3DDevice)
	{
		if (!d3d9Device)
		{
			if (FAILED(CreateD3D9Device()))
			{
				LOG_LIMIT(100, FunctionName << " Error: d3d9 device not setup!");
				return DDERR_GENERIC;
			}
		}
	}

	return DD_OK;
}

// Creates d3d9 device, destroying the old one if exists
HRESULT m_IDirectDrawX::CreateD3D9Device()
{
	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, false)))
	{
		return DDERR_GENERIC;
	}

	// Release all existing surfaces
	ReleaseAllD9Surfaces();

	// Release existing d3d9device
	ReleaseD3d9Device();

	// Check device caps to make sure it supports dynamic textures
	D3DCAPS9 d3dcaps;
	ZeroMemory(&d3dcaps, sizeof(D3DCAPS9));
	if (FAILED(d3d9Object->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dcaps)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to retrieve device-specific information about the device");
		return DDERR_GENERIC;
	}

	// Is dynamic textures flag set?
	if (!(d3dcaps.Caps2 & D3DCAPS2_DYNAMICTEXTURES))
	{
		// No dynamic textures
		LOG_LIMIT(100, __FUNCTION__ << " Error: device does not support dynamic textures");
		return DDERR_GENERIC;
	}

	// Get width and height
	DWORD BackBufferWidth = displayWidth;
	DWORD BackBufferHeight = displayHeight;
	if (!BackBufferWidth || !BackBufferHeight)
	{
		BackBufferWidth = GetSystemMetrics(SM_CXSCREEN);
		BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);
	}

	// Set display window
	ZeroMemory(&presParams, sizeof(presParams));

	// Discard swap
	presParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	// Backbuffer
	presParams.BackBufferCount = 1;
	// Auto stencel
	presParams.EnableAutoDepthStencil = true;
	// Auto stencel format
	presParams.AutoDepthStencilFormat = D3DFMT_D24S8;
	// Interval level
	presParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Set parameters for the current display mode
	if (isWindowed || !MainhWnd)
	{
		// Window mode
		presParams.Windowed = TRUE;
		// Width/height
		presParams.BackBufferWidth = BackBufferWidth;
		presParams.BackBufferHeight = BackBufferHeight;
	}
	else
	{
		// Enumerate modes for format XRGB
		UINT modeCount = d3d9Object->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);

		// Check for ModeX resolutions
		if ((BackBufferWidth == 320 && BackBufferHeight == 200) ||
			(BackBufferWidth == 640 && BackBufferHeight == 400))
		{
			BackBufferHeight += BackBufferHeight / 5;
		}

		// Get refresh rate
		DWORD BackBufferRefreshRate = (displayRefreshRate) ? displayRefreshRate : Utils::GetRefreshRate(MainhWnd);

		// Loop through all modes looking for our requested resolution
		D3DDISPLAYMODE d3ddispmode;
		D3DDISPLAYMODE set_d3ddispmode = { NULL };
		bool modeFound = false;
		for (UINT i = 0; i < modeCount; i++)
		{
			// Get display modes here
			ZeroMemory(&d3ddispmode, sizeof(D3DDISPLAYMODE));
			if (FAILED(d3d9Object->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &d3ddispmode)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: EnumAdapterModes failed");
				return DDERR_GENERIC;
			}
			if (d3ddispmode.Width == BackBufferWidth && d3ddispmode.Height == BackBufferHeight &&		// Check height and width
				(d3ddispmode.RefreshRate == BackBufferRefreshRate || !BackBufferRefreshRate))			// Check refresh rate
			{
				// Found a match
				modeFound = true;
				memcpy(&set_d3ddispmode, &d3ddispmode, sizeof(D3DDISPLAYMODE));
			}
		}

		// No mode found
		if (!modeFound)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to find compatible fullscreen display mode " << BackBufferWidth << "x" << BackBufferHeight);
			return DDERR_GENERIC;
		}

		// Fullscreen
		presParams.Windowed = FALSE;
		// Width/height
		presParams.BackBufferWidth = set_d3ddispmode.Width;
		presParams.BackBufferHeight = set_d3ddispmode.Height;
		// Backbuffer
		presParams.BackBufferFormat = set_d3ddispmode.Format;
		// Display mode refresh
		presParams.FullScreen_RefreshRateInHz = set_d3ddispmode.RefreshRate;
	}

	// Set behavior flags
	DWORD BehaviorFlags = ((d3dcaps.VertexProcessingCaps) ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING) | D3DCREATE_MULTITHREADED |
		((MultiThreaded) ? D3DCREATE_MULTITHREADED : 0) |
		((FUPPreserve) ? D3DCREATE_FPU_PRESERVE : 0) |
		((NoWindowChanges) ? D3DCREATE_NOWINDOWCHANGES : 0);

	Logging::LogDebug() << __FUNCTION__ << " wnd: " << MainhWnd << " D3d9 Device size: " << presParams << " flags: " << BehaviorFlags;

	// Create d3d9 Device
	if (FAILED(d3d9Object->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, MainhWnd, BehaviorFlags, &presParams, &d3d9Device)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to create Direct3D9 device! " << presParams.BackBufferWidth << "x" << presParams.BackBufferHeight << " refresh: " << presParams.FullScreen_RefreshRateInHz <<
			" format: " << presParams.BackBufferFormat);
		return DDERR_GENERIC;
	}

	// Store display frequency
	monitorRefreshRate = (presParams.FullScreen_RefreshRateInHz) ? presParams.FullScreen_RefreshRateInHz : Utils::GetRefreshRate(MainhWnd);
	monitorHeight = GetSystemMetrics(SM_CYSCREEN);

	// Reset BeginScene
	IsInScene = false;

	// Success
	return DD_OK;
}

// Reinitialize d3d9 device
HRESULT m_IDirectDrawX::ReinitDevice()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true)))
	{
		return DDERR_GENERIC;
	}

	// Check if device is ready to be restored
	if (d3d9Device->TestCooperativeLevel() != D3DERR_DEVICENOTRESET)
	{
		return DD_OK;
	}

	// Release surfaces to prepare for reset
	ReleaseAllD9Surfaces(true);

	// Attempt to reset the device
	if (FAILED(d3d9Device->Reset(&presParams)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to reset Direct3D9 device");
		return DDERR_GENERIC;
	}

	// Reset BeginScene
	IsInScene = false;

	// Success
	return DD_OK;
}

// Release all surfaces from all ddraw devices
void m_IDirectDrawX::ReleaseAllDirectDrawD9Surfaces()
{
	SetCriticalSection();
	for (m_IDirectDrawX* pDDraw : DDrawVector)
	{
		pDDraw->ReleaseAllD9Surfaces(false);
	}
	ReleaseCriticalSection();
}

// Release all d3d9 surfaces
void m_IDirectDrawX::ReleaseAllD9Surfaces(bool CriticalSection)
{
	if (CriticalSection)
	{
		SetCriticalSection();
	}

	for (m_IDirectDrawSurfaceX* pSurface : SurfaceVector)
	{
		pSurface->ReleaseD9Surface();
	}

	if (CriticalSection)
	{
		ReleaseCriticalSection();
	}
}

// Release all d3d9 classes for Release()
void m_IDirectDrawX::ReleaseD3d9Device()
{
	// Release device
	if (d3d9Device)
	{
		// EndEcene
		if (IsInScene)
		{
			d3d9Device->EndScene();
		}

		d3d9Device->Release();
		d3d9Device = nullptr;
	}

	// Set is not in scene
	IsInScene = false;
}

// Add surface wrapper to vector
void m_IDirectDrawX::AddSurfaceToVector(m_IDirectDrawSurfaceX * lpSurfaceX)
{
	if (!lpSurfaceX || DoesSurfaceExist(lpSurfaceX))
	{
		return;
	}

	// Store surface
	SurfaceVector.push_back(lpSurfaceX);
}

// Remove surface wrapper from vector
void m_IDirectDrawX::RemoveSurfaceFromVector(m_IDirectDrawSurfaceX * lpSurfaceX)
{
	if (!lpSurfaceX)
	{
		return;
	}

	auto it = std::find_if(SurfaceVector.begin(), SurfaceVector.end(),
		[=](auto pSurface) -> bool { return pSurface == lpSurfaceX; });

	if (it != std::end(SurfaceVector))
	{
		SurfaceVector.erase(it);

		// Clear primary surface
		if (lpSurfaceX == PrimarySurface)
		{
			PrimarySurface = nullptr;
		}
	}

	// Remove attached surface from map
	for (m_IDirectDrawSurfaceX* pSurface : SurfaceVector)
	{
		pSurface->RemoveAttachedSurfaceFromMap(lpSurfaceX);
	}
}

// Check if surface wrapper exists
bool m_IDirectDrawX::DoesSurfaceExist(m_IDirectDrawSurfaceX * lpSurfaceX)
{
	if (!lpSurfaceX)
	{
		return false;
	}

	auto it = std::find_if(SurfaceVector.begin(), SurfaceVector.end(),
		[=](auto pSurface) -> bool { return pSurface == lpSurfaceX; });

	if (it == std::end(SurfaceVector))
	{
		return false;
	}

	return true;
}

// This method removes any texture surfaces created with the DDSCAPS2_TEXTUREMANAGE or DDSCAPS2_D3DTEXTUREMANAGE flags
void m_IDirectDrawX::EvictManagedTextures()
{
	// Check if any surfaces are locked
	for (m_IDirectDrawSurfaceX* pSurface : SurfaceVector)
	{
		if (pSurface->IsSurfaceManaged())
		{
			pSurface->ReleaseD9Surface();
		}
	}
}

// Add palette wrapper to vector
void m_IDirectDrawX::AddPaletteToVector(m_IDirectDrawPalette * lpPalette)
{
	if (!lpPalette || DoesPaletteExist(lpPalette))
	{
		return;
	}

	// Store palette
	PaletteVector.push_back(lpPalette);
}

// Remove palette wrapper from vector
void m_IDirectDrawX::RemovePaletteFromVector(m_IDirectDrawPalette * lpPalette)
{
	if (!lpPalette)
	{
		return;
	}

	auto it = std::find_if(PaletteVector.begin(), PaletteVector.end(),
		[=](auto pPalette) -> bool { return pPalette == lpPalette; });

	if (it != std::end(PaletteVector))
	{
		PaletteVector.erase(it);
	}
}

// Check if palette wrapper exists
bool m_IDirectDrawX::DoesPaletteExist(m_IDirectDrawPalette * lpPalette)
{
	if (!lpPalette)
	{
		return false;
	}

	auto it = std::find_if(PaletteVector.begin(), PaletteVector.end(),
		[=](auto pSurface) -> bool { return pSurface == lpPalette; });

	if (it == std::end(PaletteVector))
	{
		return false;
	}

	return true;
}

// Adjusts available memory, some games have issues if this is set to high
void m_IDirectDrawX::AdjustVidMemory(LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	if (lpdwTotal && *lpdwTotal > MaxVidMemory)
	{
		*lpdwTotal = MaxVidMemory;
	}
	DWORD TotalVidMem = (lpdwTotal) ? *lpdwTotal : (lpdwFree) ? *lpdwFree : MaxVidMemory;
	if (lpdwFree && *lpdwFree > TotalVidMem)
	{
		*lpdwFree = (TotalVidMem > 0x100000) ? TotalVidMem - 0x100000 : TotalVidMem;
	}
}

// Do d3d9 BeginScene if all surfaces are unlocked
HRESULT m_IDirectDrawX::BeginScene()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Check if we can run BeginScene
	if (IsInScene)
	{
		return DDERR_GENERIC;
	}

	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true)))
	{
		return DDERR_GENERIC;
	}

	// Check if any surfaces are locked
	for (m_IDirectDrawSurfaceX* it : SurfaceVector)
	{
		if (it->IsSurfaceLocked())
		{
			return DDERR_LOCKEDSURFACES;
		}
	}

	// Begin scene
	if (FAILED(d3d9Device->BeginScene()))
	{
		d3d9Device->EndScene();
		if (FAILED(d3d9Device->BeginScene()))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to begin scene");
			return DDERR_GENERIC;
		}
	}
	IsInScene = true;

	return DD_OK;
}

// Do d3d9 EndScene and Present if all surfaces are unlocked
HRESULT m_IDirectDrawX::EndScene()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Run BeginScene (ignore results)
	BeginScene();

	// Check if BeginScene has finished
	if (!IsInScene)
	{
		return DDERR_GENERIC;
	}

	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true)))
	{
		return DDERR_GENERIC;
	}

	// Draw primitive
	if (FAILED(d3d9Device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to draw primitive");
		return DDERR_GENERIC;
	}

	// Skip frame if time lapse is too small
	if (Config.AutoFrameSkip)
	{
		if (FrequencyFlag)
		{
			FrameCounter++;

			// Get screen frequency timer
			float MaxScreenTimer = (1000.0f / monitorRefreshRate);

			// Get time since last successful endscene
			bool CounterFlag = (QueryPerformanceCounter(&clickTime) != 0);
			float deltaPresentMS = ((clickTime.QuadPart - lastPresentTime.QuadPart) * 1000.0f) / clockFrequency.QuadPart;

			// Get time since last skipped frame
			float deltaFrameMS = (lastFrameTime) ? ((clickTime.QuadPart - lastFrameTime) * 1000.0f) / clockFrequency.QuadPart : deltaPresentMS;
			lastFrameTime = clickTime.QuadPart;

			// Use last frame time and average frame time to decide if next frame will be less than the screen frequency timer
			if (CounterFlag && (deltaPresentMS + (deltaFrameMS * 1.1f) < MaxScreenTimer) && (deltaPresentMS + ((deltaPresentMS / FrameCounter) * 1.1f) < MaxScreenTimer))
			{
				Logging::LogDebug() << __func__ << " Skipping frame " << deltaPresentMS << "ms screen frequancy " << MaxScreenTimer;
				return D3D_OK;
			}
			Logging::LogDebug() << __func__ << " Drawing frame " << deltaPresentMS << "ms screen frequancy " << MaxScreenTimer;
		}
	}

	// End scene
	if (FAILED(d3d9Device->EndScene()))
	{
		d3d9Device->BeginScene();
		if (FAILED(d3d9Device->EndScene()))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to end scene");
			return DDERR_GENERIC;
		}
	}
	IsInScene = false;

	// Present everthing
	HRESULT hr = d3d9Device->Present(nullptr, nullptr, nullptr, nullptr);

	// Device lost
	if (hr == D3DERR_DEVICELOST)
	{
		// Attempt to reinit device
		hr = ReinitDevice();
	}
	else if (FAILED(hr))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to present scene");
		return DDERR_GENERIC;
	}

	// Store new click time after frame draw is complete
	if (Config.AutoFrameSkip)
	{
		if (QueryPerformanceCounter(&clickTime))
		{
			lastPresentTime.QuadPart = clickTime.QuadPart;
			lastFrameTime = 0;
			FrameCounter = 0;
		}
	}

	// BeginScene after EndScene is done
	BeginScene();

	return DD_OK;
}
