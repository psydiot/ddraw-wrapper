#pragma once
#include "../External/Logging/Logging.h"
#include "Versions/IDirect3D9.h"
#include "Versions/IDirect3DDevice9.h"
#include "IDirectDrawPalette.h"
#include "Versions/IDirectDraw7.h"
#include "Versions/IDirectDraw2.h"
#include "Versions/IDirectDraw3.h"
#include "Versions/IDirectDraw4.h"
#include "Versions/IDirectDraw.h"
#define DDWRAPPER_TYPEX 0x80

class m_IDirectDrawX : public IUnknown
{
private:
	IDirectDraw7 *ProxyInterface = nullptr;
	m_IDirectDraw7 *WrapperInterface = nullptr;
	DWORD ProxyDirectXVersion;
	ULONG RefCount = 1;

	// Fix exclusive mode issue
	HHOOK g_hook = nullptr;
	HWND chWnd = nullptr;

	// Store ddraw version wrappers
	std::unique_ptr<m_IDirectDraw> UniqueProxyInterface = nullptr;
	std::unique_ptr<m_IDirectDraw2> UniqueProxyInterface2 = nullptr;
	std::unique_ptr<m_IDirectDraw3> UniqueProxyInterface3 = nullptr;
	std::unique_ptr<m_IDirectDraw4> UniqueProxyInterface4 = nullptr;
	std::unique_ptr<m_IDirectDraw7> UniqueProxyInterface7 = nullptr;

	// Store primary surface
	m_IDirectDrawSurfaceX *PrimarySurface = nullptr;

	// Store a list of surfaces
	std::vector<m_IDirectDrawSurfaceX*> SurfaceVector;

	// Store a list of palettes
	std::vector<m_IDirectDrawPalette*> PaletteVector;

