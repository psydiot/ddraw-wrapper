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
* Code initial taken from: https://github.com/strangebytes/diablo-ddrawwrapper
*
* Code to create emulated surface taken from: https://github.com/CnCNet/cnc-ddraw
*/

#include "IDirectDrawSurfaceX.h"
#include "Versions/IDirectDrawSurface7.h"
#include "Versions/IDirectDrawSurface4.h"
#include "Versions/IDirectDrawSurface3.h"
#include "Versions/IDirectDrawSurface2.h"
#include "Versions/IDirectDrawSurface.h"
#include "IDirectDrawEnumCallback.h"
#include "IDirectDrawTypes.h"
const BYTE PalettePixelShaderSrc[] =
{
	  0,   2, 255, 255, 254, 255,
	 44,   0,  67,  84,  65,  66,
	 28,   0,   0,   0, 131,   0,
	  0,   0,   0,   2, 255, 255,
	  2,   0,   0,   0,  28,   0,
	  0,   0,   0,   1,   0,   0,
	124,   0,   0,   0,  68,   0,
	  0,   0,   3,   0,   1,   0,
	  1,   0,   0,   0,  80,   0,
	  0,   0,   0,   0,   0,   0,
	 96,   0,   0,   0,   3,   0,
	  0,   0,   1,   0,   0,   0,
	108,   0,   0,   0,   0,   0,
	  0,   0,  80,  97, 108, 101,
	116, 116, 101,  84, 101, 120,
	  0, 171,   4,   0,  12,   0,
	  1,   0,   1,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	 83, 117, 114, 102,  97,  99,
	101,  84, 101, 120,   0, 171,
	  4,   0,  12,   0,   1,   0,
	  1,   0,   1,   0,   0,   0,
	  0,   0,   0,   0, 112, 115,
	 95,  50,  95,  48,   0,  77,
	105,  99, 114, 111, 115, 111,
	102, 116,  32,  40,  82,  41,
	 32,  72,  76,  83,  76,  32,
	 83, 104,  97, 100, 101, 114,
	 32,  67, 111, 109, 112, 105,
	108, 101, 114,  32,  49,  48,
	 46,  49,   0, 171,  81,   0,
	  0,   5,   0,   0,  15, 160,
	  0,   0, 127,  63,   0,   0,
	  0,  59,   0,   0,   0,   0,
	  0,   0,   0,   0,  31,   0,
	  0,   2,   0,   0,   0, 128,
	  0,   0,   3, 176,  31,   0,
	  0,   2,   0,   0,   0, 144,
	  0,   8,  15, 160,  31,   0,
	  0,   2,   0,   0,   0, 144,
	  1,   8,  15, 160,  66,   0,
	  0,   3,   0,   0,  15, 128,
	  0,   0, 228, 176,   0,   8,
	228, 160,   4,   0,   0,   4,
	  0,   0,   1, 128,   0,   0,
	  0, 128,   0,   0,   0, 160,
	  0,   0,  85, 160,   1,   0,
	  0,   2,   0,   0,   2, 128,
	  0,   0, 170, 160,  66,   0,
	  0,   3,   0,   0,  15, 128,
	  0,   0, 228, 128,   1,   8,
	228, 160,   1,   0,   0,   2,
	  0,   8,  15, 128,   0,   0,
	228, 128, 255, 255,   0,   0
};
// Used to allow presenting non-primary surfaces in case the primary surface present fails
bool SceneReady = false;
bool IsPresentRunning = false;
std::unique_ptr<m_IDirect3DDevice9Ex> UniqueProxyInterface = nullptr;
std::unique_ptr<m_IDirect3DDevice9Ex> UniqueProxyInterface2 = nullptr;
std::unique_ptr<m_IDirect3DDevice9Ex> UniqueProxyInterface3 = nullptr;
std::unique_ptr<m_IDirect3DDevice9Ex> UniqueProxyInterface7 = nullptr;
// Used for sharing emulated memory
bool ShareEmulatedMemory = false;
CRITICAL_SECTION smcs;
std::vector<EMUSURFACE*> memorySurfaces;

/************************/
void* m_IDirect3DDevice9Ex::GetWrapperInterfaceX(DWORD DirectXVersion)
{
	switch (DirectXVersion)
	{
	case 1:
		if (!UniqueProxyInterface.get())
		{
			UniqueProxyInterface = std::make_unique<m_IDirect3DDevice9Ex>(this,nullptr);
		}
		return UniqueProxyInterface.get();
	
	default:
		LOG_LIMIT(100, __FUNCTION__ << " Error: wrapper interface version not found: " << DirectXVersion);
		return nullptr;
	}
}/************************/

HRESULT m_IDirectDrawSurfaceX::QueryInterface(REFIID riid, LPVOID FAR * ppvObj, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if ((riid == IID_IDirectDrawSurface || riid == IID_IDirectDrawSurface2 || riid == IID_IDirectDrawSurface3 || riid == IID_IDirectDrawSurface4 || riid == IID_IDirectDrawSurface7 || riid == IID_IUnknown) && ppvObj)
		{
			DWORD DxVersion = (riid == IID_IUnknown) ? DirectXVersion : GetIIDVersion(riid);

			*ppvObj = GetWrapperInterfaceX(DxVersion);

			::AddRef(*ppvObj);

			return DD_OK;
		}
		if ((riid == IID_IDirect3DDevice9 || riid == IID_IDirect3DDevice9 || riid == IID_IDirect3DDevice9 || riid == IID_IDirect3DDevice9) && ppvObj)
		{
			// Check for device interface
			if (FAILED(CheckInterface(__FUNCTION__, false, false)))
			{
				return DDERR_GENERIC;
			}

			m_IDirect3DDevice9Ex*D3DDeviceX = *ddrawParent->GetCurrentD3DDevice();

			if (D3DDeviceX)
			{
				*ppvObj = D3DDeviceX->GetWrapperInterfaceX(DirectXVersion);

				::AddRef(*ppvObj);

				return DD_OK;
			}

			m_IDirect3D9Ex *D3DX = *ddrawParent->GetCurrentD3D();

			if (D3DX)
			{
				//D3DX->CreateDevice(riid, (LPDIRECTDRAWSURFACE7)this, (LPDIRECT3DDEVICE9*)&D3DDeviceX, nullptr, DirectXVersion);
				D3DX->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, NULL,(LPDIRECT3DDEVICE9*)& D3DDeviceX);
				*ppvObj = D3DDeviceX;

				return DD_OK;
			}

			LOG_LIMIT(100, __FUNCTION__ << " Query failed for " << riid << " from " << GetWrapperType(DirectXVersion));

			return E_NOINTERFACE;
		}
	}

	if (Config.ConvertToDirect3D7 && (riid == IID_IDirect3DTexture9 || riid == IID_IDirect3DTexture9) && ppvObj)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false, false)))
		{
			return DDERR_GENERIC;
		}

	
		*ppvObj = new m_IDirect3DTexture9((LPDIRECT3DTEXTURE9)ProxyInterface, *ddrawParent->GetCurrentD3DDevice());
		return DD_OK;
	}

	return ProxyQueryInterface(ProxyInterface, riid, ppvObj, GetWrapperType(DirectXVersion), WrapperInterface);
}

void *m_IDirectDrawSurfaceX::GetWrapperInterfaceX(DWORD DirectXVersion)
{
	switch (DirectXVersion)
	{
	case 1:
		if (!UniqueProxyInterface.get())
		{
			UniqueProxyInterface = std::make_unique<m_IDirectDrawSurface>(this);
		}
		return UniqueProxyInterface.get();
	case 2:
		if (!UniqueProxyInterface2.get())
		{
			UniqueProxyInterface2 = std::make_unique<m_IDirectDrawSurface2>(this);
		}
		return UniqueProxyInterface2.get();
	case 3:
		if (!UniqueProxyInterface3.get())
		{
			UniqueProxyInterface3 = std::make_unique<m_IDirectDrawSurface3>(this);
		}
		return UniqueProxyInterface3.get();
	case 4:
		if (!UniqueProxyInterface4.get())
		{
			UniqueProxyInterface4 = std::make_unique<m_IDirectDrawSurface4>(this);
		}
		return UniqueProxyInterface4.get();
	case 7:
		if (!UniqueProxyInterface7.get())
		{
			UniqueProxyInterface7 = std::make_unique<m_IDirectDrawSurface7>(this);
		}
		return UniqueProxyInterface7.get();
	default:
		LOG_LIMIT(100, __FUNCTION__ << " Error: wrapper interface version not found: " << DirectXVersion);
		return nullptr;
	}
}

ULONG m_IDirectDrawSurfaceX::AddRef()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		return InterlockedIncrement(&RefCount);
	}

	return ProxyInterface->AddRef();
}

ULONG m_IDirectDrawSurfaceX::Release()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	ULONG ref;

	if (Config.Dd7to9)
	{
		ref = InterlockedDecrement(&RefCount);
	}
	else
	{
		ref = ProxyInterface->Release();
	}

	// A complex structure can be destroyed only by destroying the root
	if (ref == 0 && !CanSurfaceBeDeleted())
	{
		InterlockedIncrement(&RefCount);
	}
	else if (ref == 0)
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

/**********************************/
/*** IDirectDrawSurface methods ***/
/**********************************/

HRESULT m_IDirectDrawSurfaceX::AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSurface)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpDDSurface)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false, false)))
		{
			return DDERR_GENERIC;
		}

		m_IDirectDrawSurfaceX *lpAttachedSurface = ((m_IDirectDrawSurface*)lpDDSurface)->GetWrapperInterface();

		if (lpAttachedSurface == this)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: cannot attach self");
			return DDERR_CANNOTATTACHSURFACE;
		}

		if (!ddrawParent->DoesSurfaceExist(lpAttachedSurface))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: invalid surface!");
			return DDERR_INVALIDPARAMS;
		}

		if (DoesAttachedSurfaceExist(lpAttachedSurface))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: attached surface already exists");
			return DDERR_SURFACEALREADYATTACHED;
		}

		DWORD AttachedSurfaceCaps = lpAttachedSurface->GetSurfaceCaps().dwCaps;
		if (!(((AttachedSurfaceCaps & DDSCAPS_BACKBUFFER) && (surfaceDesc2.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)) ||
			((AttachedSurfaceCaps & DDSCAPS_FRONTBUFFER) && (surfaceDesc2.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)) ||
			((AttachedSurfaceCaps & DDSCAPS_MIPMAP) && (surfaceDesc2.ddsCaps.dwCaps & DDSCAPS_MIPMAP)) ||
			(AttachedSurfaceCaps & DDSCAPS_ZBUFFER)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: cannot attach surface with this method. dwCaps: " << lpAttachedSurface->GetSurfaceCaps().dwCaps);
			return DDERR_CANNOTATTACHSURFACE;
		}

		AddAttachedSurfaceToMap(lpAttachedSurface, true);

		lpAttachedSurface->AddRef();

		return DD_OK;
	}

	if (lpDDSurface)
	{
		lpDDSurface = static_cast<m_IDirectDrawSurface7 *>(lpDDSurface)->GetProxyInterface();
	}

	return ProxyInterface->AddAttachedSurface(lpDDSurface);
}

HRESULT m_IDirectDrawSurfaceX::AddOverlayDirtyRect(LPRECT lpRect)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	}

	return ProxyInterface->AddOverlayDirtyRect(lpRect);
}

HRESULT m_IDirectDrawSurfaceX::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx, BOOL isSkipScene)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for required DDBLTFX structure
		if (!lpDDBltFx && (dwFlags & (DDBLT_DDFX | DDBLT_COLORFILL | DDBLT_DEPTHFILL | DDBLT_DDROPS | DDBLT_KEYDESTOVERRIDE | DDBLT_KEYSRCOVERRIDE | DDBLT_ROP | DDBLT_ROTATIONANGLE)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: DDBLTFX structure not found");
			return DDERR_INVALIDPARAMS;
		}

		// Check for non-Win32 raster operations flag
		if (dwFlags & DDBLT_DDROPS)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Non-Win32 raster operations Not Implemented ");
			return DDERR_NODDROPSHW;
		}

		// Check for depth fill flag
		if (dwFlags & DDBLT_DEPTHFILL)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Depth Fill Not Implemented");
			return DDERR_NOZBUFFERHW;
		}

		// Check for ZBuffer flags
		if ((dwFlags & DDBLT_ZBUFFER) || ((dwFlags & DDBLT_DDFX) && (lpDDBltFx->dwDDFX & (DDBLTFX_ZBUFFERBASEDEST | DDBLTFX_ZBUFFERRANGE))))
		{
			LOG_LIMIT(100, __FUNCTION__ << " ZBuffer Not Implemented");
			return DDERR_NOZBUFFERHW;
		}

		// Check for rotation flags
		if (((dwFlags & DDBLT_DDFX) && (lpDDBltFx->dwDDFX & DDBLTFX_ROTATE180)))
		{
			// 180 degree rotate is the same as rotating both left to right and up to down
			dwFlags |= DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
		}
		else if ((dwFlags & DDBLT_ROTATIONANGLE) || ((dwFlags & DDBLT_DDFX) && (lpDDBltFx->dwDDFX & (DDBLTFX_ROTATE90 | DDBLTFX_ROTATE270))))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Rotation operations Not Implemented");
			return DDERR_NOROTATIONHW;
		}

		// Unused flags (can be safely ignored?)
		// DDBLT_ALPHA
		// DDBLT_ASYNC
		// DDBLT_DONOTWAIT
		// DDBLT_WAIT
		// DDBLTFX_NOTEARING

		// Use WaitForVerticalBlank for wait timer
		if ((dwFlags & DDBLT_DDFX) && (lpDDBltFx->dwDDFX & DDBLTFX_NOTEARING))
		{
			ddrawParent->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, nullptr);
		}

		HRESULT hr = DD_OK;
		m_IDirectDrawSurfaceX *lpDDSrcSurfaceX = nullptr;
		do {
			IsInBlt = true;

			// Check if the scene needs to be presented
			isSkipScene |= ((lpDestRect) ? (abs(lpDestRect->bottom - lpDestRect->top) < 2 || abs(lpDestRect->right - lpDestRect->left) < 2) : FALSE);

			// Do color fill
			if (dwFlags & DDBLT_COLORFILL)
			{
				hr = ColorFill(lpDestRect, lpDDBltFx->dwFillColor);
				break;
			}

			// Do supported raster operations
			if (dwFlags & DDBLT_ROP)
			{
				if (lpDDBltFx->dwROP == SRCCOPY)
				{
					// Do nothing
				}
				else if (lpDDBltFx->dwROP == BLACKNESS)
				{
					hr = ColorFill(lpDestRect, 0x00000000);
					break;
				}
				else if (lpDDBltFx->dwROP == WHITENESS)
				{
					hr = ColorFill(lpDestRect, 0xFFFFFFFF);
					break;
				}
				else
				{
					LOG_LIMIT(100, __FUNCTION__ << " Raster operation Not Implemented " << lpDDBltFx->dwROP);
					hr = DDERR_NORASTEROPHW;
					break;
				}
			}

			// Get source surface
			lpDDSrcSurfaceX = (m_IDirectDrawSurfaceX*)lpDDSrcSurface;
			if (!lpDDSrcSurfaceX)
			{
				lpDDSrcSurfaceX = this;
			}
			else
			{
				lpDDSrcSurfaceX = ((m_IDirectDrawSurface*)lpDDSrcSurfaceX)->GetWrapperInterface();

				// Check if source Surface exists
				if (!ddrawParent || !ddrawParent->DoesSurfaceExist(lpDDSrcSurfaceX))
				{
					LOG_LIMIT(100, __FUNCTION__ << " Error: could not find source surface");
					hr = DDERR_INVALIDOBJECT;
					break;
				}
			}

			// Get surface copy flags
			DWORD Flags = ((dwFlags & (DDBLT_KEYDESTOVERRIDE | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYDEST | DDBLT_KEYSRC)) ? DDBLT_KEYDEST : 0) |		// Color key flags
				((lpDDBltFx && (dwFlags & DDBLT_DDFX)) ? (lpDDBltFx->dwDDFX & (DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN)) : 0);			// Mirror flags

			// Check if color key is set
			if (((dwFlags & DDBLT_KEYDEST) && !ColorKeys[0].IsSet) || ((dwFlags & DDBLT_KEYSRC) && !lpDDSrcSurfaceX->ColorKeys[2].IsSet))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: color key not set");
				Flags &= ~(DDBLT_KEYDESTOVERRIDE | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYDEST | DDBLT_KEYSRC);
			}

			// Get color key
			DDCOLORKEY ColorKey = (dwFlags & DDBLT_KEYDESTOVERRIDE) ? lpDDBltFx->ddckDestColorkey :
				(dwFlags & DDBLT_KEYSRCOVERRIDE) ? lpDDBltFx->ddckSrcColorkey :
				(dwFlags & DDBLT_KEYDEST) ? ColorKeys[0].Key :
				(dwFlags & DDBLT_KEYSRC) ? lpDDSrcSurfaceX->ColorKeys[2].Key : ColorKeys[0].Key;

			hr = CopySurface(lpDDSrcSurfaceX, lpSrcRect, lpDestRect, D3DTEXF_NONE, ColorKey, Flags);

		} while (false);

		// If successful
		if (SUCCEEDED(hr))
		{
			// Set dirty flag
			dirtyFlag = true;

			// Present surface
			PresentSurface(isSkipScene);
		}

		IsInBlt = false;

		// Return
		return hr;
	}

	if (lpDDSrcSurface)
	{
		lpDDSrcSurface = static_cast<m_IDirectDrawSurface7 *>(lpDDSrcSurface)->GetProxyInterface();
	}

	return ProxyInterface->Blt(lpDestRect, lpDDSrcSurface, lpSrcRect, dwFlags, lpDDBltFx);
}

