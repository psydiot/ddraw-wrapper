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

#include "ddraw.h"
#include "../Utils/Utils.h"
#include "IDirect3DX.h"
#include "IDirect3DEnumCallback.h"
#include "IDirect3DTypes.h"
#include "IDirect3DLight.h"
#include "IDirect3DMaterialX.h"
#include "IDirect3DViewportX.h"
#include "IDirect3DDeviceX.h"
#include "../DirectDrawWrapper.h"
#include "IDirect3DVertexBufferX.h"
bool GetD3DPath = true;
char D3DImPath[MAX_PATH] = { '\0' };
char D3DIm700Path[MAX_PATH] = { '\0' };
HMODULE hD3DIm = nullptr;
HMODULE hD3DIm700 = nullptr;
typedef float D3DVALUE, * LPD3DVALUE;
typedef DWORD D3DCOLORMODEL;
typedef struct _D3DTRANSFORMCAPS {
	DWORD dwSize;
	DWORD dwCaps;
} D3DTRANSFORMCAPS, * LPD3DTRANSFORMCAPS;
typedef struct _D3DLIGHTINGCAPS {
	DWORD dwSize;
	DWORD dwCaps;                   /* Lighting caps */
	DWORD dwLightingModel;          /* Lighting model - RGB or mono */
	DWORD dwNumLights;              /* Number of lights that can be handled */
} D3DLIGHTINGCAPS, * LPD3DLIGHTINGCAPS;
typedef struct _D3DPrimCaps {
	DWORD dwSize;
	DWORD dwMiscCaps;                 /* Capability flags */
	DWORD dwRasterCaps;
	DWORD dwZCmpCaps;
	DWORD dwSrcBlendCaps;
	DWORD dwDestBlendCaps;
	DWORD dwAlphaCmpCaps;
	DWORD dwShadeCaps;
	DWORD dwTextureCaps;
	DWORD dwTextureFilterCaps;
	DWORD dwTextureBlendCaps;
	DWORD dwTextureAddressCaps;
	DWORD dwStippleWidth;             /* maximum width and height of */
	DWORD dwStippleHeight;            /* of supported stipple (up to 32x32) */
} D3DPRIMCAPS, * LPD3DPRIMCAPS;
typedef struct _D3DDeviceDesc {
	DWORD            dwSize;                 /* Size of D3DDEVICEDESC structure */
	DWORD            dwFlags;                /* Indicates which fields have valid data */
	D3DCOLORMODEL    dcmColorModel;          /* Color model of device */
	DWORD            dwDevCaps;              /* Capabilities of device */
	D3DTRANSFORMCAPS dtcTransformCaps;       /* Capabilities of transform */
	BOOL             bClipping;              /* Device can do 3D clipping */
	D3DLIGHTINGCAPS  dlcLightingCaps;        /* Capabilities of lighting */
	D3DPRIMCAPS      dpcLineCaps;
	D3DPRIMCAPS      dpcTriCaps;
	DWORD            dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc.. */
	DWORD            dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */
	DWORD            dwMaxBufferSize;        /* Maximum execute buffer size */
	DWORD            dwMaxVertexCount;       /* Maximum vertex count */
#if(DIRECT3D_VERSION >= 0x0500)
	// *** New fields for DX5 *** //

	// Width and height caps are 0 for legacy HALs.
	DWORD        dwMinTextureWidth, dwMinTextureHeight;
	DWORD        dwMaxTextureWidth, dwMaxTextureHeight;
	DWORD        dwMinStippleWidth, dwMaxStippleWidth;
	DWORD        dwMinStippleHeight, dwMaxStippleHeight;
#endif /* DIRECT3D_VERSION >= 0x0500 */

#if(DIRECT3D_VERSION >= 0x0600)
	// New fields for DX6
	DWORD       dwMaxTextureRepeat;
	DWORD       dwMaxTextureAspectRatio;
	DWORD       dwMaxAnisotropy;

	// Guard band that the rasterizer can accommodate
	// Screen-space vertices inside this space but outside the viewport
	// will get clipped properly.
	D3DVALUE    dvGuardBandLeft;
	D3DVALUE    dvGuardBandTop;
	D3DVALUE    dvGuardBandRight;
	D3DVALUE    dvGuardBandBottom;

	D3DVALUE    dvExtentsAdjust;
	DWORD       dwStencilCaps;

	DWORD       dwFVFCaps;
	DWORD       dwTextureOpCaps;
	WORD        wMaxTextureBlendStages;
	WORD        wMaxSimultaneousTextures;
#endif /* DIRECT3D_VERSION >= 0x0600 */
} D3DDEVICEDESC, * LPD3DDEVICEDESC;

