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

#include "..\ddraw.h"

HRESULT m_IDirect3DDevice::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	return ProxyInterface->QueryInterface(riid, ppvObj, DirectXVersion);
}

ULONG m_IDirect3DDevice::AddRef()
{
	return ProxyInterface->AddRef();
}

ULONG m_IDirect3DDevice::Release()
{
	return ProxyInterface->Release();
}

HRESULT m_IDirect3DDevice::Initialize(LPDIRECT3D a, LPGUID b, LPD3DDEVICEDESC c)
{
	return ProxyInterface->Initialize(a, b, c);
}

HRESULT m_IDirect3DDevice::GetCaps(LPD3DDEVICEDESC a, LPD3DDEVICEDESC b)
{
	return ProxyInterface->GetCaps(a, b);
}

HRESULT m_IDirect3DDevice::SwapTextureHandles(LPDIRECT3DTEXTURE a, LPDIRECT3DTEXTURE b)
{
	return ProxyInterface->SwapTextureHandles((LPDIRECT3DTEXTURE2)a, (LPDIRECT3DTEXTURE2)b);
}

HRESULT m_IDirect3DDevice::CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC a, LPDIRECT3DEXECUTEBUFFER * b, IUnknown * c)
{
	return ProxyInterface->CreateExecuteBuffer(a, b, c);
}

HRESULT m_IDirect3DDevice::GetStats(LPD3DSTATS a)
{
	return ProxyInterface->GetStats(a);
}

HRESULT m_IDirect3DDevice::Execute(LPDIRECT3DEXECUTEBUFFER a, LPDIRECT3DVIEWPORT b, DWORD c)
{
	return ProxyInterface->Execute(a, b, c);
}

HRESULT m_IDirect3DDevice::AddViewport(LPDIRECT3DVIEWPORT a)
{
	return ProxyInterface->AddViewport((LPDIRECT3DVIEWPORT3)a);
}

HRESULT m_IDirect3DDevice::DeleteViewport(LPDIRECT3DVIEWPORT a)
{
	return ProxyInterface->DeleteViewport((LPDIRECT3DVIEWPORT3)a);
}

HRESULT m_IDirect3DDevice::NextViewport(LPDIRECT3DVIEWPORT a, LPDIRECT3DVIEWPORT * b, DWORD c)
{
	return ProxyInterface->NextViewport((LPDIRECT3DVIEWPORT3)a, (LPDIRECT3DVIEWPORT3*)b, c, DirectXVersion);
}

HRESULT m_IDirect3DDevice::Pick(LPDIRECT3DEXECUTEBUFFER a, LPDIRECT3DVIEWPORT b, DWORD c, LPD3DRECT d)
{
	return ProxyInterface->Pick(a, b, c, d);
}

HRESULT m_IDirect3DDevice::GetPickRecords(LPDWORD a, LPD3DPICKRECORD b)
{
	return ProxyInterface->GetPickRecords(a, b);
}

HRESULT m_IDirect3DDevice::EnumTextureFormats(LPD3DENUMTEXTUREFORMATSCALLBACK a, LPVOID b)
{
	return ProxyInterface->EnumTextureFormats(a, b);
}

HRESULT m_IDirect3DDevice::CreateMatrix(LPD3DMATRIXHANDLE a)
{
	return ProxyInterface->CreateMatrix(a);
}

HRESULT m_IDirect3DDevice::SetMatrix(D3DMATRIXHANDLE a, const LPD3DMATRIX b)
{
	return ProxyInterface->SetMatrix(a, b);
}

HRESULT m_IDirect3DDevice::GetMatrix(D3DMATRIXHANDLE a, LPD3DMATRIX b)
{
	return ProxyInterface->GetMatrix(a, b);
}

HRESULT m_IDirect3DDevice::DeleteMatrix(D3DMATRIXHANDLE a)
{
	return ProxyInterface->DeleteMatrix(a);
}

HRESULT m_IDirect3DDevice::BeginScene()
{
	return ProxyInterface->BeginScene();
}

HRESULT m_IDirect3DDevice::EndScene()
{
	return ProxyInterface->EndScene();
}

HRESULT m_IDirect3DDevice::GetDirect3D(LPDIRECT3D * a)
{
	return ProxyInterface->GetDirect3D((LPDIRECT3D7*)a, DirectXVersion);
}
