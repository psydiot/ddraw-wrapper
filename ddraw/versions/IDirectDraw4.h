#pragma once

class m_IDirectDraw4 : public IDirectDraw4, public AddressLookupTableD3d9Object
{
private:
	std::unique_ptr<m_IDirectDrawX> UniqueProxyInterface;
	m_IDirectDrawX *ProxyInterface;
	IDirectDraw4 *RealInterface;
	REFIID WrapperID = IID_IDirectDraw4;
	const DWORD DirectXVersion = 4;

public:
	m_IDirectDraw4(IDirectDraw4 *aOriginal) : RealInterface(aOriginal)
	{
		UniqueProxyInterface = std::make_unique<m_IDirectDrawX>((IDirectDraw7*)RealInterface, DirectXVersion, (m_IDirectDraw7*)this);
		ProxyInterface = UniqueProxyInterface.get();
		ProxyAddressLookupTable.SaveAddress(this, RealInterface);
	}
	m_IDirectDraw4(m_IDirectDrawX *aOriginal) : ProxyInterface(aOriginal)
	{
		RealInterface = nullptr;
	}
	~m_IDirectDraw4()
	{
		ProxyAddressLookupTable.DeleteAddress(this);
	}

	DWORD GetDirectXVersion() { return DirectXVersion; }
	REFIID GetWrapperType() { return WrapperID; }
	IDirectDraw4 *GetProxyInterface() { return RealInterface; }
	m_IDirectDrawX *GetWrapperInterface() { return ProxyInterface; }

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef) (THIS);
	STDMETHOD_(ULONG, Release) (THIS);

	/*** IDirectDraw methods ***/
	STDMETHOD(Compact)(THIS);
	STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR *);
	STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR *);
	STDMETHOD(CreateSurface)(THIS_ LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE4 FAR *, IUnknown FAR *);
	STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4 FAR *);
	STDMETHOD(EnumDisplayModes)(THIS_ DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2);
	STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMSURFACESCALLBACK2);
	STDMETHOD(FlipToGDISurface)(THIS);
	STDMETHOD(GetCaps)(THIS_ LPDDCAPS, LPDDCAPS);
	STDMETHOD(GetDisplayMode)(THIS_ LPDDSURFACEDESC2);
	STDMETHOD(GetFourCCCodes)(THIS_ LPDWORD, LPDWORD);
	STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE4 FAR *);
	STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD);
	STDMETHOD(GetScanLine)(THIS_ LPDWORD);
	STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL);
	STDMETHOD(Initialize)(THIS_ GUID FAR *);
	STDMETHOD(RestoreDisplayMode)(THIS);
	STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD);
	STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD, DWORD, DWORD, DWORD);
	STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE);
	/*** Added in the v2 interface ***/
	STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS2, LPDWORD, LPDWORD);
	/*** Added in the V4 Interface ***/
	STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, LPDIRECTDRAWSURFACE4 *);
	STDMETHOD(RestoreAllSurfaces)(THIS);
	STDMETHOD(TestCooperativeLevel)(THIS);
	STDMETHOD(GetDeviceIdentifier)(THIS_ LPDDDEVICEIDENTIFIER, DWORD);
};