typedef struct _D3DDeviceDesc7 {
	DWORD            dwDevCaps;              /* Capabilities of device */
	D3DPRIMCAPS      dpcLineCaps;
	D3DPRIMCAPS      dpcTriCaps;
	DWORD            dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc.. */
	DWORD            dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */

	DWORD       dwMinTextureWidth, dwMinTextureHeight;
	DWORD       dwMaxTextureWidth, dwMaxTextureHeight;

	DWORD       dwMaxTextureRepeat;
	DWORD       dwMaxTextureAspectRatio;
	DWORD       dwMaxAnisotropy;

	D3DVALUE    dvGuardBandLeft;
	D3DVALUE    dvGuardBandTop;
	D3DVALUE    dvGuardBandRight;
	D3DVALUE    dvGuardBandBottom;

	D3DVALUE    dvExtentsAdjust;
	DWORD       dwStencilCaps;

	DWORD       dwFVFCaps;
	DWORD       dwTextureOpCaps;
	WORD        wMaxTextureBlendStages;
	WORD        wMaxSimultaneousTextures;

	DWORD       dwMaxActiveLights;
	D3DVALUE    dvMaxVertexW;
	GUID        deviceGUID;

	WORD        wMaxUserClipPlanes;
	WORD        wMaxVertexBlendMatrices;

	DWORD       dwVertexProcessingCaps;

	DWORD       dwReserved1;
	DWORD       dwReserved2;
	DWORD       dwReserved3;
	DWORD       dwReserved4;
} D3DDEVICEDESC7, * LPD3DDEVICEDESC7;
HRESULT m_IDirect3DX::QueryInterface(REFIID riid, LPVOID * ppvObj, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if ((riid == IID_IDirect3D9 || riid == IID_IDirect3D9 || riid == IID_IDirect3D9 || riid == IID_IDirect3D9 || riid == IID_IUnknown) && ppvObj)
		{
			DWORD DxVersion = (riid == IID_IUnknown) ? DirectXVersion : GetIIDVersion(riid);

			*ppvObj = GetWrapperInterfaceX(DxVersion);

			::AddRef(*ppvObj);

			return D3D_OK;
		}
	}
	return ProxyQueryInterface(ProxyInterface, riid, ppvObj, GetWrapperType(DirectXVersion), WrapperInterface);
}

void *m_IDirect3DX::GetWrapperInterfaceX(DWORD DirectXVersion)
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
			UniqueProxyInterface = std::make_unique<m_IDirect3D>(this);
		}
		return UniqueProxyInterface.get();
	case 2:
		if (!UniqueProxyInterface2.get())
		{
			UniqueProxyInterface2 = std::make_unique<m_IDirect3D2>(this);
		}
		return UniqueProxyInterface2.get();
	case 3:
		if (!UniqueProxyInterface3.get())
		{
			UniqueProxyInterface3 = std::make_unique<m_IDirect3D3>(this);
		}
		return UniqueProxyInterface3.get();
	case 7:
		if (!UniqueProxyInterface7.get())
		{
			UniqueProxyInterface7 = std::make_unique<m_IDirect3D7>(this);
		}
		return UniqueProxyInterface7.get();
	default:
		LOG_LIMIT(100, __FUNCTION__ << " Error: wrapper interface version not found: " << DirectXVersion);
		return nullptr;
	}
}

ULONG m_IDirect3DX::AddRef()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		return InterlockedIncrement(&RefCount);
	}

	return ProxyInterface->AddRef();
}