HRESULT m_IDirectDrawSurfaceX::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpDDBltBatch)
		{
			return DDERR_INVALIDPARAMS;
		}

		HRESULT hr;

		for (DWORD x = 0; x < dwCount; x++)
		{
			hr = Blt(lpDDBltBatch[x].lprDest, (LPDIRECTDRAWSURFACE7)lpDDBltBatch[x].lpDDSSrc, lpDDBltBatch[x].lprSrc, lpDDBltBatch[x].dwFlags, lpDDBltBatch[x].lpDDBltFx, (x != dwCount - 1));
			if (FAILED(hr))
			{
				return hr;
			}
		}

		return DD_OK;
	}

	if (lpDDBltBatch && lpDDBltBatch->lpDDSSrc)
	{
		lpDDBltBatch->lpDDSSrc = static_cast<m_IDirectDrawSurface *>(lpDDBltBatch->lpDDSSrc)->GetProxyInterface();
	}

	return ProxyInterface->BltBatch(lpDDBltBatch, dwCount, dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Convert BltFast flags into Blt flags
		DWORD Flags = 0;
		if (dwFlags & DDBLTFAST_SRCCOLORKEY)
		{
			Flags |= DDBLT_KEYSRC;
		}
		if (dwFlags & DDBLTFAST_DESTCOLORKEY)
		{
			Flags |= DDBLT_KEYDEST;
		}
		if (dwFlags & DDBLTFAST_WAIT)
		{
			Flags |= DDBLT_WAIT;
		}

		// Get SrcRect
		RECT SrcRect;
		m_IDirectDrawSurfaceX *lpDDSrcSurfaceX = (!lpDDSrcSurface) ? this : ((m_IDirectDrawSurface*)lpDDSrcSurface)->GetWrapperInterface();

		// Check if source Surface exists
		if (!ddrawParent || !ddrawParent->DoesSurfaceExist(lpDDSrcSurfaceX))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: could not find source surface");
			return DD_OK;		// Return OK to allow some games to continue to work
		}

		lpDDSrcSurfaceX->CheckCoordinates(&SrcRect, lpSrcRect);

		// Create DestRect
		RECT DestRect = { (LONG)dwX, (LONG)dwY, SrcRect.right - SrcRect.left + (LONG)dwX , SrcRect.bottom - SrcRect.top + (LONG)dwY };

		// Call Blt
		return Blt(&DestRect, lpDDSrcSurface, &SrcRect, Flags, nullptr);
	}

	if (lpDDSrcSurface)
	{
		lpDDSrcSurface = static_cast<m_IDirectDrawSurface7 *>(lpDDSrcSurface)->GetProxyInterface();
	}
	
	return ProxyInterface->BltFast(dwX, dwY, lpDDSrcSurface, lpSrcRect, dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// dwFlags: Reserved. Must be zero.
		if (!lpDDSAttachedSurface || dwFlags)
		{
			return DDERR_INVALIDPARAMS;
		}

		m_IDirectDrawSurfaceX *lpAttachedSurface = ((m_IDirectDrawSurface*)lpDDSAttachedSurface)->GetWrapperInterface();

		if (!DoesAttachedSurfaceExist(lpAttachedSurface))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: could not find attached surface");
			return DDERR_SURFACENOTATTACHED;
		}

		if (!WasAttachedSurfaceAdded(lpAttachedSurface))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: only surfaces added with AddAttachedSurface can be deleted with this method");
			return DDERR_CANNOTDETACHSURFACE;
		}

		RemoveAttachedSurfaceFromMap(lpAttachedSurface);

		lpAttachedSurface->Release();

		return DD_OK;
	}

	if (lpDDSAttachedSurface)
	{
		lpDDSAttachedSurface = static_cast<m_IDirectDrawSurface7 *>(lpDDSAttachedSurface)->GetProxyInterface();
	}

	return ProxyInterface->DeleteAttachedSurface(dwFlags, lpDDSAttachedSurface);
}

HRESULT m_IDirectDrawSurfaceX::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!lpEnumSurfacesCallback)
	{
		return DDERR_INVALIDPARAMS;
	}

	if (ProxyDirectXVersion > 3)
	{
		return EnumAttachedSurfaces2(lpContext, (LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback, DirectXVersion);
	}

	ENUMSURFACE CallbackContext;
	CallbackContext.lpContext = lpContext;
	CallbackContext.lpCallback = lpEnumSurfacesCallback;
	CallbackContext.DirectXVersion = DirectXVersion;

	return GetProxyInterfaceV3()->EnumAttachedSurfaces(&CallbackContext, m_IDirectDrawEnumSurface::ConvertCallback);
}

HRESULT m_IDirectDrawSurfaceX::EnumAttachedSurfaces2(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback7, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!lpEnumSurfacesCallback7)
	{
		return DDERR_INVALIDPARAMS;
	}

	if (Config.Dd7to9)
	{
		for (auto it : AttachedSurfaceMap)
		{
			DDSURFACEDESC2 Desc2;
			Desc2.dwSize = sizeof(DDSURFACEDESC2);
			Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
			it.second.pSurface->GetSurfaceDesc2(&Desc2);
			if (lpEnumSurfacesCallback7((LPDIRECTDRAWSURFACE7)it.second.pSurface->GetWrapperInterfaceX(DirectXVersion), &Desc2, lpContext) == DDENUMRET_CANCEL)
			{
				return DD_OK;
			}
		}

		return DD_OK;
	}

	ENUMSURFACE CallbackContext;
	CallbackContext.lpContext = lpContext;
	CallbackContext.lpCallback7 = lpEnumSurfacesCallback7;
	CallbackContext.DirectXVersion = DirectXVersion;
	CallbackContext.ConvertSurfaceDescTo2 = (ProxyDirectXVersion > 3 && DirectXVersion < 4);

	return ProxyInterface->EnumAttachedSurfaces(&CallbackContext, m_IDirectDrawEnumSurface::ConvertCallback2);
}

HRESULT m_IDirectDrawSurfaceX::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!lpfnCallback)
	{
		return DDERR_INVALIDPARAMS;
	}

	if (ProxyDirectXVersion > 3)
	{
		return EnumOverlayZOrders2(dwFlags, lpContext, (LPDDENUMSURFACESCALLBACK7)lpfnCallback, DirectXVersion);
	}

	ENUMSURFACE CallbackContext;
	CallbackContext.lpContext = lpContext;
	CallbackContext.lpCallback = lpfnCallback;
	CallbackContext.DirectXVersion = DirectXVersion;

	return GetProxyInterfaceV3()->EnumOverlayZOrders(dwFlags, &CallbackContext, m_IDirectDrawEnumSurface::ConvertCallback);
}

HRESULT m_IDirectDrawSurfaceX::EnumOverlayZOrders2(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback7, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!lpfnCallback7)
	{
		return DDERR_INVALIDPARAMS;
	}

	if (Config.Dd7to9)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	}

	ENUMSURFACE CallbackContext;
	CallbackContext.lpContext = lpContext;
	CallbackContext.lpCallback7 = lpfnCallback7;
	CallbackContext.DirectXVersion = DirectXVersion;
	CallbackContext.ConvertSurfaceDescTo2 = (ProxyDirectXVersion > 3 && DirectXVersion < 4);

	return ProxyInterface->EnumOverlayZOrders(dwFlags, &CallbackContext, m_IDirectDrawEnumSurface::ConvertCallback2);
}

HRESULT m_IDirectDrawSurfaceX::FlipBackBuffer()
{
	DWORD dwCaps = 0;
	m_IDirectDrawSurfaceX *lpTargetSurface = nullptr;

	// Loop through each surface
	for (auto it : AttachedSurfaceMap)
	{
		dwCaps = it.second.pSurface->GetSurfaceCaps().dwCaps;
		if (dwCaps & DDSCAPS_FLIP)
		{
			lpTargetSurface = it.second.pSurface;

			break;
		}
	}

	// Check if backbuffer was found
	if (!lpTargetSurface)
	{
		return DDERR_GENERIC;
	}

	// Check surface interface
	if (FAILED(lpTargetSurface->CheckInterface(__FUNCTION__, true, true)))
	{
		return DDERR_GENERIC;
	}

	// Check if surface is busy
	if (lpTargetSurface->WaitForLockState() || lpTargetSurface->IsSurfaceInDC())
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: backbuffer surface is busy!");
		return DDERR_SURFACEBUSY;
	}

	// Stop flipping when frontbuffer is found
	if (dwCaps & DDSCAPS_FRONTBUFFER)
	{
		return DD_OK;
	}

	// Swap surface
	SwapSurface(this, lpTargetSurface);

	// Flip next surface
	return lpTargetSurface->FlipBackBuffer();
}

HRESULT m_IDirectDrawSurfaceX::Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		// Flip can be called only for a surface that has the DDSCAPS_FLIP and DDSCAPS_FRONTBUFFER capabilities
		if ((surfaceDesc2.ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER)) != (DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: This surface cannot be flipped");
			return DDERR_INVALIDOBJECT;
		}

		// Check if surface is locked or has an open DC
		if (WaitForLockState() || IsSurfaceInDC())
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: surface is busy!");
			return DDERR_SURFACEBUSY;
		}

		// Unneeded flags (can be safely ignored?)
		// Note: vsync handled by d3d9 PresentationInterval
		// - DDFLIP_DONOTWAIT
		// - DDFLIP_WAIT

		if ((dwFlags & (DDFLIP_INTERVAL2 | DDFLIP_INTERVAL3 | DDFLIP_INTERVAL4)) && (surfaceDesc2.ddsCaps.dwCaps2 & DDCAPS2_FLIPINTERVAL))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Interval flipping not fully implemented");
		}

		// Use WaitForVerticalBlank for wait timer
		if ((dwFlags & DDFLIP_NOVSYNC) == 0)
		{
			ddrawParent->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, nullptr);
		}

		HRESULT hr = DD_OK;
		do {
			IsInFlip = true;

			// If SurfaceTargetOverride then use that surface
			if (lpDDSurfaceTargetOverride)
			{
				m_IDirectDrawSurfaceX *lpTargetSurface = ((m_IDirectDrawSurface*)lpDDSurfaceTargetOverride)->GetWrapperInterface();

				// Check if target surface exists
				if (!DoesFlipBackBufferExist(lpTargetSurface) || lpTargetSurface == this)
				{
					LOG_LIMIT(100, __FUNCTION__ << " Error: invalid surface!");
					hr = DDERR_INVALIDPARAMS;
					break;
				}

				// Check for device interface
				if (FAILED(lpTargetSurface->CheckInterface(__FUNCTION__, true, true)))
				{
					hr = DDERR_GENERIC;
					break;
				}

				// Check if surface is locked or has an open DC
				if (lpTargetSurface->WaitForLockState() || lpTargetSurface->IsSurfaceInDC())
				{
					LOG_LIMIT(100, __FUNCTION__ << " Error: backbuffer surface is busy!");
					hr = DDERR_SURFACEBUSY;
					break;
				}

				// Swap surface
				SwapSurface(this, lpTargetSurface);
			}

			// Execute flip for all attached surfaces
			else
			{
				if ((dwFlags & (DDFLIP_EVEN | DDFLIP_ODD)) == (DDFLIP_EVEN | DDFLIP_ODD))
				{
					LOG_LIMIT(100, __FUNCTION__ << " Error: invalid flags!");
					hr = DDERR_INVALIDPARAMS;
					break;
				}

				if (dwFlags & DDFLIP_STEREO)
				{
					LOG_LIMIT(100, __FUNCTION__ << " Stereo flipping not implemented");
					hr = DDERR_NOSTEREOHARDWARE;
					break;
				}

				if (dwFlags & (DDFLIP_ODD | DDFLIP_EVEN))
				{
					LOG_LIMIT(100, __FUNCTION__ << " Even and odd flipping not implemented");
					hr = DDERR_UNSUPPORTED;
					break;
				}

				// Flip surface
				hr = FlipBackBuffer();
			}

		} while (false);

		// Present surface
		if (SUCCEEDED(hr))
		{
			PresentSurface();
		}

		// Reset flip flag
		IsInFlip = false;

		return hr;
	}

	if (lpDDSurfaceTargetOverride)
	{
		lpDDSurfaceTargetOverride = static_cast<m_IDirectDrawSurface7 *>(lpDDSurfaceTargetOverride)->GetProxyInterface();
	}

	return ProxyInterface->Flip(lpDDSurfaceTargetOverride, dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR * lplpDDAttachedSurface, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Game using old DirectX, Convert DDSCAPS to DDSCAPS2
	if (ProxyDirectXVersion > 3)
	{
		if (!lplpDDAttachedSurface)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSCAPS2 Caps2;
		ConvertCaps(Caps2, *lpDDSCaps);

		return GetAttachedSurface2((lpDDSCaps) ? &Caps2 : nullptr, lplpDDAttachedSurface, DirectXVersion);
	}

	HRESULT hr = GetProxyInterfaceV3()->GetAttachedSurface(lpDDSCaps, (LPDIRECTDRAWSURFACE3*)lplpDDAttachedSurface);

	if (SUCCEEDED(hr) && lplpDDAttachedSurface)
	{
		*lplpDDAttachedSurface = ProxyAddressLookupTable.FindAddress<m_IDirectDrawSurface7>(*lplpDDAttachedSurface);
	}

	return hr;
}