	// Store d3d interface
	m_IDirect3D9Ex *D3DInterface = nullptr;
	m_IDirect3DDevice9Ex *D3DDeviceInterface = nullptr;

public:
	m_IDirectDrawX(IDirectDraw7 *aOriginal, DWORD DirectXVersion, m_IDirectDraw7 *Interface) : ProxyInterface(aOriginal), WrapperInterface(Interface)
	{
		ProxyDirectXVersion = GetIIDVersion(ConvertREFIID(GetWrapperType(DirectXVersion)));

		if (ProxyDirectXVersion != DirectXVersion)
		{
			LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ")" << " converting device from v" << DirectXVersion << " to v" << ProxyDirectXVersion);
		}
		else
		{
			LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ") v" << DirectXVersion);
		}
	}
	m_IDirectDrawX(DWORD DirectXVersion)
	{
		ProxyDirectXVersion = 9;

		LOG_LIMIT(3, "Creating device " << __FUNCTION__ << "(" << this << ")" << " converting device from v" << DirectXVersion << " to v" << ProxyDirectXVersion);

		InitDdrawSettings();
	}
	~m_IDirectDrawX()
	{
		LOG_LIMIT(3, __FUNCTION__ << "(" << this << ")" << " deleting device!");

		if (g_hook)
		{
			UnhookWindowsHookEx(g_hook);
		}

		if (Config.Dd7to9 && !Config.Exiting)
		{
			ReleaseDdraw();
		}
	}

	DWORD GetDirectXVersion() { return DDWRAPPER_TYPEX; }
	REFIID GetWrapperType() { return IID_IUnknown; }
	IDirectDraw7 *GetProxyInterface() { return ProxyInterface; }
	m_IDirectDraw7 *GetWrapperInterface() { return WrapperInterface; }

	void SetDdrawParent(m_IDirectDrawX* ddraw);

	void ClearDdraw();

	/*** IUnknown methods ***/
	HRESULT QueryInterface(REFIID riid, LPVOID FAR * ppvObj, DWORD DirectXVersion);
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) { return QueryInterface(riid, ppvObj, ProxyDirectXVersion); }
	STDMETHOD_(ULONG, AddRef) (THIS);
	STDMETHOD_(ULONG, Release) (THIS);

	/*** IDirectDraw methods ***/
	STDMETHOD(Compact)(THIS);
	STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR *, IUnknown FAR *);
	STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR *, IUnknown FAR *);
	HRESULT CreateSurface(LPDDSURFACEDESC, LPDIRECTDRAWSURFACE7 FAR *, IUnknown FAR *, DWORD);
	HRESULT CreateSurface2(LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE7 FAR *, IUnknown FAR *, DWORD);
	STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7 FAR *, DWORD);
	HRESULT EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK);
	HRESULT EnumDisplayModes2(DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2);
	HRESULT EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK, DWORD);
	HRESULT EnumSurfaces2(DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMSURFACESCALLBACK7, DWORD);
	STDMETHOD(FlipToGDISurface)(THIS);
	STDMETHOD(GetCaps)(THIS_ LPDDCAPS, LPDDCAPS);
	HRESULT GetDisplayMode(LPDDSURFACEDESC);
	HRESULT GetDisplayMode2(LPDDSURFACEDESC2);
	STDMETHOD(GetFourCCCodes)(THIS_ LPDWORD, LPDWORD);
	STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE7 FAR *, DWORD);
	STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD);
	STDMETHOD(GetScanLine)(THIS_ LPDWORD);
	STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL);
	STDMETHOD(Initialize)(THIS_ GUID FAR *);
	STDMETHOD(RestoreDisplayMode)(THIS);
	STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD);
	STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD, DWORD, DWORD, DWORD);
	STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE);

	/*** Added in the v2 interface ***/
	HRESULT GetAvailableVidMem(LPDDSCAPS, LPDWORD, LPDWORD);
	HRESULT GetAvailableVidMem2(LPDDSCAPS2, LPDWORD, LPDWORD);

	/*** Added in the V4 Interface ***/
	STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, LPDIRECTDRAWSURFACE7 *, DWORD DirectXVersion);
	STDMETHOD(RestoreAllSurfaces)(THIS);
	STDMETHOD(TestCooperativeLevel)(THIS);
	HRESULT GetDeviceIdentifier(LPDDDEVICEIDENTIFIER, DWORD);
	HRESULT GetDeviceIdentifier2(LPDDDEVICEIDENTIFIER2, DWORD);

	/*** Added in the V7 Interface ***/
	STDMETHOD(StartModeTest)(THIS_ LPSIZE, DWORD, DWORD);
	STDMETHOD(EvaluateMode)(THIS_ DWORD, DWORD *);

	// Wrapper interface functions
	REFIID GetWrapperType(DWORD DirectXVersion)
	{
		return (DirectXVersion == 1) ? IID_IDirectDraw :
			(DirectXVersion == 2) ? IID_IDirectDraw2 :
			(DirectXVersion == 3) ? IID_IDirectDraw3 :
			(DirectXVersion == 4) ? IID_IDirectDraw4 :
			(DirectXVersion == 7) ? IID_IDirectDraw7 : IID_IUnknown;
	}
	IDirectDraw *GetProxyInterfaceV1() { return (IDirectDraw *)ProxyInterface; }
	IDirectDraw2 *GetProxyInterfaceV2() { return (IDirectDraw2 *)ProxyInterface; }
	IDirectDraw3 *GetProxyInterfaceV3() { return (IDirectDraw3 *)ProxyInterface; }
	IDirectDraw4 *GetProxyInterfaceV4() { return (IDirectDraw4 *)ProxyInterface; }
	IDirectDraw7 *GetProxyInterfaceV7() { return ProxyInterface; }
	void *GetWrapperInterfaceX(DWORD DirectXVersion);

	// Direct3D9 interfaces
	LPDIRECT3D9 GetDirect3D9Object();
	LPDIRECT3DDEVICE9 *GetDirect3D9Device();
	void ClearD3D() { D3DInterface = nullptr; }
	m_IDirect3D9Ex **GetCurrentD3D() { return &D3DInterface; }
	void ClearD3DDevice() { D3DDeviceInterface = nullptr; }
	void SetD3DDevice(m_IDirect3DDevice9Ex *D3DDevice) { D3DDeviceInterface = D3DDevice; }
	m_IDirect3DDevice9Ex **GetCurrentD3DDevice() { return &D3DDeviceInterface; }

	// Device information functions
	void InitDdrawSettings();
	void SetDdrawDefaults();
	void ReleaseDdraw();
	HWND GetHwnd();
	HDC GetDC();
	bool IsExclusiveMode();

	// Direct3D9 interface functions
	HRESULT CheckInterface(char *FunctionName, bool CheckD3DDevice);
	HRESULT CreateD3D9Device();
	HRESULT ReinitDevice();
	void ReleaseAllDirectDrawD9Surfaces();
	void ReleaseAllD9Surfaces(bool CriticalSection = true);
	void ReleaseD3d9Device();

	// Surface vector functions
	void AddSurfaceToVector(m_IDirectDrawSurfaceX* lpSurfaceX);
	void RemoveSurfaceFromVector(m_IDirectDrawSurfaceX* lpSurfaceX);
	bool DoesSurfaceExist(m_IDirectDrawSurfaceX* lpSurfaceX);
	m_IDirectDrawSurfaceX *GetPrimarySurface() { return PrimarySurface; }
	void EvictManagedTextures();

	// Palette vector functions
	void AddPaletteToVector(m_IDirectDrawPalette* lpPalette);
	void RemovePaletteFromVector(m_IDirectDrawPalette* lpPalette);
	bool DoesPaletteExist(m_IDirectDrawPalette* lpPalette);

	// Video memory size
	static void AdjustVidMemory(LPDWORD lpdwTotal, LPDWORD lpdwFree);

	// Begin & end scene
	HRESULT BeginScene();
	HRESULT EndScene();
 };