ULONG m_IDirect3DX::Release()
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

	if (ref == 0)
	{
		// ToDo: Release all m_IDirect3DDeviceX devices when using Dd7to9.
		if (WrapperInterface)
		{
			delete this;
		}
		else
		{
			delete this;
		}
	}

	return ref;
}

HRESULT m_IDirect3DX::Initialize(REFCLSID rclsid)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (ProxyDirectXVersion != 1)
	{
		return D3D_OK;
	}

	return 0;
}

typedef HRESULT(CALLBACK* LPD3DENUMDEVICESCALLBACK7)(LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC7, LPVOID);

typedef HRESULT(CALLBACK* LPD3DENUMDEVICESCALLBACK)(GUID FAR* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID);
HRESULT m_IDirect3DX::EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	switch (ProxyDirectXVersion)
	{
	case 1:
		return GetProxyInterfaceV1()->EnumDevices(lpEnumDevicesCallback, lpUserArg);
	case 2:
		return GetProxyInterfaceV2()->EnumDevices(lpEnumDevicesCallback, lpUserArg);
	case 3:
		return GetProxyInterfaceV3()->EnumDevices(lpEnumDevicesCallback, lpUserArg);
	case 7:
	{
		if (!lpEnumDevicesCallback)
		{
			return DDERR_INVALIDPARAMS;
		}

		ENUMDEVICES CallbackContext;
		CallbackContext.lpContext = lpUserArg;
		CallbackContext.lpCallback = lpEnumDevicesCallback;

		return EnumDevices7(m_IDirect3DEnumDevices::ConvertCallback, &CallbackContext);
	}
	case 9:
		return EnumDevices7((LPD3DENUMDEVICESCALLBACK7)lpEnumDevicesCallback, lpUserArg, true);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT m_IDirect3DX::EnumDevices7(LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback7, LPVOID lpUserArg, bool ConvertCallback)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lpEnumDevicesCallback7)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device
		if (!ddrawParent)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
			return DDERR_GENERIC;
		}

		// Get d3d9Object
		IDirect3D9 *d3d9Object = ddrawParent->GetDirect3D9Object();
		UINT AdapterCount = d3d9Object->GetAdapterCount();

		// Conversion callback
		LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback = (LPD3DENUMDEVICESCALLBACK)lpEnumDevicesCallback7;

		// Loop through all adapters
		for (UINT i = 0; i < AdapterCount; i++)
		{
			for (D3DDEVTYPE Type : {D3DDEVTYPE_REF, D3DDEVTYPE_HAL, (D3DDEVTYPE)(D3DDEVTYPE_HAL + 0x10)})
			{
				// Get Device Caps
				D3DCAPS9 Caps9;
				HRESULT hr = d3d9Object->GetDeviceCaps(i, (D3DDEVTYPE)((DWORD)Type & ~0x10), &Caps9);

				if (SUCCEEDED(hr))
				{
					// Convert device desc
					D3DDEVICEDESC7 DeviceDesc7;
					Caps9.DeviceType = Type;
					ConvertDeviceDesc;

					GUID deviceGUID = DeviceDesc7.deviceGUID;
					D3DDEVICEDESC D3DHWDevDesc, D3DHELDevDesc;
					D3DHWDevDesc.dwSize = sizeof(D3DDEVICEDESC);
					D3DHELDevDesc.dwSize = sizeof(D3DDEVICEDESC);

					LPSTR lpDescription, lpName;

					switch ((DWORD)Type)
					{
					case D3DDEVTYPE_REF:
						lpDescription = "Microsoft Direct3D RGB Software Emulation";
						lpName = "RGB Emulation";
						if (ConvertCallback)
						{
							// Get Device Caps D3DDEVTYPE_REF
							ConvertDeviceDesc;

							// Get Device Caps D3DDEVTYPE_HAL
							if (SUCCEEDED(d3d9Object->GetDeviceCaps(i, D3DDEVTYPE_HAL, &Caps9)))
							{
								Caps9.DeviceType = D3DDEVTYPE_HAL;
								ConvertDeviceDesc;
								ConvertDeviceDesc;

								if (lpEnumDevicesCallback(&deviceGUID, lpDescription, lpName, &D3DHWDevDesc, &D3DHELDevDesc, lpUserArg) == DDENUMRET_CANCEL)
								{
									return D3D_OK;
								}
							}
						}
						break;
					case D3DDEVTYPE_HAL:
						lpDescription = "Microsoft Direct3D Hardware acceleration through Direct3D HAL";
						lpName = "Direct3D HAL";
						if (ConvertCallback)
						{
							// Get Device Caps D3DDEVTYPE_HAL
							ConvertDeviceDesc;

							// Get Device Caps D3DDEVTYPE_REF
							if (SUCCEEDED(d3d9Object->GetDeviceCaps(i, D3DDEVTYPE_REF, &Caps9)))
							{
								Caps9.DeviceType = D3DDEVTYPE_REF;
								ConvertDeviceDesc;
								ConvertDeviceDesc;

								if (lpEnumDevicesCallback(&deviceGUID, lpDescription, lpName, &D3DHWDevDesc, &D3DHELDevDesc, lpUserArg) == DDENUMRET_CANCEL)
								{
									return D3D_OK;
								}
							}
						}
						break;
					default:
					case D3DDEVTYPE_HAL + 0x10:
						lpDescription = "Microsoft Direct3D Hardware Transform and Lighting acceleration capable device";
						lpName = "Direct3D T&L HAL";
						break;
					}

					if (!ConvertCallback)
					{
						if (lpEnumDevicesCallback7(lpDescription, lpName, &DeviceDesc7, lpUserArg) == DDENUMRET_CANCEL)
						{
							return D3D_OK;
						}
					}
				}
				else
				{
					LOG_LIMIT(100, __FUNCTION__ << " Error: failed to GetCaps!");
				}
			}
		}

		return D3D_OK;
	}

	return GetProxyInterfaceV7()->EnumDevices(lpEnumDevicesCallback7, lpUserArg);
}
typedef struct IDirect3DLight* LPDIRECT3DLIGHT;
HRESULT m_IDirect3DX::CreateLight(LPDIRECT3DLIGHT * lplpDirect3DLight, LPUNKNOWN pUnkOuter)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	HRESULT hr = DDERR_GENERIC;

	switch (ProxyDirectXVersion)
	{
	case 1:
		hr = GetProxyInterfaceV1()->CreateLight(lplpDirect3DLight, pUnkOuter);
		break;
	case 2:
		hr = GetProxyInterfaceV2()->CreateLight(lplpDirect3DLight, pUnkOuter);
		break;
	case 3:
		hr = GetProxyInterfaceV3()->CreateLight(lplpDirect3DLight, pUnkOuter);
		break;
	case 7:
	case 9:
	{
		if (!lplpDirect3DLight)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device
		if (!ddrawParent)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
			return DDERR_GENERIC;
		}

		*lplpDirect3DLight = (LPDIRECT3DLIGHT)new m_IDirect3DLight(ddrawParent->GetCurrentD3DDevice());
		return D3D_OK;
	}
	default:
		return DDERR_GENERIC;
	}

	if (SUCCEEDED(hr) && lplpDirect3DLight)
	{
		*lplpDirect3DLight =(LPDIRECT3DLIGHT) ProxyAddressLookupTable.FindAddress<m_IDirect3DLight>(*lplpDirect3DLight);
	}

	return hr;
}
typedef struct IDirect3DMaterial3* LPDIRECT3DMATERIAL3;
HRESULT m_IDirect3DX::CreateMaterial(LPDIRECT3DMATERIAL3 * lplpDirect3DMaterial, LPUNKNOWN pUnkOuter, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	HRESULT hr = DDERR_GENERIC;

	switch (ProxyDirectXVersion)
	{
	case 1:
		hr = GetProxyInterfaceV1()->CreateMaterial((LPDIRECT3DMATERIAL*)lplpDirect3DMaterial, pUnkOuter);
		break;
	case 2:
		hr = GetProxyInterfaceV2()->CreateMaterial((LPDIRECT3DMATERIAL2*)lplpDirect3DMaterial, pUnkOuter);
		break;
	case 3:
		hr = GetProxyInterfaceV3()->CreateMaterial(lplpDirect3DMaterial, pUnkOuter);
		break;
	case 7:
	case 9:
	{
		if (!lplpDirect3DMaterial)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device
		if (!ddrawParent)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
			return DDERR_GENERIC;
		}

		*lplpDirect3DMaterial = (LPDIRECT3DMATERIAL3)new m_IDirect3DMaterialX(ddrawParent->GetCurrentD3DDevice(), DirectXVersion);
		return D3D_OK;
	}
	default:
		return DDERR_GENERIC;
	}

	if (SUCCEEDED(hr) && lplpDirect3DMaterial)
	{
		*lplpDirect3DMaterial =(LPDIRECT3DMATERIAL3) ProxyAddressLookupTable.FindAddress<m_IDirect3DMaterial3>(*lplpDirect3DMaterial, DirectXVersion);
	}

	return hr;
}
typedef struct IDirect3DViewport3* LPDIRECT3DVIEWPORT3;
typedef struct IDirect3DViewport* LPDIRECT3DVIEWPORT;
typedef struct IDirect3DViewport2* LPDIRECT3DVIEWPORT2;
HRESULT m_IDirect3DX::CreateViewport(LPDIRECT3DVIEWPORT3 * lplpD3DViewport, LPUNKNOWN pUnkOuter, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	HRESULT hr = DDERR_GENERIC;

	switch (ProxyDirectXVersion)
	{
	case 1:
		hr = GetProxyInterfaceV1()->CreateViewport((LPDIRECT3DVIEWPORT*)lplpD3DViewport, pUnkOuter);
		break;
	case 2:
		hr = GetProxyInterfaceV2()->CreateViewport((LPDIRECT3DVIEWPORT2*)lplpD3DViewport, pUnkOuter);
		break;
	case 3:
		hr = GetProxyInterfaceV3()->CreateViewport(lplpD3DViewport, pUnkOuter);
		break;
	case 7:
	case 9:
	{
		if (!lplpD3DViewport)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device
		if (!ddrawParent)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
			return DDERR_GENERIC;
		}

		*lplpD3DViewport = (LPDIRECT3DVIEWPORT3)new m_IDirect3DViewportX(ddrawParent->GetCurrentD3DDevice(), DirectXVersion);
		return D3D_OK;
	}
	default:
		return DDERR_GENERIC;
	}

	if (SUCCEEDED(hr) && lplpD3DViewport)
	{
		*lplpD3DViewport = ProxyAddressLookupTable.FindAddress<m_IDirect3DViewport3>(*lplpD3DViewport, DirectXVersion);
	}

	return hr;
}

