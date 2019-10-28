#pragma once

class m_IDirectDraw3 : public IDirectDraw3, public AddressLookupTableD3d9Object
{
private:
	std::unique_ptr<m_IDirectDrawX> UniqueProxyInterface;
	m_IDirectDrawX *ProxyInterface;
	IDirectDraw3 *RealInterface;
	REFIID WrapperID = IID_IDirectDraw3;
	const DWORD DirectXVersion = 3;

public:
	m_IDirectDraw3(IDirectDraw3 *aOriginal) : RealInterface(aOriginal)
	{
		UniqueProxyInterface = std::make_unique<m_IDirectDrawX>((IDirectDraw7*)RealInterface, DirectXVersion, (m_IDirectDraw7*)this);
		ProxyInterface = UniqueProxyInterface.get();
		ProxyAddressLookupTable.SaveAddress(this, RealInterface);
	}
	m_IDirectDraw3(m_IDirectDrawX *aOriginal) : ProxyInterface(aOriginal)
	{
		RealInterface = nullptr;
	}
	~m_IDirectDraw3()
	{
		ProxyAddressLookupTable.DeleteAddress(this);
	}

	DWORD GetDirectXVersion() { return DirectXVersion; }
	REFIID GetWrapperType() { return WrapperID; }
	IDirectDraw3 *GetProxyInterface() { return RealInterface; }
	m_IDirectDrawX *GetWrapperInterface() { return ProxyInterface; }

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef) (THIS);
	STDMETHOD_(ULONG, Release) (THIS);

	/*** IDirectDraw methods ***/
	STDMETHOD(Compact)(THIS);
	STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR *);
	STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR *);
	STDMETHOD(CreateSurface)(THIS_ LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *);
	STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR *);
	STDMETHOD(EnumDisplayModes)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK);
	STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK);
	STDMETHOD(FlipToGDISurface)(THIS);
	STDMETHOD(GetCaps)(THIS_ LPDDCAPS, LPDDCAPS);
	STDMETHOD(GetDisplayMode)(THIS_ LPDDSURFACEDESC);
	STDMETHOD(GetFourCCCodes)(THIS_ LPDWORD, LPDWORD);
	STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR *);
	STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD);
	STDMETHOD(GetScanLine)(THIS_ LPDWORD);
	STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL);
	STDMETHOD(Initialize)(THIS_ GUID FAR *);
	STDMETHOD(RestoreDisplayMode)(THIS);
	STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD);
	STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD, DWORD, DWORD, DWORD);
	STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE);
	/*** Added in the v2 interface ***/
	STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS, LPDWORD, LPDWORD);
	/*** IDirectDraw3 methods ***/
	STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, IDirectDrawSurface **);
};
