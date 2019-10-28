#pragma once
#include "../../External/Logging/Logging.h"

class m_IDirect3DTexture9 : public IDirect3DTexture9, public AddressLookupTableD3d9Object
{
private:
	LPDIRECT3DTEXTURE9 ProxyInterface;
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
	m_IDirect3DTexture9(LPDIRECT3DTEXTURE9 pTexture9, m_IDirect3DDevice9Ex* pDevice) : ProxyInterface(pTexture9), m_pDeviceEx(pDevice)
	{
		LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ")");

	//	pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
		SaveAddress(pDevice, ProxyInterface);
	}
	~m_IDirect3DTexture9()
	{
		LOG_LIMIT(3, __FUNCTION__ << "(" << this << ")" << " deleting device!");
	}

	LPDIRECT3DTEXTURE9 GetProxyInterface() { return ProxyInterface; }

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	/*** IDirect3DBaseTexture9 methods ***/
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice);
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData);
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid);
	STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew);
	STDMETHOD_(DWORD, GetPriority)(THIS);
	STDMETHOD_(void, PreLoad)(THIS);
	STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS);
	STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew);
	STDMETHOD_(DWORD, GetLOD)(THIS);
	STDMETHOD_(DWORD, GetLevelCount)(THIS);
	STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType);
	STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS);
	STDMETHOD_(void, GenerateMipSubLevels)(THIS);
	STDMETHOD(GetLevelDesc)(THIS_ UINT Level, D3DSURFACE_DESC *pDesc);
	STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level, IDirect3DSurface9** ppSurfaceLevel);
	STDMETHOD(LockRect)(THIS_ UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
	STDMETHOD(UnlockRect)(THIS_ UINT Level);
	STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect);
};
