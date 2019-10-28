#pragma once
#include "../External/Logging/Logging.h"

class m_IDirect3DLight : public IDirect3DLight, public AddressLookupTableDdrawObject
{
private:
	IDirect3DLight *ProxyInterface = nullptr;
	REFIID WrapperID = IID_IDirect3DLight;
	ULONG RefCount = 1;

	// Convert Material
	m_IDirect3DDeviceX **D3DDeviceInterface = nullptr;

public:
	m_IDirect3DLight(IDirect3DLight *aOriginal) : ProxyInterface(aOriginal)
	{
		LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ")");

		ProxyAddressLookupTable.SaveAddress(this, ProxyInterface);
	}
	m_IDirect3DLight(m_IDirect3DDeviceX **D3DDInterface) : D3DDeviceInterface(D3DDInterface)
	{
		LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ")");
	}
	~m_IDirect3DLight()
	{
		LOG_LIMIT(3, __FUNCTION__ << "(" << this << ")" << " deleting device!");

		ProxyAddressLookupTable.DeleteAddress(this);
	}

	DWORD GetDirectXVersion() { return 1; }
	REFIID GetWrapperType() { return WrapperID; }
	IDirect3DLight *GetProxyInterface() { return ProxyInterface; }
	m_IDirect3DLight *GetWrapperInterface() { return this; }

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	/*** IDirect3DLight methods ***/
	STDMETHOD(Initialize)(THIS_ LPDIRECT3D);
	STDMETHOD(SetLight)(THIS_ LPD3DLIGHT);
	STDMETHOD(GetLight)(THIS_ LPD3DLIGHT);
};