HRESULT m_IDirectDrawSurfaceX::GetAttachedSurface2(LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE7 FAR * lplpDDAttachedSurface, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lplpDDAttachedSurface || !lpDDSCaps2)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false, false)))
		{
			return DDERR_GENERIC;
		}

		m_IDirectDrawSurfaceX *lpFoundSurface = nullptr;

		// Check if attached surface exists
		for (auto it : AttachedSurfaceMap)
		{
			m_IDirectDrawSurfaceX *lpSurface = it.second.pSurface;

			if ((lpSurface->GetSurfaceCaps().dwCaps & lpDDSCaps2->dwCaps) == lpDDSCaps2->dwCaps)
			{
				if (lpFoundSurface)
				{
					LOG_LIMIT(100, __FUNCTION__ << " Error: more than one surface is attached that matches the capabilities requested.");
					return DDERR_GENERIC;
				}

				lpFoundSurface = lpSurface;
			}
		}

		// No attached surface found
		if (!lpFoundSurface)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to find attached surface that matches the capabilities requested: " << *lpDDSCaps2);

			return DDERR_NOTFOUND;
		}

		m_IDirectDrawSurfaceX *lpAttachedSurface = (m_IDirectDrawSurfaceX *)lpFoundSurface->GetWrapperInterfaceX(DirectXVersion);

		lpAttachedSurface->AddRef();

		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)lpAttachedSurface;

		return DD_OK;
	}

	HRESULT hr = ProxyInterface->GetAttachedSurface(lpDDSCaps2, lplpDDAttachedSurface);

	if (SUCCEEDED(hr) && lplpDDAttachedSurface)
	{
		*lplpDDAttachedSurface = ProxyAddressLookupTable.FindAddress<m_IDirectDrawSurface7>(*lplpDDAttachedSurface);
	}

	return hr;
}

HRESULT m_IDirectDrawSurfaceX::GetBltStatus(DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Inquires whether a blit involving this surface can occur immediately, and returns DD_OK if the blit can be completed.
		if (dwFlags == DDGBS_CANBLT)
		{
			if (IsInBlt || IsLocked)
			{
				return DDERR_WASSTILLDRAWING;
			}
			return DD_OK;
		}
		// Inquires whether the blit is done, and returns DD_OK if the last blit on this surface has completed.
		else if (dwFlags == DDGBS_ISBLTDONE)
		{
			if (IsInBlt)
			{
				return DDERR_WASSTILLDRAWING;
			}
			return DD_OK;
		}

		return DDERR_INVALIDPARAMS;
	}

	return ProxyInterface->GetBltStatus(dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::GetCaps(LPDDSCAPS lpDDSCaps)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Game using old DirectX, Convert DDSCAPS to DDSCAPS2
	if (ProxyDirectXVersion > 3)
	{
		if (!lpDDSCaps)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSCAPS2 Caps2;

		HRESULT hr = GetCaps2(&Caps2);

		// Convert back to DDSCAPS
		if (SUCCEEDED(hr))
		{
			ConvertCaps(*lpDDSCaps, Caps2);
		}

		return hr;
	}

	return GetProxyInterfaceV3()->GetCaps(lpDDSCaps);
}

HRESULT m_IDirectDrawSurfaceX::GetCaps2(LPDDSCAPS2 lpDDSCaps2)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpDDSCaps2)
		{
			return DDERR_INVALIDPARAMS;
		}

		ConvertCaps(*lpDDSCaps2, surfaceDesc2.ddsCaps);

		return DD_OK;
	}

	return ProxyInterface->GetCaps(lpDDSCaps2);
}

HRESULT m_IDirectDrawSurfaceX::GetClipper(LPDIRECTDRAWCLIPPER FAR * lplpDDClipper)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// No clipper attached
		if (!attachedClipper)
		{
			return DDERR_NOCLIPPERATTACHED;
		}

		// Return attached clipper
		*lplpDDClipper = (LPDIRECTDRAWCLIPPER)attachedClipper;

		// Success
		return DD_OK;
	}

	HRESULT hr = ProxyInterface->GetClipper(lplpDDClipper);

	if (SUCCEEDED(hr) && lplpDDClipper)
	{
		*lplpDDClipper = ProxyAddressLookupTable.FindAddress<m_IDirectDrawClipper>(*lplpDDClipper);
	}

	return hr;
}

HRESULT m_IDirectDrawSurfaceX::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Get color key index
		int x = (dwFlags == DDCKEY_DESTBLT) ? 0 :
			(dwFlags == DDCKEY_DESTOVERLAY) ? 1 :
			(dwFlags == DDCKEY_SRCBLT) ? 2 :
			(dwFlags == DDCKEY_SRCOVERLAY) ? 3 : -1;

		// Check index
		if (!lpDDColorKey || x == -1)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check if color key is set
		if (!ColorKeys[x].IsSet)
		{
			return DDERR_NOCOLORKEY;
		}

		// Copy color key
		lpDDColorKey->dwColorSpaceHighValue = ColorKeys[x].Key.dwColorSpaceHighValue;
		lpDDColorKey->dwColorSpaceLowValue = ColorKeys[x].Key.dwColorSpaceLowValue;

		// Return
		return DD_OK;
	}

	return ProxyInterface->GetColorKey(dwFlags, lpDDColorKey);
}

HRESULT m_IDirectDrawSurfaceX::GetDC(HDC FAR * lphDC)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lphDC)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		if (IsSurfaceInDC())
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: does not support getting device context twice!");
			return DDERR_GENERIC;
		}

		if (IsSurfaceEmulated || DCRequiresEmulation)
		{
			if (!emu)
			{
				CreateDCSurface();
			}

			if (!emu || !emu->surfaceDC || !emu->surfacepBits)
			{
				return DDERR_GENERIC;
			}

			// Copy surface data to device context
			if (!IsSurfaceEmulated)
			{
				CopyEmulatedSurface(nullptr, false);
			}

			UpdatePaletteData();

			*lphDC = emu->surfaceDC;
		}
		else
		{
			if (!contextSurface && FAILED(surfaceTexture->GetSurfaceLevel(0, &contextSurface)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: could not get surface level!");
				return DDERR_GENERIC;
			}

			if (FAILED(contextSurface->GetDC(lphDC)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: could not get device context!");
				return DDERR_GENERIC;
			}
		}

		IsInDC = true;

		return DD_OK;
	}

	return ProxyInterface->GetDC(lphDC);
}

HRESULT m_IDirectDrawSurfaceX::GetFlipStatus(DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Get backbuffer
		m_IDirectDrawSurfaceX *lpBackBuffer = this;
		for (auto it : AttachedSurfaceMap)
		{
			if (!(it.second.pSurface->GetSurfaceCaps().dwCaps & DDSCAPS_BACKBUFFER))
			{
				lpBackBuffer = it.second.pSurface;

				break;
			}
		}

		// Queries whether the surface can flip now. The method returns DD_OK if the flip can be completed.
		if ((dwFlags == DDGFS_CANFLIP))
		{
			if (IsInFlip || IsSurfaceLocked() || IsSurfaceInDC() || lpBackBuffer->IsSurfaceLocked() || lpBackBuffer->IsSurfaceInDC())
			{
				return DDERR_WASSTILLDRAWING;
			}
			return DD_OK;
		}
		// Queries whether the flip is done. The method returns DD_OK if the last flip on this surface has completed.
		else if (dwFlags == DDGFS_ISFLIPDONE)
		{
			if (IsInFlip)
			{
				return DDERR_WASSTILLDRAWING;
			}
			return DD_OK;
		}

		return DDERR_INVALIDPARAMS;
	}

	return ProxyInterface->GetFlipStatus(dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Not fully Implemented.");

		if (!lplX || !lplY)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Set lplX and lplY to X, Y of this overlay surface
		*lplX = overlayX;
		*lplY = overlayY;

		return DD_OK;
	}

	return ProxyInterface->GetOverlayPosition(lplX, lplY);
}

HRESULT m_IDirectDrawSurfaceX::GetPalette(LPDIRECTDRAWPALETTE FAR * lplpDDPalette)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lplpDDPalette)
		{
			return DDERR_INVALIDPARAMS;
		}

		// No palette attached
		if (!attachedPalette)
		{
			return DDERR_NOPALETTEATTACHED;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false, false)))
		{
			return DDERR_GENERIC;
		}

		// Check exclusive mode
		if (!ddrawParent->IsExclusiveMode())
		{
			return DDERR_NOEXCLUSIVEMODE;
		}

		// Return attached palette
		*lplpDDPalette = (LPDIRECTDRAWPALETTE)attachedPalette;

		// Success
		return DD_OK;
	}

	HRESULT hr = ProxyInterface->GetPalette(lplpDDPalette);

	if (SUCCEEDED(hr) && lplpDDPalette)
	{
		*lplpDDPalette = ProxyAddressLookupTable.FindAddress<m_IDirectDrawPalette>(*lplpDDPalette);
	}

	return hr;
}

HRESULT m_IDirectDrawSurfaceX::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpDDPixelFormat)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

		// Update surface description
		GetSurfaceDesc2(&Desc2);

		// Copy pixel format to lpDDPixelFormat
		ConvertPixelFormat(*lpDDPixelFormat, Desc2.ddpfPixelFormat);

		return DD_OK;
	}

	return ProxyInterface->GetPixelFormat(lpDDPixelFormat);
}

HRESULT m_IDirectDrawSurfaceX::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
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

		HRESULT hr = GetSurfaceDesc2(&Desc2);

		// Convert back to LPDDSURFACEDESC
		if (SUCCEEDED(hr))
		{
			ConvertSurfaceDesc(*lpDDSurfaceDesc, Desc2);
		}

		return hr;
	}

	return GetProxyInterfaceV3()->GetSurfaceDesc(lpDDSurfaceDesc);
}

HRESULT m_IDirectDrawSurfaceX::GetSurfaceDesc2(LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpDDSurfaceDesc2)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false, false)))
		{
			return DDERR_GENERIC;
		}

		// Copy surfacedesc to lpDDSurfaceDesc2
		if (lpDDSurfaceDesc2 != &surfaceDesc2)
		{
			ConvertSurfaceDesc(*lpDDSurfaceDesc2, surfaceDesc2);
		}

		// Surface description
		DDSURFACEDESC2 Desc2 = { NULL };
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

		// Set Height and Width
		if ((lpDDSurfaceDesc2->dwFlags & (DDSD_HEIGHT | DDSD_WIDTH)) != (DDSD_HEIGHT | DDSD_WIDTH))
		{
			if ((Desc2.dwFlags & (DDSD_HEIGHT | DDSD_WIDTH)) != (DDSD_HEIGHT | DDSD_WIDTH))
			{
				ddrawParent->GetDisplayMode2(&Desc2);
			}
			lpDDSurfaceDesc2->dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
			lpDDSurfaceDesc2->dwWidth = Desc2.dwWidth;
			lpDDSurfaceDesc2->dwHeight = Desc2.dwHeight;
		}
		// Set Refresh Rate
		if (!(lpDDSurfaceDesc2->dwFlags & DDSD_REFRESHRATE))
		{
			if (!(Desc2.dwFlags & DDSD_REFRESHRATE))
			{
				ddrawParent->GetDisplayMode2(&Desc2);
			}
			if (Desc2.dwRefreshRate)
			{
				lpDDSurfaceDesc2->dwFlags |= DDSD_REFRESHRATE;
				lpDDSurfaceDesc2->dwRefreshRate = Desc2.dwRefreshRate;
			}
		}
		// Set PixelFormat
		if (!(lpDDSurfaceDesc2->dwFlags & DDSD_PIXELFORMAT))
		{
			if (!(Desc2.dwFlags & DDSD_PIXELFORMAT))
			{
				ddrawParent->GetDisplayMode2(&Desc2);
			}
			lpDDSurfaceDesc2->dwFlags |= DDSD_PIXELFORMAT;
			SetPixelDisplayFormat(GetDisplayFormat(Desc2.ddpfPixelFormat), lpDDSurfaceDesc2->ddpfPixelFormat);
		}
		// Set lPitch
		if (!(lpDDSurfaceDesc2->dwFlags & DDSD_PITCH))
		{
			lpDDSurfaceDesc2->dwFlags |= DDSD_PITCH;
			lpDDSurfaceDesc2->lPitch = lpDDSurfaceDesc2->dwWidth * (GetBitCount(lpDDSurfaceDesc2->ddpfPixelFormat) / 8);
		}
		// Set ColorKey
		if (!(surfaceDesc2.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP))
		{
			if (ColorKeys[0].IsSet)
			{
				lpDDSurfaceDesc2->dwFlags |= DDSD_CKDESTBLT;
				lpDDSurfaceDesc2->ddckCKDestBlt.dwColorSpaceLowValue;
				lpDDSurfaceDesc2->ddckCKDestBlt.dwColorSpaceHighValue;
			}
			if (ColorKeys[1].IsSet)
			{
				lpDDSurfaceDesc2->dwFlags |= DDSD_CKDESTOVERLAY;
				lpDDSurfaceDesc2->ddckCKDestOverlay.dwColorSpaceLowValue;
				lpDDSurfaceDesc2->ddckCKDestOverlay.dwColorSpaceHighValue;
			}
			if (ColorKeys[2].IsSet)
			{
				lpDDSurfaceDesc2->dwFlags |= DDSD_CKSRCBLT;
				lpDDSurfaceDesc2->ddckCKSrcBlt.dwColorSpaceLowValue;
				lpDDSurfaceDesc2->ddckCKSrcBlt.dwColorSpaceHighValue;
			}
			if (ColorKeys[3].IsSet)
			{
				lpDDSurfaceDesc2->dwFlags |= DDSD_CKSRCOVERLAY;
				lpDDSurfaceDesc2->ddckCKSrcOverlay.dwColorSpaceLowValue;
				lpDDSurfaceDesc2->ddckCKSrcOverlay.dwColorSpaceHighValue;
			}
		}

		// Return
		return DD_OK;
	}

	return ProxyInterface->GetSurfaceDesc(lpDDSurfaceDesc2);
}

