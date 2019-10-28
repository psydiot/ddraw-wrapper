#pragma once
#include "../../External/Logging/Logging.h"

class m_IDirect3DVolume9 : public IDirect3DVolume9, public AddressLookupTableD3d9Object
{
private:
	LPDIRECT3DVOLUME9 ProxyInterface;
	m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;
	template <typename T>
	struct AddressCacheIndex { static constexpr UINT CacheIndex = 0; };
	std::unordered_map<void*, class AddressLookupTableD3d9Object*> g_map[MaxIndex];
public:
	void SaveAddress(m_IDirect3DDevice9Ex* Wrapper, void* Proxy)
	{
		constexpr UINT CacheIndex = AddressCacheIndex<m_IDirect3DDevice9Ex>::CacheIndex;
		if (Wrapper && Proxy)
		{
			g_map[CacheIndex][Proxy] = (AddressLookupTableD3d9Object*)Wrapper;
		}
	}
	m_IDirect3DVolume9(LPDIRECT3DVOLUME9 pVolume8, m_IDirect3DDevice9Ex* pDevice) : ProxyInterface(pVolume8), m_pDeviceEx(pDevice)
	{
		LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ")");

		//pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
		SaveAddress(pDevice, ProxyInterface);
	}
	~m_IDirect3DVolume9()
	{
		LOG_LIMIT(3, __FUNCTION__ << "(" << this << ")" << " deleting device!");
	}

	LPDIRECT3DVOLUME9 GetProxyInterface() { return ProxyInterface; }

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	/*** IDirect3DVolume9 methods ***/
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice);
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData);
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid);
	STDMETHOD(GetContainer)(THIS_ REFIID riid, void** ppContainer);
	STDMETHOD(GetDesc)(THIS_ D3DVOLUME_DESC *pDesc);
	STDMETHOD(LockBox)(THIS_ D3DLOCKED_BOX * pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
	STDMETHOD(UnlockBox)(THIS);
};