struct ENUMSTRUCT
{
	bool Found = false;
	GUID guid;
	D3DDEVICEDESC7 DeviceDesc7;
};

HRESULT CALLBACK D3DEnumDevicesCallback7(LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC7 lpDeviceDesc7, LPVOID lpContext)
{
	UNREFERENCED_PARAMETER(lpDeviceDescription);
	UNREFERENCED_PARAMETER(lpDeviceName);

	ENUMSTRUCT *lpEnumStruct = (ENUMSTRUCT*)lpContext;

	if (lpDeviceDesc7 && lpContext && lpDeviceDesc7->deviceGUID == lpEnumStruct->guid)
	{
		lpEnumStruct->Found = true;
		memcpy(&lpEnumStruct->DeviceDesc7, lpDeviceDesc7, sizeof(D3DDEVICEDESC7));

		return DDENUMRET_CANCEL;
	}

	return DDENUMRET_OK;
}
typedef struct _D3DFINDDEVICESEARCH {
	DWORD               dwSize;
	DWORD               dwFlags;
	BOOL                bHardware;
	D3DCOLORMODEL       dcmColorModel;
	GUID                guid;
	DWORD               dwCaps;
	D3DPRIMCAPS         dpcPrimCaps;
} D3DFINDDEVICESEARCH, * LPD3DFINDDEVICESEARCH;
typedef struct _D3DFINDDEVICERESULT {
	DWORD               dwSize;
	GUID                guid;           /* guid which matched */
	D3DDEVICEDESC       ddHwDesc;       /* hardware D3DDEVICEDESC */
	D3DDEVICEDESC       ddSwDesc;       /* software D3DDEVICEDESC */
} D3DFINDDEVICERESULT, * LPD3DFINDDEVICERESULT;
HRESULT m_IDirect3DX::FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	switch (ProxyDirectXVersion)
	{
	case 1:
		return GetProxyInterfaceV1()->FindDevice(lpD3DFDS, lpD3DFDR);
	case 2:
		return GetProxyInterfaceV2()->FindDevice(lpD3DFDS, lpD3DFDR);
	case 3:
		return GetProxyInterfaceV3()->FindDevice(lpD3DFDS, lpD3DFDR);
	case 7:
	case 9:
	{
		ENUMSTRUCT EnumStruct;
		EnumStruct.guid = lpD3DFDS->guid;

		EnumDevices7(D3DEnumDevicesCallback7, &EnumStruct, false);

		if (EnumStruct.Found)
		{
			lpD3DFDR->guid = EnumStruct.DeviceDesc7.deviceGUID;
			lpD3DFDR->ddHwDesc.dwSize = (lpD3DFDR->dwSize - sizeof(DWORD) - sizeof(GUID)) / 2;
			ConvertDeviceDesc;
			lpD3DFDR->ddSwDesc.dwSize = (lpD3DFDR->dwSize - sizeof(DWORD) - sizeof(GUID)) / 2;
			ConvertDeviceDescSoft;

			return D3D_OK;
		}

		return DDERR_NOTFOUND;
	}
	default:
		return DDERR_GENERIC;
	}
}
typedef struct IDirect3DDevice7* LPDIRECT3DDEVICE7;
typedef struct IDirect3DDevice2* LPDIRECT3DDEVICE2;
typedef struct IDirect3DDevice3* LPDIRECT3DDEVICE3;
HRESULT m_IDirect3DX::CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 * lplpD3DDevice, LPUNKNOWN pUnkOuter, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (Config.Dd7to9)
	{
		if (!lplpD3DDevice)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device
		if (!ddrawParent)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
			return DDERR_GENERIC;
		}

		m_IDirect3DDeviceX *p_IDirect3DDeviceX = new m_IDirect3DDeviceX(ddrawParent, DirectXVersion);

		*lplpD3DDevice = (LPDIRECT3DDEVICE7)p_IDirect3DDeviceX->GetWrapperInterfaceX(DirectXVersion);

		return D3D_OK;
	}

	if (lpDDS)
	{
		lpDDS = static_cast<m_IDirectDrawSurface7 *>(lpDDS)->GetProxyInterface();
	}

	HRESULT hr = DDERR_GENERIC;

	switch (ProxyDirectXVersion)
	{
	case 1:
	default:
		return DDERR_GENERIC;
	case 2:
		hr = GetProxyInterfaceV2()->CreateDevice(rclsid, (LPDIRECTDRAWSURFACE)lpDDS, (LPDIRECT3DDEVICE2*)lplpD3DDevice);
		break;
	case 3:
		hr = GetProxyInterfaceV3()->CreateDevice(rclsid, (LPDIRECTDRAWSURFACE4)lpDDS, (LPDIRECT3DDEVICE3*)lplpD3DDevice, pUnkOuter);
		break;
	case 7:
		hr = GetProxyInterfaceV7()->CreateDevice(rclsid, lpDDS, lplpD3DDevice);
		break;
	}

	if (SUCCEEDED(hr) && lplpD3DDevice)
	{
		*lplpD3DDevice =(LPDIRECT3DDEVICE7) ProxyAddressLookupTable.FindAddress<m_IDirect3DDevice7>(*lplpD3DDevice, DirectXVersion);

		if (Config.ConvertToDirect3D7 && ddrawParent && *lplpD3DDevice)
		{
			m_IDirect3DDeviceX *lpD3DDeviceX = ((m_IDirect3DDevice7*)*lplpD3DDevice)->GetWrapperInterface();

			if (lpD3DDeviceX)
			{
				lpD3DDeviceX->SetDdrawParent(ddrawParent);
			}
		}
	}

	return hr;
}