HRESULT m_IDirectDrawSurfaceX::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (ProxyDirectXVersion > 3)
	{
		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		if (lpDDSurfaceDesc)
		{
			ConvertSurfaceDesc(Desc2, *lpDDSurfaceDesc);
		}

		return Initialize2(lpDD, (lpDDSurfaceDesc) ? &Desc2 : nullptr);
	}

	if (lpDD)
	{
		lpDD = static_cast<m_IDirectDraw *>(lpDD)->GetProxyInterface();
	}

	return GetProxyInterfaceV3()->Initialize(lpDD, lpDDSurfaceDesc);
}

HRESULT m_IDirectDrawSurfaceX::Initialize2(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Not needed
		return DD_OK;
	}

	if (lpDD)
	{
		lpDD = static_cast<m_IDirectDraw *>(lpDD)->GetProxyInterface();
	}

	return ProxyInterface->Initialize(lpDD, lpDDSurfaceDesc2);
}

HRESULT m_IDirectDrawSurfaceX::IsLost()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		// Check device
		switch ((*d3d9Device)->TestCooperativeLevel())
		{
		case D3DERR_DEVICELOST:
		case D3DERR_DEVICENOTRESET:
			return DDERR_SURFACELOST;
		case D3DERR_DRIVERINTERNALERROR:
		case D3DERR_INVALIDCALL:
			return DDERR_GENERIC;
		default:
			return DD_OK;
		}
	}

	return ProxyInterface->IsLost();
}

HRESULT m_IDirectDrawSurfaceX::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Game using old DirectX, Convert to LPDDSURFACEDESC2
	if (ProxyDirectXVersion > 3)
	{
		if (!lpDDSurfaceDesc)
		{
			return DDERR_INVALIDPARAMS;
		}

		DDSURFACEDESC2 Desc2 = { NULL };
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ConvertSurfaceDesc(Desc2, *lpDDSurfaceDesc);

		HRESULT hr = Lock2(lpDestRect, &Desc2, dwFlags, hEvent);

		// Convert back to LPDDSURFACEDESC
		if (SUCCEEDED(hr))
		{
			ConvertSurfaceDesc(*lpDDSurfaceDesc, Desc2);
		}

		return hr;
	}

	return GetProxyInterfaceV3()->Lock(lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent);
}

HRESULT m_IDirectDrawSurfaceX::Lock2(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc2, DWORD dwFlags, HANDLE hEvent)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpDDSurfaceDesc2)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Convert flags to d3d9
		DWORD Flags = dwFlags & (D3DLOCK_READONLY | (!IsPrimarySurface() ? DDLOCK_NOSYSLOCK : 0) | ((!lpDestRect) ? D3DLOCK_DISCARD : 0));

		// Update rect
		RECT DestRect;
		if (lpDestRect && !CheckCoordinates(&DestRect, lpDestRect))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: invalid rect!");
			return DDERR_INVALIDRECT;
		}

		// Lock surface
		D3DLOCKED_RECT LockedRect;
		HRESULT hr = SetLock(&LockedRect, (lpDestRect) ? &DestRect : nullptr, Flags);
		if (FAILED(hr))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock texture surface!");
			return hr;
		}

		// Set surfaceDesc
		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ConvertSurfaceDesc(Desc2, surfaceDesc2);

		// Set video memory and pitch
		if (!LockedRect.pBits)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to get surface address!");
			return DDERR_GENERIC;
		}

		Desc2.dwFlags |= DDSD_LPSURFACE | DDSD_PITCH;
		Desc2.lpSurface = LockedRect.pBits;
		Desc2.lPitch = LockedRect.Pitch;

		ConvertSurfaceDesc(*lpDDSurfaceDesc2, Desc2);

		return DD_OK;
	}

	return ProxyInterface->Lock(lpDestRect, lpDDSurfaceDesc2, dwFlags, hEvent);
}

HRESULT m_IDirectDrawSurfaceX::ReleaseDC(HDC hDC)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		if (!IsSurfaceInDC())
		{
			return DDERR_GENERIC;
		}

		if (IsSurfaceEmulated || DCRequiresEmulation)
		{
			if (!emu || !emu->surfaceDC || !emu->surfacepBits)
			{
				return DDERR_GENERIC;
			}

			// Copy emulated surface to real surface
			CopyEmulatedSurface(nullptr, true);
		}
		else
		{
			if (!contextSurface || FAILED(contextSurface->ReleaseDC(hDC)))
			{
				return DDERR_GENERIC;
			}
		}

		// Reset DC flag
		IsInDC = false;

		// Present surface
		PresentSurface();

		return DD_OK;
	}

	return ProxyInterface->ReleaseDC(hDC);
}

HRESULT m_IDirectDrawSurfaceX::Restore()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		return DD_OK;
	}

	return ProxyInterface->Restore();
}

HRESULT m_IDirectDrawSurfaceX::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// If lpDDClipper is nullptr then detach the current clipper if it exists
		if (!lpDDClipper)
		{
			if (attachedClipper)
			{
				// Decrement ref count
				attachedClipper->Release();

				// Detach
				attachedClipper = nullptr;
			}

			// Reset FirstRun
			ClipperFirstRun = true;

			return DD_OK;
		}

		// Set clipper address
		attachedClipper = (m_IDirectDrawClipper *)lpDDClipper;

		// When you call SetClipper to set a clipper to a surface for the first time, 
		// SetClipper increments the clipper's reference count; subsequent calls to 
		// SetClipper do not affect the clipper's reference count.
		if (ClipperFirstRun)
		{
			attachedClipper->AddRef();
			ClipperFirstRun = false;
		}

		return DD_OK;
	}

	if (lpDDClipper)
	{
		lpDDClipper = static_cast<m_IDirectDrawClipper *>(lpDDClipper)->GetProxyInterface();
	}

	return ProxyInterface->SetClipper(lpDDClipper);
}

HRESULT m_IDirectDrawSurfaceX::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Get color key index
		DWORD Flag = (dwFlags & ~DDCKEY_COLORSPACE);
		int x = (Flag == DDCKEY_DESTBLT) ? 0 :
			(Flag == DDCKEY_DESTOVERLAY) ? 1 :
			(Flag == DDCKEY_SRCBLT) ? 2 :
			(Flag == DDCKEY_SRCOVERLAY) ? 3 : -1;

		// Check index
		if (x == -1)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Set color key
		if (!lpDDColorKey)
		{
			ColorKeys[x].IsSet = false;
			ColorKeys[x].Key.dwColorSpaceHighValue = 0;
			ColorKeys[x].Key.dwColorSpaceLowValue = 0;
		}
		else
		{
			ColorKeys[x].IsSet = true;
			ColorKeys[x].IsColorSpace = ((dwFlags & DDCKEY_COLORSPACE) != 0);
			if (ColorKeys[x].IsColorSpace)
			{
				ColorKeys[x].Key.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
				ColorKeys[x].Key.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
			}
			// You must add the flag DDCKEY_COLORSPACE, otherwise DirectDraw will collapse the range to one value
			else
			{
				ColorKeys[x].Key.dwColorSpaceHighValue = *(DWORD*)lpDDColorKey;
				ColorKeys[x].Key.dwColorSpaceLowValue = *(DWORD*)lpDDColorKey;
			}
		}

		// Return
		return DD_OK;
	}

	return ProxyInterface->SetColorKey(dwFlags, lpDDColorKey);
}

HRESULT m_IDirectDrawSurfaceX::SetOverlayPosition(LONG lX, LONG lY)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Not fully Implemented.");

		// Store the new overlay position
		overlayX = lX;
		overlayY = lY;

		return DD_OK;
	}

	return ProxyInterface->SetOverlayPosition(lX, lY);
}

HRESULT m_IDirectDrawSurfaceX::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// If lpDDPalette is nullptr then detach the current palette if it exists
		if (!lpDDPalette)
		{
			if (!attachedPalette)
			{
				return DDERR_NOPALETTEATTACHED;
			}

			// Check if palette exists
			if (ddrawParent && ddrawParent->DoesPaletteExist((m_IDirectDrawPalette *)lpDDPalette))
			{
				// Decrement ref count
				attachedPalette->Release();
			}

			// Detach
			attachedPalette = nullptr;

			// Reset FirstRun
			PaletteFirstRun = true;
		}
		// If lpDDPalette is not null
		else
		{
			// Check if palette exists
			if (!ddrawParent || !ddrawParent->DoesPaletteExist((m_IDirectDrawPalette *)lpDDPalette))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: could not find palette");
				return DDERR_INVALIDOBJECT;
			}

			// Set palette address
			attachedPalette = (m_IDirectDrawPalette *)lpDDPalette;

			// When you call SetPalette to set a palette to a surface for the first time, 
			// SetPalette increments the palette's reference count; subsequent calls to 
			// SetPalette do not affect the palette's reference count.
			if (PaletteFirstRun)
			{
				attachedPalette->AddRef();
				PaletteFirstRun = false;
			}

			// If new palette is set
			PaletteUSN++;
		}

		return DD_OK;
	}

	if (lpDDPalette)
	{
		lpDDPalette = static_cast<m_IDirectDrawPalette *>(lpDDPalette)->GetProxyInterface();
	}

	return ProxyInterface->SetPalette(lpDDPalette);
}

HRESULT m_IDirectDrawSurfaceX::Unlock(LPRECT lpRect)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Unlock surface
		return SetUnlock();
	}

	return ProxyInterface->Unlock(lpRect);
}

HRESULT m_IDirectDrawSurfaceX::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	}

	if (lpDDDestSurface)
	{
		lpDDDestSurface = static_cast<m_IDirectDrawSurface7 *>(lpDDDestSurface)->GetProxyInterface();
	}

	return ProxyInterface->UpdateOverlay(lpSrcRect, lpDDDestSurface, lpDestRect, dwFlags, lpDDOverlayFx);
}

HRESULT m_IDirectDrawSurfaceX::UpdateOverlayDisplay(DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	}

	return ProxyInterface->UpdateOverlayDisplay(dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Not Implemented");
		return DDERR_UNSUPPORTED;
	}

	if (lpDDSReference)
	{
		lpDDSReference = static_cast<m_IDirectDrawSurface7 *>(lpDDSReference)->GetProxyInterface();
	}

	return ProxyInterface->UpdateOverlayZOrder(dwFlags, lpDDSReference);
}

/*********************************/
/*** Added in the v2 interface ***/
/*********************************/

HRESULT m_IDirectDrawSurfaceX::GetDDInterface(LPVOID FAR * lplpDD, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lplpDD)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, false, false)))
		{
			return DDERR_GENERIC;
		}

		*lplpDD = ddrawParent->GetWrapperInterfaceX(DirectXVersion);

		::AddRef(*lplpDD);

		return DD_OK;
	}

	HRESULT hr = ProxyInterface->GetDDInterface(lplpDD);

	if (SUCCEEDED(hr) && lplpDD)
	{
		// Calling the GetDDInterface method from any surface created under DirectDrawEx will return a pointer to the 
		// IUnknown interface instead of a pointer to an IDirectDraw interface. Applications must use the
		// IUnknown::QueryInterface method to retrieve the IDirectDraw, IDirectDraw2, or IDirectDraw3 interfaces.
		IID tmpID = (DirectXVersion == 1) ? IID_IDirectDraw :
			(DirectXVersion == 2) ? IID_IDirectDraw2 :
			(DirectXVersion == 3) ? IID_IDirectDraw3 :
			(DirectXVersion == 4) ? IID_IDirectDraw4 :
			(DirectXVersion == 7) ? IID_IDirectDraw7 : IID_IDirectDraw7;

		IUnknown *lpDD = (IUnknown*)*lplpDD;

		hr = lpDD->QueryInterface(tmpID, lplpDD);
		if (SUCCEEDED(hr))
		{
			*lplpDD = ProxyAddressLookupTable.FindAddress<m_IDirectDraw7>(*lplpDD);
			lpDD->Release();
		}
	}

	return hr;
}

HRESULT m_IDirectDrawSurfaceX::PageLock(DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Prevents a system-memory surface from being paged out while a bit block transfer (bitblt) operation that uses direct memory access (DMA) transfers to or from system memory is in progress.
		// Not needed for d3d9 surfaces
		return DD_OK;
	}

	return ProxyInterface->PageLock(dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::PageUnlock(DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Unlocks a system-memory surface, which then allows it to be paged out.
		// Not needed for d3d9 surfaces
		return DD_OK;
	}

	return ProxyInterface->PageUnlock(dwFlags);
}

/*********************************/
/*** Added in the v3 interface ***/
/*********************************/

HRESULT m_IDirectDrawSurfaceX::SetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags)
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

		return SetSurfaceDesc2(&Desc2, dwFlags);
	}

	return GetProxyInterfaceV3()->SetSurfaceDesc(lpDDSurfaceDesc, dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::SetSurfaceDesc2(LPDDSURFACEDESC2 lpDDSurfaceDesc2, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpDDSurfaceDesc2)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check flags
		DWORD SurfaceFlags = lpDDSurfaceDesc2->dwFlags;
		if (SurfaceFlags & DDSD_LPSURFACE)
		{
			LOG_LIMIT(100, __FUNCTION__ << " lpSurface not fully Implemented.");

			SurfaceFlags &= ~DDSD_LPSURFACE;
			surfaceDesc2.dwFlags |= DDSD_LPSURFACE;
			surfaceDesc2.lpSurface = lpDDSurfaceDesc2->lpSurface;
		}
		if (SurfaceFlags)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: flags not implemented " << SurfaceFlags);
			return DDERR_UNSUPPORTED;
		}
		return DD_OK;
	}

	return ProxyInterface->SetSurfaceDesc(lpDDSurfaceDesc2, dwFlags);
}

/*********************************/
/*** Added in the v4 interface ***/
/*********************************/

HRESULT m_IDirectDrawSurfaceX::SetPrivateData(REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		return surfaceTexture->SetPrivateData(guidTag, lpData, cbSize, dwFlags);
	}

	return ProxyInterface->SetPrivateData(guidTag, lpData, cbSize, dwFlags);
}

HRESULT m_IDirectDrawSurfaceX::GetPrivateData(REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		return surfaceTexture->GetPrivateData(guidTag, lpBuffer, lpcbBufferSize);
	}

	return ProxyInterface->GetPrivateData(guidTag, lpBuffer, lpcbBufferSize);
}

HRESULT m_IDirectDrawSurfaceX::FreePrivateData(REFGUID guidTag)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Check for device interface
		if (FAILED(CheckInterface(__FUNCTION__, true, true)))
		{
			return DDERR_GENERIC;
		}

		return surfaceTexture->FreePrivateData(guidTag);
	}

	return ProxyInterface->FreePrivateData(guidTag);
}

HRESULT m_IDirectDrawSurfaceX::GetUniquenessValue(LPDWORD lpValue)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpValue)
		{
			return DDERR_INVALIDPARAMS;
		}

		// The only defined uniqueness value is 0.
		*lpValue = UniquenessValue;
		return DD_OK;
	}

	return ProxyInterface->GetUniquenessValue(lpValue);
}

HRESULT m_IDirectDrawSurfaceX::ChangeUniquenessValue()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Manually updates the uniqueness value for this surface.
		UniquenessValue = (DWORD)this + UniquenessValue + 1;
		return DD_OK;
	}

	return ProxyInterface->ChangeUniquenessValue();
}

/***********************************/
/*** Moved Texture7 methods here ***/
/***********************************/

