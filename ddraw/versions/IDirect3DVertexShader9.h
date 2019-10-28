#pragma once
#include "../../External/Logging/Logging.h"
#include "IDirect3DDevice9.h"
class m_IDirect3DVertexShader9 : public IDirect3DVertexShader9, public AddressLookupTableD3d9Object
{
private:
	LPDIRECT3DVERTEXSHADER9 ProxyInterface;
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
	m_IDirect3DVertexShader9(LPDIRECT3DVERTEXSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice) : ProxyInterface(pShader9), m_pDeviceEx(pDevice)
	{
		LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ")");

	//	pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
		SaveAddress(pDevice, ProxyInterface);
	}
	~m_IDirect3DVertexShader9()
	{
		LOG_LIMIT(3, __FUNCTION__ << "(" << this << ")" << " deleting device!");
	}

	LPDIRECT3DVERTEXSHADER9 GetProxyInterface() { return ProxyInterface; }

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	/*** IDirect3DVertexShader9 methods ***/
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice);
	STDMETHOD(GetFunction)(THIS_ void* pData, UINT* pSizeOfData);
};
