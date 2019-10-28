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
*/

#include "d3d9.h"
#include "../ddraw.h"
#include "../../IClassFactory/IClassFactory.h"
#include "../IDirectDrawFactory.h"
#include "../IDirectDrawColorControl.h"
#include "../IDirectDrawGammaControl.h"
#include "../IDirectDrawClipper.h"
#include "../IDirectDrawPalette.h"
DWORD DdrawWrapper::GetIIDVersion(REFIID riid)
{
	return (riid == IID_IDirectDraw || riid == IID_IDirectDrawSurface || riid == IID_IDirect3D9 || riid == IID_IDirect3DDevice9 ||
		 riid == IID_IDirect3DTexture9 || riid == IID_IDirect3DVertexBuffer9 || 
		riid == IID_IDirectDrawClipper || riid == IID_IDirectDrawColorControl || riid == IID_IDirectDrawGammaControl ||
		riid == IID_IDirectDrawPalette || riid == IID_IDirectDrawFactory ) ? 1 :
		(riid == IID_IDirectDraw2 || riid == IID_IDirectDrawSurface2 || riid == IID_IDirect3D9 || riid == IID_IDirect3DDevice9 || riid == IID_IDirect3DTexture9 ) ? 2 :
			(riid == IID_IDirectDraw3 || riid == IID_IDirectDrawSurface3 || riid == IID_IDirect3D9 || riid == IID_IDirect3DDevice9 ) ? 3 :
				(riid == IID_IDirectDraw4 || riid == IID_IDirectDrawSurface4) ? 4 :
		(riid == IID_IDirectDraw7 || riid == IID_IDirectDrawSurface7 || riid == IID_IDirect3D9|| riid == IID_IDirect3DDevice9 ||
			riid == IID_IDirect3DVertexBuffer9) ? 7 : 0;
}

REFCLSID DdrawWrapper::ConvertREFCLSID(REFCLSID rclsid)
{
	if ( rclsid == CLSID_DirectDraw)
	{
		return CLSID_DirectDraw7;
	}

	return rclsid;
}

REFIID DdrawWrapper::ConvertREFIID(REFIID riid)
{
	
	
		if (riid == IID_IDirectDraw || riid == IID_IDirectDraw2 || riid == IID_IDirectDraw3 || riid == IID_IDirectDraw4)
		{
			return IID_IDirectDraw7;
		}
		else if (riid == IID_IDirectDrawSurface || riid == IID_IDirectDrawSurface2 || riid == IID_IDirectDrawSurface3 || riid == IID_IDirectDrawSurface4)
		{
			return IID_IDirectDrawSurface7;
		}
	

	return riid;
}

HRESULT DdrawWrapper::ProxyQueryInterface(LPVOID ProxyInterface, REFIID riid, LPVOID* ppvObj, REFIID WrapperID, LPVOID WrapperInterface)
{
	Logging::LogDebug() << "Query for " << riid << " from " << WrapperID;

	if (!ppvObj)
	{
		return DDERR_GENERIC;
	}

	if (riid == WrapperID || riid == IID_IUnknown)
	{
		*ppvObj = WrapperInterface;

		AddRef(*ppvObj);

		return DD_OK;
	}

	if (Config.Dd7to9 || !ProxyInterface)
	{
		if (ppvObj)
		{
			*ppvObj = nullptr;

			genericQueryInterface(riid, ppvObj);

			if (*ppvObj)
			{
				return DD_OK;
			}
		}

		LOG_LIMIT(100, __FUNCTION__ << " Query Not Implemented for " << riid << " from " << WrapperID);

		return E_NOINTERFACE;
	}

	HRESULT hr = ((IUnknown*)ProxyInterface)->QueryInterface(ConvertREFIID(riid), ppvObj);

	if (SUCCEEDED(hr))
	{
		genericQueryInterface(riid, ppvObj);
	}
	else
	{
		if (ppvObj)
		{
			*ppvObj = nullptr;

			genericQueryInterface(riid, ppvObj);

			if (*ppvObj)
			{
				return DD_OK;
			}
		}

		LOG_LIMIT(100, __FUNCTION__ << "Query failed for " << riid << " Error " << hr);
	}

	return hr;
}