typedef struct _D3DVERTEXBUFFERDESC {
	DWORD dwSize;
	DWORD dwCaps;
	DWORD dwFVF;
	DWORD dwNumVertices;
} D3DVERTEXBUFFERDESC, * LPD3DVERTEXBUFFERDESC;
typedef struct IDirect3DVertexBuffer* LPDIRECT3DVERTEXBUFFER;
typedef struct IDirect3DVertexBuffer7* LPDIRECT3DVERTEXBUFFER7;
HRESULT m_IDirect3DX::CreateVertexBuffer(LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7 * lplpD3DVertexBuffer, DWORD dwFlags, LPUNKNOWN pUnkOuter, DWORD DirectXVersion)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	HRESULT hr = DDERR_GENERIC;

	switch (ProxyDirectXVersion)
	{
	case 1:
	case 2:
	default:
		return DDERR_GENERIC;
	case 3:
		hr = GetProxyInterfaceV3()->CreateVertexBuffer(lpVBDesc, (LPDIRECT3DVERTEXBUFFER*)lplpD3DVertexBuffer, dwFlags, pUnkOuter);
		break;
	case 7:
		hr = GetProxyInterfaceV7()->CreateVertexBuffer(lpVBDesc, lplpD3DVertexBuffer, dwFlags);
		break;
	case 9:
	{
		if (!lplpD3DVertexBuffer || dwFlags)
		{
			return DDERR_INVALIDPARAMS;
		}

		// Check for device
		if (!ddrawParent)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
			return DDERR_GENERIC;
		}

		*lplpD3DVertexBuffer = (LPDIRECT3DVERTEXBUFFER7)new m_IDirect3DVertexBufferX(ddrawParent->GetCurrentD3DDevice(), lpVBDesc, DirectXVersion);
		return D3D_OK;
	}
	}

	if (SUCCEEDED(hr) && lplpD3DVertexBuffer)
	{
		*lplpD3DVertexBuffer =(LPDIRECT3DVERTEXBUFFER7) ProxyAddressLookupTable.FindAddress<m_IDirect3DVertexBuffer7>(*lplpD3DVertexBuffer, (DirectXVersion == 7) ? 7 : 1);
	}

	return hr;
}
typedef HRESULT(CALLBACK* LPD3DENUMPIXELFORMATSCALLBACK)(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);
DEFINE_GUID(IID_IDirect3DRampDevice, 0xF2086B20, 0x259F, 0x11CF, 0xA3, 0x1A, 0x00, 0xAA, 0x00, 0xB9, 0x33, 0x56);
DEFINE_GUID(IID_IDirect3DRGBDevice, 0xA4665C60, 0x2673, 0x11CF, 0xA3, 0x1A, 0x00, 0xAA, 0x00, 0xB9, 0x33, 0x56);
DEFINE_GUID(IID_IDirect3DHALDevice, 0x84E63dE0, 0x46AA, 0x11CF, 0x81, 0x6F, 0x00, 0x00, 0xC0, 0x20, 0x15, 0x6E);
DEFINE_GUID(IID_IDirect3DMMXDevice, 0x881949a1, 0xd6f3, 0x11d0, 0x89, 0xab, 0x00, 0xa0, 0xc9, 0x05, 0x41, 0x29);
DEFINE_GUID(IID_IDirect3DRefDevice, 0x50936643, 0x13e9, 0x11d1, 0x89, 0xaa, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29);
DEFINE_GUID(IID_IDirect3DNullDevice, 0x8767df22, 0xbacc, 0x11d1, 0x89, 0x69, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
HRESULT m_IDirect3DX::EnumZBufferFormats(REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	switch (ProxyDirectXVersion)
	{
	case 1:
	case 2:
	default:
		return DDERR_GENERIC;
	case 3:
		return GetProxyInterfaceV3()->EnumZBufferFormats(riidDevice, lpEnumCallback, lpContext);
	case 7:
		return GetProxyInterfaceV7()->EnumZBufferFormats(riidDevice, lpEnumCallback, lpContext);
	case 9:
		if (!lpEnumCallback)
		{
			return DDERR_INVALIDPARAMS;
		}

		if (riidDevice == IID_IDirect3DRGBDevice || riidDevice == IID_IDirect3DRampDevice)
		{
			DDPIXELFORMAT PixelFormat = { NULL };
			PixelFormat.dwSize = sizeof(DDPIXELFORMAT);

			for (DWORD Num : {0, 2})
			{
				GetBufferFormat(PixelFormat, Num);

				if (lpEnumCallback(&PixelFormat, lpContext) == DDENUMRET_CANCEL)
				{
					return D3D_OK;
				}
			}
			return D3D_OK;
		}
		else if (riidDevice == IID_IDirect3DHALDevice || riidDevice == IID_IDirect3DNullDevice)
		{
			DDPIXELFORMAT PixelFormat = { NULL };
			PixelFormat.dwSize = sizeof(DDPIXELFORMAT);

			for (DWORD Num = 0; Num < 6; Num++)
			{
				GetBufferFormat(PixelFormat, Num);

				if (lpEnumCallback(&PixelFormat, lpContext) == DDENUMRET_CANCEL)
				{
					return D3D_OK;
				}
			}
			return D3D_OK;
		}
		else if (riidDevice == IID_IDirect3DMMXDevice || riidDevice == IID_IDirect3DRefDevice || riidDevice == IID_IDirect3DTnLHalDevice)
		{
			return D3D_OK;
		}
		else
		{
			LOG_LIMIT(100, __FUNCTION__ << " Not Implemented " << riidDevice);
			return DDERR_NOZBUFFERHW;
		}
	}
}

HRESULT m_IDirect3DX::EvictManagedTextures()
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	switch (ProxyDirectXVersion)
	{
	case 1:
	case 2:
	default:
		return DDERR_GENERIC;
	case 3:
		return GetProxyInterfaceV3()->EvictManagedTextures();
	case 7:
		return GetProxyInterfaceV7()->EvictManagedTextures();
	case 9:
		// Check for device
		if (!ddrawParent)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: no ddraw parent!");
			return DDERR_GENERIC;
		}

		// Evict managed texture surfaces
		ddrawParent->EvictManagedTextures();
		return D3D_OK;
	}
}