HRESULT m_IDirectDrawSurfaceX::SetPriority(DWORD dwPriority)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Applications can call this method only for managed textures (those surfaces that were created with the DDSCAPS2_TEXTUREMANAGE flag).
		if ((surfaceDesc2.ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) == 0)
		{
			return DDERR_INVALIDOBJECT;
		}

		Priority = dwPriority;

		return DD_OK;
	}

	return ProxyInterface->SetPriority(dwPriority);
}

HRESULT m_IDirectDrawSurfaceX::GetPriority(LPDWORD lpdwPriority)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpdwPriority)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Applications can call this method only for managed textures (those surfaces that were created with the DDSCAPS2_TEXTUREMANAGE flag).
		if ((surfaceDesc2.ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) == 0)
		{
			return DDERR_INVALIDOBJECT;
		}

		*lpdwPriority = Priority;

		return DD_OK;
	}

	return ProxyInterface->GetPriority(lpdwPriority);
}

HRESULT m_IDirectDrawSurfaceX::SetLOD(DWORD dwMaxLOD)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		// Applications can call this method only for managed textures (those surfaces that were created with the DDSCAPS2_TEXTUREMANAGE flag).
		if ((surfaceDesc2.ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) == 0)
		{
			return DDERR_INVALIDOBJECT;
		}

		MaxLOD = dwMaxLOD;

		return DD_OK;
	}

	return ProxyInterface->SetLOD(dwMaxLOD);
}

HRESULT m_IDirectDrawSurfaceX::GetLOD(LPDWORD lpdwMaxLOD)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpdwMaxLOD)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Applications can call this method only for managed textures (those surfaces that were created with the DDSCAPS2_TEXTUREMANAGE flag).
		if ((surfaceDesc2.ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) == 0)
		{
			return DDERR_INVALIDOBJECT;
		}

		*lpdwMaxLOD = MaxLOD;

		return DD_OK;
	}

	return ProxyInterface->GetLOD(lpdwMaxLOD);
}

/************************/
/*** Helper functions ***/
/************************/

HRESULT m_IDirectDrawSurfaceX::CheckInterface(char *FunctionName, bool CheckD3DDevice, bool CheckD3DSurface)
{
	// Check for device
	if (!ddrawParent)
	{
		LOG_LIMIT(100, FunctionName << " Error: no ddraw parent!");
		return DDERR_GENERIC;
	}

	// Check for device, if not then create it
	if (CheckD3DDevice)
	{
		if (!d3d9Device || !*d3d9Device)
		{
			ddrawParent->CreateD3D9Device();

			if (!d3d9Device || !*d3d9Device)
			{
				LOG_LIMIT(100, FunctionName << " Error: d3d9 device not setup!");
				return DDERR_GENERIC;
			}
		}
	}

	// Make sure surface exists, if not then create it
	if (CheckD3DSurface)
	{
		if (!surfaceTexture)
		{
			if (FAILED(CreateD3d9Surface()))
			{
				LOG_LIMIT(100, FunctionName << " Error: d3d9 surface texture not setup!");
				return DDERR_GENERIC;
			}
		}
	}

	return DD_OK;
}