void WINAPI DdrawWrapper::genericQueryInterface(REFIID riid, LPVOID* ppvObj)
{
	if (!ppvObj)
	{
		return;
	}

	if (!*ppvObj)
	{
		if (riid == IID_IClassFactory)
		{
			*ppvObj = new m_IClassFactory(nullptr, genericQueryInterface);
			return;
		}
		if (riid == IID_IDirectDrawFactory)
		{
			*ppvObj = new m_IDirectDrawFactory(nullptr);
			return;
		}
		if (riid == IID_IDirectDrawColorControl)
		{
			*ppvObj = new m_IDirectDrawColorControl(nullptr);
			return;
		}
		if (riid == IID_IDirectDrawGammaControl)
		{
			*ppvObj = new m_IDirectDrawGammaControl(nullptr);
			return;
		}
		if (riid == IID_IDirectDrawClipper)
		{
			*ppvObj = new m_IDirectDrawClipper(nullptr);
			return;
		}
		if (Config.Dd7to9 &&
			(riid == IID_IDirectDraw ||
				riid == IID_IDirectDraw2 ||
				riid == IID_IDirectDraw3 ||
				riid == IID_IDirectDraw4 ||
				riid == IID_IDirectDraw7))
		{
			dd_DirectDrawCreateEx(nullptr, ppvObj, riid, nullptr);
			return;
		}

		return;
	}

#define QUERYINTERFACE(x) \
	if (riid == IID_ ## x) \
		{ \
			*ppvObj = ProxyAddressLookupTable.FindAddress<m_ ## x>(*ppvObj); \
		}


	QUERYINTERFACE(IDirectDraw);
	QUERYINTERFACE(IDirectDraw2);
	QUERYINTERFACE(IDirectDraw3);
	QUERYINTERFACE(IDirectDraw4);
	QUERYINTERFACE(IDirectDraw7);
	QUERYINTERFACE(IDirectDrawClipper);
	QUERYINTERFACE(IDirectDrawColorControl);
	QUERYINTERFACE(IDirectDrawGammaControl);
	QUERYINTERFACE(IDirectDrawPalette);
	QUERYINTERFACE(IDirectDrawSurface);
	QUERYINTERFACE(IDirectDrawSurface2);
	QUERYINTERFACE(IDirectDrawSurface3);
	QUERYINTERFACE(IDirectDrawSurface4);
	QUERYINTERFACE(IDirectDrawSurface7);

	if (riid == IID_IClassFactory)
	{
		*ppvObj = new m_IClassFactory((IClassFactory*)* ppvObj, genericQueryInterface);
	}
	if (riid == IID_IDirectDrawFactory)
	{
		*ppvObj = new m_IDirectDrawFactory((IDirectDrawFactory*)* ppvObj);
	}
}


void WINAPI D3d9Wrapper::genericQueryInterface(REFIID riid, LPVOID *ppvObj, m_IDirect3DDevice9Ex* m_pDeviceEx)
{
	if (!ppvObj || !*ppvObj || !m_pDeviceEx)
	{
		return;
	}

	if (riid == IID_IDirect3D9Ex || riid == IID_IDirect3D9Ex)
	{
		IDirect3D9 *pD3D9 = nullptr;
		if (SUCCEEDED(m_pDeviceEx->GetDirect3D(&pD3D9)) && pD3D9)
		{
			IDirect3D9 *pD3D9wrapper = nullptr;
			if (SUCCEEDED(pD3D9->QueryInterface(riid, (LPVOID*)&pD3D9wrapper)) && pD3D9wrapper)
			{
				pD3D9wrapper->Release();
			}
			pD3D9->Release();
			return;
		}
	}

	if (riid == IID_IDirect3DDevice9 || riid == IID_IDirect3DDevice9Ex)
	{
		IDirect3DDevice9 *pD3DDevice9 = nullptr;
		if (SUCCEEDED(m_pDeviceEx->QueryInterface(riid, (LPVOID*)&pD3DDevice9)) && pD3DDevice9)
		{
			pD3DDevice9->Release();
		}
		return;
	}

#define QUERYINTERFACE(x) \
	if (riid == IID_ ## x) \
		{ \
			*ppvObj = m_pDeviceEx->ProxyAddressLookupTable->FindAddress<m_ ## x>(*ppvObj); \
			return; \
		}

	QUERYINTERFACE(IDirect3DCubeTexture9);
	QUERYINTERFACE(IDirect3DIndexBuffer9);
	QUERYINTERFACE(IDirect3DPixelShader9);
	QUERYINTERFACE(IDirect3DQuery9);
	QUERYINTERFACE(IDirect3DStateBlock9);
	QUERYINTERFACE(IDirect3DSurface9);
	QUERYINTERFACE(IDirect3DSwapChain9);
	QUERYINTERFACE(IDirect3DTexture9);
	QUERYINTERFACE(IDirect3DVertexBuffer9);
	QUERYINTERFACE(IDirect3DVertexDeclaration9);
	QUERYINTERFACE(IDirect3DVertexShader9);
	QUERYINTERFACE(IDirect3DVolume9);
	QUERYINTERFACE(IDirect3DVolumeTexture9);
}