void m_IDirect3DX::ResolutionHack()
{
	if (Config.DdrawResolutionHack)
	{
		if (GetD3DPath)
		{
			GetD3DPath = false;
			GetSystemDirectory((LPWSTR)D3DImPath, MAX_PATH);
			strcpy_s(D3DIm700Path, MAX_PATH, D3DImPath);
			strcat_s(D3DImPath, MAX_PATH, "\\d3dim.dll");
			strcat_s(D3DIm700Path, MAX_PATH, "\\d3dim700.dll");
		}

		if (!hD3DIm)
		{
			hD3DIm = GetModuleHandle((LPWSTR)D3DImPath);
			if (hD3DIm)
			{
				LOG_LIMIT(3, __FUNCTION__ << " Found loaded dll: 'd3dim.dll'");
				Utils::DDrawResolutionHack(hD3DIm);
			}
		}

		if (!hD3DIm700)
		{
			hD3DIm700 = GetModuleHandle((LPWSTR)D3DIm700Path);
			if (hD3DIm700)
			{
				LOG_LIMIT(3, __FUNCTION__ << " Found loaded dll: 'd3dim700.dll'");
				Utils::DDrawResolutionHack(hD3DIm700);
			}
		}
	}
}

void m_IDirect3DX::ReleaseInterface()
{
	SetCriticalSection();
	if (ddrawParent)
	{
		ddrawParent->ClearD3D();
	}
	ReleaseCriticalSection();
}