// Create surface
HRESULT m_IDirectDrawSurfaceX::CreateD3d9Surface()
{
	// Don't recreate surface while it is locked
	if (surfaceTexture && IsLocked)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: surface is locked!");
		return DDERR_GENERIC;
	}

	// Release existing surface
	ReleaseD9Surface();

	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true, false)))
	{
		return DDERR_GENERIC;
	}

	// Update surface description
	if ((surfaceDesc2.dwFlags & (DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT)) != (DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT))
	{
		DDSURFACEDESC2 Desc2 = { NULL };
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		GetSurfaceDesc2(&Desc2);

		// Set Height and Width
		if ((Desc2.dwFlags & (DDSD_HEIGHT | DDSD_WIDTH)) == (DDSD_HEIGHT | DDSD_WIDTH) &&
			(surfaceDesc2.dwFlags & (DDSD_HEIGHT | DDSD_WIDTH)) != (DDSD_HEIGHT | DDSD_WIDTH))
		{
			surfaceDesc2.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
			surfaceDesc2.dwWidth = Desc2.dwWidth;
			surfaceDesc2.dwHeight = Desc2.dwHeight;
		}
		// Set Refresh Rate
		if ((Desc2.dwFlags & DDSD_REFRESHRATE) && !(surfaceDesc2.dwFlags & DDSD_REFRESHRATE))
		{
			surfaceDesc2.dwFlags |= DDSD_REFRESHRATE;
			surfaceDesc2.dwRefreshRate = Desc2.dwRefreshRate;
		}
		// Set PixelFormat
		if ((Desc2.dwFlags & DDSD_PIXELFORMAT) && !(surfaceDesc2.dwFlags & DDSD_PIXELFORMAT))
		{
			surfaceDesc2.dwFlags |= DDSD_PIXELFORMAT;
			ConvertPixelFormat(surfaceDesc2.ddpfPixelFormat, Desc2.ddpfPixelFormat);
		}
	}

	// Get texture data
	surfaceFormat = GetDisplayFormat(surfaceDesc2.ddpfPixelFormat);
	surfaceBitCount = GetBitCount(surfaceFormat);
	IsSurfaceEmulated = (surfaceFormat == D3DFMT_R8G8B8) ||
		(Config.DdrawEmulateSurface && (surfaceFormat == D3DFMT_P8 || surfaceFormat == D3DFMT_R5G6B5 || surfaceFormat == D3DFMT_A1R5G5B5 || surfaceFormat == D3DFMT_X1R5G5B5 ||
			surfaceFormat == D3DFMT_A8R8G8B8 || surfaceFormat == D3DFMT_X8R8G8B8));
	DCRequiresEmulation = (surfaceFormat != D3DFMT_R5G6B5 && surfaceFormat != D3DFMT_X1R5G5B5 && surfaceFormat != D3DFMT_R8G8B8 && surfaceFormat != D3DFMT_X8R8G8B8);
	D3DFORMAT Format = (surfaceFormat == D3DFMT_P8) ? D3DFMT_L8 : (surfaceFormat == D3DFMT_R8G8B8) ? D3DFMT_X8R8G8B8 : surfaceFormat;

	// Create emulated surface using device context for creation
	if (IsSurfaceEmulated)
	{
		CreateDCSurface();
	}

	Logging::LogDebug() << __FUNCTION__ << " D3d9 Surface size: " << surfaceDesc2.dwWidth << "x" << surfaceDesc2.dwHeight << " Format: " << Format;

	// Create surface
	if (FAILED((*d3d9Device)->CreateTexture(surfaceDesc2.dwWidth, surfaceDesc2.dwHeight, 1, 0, Format, D3DPOOL_SYSTEMMEM, &surfaceTexture, nullptr)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to create surface size: " << surfaceDesc2.dwWidth << "x" << surfaceDesc2.dwHeight << " Format: " << surfaceFormat);
		return DDERR_GENERIC;
	}

	// Create display texture
	if (IsPrimarySurface())
	{
		if (FAILED((*d3d9Device)->CreateTexture(surfaceDesc2.dwWidth, surfaceDesc2.dwHeight, 1, 0, Format, D3DPOOL_DEFAULT, &displayTexture, nullptr)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to create display surface size: " << surfaceDesc2.dwWidth << "x" << surfaceDesc2.dwHeight << " Format: " << surfaceFormat);
			return DDERR_GENERIC;
		}
	}

	// Create palette surface
	if (surfaceFormat == D3DFMT_P8 && IsPrimarySurface())
	{
		if (FAILED((*d3d9Device)->CreateTexture(256, 256, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &paletteTexture, nullptr)) ||
			FAILED((*d3d9Device)->CreatePixelShader((DWORD*)PalettePixelShaderSrc, &pixelShader)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to create palette surface");
			return DDERR_GENERIC;
		}
	}

	// Reset d3d9 surface texture data
	if (surfaceBackup.size() && surfaceTexture)
	{
		do {
			D3DLOCKED_RECT LockRect;
			if (FAILED(surfaceTexture->LockRect(0, &LockRect, nullptr, 0)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock texture surface!");
				break;
			}

			size_t size = surfaceDesc2.dwHeight * LockRect.Pitch;

			if (size == surfaceBackup.size())
			{
				Logging::LogDebug() << __FUNCTION__ << " Resetting Direct3D9 texture surface data";

				memcpy(LockRect.pBits, &surfaceBackup[0], size);
			}

			surfaceTexture->UnlockRect(0);
		} while (false);
	}

	// Data is no longer needed
	surfaceBackup.clear();

	// Maark palette as updated
	PaletteUSN++;

	// Only display surface if it is primary for now...
	if (!IsPrimarySurface())
	{
		return DD_OK;
	}

	// Set vertex shader
	if (FAILED((*d3d9Device)->SetVertexShader(nullptr)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set vertex shader");
		return DDERR_GENERIC;
	}

	// Set fv format
	if (FAILED((*d3d9Device)->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set the current vertex stream format");
		return DDERR_GENERIC;
	}

	// Create vertex buffer
	if (FAILED((*d3d9Device)->CreateVertexBuffer(sizeof(TLVERTEX) * 4, D3DUSAGE_DYNAMIC, (D3DFVF_XYZRHW | D3DFVF_TEX1), D3DPOOL_DEFAULT, &vertexBuffer, nullptr)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to create vertex buffer");
		return DDERR_GENERIC;
	}

	// Set stream source
	if (FAILED((*d3d9Device)->SetStreamSource(0, vertexBuffer, 0, sizeof(TLVERTEX))))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set vertex buffer stream source");
		return DDERR_GENERIC;
	}

	// Set render states(no lighting)
	if (FAILED((*d3d9Device)->SetRenderState(D3DRS_LIGHTING, FALSE)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set device render state(no lighting)");
		return DDERR_GENERIC;
	}

	// Set scale mode to linear
	if (FAILED((*d3d9Device)->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set D3D device to LINEAR sampling");
	}

	// Setup verticies (0,0,currentWidth,currentHeight)
	TLVERTEX* vertices;

	// Lock vertex buffer
	if (FAILED(vertexBuffer->Lock(0, 0, (void**)&vertices, 0)))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock vertex buffer");
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

	// Calculate width and height with original aspect ratio
	DWORD xpad = 0;
	DWORD ypad = 0;
	if (Config.DdrawMaintainAspectRatio)
	{
		if (surfaceDesc2.dwWidth * displayHeight < surfaceDesc2.dwHeight * displayWidth)
		{
			// 4:3 displayed on 16:9
			BackBufferWidth = displayHeight * surfaceDesc2.dwWidth / surfaceDesc2.dwHeight;
		}
		else
		{
			// 16:9 displayed on 4:3
			BackBufferHeight = displayWidth * surfaceDesc2.dwHeight / surfaceDesc2.dwWidth;
		}
		xpad = (displayWidth - BackBufferWidth) / 2;
		ypad = (displayHeight - BackBufferHeight) / 2;
	}

	Logging::LogDebug() << __FUNCTION__ << " D3d9 Vertex size: " << BackBufferWidth << "x" << BackBufferHeight;
	// Set vertex points
	// 0, 0
	vertices[0].x = -0.5f + xpad;
	vertices[0].y = -0.5f + ypad;
	vertices[0].z = 0.0f;
	vertices[0].rhw = 1.0f;
	vertices[0].u = 0.0f;
	vertices[0].v = 0.0f;

	// scaledWidth, 0
	vertices[1].x = -0.5f + xpad + BackBufferWidth;
	vertices[1].y = vertices[0].y;
	vertices[1].z = 0.0f;
	vertices[1].rhw = 1.0f;
	vertices[1].u = 1.0f;
	vertices[1].v = 0.0f;

	// scaledWidth, scaledHeight
	vertices[2].x = vertices[1].x;
	vertices[2].y = -0.5f + ypad + BackBufferHeight;
	vertices[2].z = 0.0f;
	vertices[2].rhw = 1.0f;
	vertices[2].u = 1.0f;
	vertices[2].v = 1.0f;

	// 0, scaledHeight
	vertices[3].x = vertices[0].x;
	vertices[3].y = vertices[2].y;
	vertices[3].z = 0.0f;
	vertices[3].rhw = 1.0f;
	vertices[3].u = 0.0f;
	vertices[3].v = 1.0f;

	// Unlock vertex buffer
	if (FAILED(vertexBuffer->Unlock()))
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to unlock vertex buffer");
		return DDERR_GENERIC;
	}

	return DD_OK;
}

HRESULT m_IDirectDrawSurfaceX::CreateDCSurface()
{
	// Check if color masks are needed
	bool ColorMaskReq = ((surfaceBitCount == 16 || surfaceBitCount == 32) &&																// Only valid when used with 16 bit and 32 bit surfaces
		(surfaceDesc2.ddpfPixelFormat.dwFlags & DDPF_RGB) &&																				// Check to make sure it is an RGB surface
		(surfaceDesc2.ddpfPixelFormat.dwRBitMask && surfaceDesc2.ddpfPixelFormat.dwGBitMask && surfaceDesc2.ddpfPixelFormat.dwBBitMask));	// Check to make sure the masks actually exist
	
	// Check if emulated surface already exists
	if (emu)
	{
		// Check if emulated memory is good
		if (!emu->surfaceDC || !emu->surfacepBits)
		{
			DeleteSharedEmulatedMemory(&emu);
		}
		else
		{
			// Check if current emulated surface is still ok
			if (emu->bmi->bmiHeader.biWidth == (LONG)surfaceDesc2.dwWidth && -emu->bmi->bmiHeader.biHeight == (LONG)surfaceDesc2.dwHeight &&
				emu->bmi->bmiHeader.biBitCount == surfaceBitCount &&
				(!ColorMaskReq || ((DWORD*)emu->bmi->bmiColors)[0] == surfaceDesc2.ddpfPixelFormat.dwRBitMask &&
				((DWORD*)emu->bmi->bmiColors)[1] == surfaceDesc2.ddpfPixelFormat.dwGBitMask &&
					((DWORD*)emu->bmi->bmiColors)[2] == surfaceDesc2.ddpfPixelFormat.dwBBitMask))
			{
				return DD_OK;
			}

			// Save current emulated surface and prepare for creating a new one.
			if (ShareEmulatedMemory)
			{
				EnterCriticalSection(&smcs);
				memorySurfaces.push_back(emu);
				emu = nullptr;
				LeaveCriticalSection(&smcs);
			}
			else
			{
				DeleteSharedEmulatedMemory(&emu);
			}
		}
	}

	// If sharing memory than check the shared memory vector for a surface that matches
	if (ShareEmulatedMemory)
	{
		EnterCriticalSection(&smcs);
		for (auto it = memorySurfaces.begin(); it != memorySurfaces.end(); it++)
		{
			EMUSURFACE* pEmuSurface = *it;

			if (pEmuSurface->bmi->bmiHeader.biWidth == (LONG)surfaceDesc2.dwWidth && -pEmuSurface->bmi->bmiHeader.biHeight == (LONG)surfaceDesc2.dwHeight &&
				pEmuSurface->bmi->bmiHeader.biBitCount == surfaceBitCount &&
				(!ColorMaskReq || ((DWORD*)pEmuSurface->bmi->bmiColors)[0] == surfaceDesc2.ddpfPixelFormat.dwRBitMask &&
				((DWORD*)pEmuSurface->bmi->bmiColors)[1] == surfaceDesc2.ddpfPixelFormat.dwGBitMask &&
					((DWORD*)pEmuSurface->bmi->bmiColors)[2] == surfaceDesc2.ddpfPixelFormat.dwBBitMask))
			{
				emu = pEmuSurface;

				it = memorySurfaces.erase(it);

				break;
			}
		}
		LeaveCriticalSection(&smcs);

		if (emu)
		{
			ZeroMemory(emu->surfacepBits, emu->surfaceSize);

			return DD_OK;
		}
	}

	// Create new emulated surface structure
	emu = new EMUSURFACE;

	// Add some padding to surface size to a avoid overflow with some games
	DWORD padding = 200;

	// Create device context memory
	ZeroMemory(emu->bmiMemory, sizeof(emu->bmiMemory));
	emu->bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	emu->bmi->bmiHeader.biWidth = surfaceDesc2.dwWidth;
	emu->bmi->bmiHeader.biHeight = -(LONG)(surfaceDesc2.dwHeight + padding);
	emu->bmi->bmiHeader.biPlanes = 1;
	emu->bmi->bmiHeader.biBitCount = (WORD)surfaceBitCount;
	emu->bmi->bmiHeader.biCompression = (ColorMaskReq) ? BI_BITFIELDS : BI_RGB;
	emu->bmi->bmiHeader.biSizeImage = surfaceDesc2.dwWidth * (surfaceBitCount / 8) * surfaceDesc2.dwHeight;

	if (surfaceBitCount == 8)
	{
		for (int i = 0; i < 256; i++)
		{
			emu->bmi->bmiColors[i].rgbRed = (byte)i;
			emu->bmi->bmiColors[i].rgbGreen = (byte)i;
			emu->bmi->bmiColors[i].rgbBlue = (byte)i;
			emu->bmi->bmiColors[i].rgbReserved = 0;
		}
	}
	else if (ColorMaskReq)
	{
		((DWORD*)emu->bmi->bmiColors)[0] = surfaceDesc2.ddpfPixelFormat.dwRBitMask;
		((DWORD*)emu->bmi->bmiColors)[1] = surfaceDesc2.ddpfPixelFormat.dwGBitMask;
		((DWORD*)emu->bmi->bmiColors)[2] = surfaceDesc2.ddpfPixelFormat.dwBBitMask;
	}

	emu->surfaceDC = CreateCompatibleDC(ddrawParent->GetDC());
	if (!emu->surfaceDC)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to create compatible DC!");
		return DDERR_GENERIC;
	}
	emu->bitmap = CreateDIBSection(ddrawParent->GetDC(), emu->bmi, (surfaceBitCount == 8) ? DIB_PAL_COLORS : DIB_RGB_COLORS, (void **)&emu->surfacepBits, nullptr, 0);
	emu->bmi->bmiHeader.biHeight = -(LONG)surfaceDesc2.dwHeight;
	if (!emu->bitmap)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to create DIB section!");
		return DDERR_GENERIC;
	}
	emu->OldDCObject = SelectObject(emu->surfaceDC, emu->bitmap);
	if (!emu->OldDCObject)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: failed to replace object in DC!");
		return DDERR_GENERIC;
	}
	emu->surfacePitch = surfaceDesc2.dwWidth * (surfaceBitCount / 8);
	emu->surfaceSize = surfaceDesc2.dwHeight * emu->surfacePitch;
	PaletteUSN++;

	return DD_OK;
}

template <typename T>
void m_IDirectDrawSurfaceX::ReleaseD9Interface(T **ppInterface)
{
	if (ppInterface && *ppInterface)
	{
		DWORD x = 0, z = 0;
		do
		{
			z = (*ppInterface)->Release();
		} while (z != 0 && ++x < 100);

		// Error checking
		if (z != 0)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to release Direct3D9 interface");
		}

		(*ppInterface) = nullptr;
	}
}

// Release surface and vertext buffer
void m_IDirectDrawSurfaceX::ReleaseD9Surface(bool BackupData)
{
	// Store d3d9 surface texture
	if (BackupData && surfaceTexture)
	{
		Logging::LogDebug() << __FUNCTION__ << " Storing Direct3D9 texture surface data";
		do {
			surfaceTexture->UnlockRect(0);

			D3DLOCKED_RECT LockRect;
			if (FAILED(surfaceTexture->LockRect(0, &LockRect, nullptr, 0)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock texture surface!");
				break;
			}

			size_t size = surfaceDesc2.dwHeight * LockRect.Pitch;

			surfaceBackup.resize(size);

			memcpy(&surfaceBackup[0], LockRect.pBits, size);

			surfaceTexture->UnlockRect(0);
		} while (false);
	}

	// Release d3d9 context surface
	if (contextSurface)
	{
		Logging::LogDebug() << __FUNCTION__ << " Releasing Direct3D9 context surface";
		contextSurface->Release();
		contextSurface = nullptr;
	}

	// Release d3d9 surface texture
	if (surfaceTexture)
	{
		Logging::LogDebug() << __FUNCTION__ << " Releasing Direct3D9 texture surface";
		surfaceTexture->UnlockRect(0);
		ReleaseD9Interface(&surfaceTexture);
	}

	// Release d3d9 display surface texture
	if (displayTexture)
	{
		Logging::LogDebug() << __FUNCTION__ << " Releasing Direct3D9 display texture surface";
		ReleaseD9Interface(&displayTexture);
	}

	// Release d3d9 palette surface texture
	if (paletteTexture)
	{
		Logging::LogDebug() << __FUNCTION__ << " Releasing Direct3D9 palette texture surface";
		paletteTexture->UnlockRect(0);
		ReleaseD9Interface(&paletteTexture);
	}

	// Release d3d9 pixel shader
	if (pixelShader)
	{
		Logging::LogDebug() << __FUNCTION__ << " Releasing Direct3D9 pixel shader";
		ReleaseD9Interface(&pixelShader);
	}

	// Release d3d9 vertex buffer
	if (vertexBuffer)
	{
		Logging::LogDebug() << __FUNCTION__ << " Releasing Direct3D9 vertext buffer";
		ReleaseD9Interface(&vertexBuffer);
	}

	// Emulated surface
	if (emu && !BackupData)
	{
		if (!ShareEmulatedMemory || !emu->surfaceDC || !emu->surfacepBits)
		{
			DeleteSharedEmulatedMemory(&emu);
		}
		else
		{
			EnterCriticalSection(&smcs);
			memorySurfaces.push_back(emu);
			emu = nullptr;
			LeaveCriticalSection(&smcs);
		}
	}

	// Set flags
	IsInDC = false;
	IsLocked = false;
}

// Present surface
HRESULT m_IDirectDrawSurfaceX::PresentSurface(BOOL isSkipScene)
{
	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true, true)))
	{
		return DDERR_GENERIC;
	}

	// Check if is not primary surface or if scene should be skipped
	if (!IsPrimarySurface() && SceneReady && !IsPresentRunning)
	{
		m_IDirectDrawSurfaceX *lpDDSrcSurfaceX = ddrawParent->GetPrimarySurface();
		if (lpDDSrcSurfaceX)
		{
			lpDDSrcSurfaceX->PresentSurface(isSkipScene);
		}
		return DDERR_GENERIC;
	}
	else if (!IsPrimarySurface())
	{
		return DDERR_GENERIC;
	}
	else if (isSkipScene && !SceneReady)
	{
		Logging::LogDebug() << __FUNCTION__ << " Skipping scene!";
		return DDERR_GENERIC;
	}

	// Set scene ready
	SceneReady = true;

	// Check if surface is locked or has an open DC
	if (WaitForLockState() || IsSurfaceInDC() || IsPresentRunning)
	{
		Logging::LogDebug() << __FUNCTION__ << " Surface is busy!";
		return DDERR_SURFACEBUSY;
	}
	IsPresentRunning = true;

	// Preset surface
	HRESULT hr = DD_OK;
	do {
		// Set texture
		if (displayTexture)
		{
			if (FAILED((*d3d9Device)->UpdateTexture(surfaceTexture, displayTexture)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to update texture");
				hr = DDERR_GENERIC;
				break;
			}
			if (FAILED((*d3d9Device)->SetTexture(0, displayTexture)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set texture");
				hr = DDERR_GENERIC;
				break;
			}
		}
		else if (surfaceTexture)
		{
			if (FAILED((*d3d9Device)->SetTexture(0, surfaceTexture)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set texture");
				hr = DDERR_GENERIC;
				break;
			}
		}

		// Set palette texture
		if (paletteTexture)
		{
			UpdatePaletteData();
			if (FAILED((*d3d9Device)->SetTexture(1, paletteTexture)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock palette texture!");
				hr = DDERR_GENERIC;
				break;
			}
		}

		// Set pixel shader
		if (pixelShader)
		{
			if (FAILED((*d3d9Device)->SetPixelShader(pixelShader)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to set pixel shader");
				hr = DDERR_GENERIC;
				break;
			}
		}

		// EndScene
		if (FAILED(ddrawParent->EndScene()))
		{
			hr = DDERR_GENERIC;
			break;
		}

		// Reset dirty flag
		dirtyFlag = false;

		// Reset scene ready
		SceneReady = false;

	} while (false);

	// Reset present flag
	IsPresentRunning = false;

	return hr;
}

// Reset primary surface display settings
void m_IDirectDrawSurfaceX::RestoreSurfaceDisplay()
{
	// Reset surface desc
	if (RestoreSurfaceFlags)
	{
		surfaceDesc2.dwFlags &= ~(RestoreSurfaceFlags);
	}
}

// Swap surface addresses for Flip
void m_IDirectDrawSurfaceX::SwapSurface(m_IDirectDrawSurfaceX *lpTargetSurface1, m_IDirectDrawSurfaceX *lpTargetSurface2)
{
	if (!lpTargetSurface1 || !lpTargetSurface2 || lpTargetSurface1 == lpTargetSurface2)
	{
		return;
	}

	// Swap surface textures
	SwapAddresses(lpTargetSurface1->GetSurfaceTexture(), lpTargetSurface2->GetSurfaceTexture());

	// Swap context surfaces
	SwapAddresses(lpTargetSurface1->GetContextSurface(), lpTargetSurface2->GetContextSurface());

	// Swap emulated surfaces
	SwapAddresses(lpTargetSurface1->GetEmulatedSurface(), lpTargetSurface2->GetEmulatedSurface());
}

// Check surface reck dimensions and copy rect to new rect
bool m_IDirectDrawSurfaceX::CheckCoordinates(LPRECT lpOutRect, LPRECT lpInRect)
{
	if (!lpOutRect)
	{
		return false;
	}

	// Check device coordinates
	if (!surfaceDesc2.dwWidth || !surfaceDesc2.dwHeight)
	{
		return false;
	}

	if (lpInRect)
	{
		lpOutRect->left = lpInRect->left;
		lpOutRect->top = lpInRect->top;
		lpOutRect->right = lpInRect->right;
		lpOutRect->bottom = lpInRect->bottom;
	}
	else
	{
		lpOutRect->left = 0;
		lpOutRect->top = 0;
		lpOutRect->right = surfaceDesc2.dwWidth;
		lpOutRect->bottom = surfaceDesc2.dwHeight;
	}

	if (lpOutRect->left < 0)
	{
		lpOutRect->left = 0;
	}

	if (lpOutRect->top < 0)
	{
		lpOutRect->top = 0;
	}

	if (lpOutRect->left > (LONG)surfaceDesc2.dwWidth)
	{
		lpOutRect->left = surfaceDesc2.dwWidth;
	}

	if (lpOutRect->right > (LONG)surfaceDesc2.dwWidth)
	{
		lpOutRect->right = surfaceDesc2.dwWidth;
	}

	if (lpOutRect->top > (LONG)surfaceDesc2.dwHeight)
	{
		lpOutRect->top = surfaceDesc2.dwHeight;
	}

	if (lpOutRect->bottom > (LONG)surfaceDesc2.dwHeight)
	{
		lpOutRect->bottom = surfaceDesc2.dwHeight;
	}

	return lpOutRect->left < lpOutRect->right && lpOutRect->top < lpOutRect->bottom;
}

// Wait for other thread to unlock surface
bool m_IDirectDrawSurfaceX::WaitForLockState()
{
	if (IsLocked && LockThreadID != GetCurrentThreadId())
	{
		do {
			Sleep(0);
		} while (IsLocked);
	}

	return IsLocked;
}

// Lock the d3d9 surface
HRESULT m_IDirectDrawSurfaceX::SetLock(D3DLOCKED_RECT* pLockedRect, LPRECT lpDestRect, DWORD dwFlags, BOOL isSkipScene)
{
	if (!pLockedRect)
	{
		return DDERR_INVALIDPARAMS;
	}

	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true, true)))
	{
		return DDERR_GENERIC;
	}

	// Check if the scene needs to be presented
	isSkipScene |= ((lpDestRect) ? (abs(lpDestRect->bottom - lpDestRect->top) < 2 || abs(lpDestRect->right - lpDestRect->left) < 2) : FALSE);

	// Check if locked
	if (IsLocked)
	{
		// Wait for other threads, unlock and try again
		WaitForLockState();
		if (IsLocked)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Locking surface twice not implemented!");
		}
	}

	// Run EndScene before locking if dirty flag is set
	if (dirtyFlag)
	{
		if (SUCCEEDED(PresentSurface(isSkipScene)))
		{
			EndSceneLock = true;
		}
	}

	// Emulated surface
	if (IsSurfaceEmulated)
	{
		// Set locked rect
		if (FAILED(LockEmulatedSurface(pLockedRect, lpDestRect)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock emulated surface!");
			return DDERR_GENERIC;
		}

		// Backup last rect
		CheckCoordinates(&LastRect, lpDestRect);
		DoCopyRect = ((dwFlags & D3DLOCK_READONLY) == 0);
	}
	// Lock surface
	else
	{
		// Try to lock the rect
		HRESULT hr = surfaceTexture->LockRect(0, pLockedRect, lpDestRect, dwFlags);
		if (FAILED(hr))
		{
			// If lock fails unlock and try again
			SetUnlock(true);
			if (FAILED(surfaceTexture->LockRect(0, pLockedRect, lpDestRect, dwFlags)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock surface");
				return (hr == D3DERR_INVALIDCALL) ? DDERR_GENERIC :
					(hr == D3DERR_WASSTILLDRAWING) ? DDERR_WASSTILLDRAWING :
					DDERR_SURFACELOST;
			}
		}
	}

	// Set lock flag
	InterlockedExchange(&LockThreadID, GetCurrentThreadId());
	IsLocked = true;

	// Set pitch
	surfaceDesc2.dwFlags |= DDSD_PITCH;
	surfaceDesc2.lPitch = pLockedRect->Pitch;

	// Set dirty flag
	dirtyFlag = true;

	// Success
	return DD_OK;
}

// Unlock the d3d9 surface
HRESULT m_IDirectDrawSurfaceX::SetUnlock(BOOL isSkipScene)
{
	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true, true)))
	{
		return DDERR_GENERIC;
	}

	// Emulated surface
	if (IsSurfaceEmulated)
	{
		// Copy emulated surface to real texture
		if (DoCopyRect)
		{
			CopyEmulatedSurface(&LastRect, true);

			// Reset copy flag
			DoCopyRect = false;
		}
	}
	// Lock surface
	else
	{
		HRESULT hr = surfaceTexture->UnlockRect(0);
		if (FAILED(hr))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to unlock surface");
			return (hr == D3DERR_INVALIDCALL) ? DDERR_GENERIC :
				(hr == D3DERR_WASSTILLDRAWING) ? DDERR_WASSTILLDRAWING :
				DDERR_SURFACELOST;
		}
	}
	IsLocked = false;

	// Present surface
	if (!EndSceneLock)
	{
		PresentSurface(isSkipScene);
	}

	// Reset endscene lock
	EndSceneLock = false;

	return DD_OK;
}

