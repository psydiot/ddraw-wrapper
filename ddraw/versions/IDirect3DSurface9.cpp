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
#include "..\IDirectDrawTypes.h"

HRESULT m_IDirect3DSurface9::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if ((riid == IID_IDirect3DSurface9 || riid == IID_IUnknown || riid == IID_IDirect3DResource9) && ppvObj)
	{
		AddRef();

		*ppvObj = this;

		return D3D_OK;
	}

	HRESULT hr = ProxyInterface->QueryInterface(riid, ppvObj);

	if (SUCCEEDED(hr))
	{
		D3d9Wrapper::genericQueryInterface(riid, ppvObj, m_pDeviceEx);
	}

	return hr;
}

ULONG m_IDirect3DSurface9::AddRef(THIS)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->AddRef();
}

ULONG m_IDirect3DSurface9::Release(THIS)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->Release();
}

HRESULT m_IDirect3DSurface9::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!ppDevice)
	{
		return D3DERR_INVALIDCALL;
	}

	m_pDeviceEx->AddRef();

	*ppDevice = m_pDeviceEx;

	return D3D_OK;
}

HRESULT m_IDirect3DSurface9::SetPrivateData(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT m_IDirect3DSurface9::GetPrivateData(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT m_IDirect3DSurface9::FreePrivateData(THIS_ REFGUID refguid)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->FreePrivateData(refguid);
}

DWORD m_IDirect3DSurface9::SetPriority(THIS_ DWORD PriorityNew)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->SetPriority(PriorityNew);
}

DWORD m_IDirect3DSurface9::GetPriority(THIS)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->GetPriority();
}

void m_IDirect3DSurface9::PreLoad(THIS)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->PreLoad();
}

D3DRESOURCETYPE m_IDirect3DSurface9::GetType(THIS)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->GetType();
}

HRESULT m_IDirect3DSurface9::GetContainer(THIS_ REFIID riid, void** ppContainer)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	HRESULT hr = ProxyInterface->GetContainer(riid, ppContainer);

	if (SUCCEEDED(hr))
	{
		D3d9Wrapper::genericQueryInterface(riid, ppContainer, m_pDeviceEx);
	}

	return hr;
}

HRESULT m_IDirect3DSurface9::GetDesc(THIS_ D3DSURFACE_DESC *pDesc)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->GetDesc(pDesc);
}

HRESULT m_IDirect3DSurface9::LockRect(THIS_ D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	if (!pLockedRect)
	{
		return D3DERR_INVALIDCALL;
	}

	HRESULT hr = ProxyInterface->LockRect(pLockedRect, pRect, Flags);

	if (FAILED(hr) && DeviceMultiSampleFlag)
	{
		D3DSURFACE_DESC Desc;
		if (SUCCEEDED(GetDesc(&Desc)))
		{
			DWORD bits = GetBitCount(Desc.Format) / 8;
			DWORD size = bits * Desc.Width * Desc.Height;
			if (!bits || (IsLocked && surfaceArray.size() < size))
			{
				return hr;
			}
			else if (surfaceArray.size() < size)
			{
				surfaceArray.resize(size);
			}

			LOG_LIMIT(100, __FUNCTION__ << " Emulating the surface lock. Data may be lost here!");

			DWORD start = (pRect) ? ((pRect->top * Desc.Width * bits) + (pRect->left * bits)) : 0;

			// ToDo: copy surface data to memory
			pLockedRect->pBits = &surfaceArray[start];
			pLockedRect->Pitch = Desc.Width * bits;

			IsLocked = true;

			return D3D_OK;
		}
	}

	return hr;
}

HRESULT m_IDirect3DSurface9::UnlockRect(THIS)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	HRESULT hr = ProxyInterface->UnlockRect();

	if (SUCCEEDED(hr))
	{
		// ToDo: copy data back from emulated surface
		IsLocked = false;
	}

	return hr;
}

HRESULT m_IDirect3DSurface9::GetDC(THIS_ HDC *phdc)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->GetDC(phdc);
}

HRESULT m_IDirect3DSurface9::ReleaseDC(THIS_ HDC hdc)
{
	Logging::LogDebug() << __FUNCTION__ << " (" << this << ")";

	return ProxyInterface->ReleaseDC(hdc);
}