HRESULT m_IDirectDrawSurfaceX::LockEmulatedSurface(D3DLOCKED_RECT* pLockedRect, LPRECT lpDestRect)
{
	if (!pLockedRect || !emu || !emu->surfacepBits)
	{
		return DDERR_GENERIC;
	}

	pLockedRect->Pitch = emu->surfacePitch;
	DWORD Delta = (lpDestRect && (lpDestRect->top || lpDestRect->left)) ? ((lpDestRect->top * pLockedRect->Pitch) + (lpDestRect->left * (surfaceBitCount / 8))) : 0;
	pLockedRect->pBits = (void*)((DWORD)emu->surfacepBits + Delta);

	return DD_OK;
}

// Update surface description and create backbuffers
void m_IDirectDrawSurfaceX::InitSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc2, DWORD DirectXVersion)
{
	if (!lpDDSurfaceDesc2)
	{
		return;
	}

	// Backup missing display surface description flags from primary surface
	if (surfaceDesc2.ddsCaps.dwCaps4 & DDSCAPS4_PRIMARYSURFACE)
	{
		RestoreSurfaceFlags = ((DDSD_WIDTH | DDSD_HEIGHT | DDSD_REFRESHRATE | DDSD_PIXELFORMAT) & ~surfaceDesc2.dwFlags);
	}

	// Copy surface description
	surfaceDesc2.dwSize = sizeof(DDSURFACEDESC2);
	surfaceDesc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ConvertSurfaceDesc(surfaceDesc2, *lpDDSurfaceDesc2);

	// Update dds caps flags
	surfaceDesc2.dwFlags |= DDSD_CAPS;
	if (surfaceDesc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		surfaceDesc2.ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
	}
	if (!(surfaceDesc2.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) && !(surfaceDesc2.ddsCaps.dwCaps2 & DDSCAPS2_DONOTPERSIST))
	{
		surfaceDesc2.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
		surfaceDesc2.ddsCaps.dwCaps &= ~DDSCAPS_NONLOCALVIDMEM;
	}

	// Create backbuffers
	if (surfaceDesc2.dwBackBufferCount)
	{
		DDSURFACEDESC2 Desc2;
		Desc2.dwSize = sizeof(DDSURFACEDESC2);
		Desc2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ConvertSurfaceDesc(Desc2, surfaceDesc2);
		Desc2.ddsCaps.dwCaps4 &= ~(DDSCAPS4_CREATESURFACE);	// Clear surface creation flag
		Desc2.dwBackBufferCount--;
		if (Desc2.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
		{
			Desc2.ddsCaps.dwCaps &= ~(DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER);
			Desc2.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
			Desc2.dwReserved = (DWORD)this;
		}
		else if (Desc2.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
		{
			Desc2.ddsCaps.dwCaps &= ~DDSCAPS_BACKBUFFER;
		}

		// Create complex surfaces
		if (Desc2.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
		{
			if (surfaceDesc2.ddsCaps.dwCaps4 & DDSCAPS4_CREATESURFACE)
			{
				ComplexRoot = true;
			}

			BackBufferInterface = std::make_unique<m_IDirectDrawSurfaceX>(d3d9Device, ddrawParent, DirectXVersion, &Desc2, displayWidth, displayHeight);

			m_IDirectDrawSurfaceX *attachedSurface = BackBufferInterface.get();

			AddAttachedSurfaceToMap(attachedSurface);

			attachedSurface->AddRef();
		}
		else
		{
			m_IDirectDrawSurfaceX *attachedSurface = new m_IDirectDrawSurfaceX(d3d9Device, ddrawParent, DirectXVersion, &Desc2, displayWidth, displayHeight);

			AddAttachedSurfaceToMap(attachedSurface);
		}
	}

	// Add first surface as attached surface to the last surface in a surface chain
	else if (surfaceDesc2.dwReserved)
	{
		m_IDirectDrawSurfaceX *attachedSurface = (m_IDirectDrawSurfaceX *)surfaceDesc2.dwReserved;

		// Check if source Surface exists and add to surface map
		if (ddrawParent->DoesSurfaceExist(attachedSurface))
		{
			AddAttachedSurfaceToMap(attachedSurface);
		}
	}

	// Clear comlpex flag if not part of a complex structure
	else
	{
		surfaceDesc2.dwFlags &= ~DDSCAPS_COMPLEX;
	}

	// Clear flags used in creating a surface structure
	if (!(surfaceDesc2.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER))
	{
		surfaceDesc2.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
		surfaceDesc2.dwBackBufferCount = 0;
	}
	surfaceDesc2.ddsCaps.dwCaps4 = 0x00;
	surfaceDesc2.dwReserved = 0;

	// Set ColorKey
	if (surfaceDesc2.dwFlags & DDSD_CKDESTOVERLAY)
	{
		SetColorKey(DDCKEY_DESTOVERLAY | DDCKEY_COLORSPACE, &surfaceDesc2.ddckCKDestOverlay);
	}
	if (surfaceDesc2.dwFlags & DDSD_CKDESTBLT)
	{
		SetColorKey(DDCKEY_DESTBLT | DDCKEY_COLORSPACE, &surfaceDesc2.ddckCKDestBlt);
	}
	if (surfaceDesc2.dwFlags & DDSD_CKSRCOVERLAY)
	{
		SetColorKey(DDCKEY_SRCOVERLAY | DDCKEY_COLORSPACE, &surfaceDesc2.ddckCKSrcOverlay);
	}
	if (surfaceDesc2.dwFlags & DDSD_CKSRCBLT)
	{
		SetColorKey(DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &surfaceDesc2.ddckCKSrcBlt);
	}
	surfaceDesc2.dwFlags &= ~(DDSD_CKDESTOVERLAY | DDSD_CKDESTBLT | DDSD_CKSRCOVERLAY | DDSD_CKSRCBLT);

	// Update surface description
	GetSurfaceDesc2(&surfaceDesc2);
}

// Add attached surface to map
void m_IDirectDrawSurfaceX::AddAttachedSurfaceToMap(m_IDirectDrawSurfaceX* lpSurfaceX, bool MarkAttached)
{
	if (!lpSurfaceX)
	{
		return;
	}

	// Store surface
	AttachedSurfaceMap[++MapKey].pSurface = lpSurfaceX;
	if (MarkAttached)
	{
		AttachedSurfaceMap[MapKey].isAttachedSurfaceAdded = true;
	}
}

// Remove attached surface from map
void m_IDirectDrawSurfaceX::RemoveAttachedSurfaceFromMap(m_IDirectDrawSurfaceX* lpSurfaceX)
{
	if (!lpSurfaceX)
	{
		return;
	}

	auto it = std::find_if(AttachedSurfaceMap.begin(), AttachedSurfaceMap.end(),
		[=](auto Map) -> bool { return Map.second.pSurface == lpSurfaceX; });

	if (it != std::end(AttachedSurfaceMap))
	{
		AttachedSurfaceMap.erase(it);
	}
}

// Check if attached surface exists
bool m_IDirectDrawSurfaceX::DoesAttachedSurfaceExist(m_IDirectDrawSurfaceX* lpSurfaceX)
{
	if (!lpSurfaceX)
	{
		return false;
	}

	auto it = std::find_if(AttachedSurfaceMap.begin(), AttachedSurfaceMap.end(),
		[=](auto Map) -> bool { return Map.second.pSurface == lpSurfaceX; });

	if (it == std::end(AttachedSurfaceMap))
	{
		return false;
	}

	return true;
}

bool m_IDirectDrawSurfaceX::WasAttachedSurfaceAdded(m_IDirectDrawSurfaceX* lpSurfaceX)
{
	if (!lpSurfaceX)
	{
		return false;
	}

	auto it = std::find_if(AttachedSurfaceMap.begin(), AttachedSurfaceMap.end(),
		[=](auto Map) -> bool { return (Map.second.pSurface == lpSurfaceX) && Map.second.isAttachedSurfaceAdded; });

	if (it == std::end(AttachedSurfaceMap))
	{
		return false;
	}

	return true;
}

// Check if backbuffer surface exists
bool m_IDirectDrawSurfaceX::DoesFlipBackBufferExist(m_IDirectDrawSurfaceX* lpSurfaceX)
{
	if (!lpSurfaceX)
	{
		return false;
	}

	DWORD dwCaps = 0;
	m_IDirectDrawSurfaceX *lpTargetSurface = nullptr;

	// Loop through each surface
	for (auto it : AttachedSurfaceMap)
	{
		dwCaps = it.second.pSurface->GetSurfaceCaps().dwCaps;
		if (dwCaps & DDSCAPS_FLIP)
		{
			lpTargetSurface = it.second.pSurface;

			break;
		}
	}

	// Check if attached surface was not found
	if (!lpTargetSurface || (dwCaps & DDSCAPS_FRONTBUFFER))
	{
		return false;
	}

	// Check if attached surface was found
	if (lpTargetSurface == lpSurfaceX)
	{
		return true;
	}

	// Check next surface
	return lpTargetSurface->DoesFlipBackBufferExist(lpSurfaceX);
}

HRESULT m_IDirectDrawSurfaceX::ColorFill(RECT* pRect, D3DCOLOR dwFillColor)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true, true)))
	{
		return DDERR_GENERIC;
	}

	HRESULT hr = DD_OK;
	bool UnlockDest = false;
	do {
		// Check and copy rect
		RECT DestRect;
		if (!CheckCoordinates(&DestRect, pRect))
		{
			hr = DDERR_INVALIDRECT;
			break;
		}

		// Check if surface is not locked then lock it
		D3DLOCKED_RECT DestLockRect;
		if (FAILED(SetLock(&DestLockRect, &DestRect, 0, true)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: could not lock dest surface " << DestRect);
			hr = (IsLocked) ? DDERR_SURFACEBUSY : DDERR_GENERIC;
			break;
		}
		UnlockDest = true;

		// Get byte count
		DWORD ByteCount = DestLockRect.Pitch / surfaceDesc2.dwWidth;
		if (!ByteCount || ByteCount > 4)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: could not find correct fill color for ByteCount " << ByteCount);
			hr = DDERR_GENERIC;
			break;
		}

		// Get width and height of rect
		LONG FillWidth = DestRect.right - DestRect.left;
		LONG FillHeight = DestRect.bottom - DestRect.top;

		// Set memory address
		BYTE *SrcBuffer = (BYTE*)&dwFillColor;
		BYTE *DestBuffer = (BYTE*)DestLockRect.pBits;

		// Fill first line memory
		for (LONG x = 0; x < FillWidth; x++)
		{
			for (DWORD y = 0; y < ByteCount; y++)
			{
				*DestBuffer = SrcBuffer[y];
				DestBuffer++;
			}
		}

		// Fill rest of surface rect using the first line as a template
		SrcBuffer = (BYTE*)DestLockRect.pBits;
		DestBuffer = (BYTE*)DestLockRect.pBits + DestLockRect.Pitch;
		size_t size = FillWidth * ByteCount;
		for (LONG y = 1; y < FillHeight; y++)
		{
			memcpy(DestBuffer, SrcBuffer, size);
			DestBuffer += DestLockRect.Pitch;
		}

	} while (false);

	// Unlock surfaces if needed
	if (UnlockDest)
	{
		SetUnlock(true);
	}

	return hr;
}

// Copy surface
HRESULT m_IDirectDrawSurfaceX::CopySurface(m_IDirectDrawSurfaceX* pSourceSurface, RECT* pSourceRect, RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter, DDCOLORKEY ColorKey, DWORD dwFlags)
{
	UNREFERENCED_PARAMETER(Filter);

	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	// Check parameters
	if (!pSourceSurface)
	{
		LOG_LIMIT(100, __FUNCTION__ << " Error: invalid parameters!");
		return DDERR_INVALIDPARAMS;
	}

	// Check for device interface
	if (FAILED(CheckInterface(__FUNCTION__, true, true)) || FAILED(pSourceSurface->CheckInterface(__FUNCTION__, true, true)))
	{
		return DDERR_GENERIC;
	}

	HRESULT hr = DD_OK;
	bool UnlockSrc = false, UnlockDest = false;
	bool IsCriticalSectionSet = false;
	do {
		D3DLOCKED_RECT SrcLockRect, DestLockRect;
		DWORD DestBitCount = GetSurfaceBitCount();
		D3DFORMAT SrcFormat = pSourceSurface->GetSurfaceFormat();
		D3DFORMAT DestFormat = GetSurfaceFormat();

		// Get byte count
		DWORD ByteCount = DestBitCount / 8;
		if (!ByteCount || ByteCount > 4)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: wrong bit count " << DestBitCount);
			hr = DDERR_GENERIC;
			break;
		}

		// Check source and destination format
		if (!(SrcFormat == DestFormat ||
			((SrcFormat == D3DFMT_A1R5G5B5 || SrcFormat == D3DFMT_X1R5G5B5) && (DestFormat == D3DFMT_A1R5G5B5 || DestFormat == D3DFMT_X1R5G5B5)) ||
			((SrcFormat == D3DFMT_A8R8G8B8 || SrcFormat == D3DFMT_X8R8G8B8) && (DestFormat == D3DFMT_A8R8G8B8 || DestFormat == D3DFMT_X8R8G8B8)) ||
			((SrcFormat == D3DFMT_A8B8G8R8 || SrcFormat == D3DFMT_X8B8G8R8) && (DestFormat == D3DFMT_A8B8G8R8 || DestFormat == D3DFMT_X8B8G8R8))))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: not supported for different source and destination formats! " << SrcFormat << "-->" << DestFormat);
			hr = DDERR_GENERIC;
			break;
		}

		// Get source and dest rect
		RECT SrcRect = { 0, 0, (LONG)pSourceSurface->GetWidth(), (LONG)pSourceSurface->GetHeight() };
		RECT DestRect = { 0, 0, (LONG)surfaceDesc2.dwWidth, (LONG)surfaceDesc2.dwHeight };

		if (pSourceRect)
		{
			SrcRect.left = pSourceRect->left;
			SrcRect.top = pSourceRect->top;
			SrcRect.right = pSourceRect->right;
			SrcRect.bottom = pSourceRect->bottom;
		}

		if (pDestRect)
		{
			DestRect.left = pDestRect->left;
			DestRect.top = pDestRect->top;
			DestRect.right = pDestRect->right;
			DestRect.bottom = pDestRect->bottom;
		}

		// Get copy flags
		bool IsStretchRect = (abs((DestRect.right - DestRect.left) - (SrcRect.right - SrcRect.left)) > 1 || abs((DestRect.bottom - DestRect.top) - (SrcRect.bottom - SrcRect.top)) > 1);
		bool IsColorKey = ((dwFlags & DDBLT_KEYDEST) != 0);
		bool IsMirrorLeftRight = ((dwFlags & DDBLTFX_MIRRORLEFTRIGHT) != 0);
		bool IsMirrorUpDown = ((dwFlags & DDBLTFX_MIRRORUPDOWN) != 0);

		// Check and copy rect and do clipping
		if (!pSourceSurface->CheckCoordinates(&SrcRect, pSourceRect) || !CheckCoordinates(&DestRect, pDestRect))
		{
			hr = DDERR_INVALIDRECT;
			break;
		}

		// Get width and height of rect
		LONG SrcRectWidth = SrcRect.right - SrcRect.left;
		LONG SrcRectHeight = SrcRect.bottom - SrcRect.top;
		LONG DestRectWidth = DestRect.right - DestRect.left;
		LONG DestRectHeight = DestRect.bottom - DestRect.top;

		if (!IsStretchRect)
		{
			SrcRectWidth = min(SrcRectWidth, DestRectWidth);
			SrcRectHeight = min(SrcRectHeight, DestRectHeight);
			DestRectWidth = min(SrcRectWidth, DestRectWidth);
			DestRectHeight = min(SrcRectHeight, DestRectHeight);
		}

		// Check if source surface is not locked then lock it
		if (FAILED(pSourceSurface->SetLock(&SrcLockRect, &SrcRect, D3DLOCK_READONLY, true)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: could not lock source surface " << SrcRect);
			hr = (pSourceSurface->IsSurfaceLocked()) ? DDERR_SURFACEBUSY : DDERR_GENERIC;
			break;
		}
		UnlockSrc = true;

		// Check if source and destination memory addresses are overlapping
		if (this == pSourceSurface)
		{
			IsCriticalSectionSet = true;
			EnterCriticalSection(&ddscs);

			size_t size = SrcRectWidth * ByteCount * SrcRectHeight;
			if (size > surfaceArray.size())
			{
				surfaceArray.resize(size);
			}
			BYTE *SrcBuffer = (BYTE*)SrcLockRect.pBits;
			BYTE *DestBuffer = (BYTE*)&surfaceArray[0];
			INT DestPitch = SrcRectWidth * ByteCount;
			for (LONG y = 0; y < SrcRectHeight; y++)
			{
				memcpy(DestBuffer, SrcBuffer, SrcRectWidth * ByteCount);
				SrcBuffer += SrcLockRect.Pitch;
				DestBuffer += DestPitch;
			}
			SrcLockRect.pBits = &surfaceArray[0];
			SrcLockRect.Pitch = DestPitch;
			SetUnlock(true);
			UnlockSrc = false;
		}

		// Check if destination surface is not locked then lock it
		if (FAILED(SetLock(&DestLockRect, &DestRect, 0, true)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: could not lock destination surface " << DestRect);
			hr = (IsLocked) ? DDERR_SURFACEBUSY : DDERR_GENERIC;
			break;
		}
		UnlockDest = true;

		// Create buffer variables
		BYTE *SrcBuffer = (BYTE*)SrcLockRect.pBits;
		BYTE *DestBuffer = (BYTE*)DestLockRect.pBits;

		// For mirror copy up/down
		INT DestPitch = DestLockRect.Pitch;
		if (IsMirrorUpDown)
		{
			DestPitch = -DestLockRect.Pitch;
			DestBuffer += DestLockRect.Pitch * (DestRectHeight - 1);
		}

		// Copy memory (simple)
		if (!IsStretchRect && !IsColorKey & !IsMirrorLeftRight)
		{
			if ((DestRectWidth * (INT)ByteCount) == DestPitch && SrcLockRect.Pitch == DestLockRect.Pitch)
			{
				memcpy(DestBuffer, SrcBuffer, DestRectHeight * DestLockRect.Pitch);
			}
			else
			{
				for (LONG y = 0; y < DestRectHeight; y++)
				{
					memcpy(DestBuffer, SrcBuffer, DestRectWidth * ByteCount);
					SrcBuffer += SrcLockRect.Pitch;
					DestBuffer += DestPitch;
				}
			}
			break;
		}

		// Get ratio
		float WidthRatio = (float)SrcRectWidth / (float)DestRectWidth;
		float HeightRatio = (float)SrcRectHeight / (float)DestRectHeight;

		// Set color varables
		DWORD ByteMask = (ByteCount == 1) ? 0x000000FF : (ByteCount == 2) ? 0x0000FFFF : (ByteCount == 3) ? 0x00FFFFFF : 0xFFFFFFFF;
		DWORD ColorKeyLow = ColorKey.dwColorSpaceLowValue & ByteMask;
		DWORD ColorKeyHigh = ColorKey.dwColorSpaceHighValue & ByteMask;

		// Copy memory (complex)
		for (LONG y = 0; y < DestRectHeight; y++)
		{
			BYTE *LoopBuffer = DestBuffer;
			for (LONG x = 0; x < DestRectWidth; x++)
			{
				DWORD r = (IsStretchRect) ? (DWORD)((float)x * WidthRatio) : x;
				BYTE *NewPixel = (IsMirrorLeftRight) ? SrcBuffer + ((SrcRectWidth - r - 1) * ByteCount) : SrcBuffer + (r * ByteCount);
				DWORD PixelColor = (*(DWORD*)NewPixel) & ByteMask;

				if (!IsColorKey || PixelColor < ColorKeyLow || PixelColor > ColorKeyHigh)
				{
					for (DWORD i = 0; i < ByteCount; i++)
					{
						*LoopBuffer = NewPixel[i];
						LoopBuffer++;
					}
				}
				else
				{
					LoopBuffer += ByteCount;
				}
			}
			SrcBuffer = (IsStretchRect) ? (BYTE*)SrcLockRect.pBits + (DWORD)((float)y * HeightRatio) * SrcLockRect.Pitch : SrcBuffer + SrcLockRect.Pitch;
			DestBuffer += DestPitch;
		}

	} while (false);

	// Leave Critical Section if needed
	if (IsCriticalSectionSet)
	{
		LeaveCriticalSection(&ddscs);
	}

	// Unlock surfaces if needed
	if (UnlockDest)
	{
		SetUnlock(true);
	}
	if (UnlockSrc)
	{
		pSourceSurface->SetUnlock(true);
	}

	// Return
	return hr;
}

// Copy from emulated surface
HRESULT m_IDirectDrawSurfaceX::CopyEmulatedSurface(LPRECT lpDestRect, bool CopyToRealSurfaceTexture)
{
	HRESULT hr = D3D_OK;

	do {
		// Update rect
		RECT DestRect;
		if (!CheckCoordinates(&DestRect, lpDestRect))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: invalid rect!");
			hr = DDERR_INVALIDRECT;
			break;
		}

		// Check if destination surface is not locked then lock it
		D3DLOCKED_RECT EmulatedLockRect, SurfaceLockRect;
		if (FAILED(LockEmulatedSurface(&EmulatedLockRect, &DestRect)) ||
			FAILED(surfaceTexture->LockRect(0, &SurfaceLockRect, &DestRect, (!CopyToRealSurfaceTexture) ? D3DLOCK_READONLY : 0)))
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: could not lock surface!");
			hr = DDERR_GENERIC;
			break;
		}

		// Create buffer variables
		BYTE *EmulatedBuffer = (BYTE*)EmulatedLockRect.pBits;
		BYTE *SurfaceBuffer = (BYTE*)SurfaceLockRect.pBits;
		INT WidthPitch = (SurfaceLockRect.Pitch / surfaceDesc2.dwWidth) * (DestRect.right - DestRect.left);
		LONG RectHeight = (DestRect.bottom - DestRect.top);

		// Copy data
		switch (surfaceFormat)
		{
		case D3DFMT_R8G8B8:
			if (CopyToRealSurfaceTexture)
			{
				for (LONG x = DestRect.top; x < DestRect.bottom; x++)
				{
					TRIBYTE *EmulatedBufferLoop = (TRIBYTE*)EmulatedBuffer;
					DWORD *SurfaceBufferLoop = (DWORD*)SurfaceBuffer;
					for (LONG y = DestRect.left; y < DestRect.right; y++)
					{
						*SurfaceBufferLoop = *(DWORD*)EmulatedBufferLoop;
						EmulatedBufferLoop++;
						SurfaceBufferLoop++;
					}
					EmulatedBuffer += EmulatedLockRect.Pitch;
					SurfaceBuffer += SurfaceLockRect.Pitch;
				}
			}
			else
			{
				for (LONG x = DestRect.top; x < DestRect.bottom; x++)
				{
					TRIBYTE *EmulatedBufferLoop = (TRIBYTE*)EmulatedBuffer;
					DWORD *SurfaceBufferLoop = (DWORD*)SurfaceBuffer;
					for (LONG y = DestRect.left; y < DestRect.right; y++)
					{
						*EmulatedBufferLoop = *(TRIBYTE*)SurfaceBufferLoop;
						EmulatedBufferLoop++;
						SurfaceBufferLoop++;
					}
					EmulatedBuffer += EmulatedLockRect.Pitch;
					SurfaceBuffer += SurfaceLockRect.Pitch;
				}
			}
			break;
		default:
			if (CopyToRealSurfaceTexture)
			{
				if (WidthPitch == SurfaceLockRect.Pitch)
				{
					memcpy(SurfaceBuffer, EmulatedBuffer, SurfaceLockRect.Pitch * RectHeight);
				}
				else
				{
					for (LONG x = DestRect.top; x < DestRect.bottom; x++)
					{
						memcpy(SurfaceBuffer, EmulatedBuffer, WidthPitch);
						EmulatedBuffer += EmulatedLockRect.Pitch;
						SurfaceBuffer += SurfaceLockRect.Pitch;
					}
				}
			}
			else
			{
				if (WidthPitch == EmulatedLockRect.Pitch)
				{
					memcpy(EmulatedBuffer, SurfaceBuffer, EmulatedLockRect.Pitch * RectHeight);
				}
				else
				{
					for (LONG x = DestRect.top; x < DestRect.bottom; x++)
					{
						memcpy(EmulatedBuffer, SurfaceBuffer, WidthPitch);
						EmulatedBuffer += EmulatedLockRect.Pitch;
						SurfaceBuffer += SurfaceLockRect.Pitch;
					}
				}
			}
		}

		// Unlock surface texture
		surfaceTexture->UnlockRect(0);

	} while (false);

	return hr;
}

void m_IDirectDrawSurfaceX::UpdatePaletteData()
{
	// Check surface format
	if (surfaceFormat != D3DFMT_P8)
	{
		return;
	}

	DWORD CurrentPaletteUSN = 0;
	DWORD entryCount = 0;
	D3DCOLOR *rgbPalette = nullptr;

	// Get palette data
	if (attachedPalette && attachedPalette->GetRgbPalette())
	{
		CurrentPaletteUSN = PaletteUSN + attachedPalette->GetPaletteUSN();
		if (CurrentPaletteUSN && CurrentPaletteUSN != LastPaletteUSN)
		{
			rgbPalette = (D3DCOLOR*)attachedPalette->GetRgbPalette();
			entryCount = attachedPalette->GetEntryCount();
		}
	}
	// Get palette from primary surface if this is not primary
	else if (!IsPrimarySurface())
	{
		m_IDirectDrawSurfaceX *lpPrimarySurface = ddrawParent->GetPrimarySurface();
		if (lpPrimarySurface)
		{
			m_IDirectDrawPalette *lpPalette = lpPrimarySurface->GetAttachedPalette();
			if (lpPalette && lpPalette->GetRgbPalette())
			{
				CurrentPaletteUSN = lpPrimarySurface->GetPaletteUSN() + lpPalette->GetPaletteUSN();
				if (CurrentPaletteUSN && CurrentPaletteUSN != LastPaletteUSN)
				{
					rgbPalette = (D3DCOLOR*)lpPalette->GetRgbPalette();
					entryCount = lpPalette->GetEntryCount();
				}
			}
		}
	}

	// No new palette data found
	if (!rgbPalette)
	{
		return;
	}

	// Set color palette for device context
	if (emu && emu->surfaceDC)
	{
		SetDIBColorTable(emu->surfaceDC, 0, entryCount, (RGBQUAD*)rgbPalette);
	}

	// If new palette data then write it to texture
	if (paletteTexture)
	{
		do {
			D3DLOCKED_RECT LockRect;
			RECT Rect = { 0, 0, (LONG)entryCount, 1 };

			if (FAILED(paletteTexture->LockRect(0, &LockRect, &Rect, 0)))
			{
				LOG_LIMIT(100, __FUNCTION__ << " Error: failed to lock palette texture!");
				break;
			}

			memcpy(LockRect.pBits, rgbPalette, entryCount * sizeof(D3DCOLOR));

			paletteTexture->UnlockRect(0);

		} while (false);
	}

	LastPaletteUSN = CurrentPaletteUSN;
}

void m_IDirectDrawSurfaceX::StartSharedEmulatedMemory()
{
	ShareEmulatedMemory = true;
	InitializeCriticalSection(&smcs);
}

void m_IDirectDrawSurfaceX::DeleteSharedEmulatedMemory(EMUSURFACE **ppEmuSurface)
{
	if (!ppEmuSurface || !*ppEmuSurface)
	{
		return;
	}

	// Release device context memory
	if ((*ppEmuSurface)->surfaceDC)
	{
		SelectObject((*ppEmuSurface)->surfaceDC, (*ppEmuSurface)->OldDCObject);
		DeleteDC((*ppEmuSurface)->surfaceDC);
	}
	if ((*ppEmuSurface)->bitmap)
	{
		DeleteObject((*ppEmuSurface)->bitmap);
	}
	delete (*ppEmuSurface);
	ppEmuSurface = nullptr;
}

void m_IDirectDrawSurfaceX::CleanupSharedEmulatedMemory()
{
	// Disable shared memory
	ShareEmulatedMemory = false;
	
	// Make sure that vectore is not in use
	EnterCriticalSection(&smcs);
	LeaveCriticalSection(&smcs);

	// Deleted critical section
	DeleteCriticalSection(&smcs);

	LOG_LIMIT(100, __FUNCTION__ << " Deleting " << memorySurfaces.size() << " emulated surfaces!");

	// Clean up unused emulated surfaces
	for (EMUSURFACE *pEmuSurface: memorySurfaces)
	{
		DeleteSharedEmulatedMemory(&pEmuSurface);
	}
	memorySurfaces.clear();
}

void m_IDirectDrawSurfaceX::ReleaseInterface()
{
	SetCriticalSection();
	if (ddrawParent)
	{
		ddrawParent->RemoveSurfaceFromVector(this);
	}
	ReleaseCriticalSection();
}
