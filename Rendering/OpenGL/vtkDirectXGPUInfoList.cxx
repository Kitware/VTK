/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectXGPUInfoList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDirectXGPUInfoList.h"

#include "vtkGPUInfoListArray.h"

#include "vtkObjectFactory.h"
#include <cassert>

// DirectX, DXGI api
#include <dxgi.h>

// DirectX, WMI api
#include <wbemidl.h>

#include <ddraw.h> // for LPDIRECTDRAWENUMERATEEXA

vtkStandardNewMacro(vtkDirectXGPUInfoList);

// Used by ProbeInfoWithDXGI.
typedef HRESULT (WINAPI *LPCREATEDXGIFACTORY)(REFIID,void **);

// Used by ProbeInfoWithWMI.
typedef BOOL (WINAPI *PfnCoSetProxyBlanket)(IUnknown *pProxy,
                                            DWORD dwAuthnSvc,
                                            DWORD dwAuthzSvc,
                                            OLECHAR *pServerPrincName,
                                            DWORD dwAuthnLevel,
                                            DWORD dwImpLevel,
                                            RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
                                            DWORD dwCapabilities);

struct DDRAW_MATCH
{
    GUID guid;
    HMONITOR hMonitor;
    CHAR strDriverName[512];
    bool bFound;
};

// ----------------------------------------------------------------------------
BOOL WINAPI DDEnumCallbackEx(GUID FAR *lpGUID,
                             LPSTR vtkNotUsed(lpDriverDescription),
                             LPSTR lpDriverName,
                             LPVOID lpContext,
                             HMONITOR hm)
{
  DDRAW_MATCH *pDDMatch=static_cast<DDRAW_MATCH *>(lpContext);
  if(pDDMatch->hMonitor==hm)
  {
    pDDMatch->bFound=true;
    size_t l=strlen(lpDriverName);
    if(l>511) // because of CHAR strDriverName[512];
    {
      l=511;
    }
    strncpy(pDDMatch->strDriverName,lpDriverName,l+1);
    if(l==511)
    {
      pDDMatch->strDriverName[l]='\0';
    }
    memcpy(&pDDMatch->guid,lpGUID,sizeof(GUID));
  }
  return TRUE;
}

// ----------------------------------------------------------------------------
// Description:
// Build the list of vtkInfoGPU if not done yet.
// \post probed: IsProbed()
void vtkDirectXGPUInfoList::Probe()
{
  if(!this->Probed)
  {
    this->Probed=true;
    this->Array=new vtkGPUInfoListArray;

    IDirect3D9 *pD3D9=0;
    pD3D9=Direct3DCreate9(D3D_SDK_VERSION);
    if(pD3D9!=0)
    {
      size_t c=pD3D9->GetAdapterCount(); // there are `c' GPUS.
      this->Array->v.resize(c);
      size_t i=0;
      while(i<c)
      {
        HMONITOR m=pD3D9->GetAdapterMonitor(static_cast<UINT>(i));
        vtkGPUInfo *info=vtkGPUInfo::New();

        // DXGI API (Windows Vista and later)
        bool status=this->ProbeInfoWithDXGI(m,info);
        if(!status)
        {
          // something went bad. Maybe DXGI is not supported or
          // the memory was not found.
          // Try WMI API (Windows XP)
          this->ProbeInfoWithWMI(m,info);
        }
        this->Array->v[i]=info;
        ++i;
      }
      pD3D9->Release();
    }
    else
    {
      this->Array->v.resize(0); // no GPU.
    }
  }
  assert("post: probed" && this->IsProbed());
}

// ----------------------------------------------------------------------------
bool vtkDirectXGPUInfoList::ProbeInfoWithDXGI(HMONITOR m,
                                                 vtkGPUInfo *info)
{
  assert("pre: m_exists" && m!=0);
  assert("pre: info_exist" && info!=0);

  bool result; // true=supports DXGI and memory found.

  // DXGI API initialization
#ifdef UNICODE
  HINSTANCE hDXGI=LoadLibrary(L"dxgi.dll");
#else
  HINSTANCE hDXGI=LoadLibrary("dxgi.dll");
#endif
  result=hDXGI!=0;
  if(result)
  {
    LPCREATEDXGIFACTORY pCreateDXGIFactory=NULL;
    IDXGIFactory *pDXGIFactory=NULL;
    pCreateDXGIFactory=reinterpret_cast<LPCREATEDXGIFACTORY>(
      GetProcAddress(hDXGI,"CreateDXGIFactory"));
    pCreateDXGIFactory(__uuidof(IDXGIFactory),
                       reinterpret_cast<LPVOID *>(&pDXGIFactory));

    // Find the adapter that matches monitor m.
    IDXGIAdapter *pAdapter=NULL;
    bool found=false;
    bool done=false;
    int i=0;
    while(!found && !done)
    {
      HRESULT hr=pDXGIFactory->EnumAdapters(i,&pAdapter);
      done=FAILED(hr); // DXGIERR_NOT_FOUND is expected when the end of the list is hit
      if(!done)
      {
        int j=0;
        while(!found && !done)
        {
          IDXGIOutput *pOutput=NULL;
          hr=pAdapter->EnumOutputs(j,&pOutput);
          done=FAILED(hr); // DXGIERR_NOT_FOUND is expected when the end of the list is hit
          if(!done)
          {
            DXGI_OUTPUT_DESC outputDesc;
            ZeroMemory(&outputDesc,sizeof(DXGI_OUTPUT_DESC));
            if(SUCCEEDED(pOutput->GetDesc(&outputDesc)))
            {
              found=m==outputDesc.Monitor;
            }
            pOutput->Release();
          }
            ++j;
        }
      }
      ++i;
    }

    if(found)
    {
      DXGI_ADAPTER_DESC desc;
      ZeroMemory(&desc,sizeof(DXGI_ADAPTER_DESC));
      result=SUCCEEDED(pAdapter->GetDesc(&desc));
      if(result)
      {
        info->SetDedicatedVideoMemory(desc.DedicatedVideoMemory);
        info->SetDedicatedSystemMemory(desc.DedicatedSystemMemory);
        info->SetSharedSystemMemory(desc.SharedSystemMemory);
      }
    }
    FreeLibrary(hDXGI);
  }
  return result;
}

// ----------------------------------------------------------------------------
void vtkDirectXGPUInfoList::ProbeInfoWithWMI(HMONITOR m,
                                                vtkGPUInfo *info)
{
  assert("pre: m_exists" && m!=0);
  assert("pre: info_exist" && info!=0);

  HRESULT hrCoInitialize=CoInitialize(0);
  IWbemLocator *pIWbemLocator=NULL;
  HRESULT hr=CoCreateInstance(CLSID_WbemLocator,NULL,CLSCTX_INPROC_SERVER,
                              IID_IWbemLocator,
                              reinterpret_cast<LPVOID *>(&pIWbemLocator));

  if(SUCCEEDED(hr) && pIWbemLocator!=0)
  {
    // Using the locator, connect to WMI in the given namespace.
    BSTR pNamespace=SysAllocString(L"\\\\.\\root\\cimv2");
    IWbemServices *pIWbemServices=NULL;
    hr=pIWbemLocator->ConnectServer(pNamespace,NULL,NULL,0L,0L,NULL,NULL,
                                    &pIWbemServices);
    if(SUCCEEDED(hr) && pIWbemServices!=NULL)
    {
      HINSTANCE hinstOle32=NULL;
      hinstOle32=LoadLibraryW(L"ole32.dll");
      if(hinstOle32!=0)
      {
        PfnCoSetProxyBlanket pfnCoSetProxyBlanket=NULL;

        pfnCoSetProxyBlanket=reinterpret_cast<PfnCoSetProxyBlanket>(
          GetProcAddress(hinstOle32,"CoSetProxyBlanket"));
        if(pfnCoSetProxyBlanket!=NULL)
        {
          // Switch security level to IMPERSONATE.
          pfnCoSetProxyBlanket(pIWbemServices,RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE,NULL,RPC_C_AUTHN_LEVEL_CALL,
                               RPC_C_IMP_LEVEL_IMPERSONATE,NULL,0);
        }
        FreeLibrary(hinstOle32);
      }

      IEnumWbemClassObject *pEnumVideoControllers=NULL;
      BSTR pClassName=NULL;
      pClassName=SysAllocString(L"Win32_VideoController");
      hr=pIWbemServices->CreateInstanceEnum(pClassName,0,NULL,
                                            &pEnumVideoControllers);

      if(SUCCEEDED(hr) && pEnumVideoControllers!=0)
      {
        const int videoControllersCapacity=10; // just get the first 10.
        IWbemClassObject *pVideoControllers[videoControllersCapacity]={0};
        DWORD uReturned=0;

        // Get the first one in the list
        pEnumVideoControllers->Reset();
        hr=pEnumVideoControllers->Next(5000, // timeout in 5 seconds
                                       videoControllersCapacity,
                                       pVideoControllers,&uReturned);

        if(SUCCEEDED(hr))
        {
          WCHAR strInputDeviceID[512];
          this->GetDeviceIDFromHMonitor(m,strInputDeviceID,512);

          bool found=false;
          UINT iController=0;
          while(!found && iController<uReturned)
          {
            BSTR pPropName=SysAllocString(L"PNPDeviceID");
            VARIANT var;
            hr=pVideoControllers[iController]->Get(pPropName,0L,&var,NULL,
                                                   NULL);
            if(SUCCEEDED(hr))
            {
              found=wcsstr( var.bstrVal,strInputDeviceID)!=0;
            }
            VariantClear(&var);
            if(pPropName!=0)
            {
              SysFreeString(pPropName);
            }
            if(found)
            {
              pPropName=SysAllocString(L"AdapterRAM");
              hr=pVideoControllers[iController]->Get(pPropName,0L,&var,NULL,
                                                     NULL);
              if(SUCCEEDED(hr))
              {
                info->SetDedicatedVideoMemory(var.ulVal);
              }
              VariantClear(&var);
              if(pPropName!=0)
              {
                SysFreeString(pPropName);
              }
            }
            pVideoControllers[iController]->Release();
            ++iController;
          }
        }
      }

      if(pClassName!=0)
      {
        SysFreeString(pClassName);
      }
      if(pEnumVideoControllers!=0)
      {
        pEnumVideoControllers->Release();
      }
    }

    if(pNamespace!=0)
    {
      SysFreeString(pNamespace);
    }
    if(pIWbemServices!=0)
    {
      pIWbemServices->Release();
    }
  }

  if(pIWbemLocator!=0)
  {
    pIWbemLocator->Release();
  }

  if(SUCCEEDED(hrCoInitialize))
  {
    CoUninitialize();
  }
}

// ----------------------------------------------------------------------------
bool vtkDirectXGPUInfoList::GetDeviceIDFromHMonitor(HMONITOR hm,
                                                       WCHAR *strDeviceID,
                                                       int cchDeviceID)
{
  assert("pre: hm_exists" && hm!=0);
  assert("pre: strDeviceID_exists" && strDeviceID!=0);
  assert("pre: cchDeviceID_is_positive" && cchDeviceID>0);

  bool result=false;
#ifdef UNICODE
  HINSTANCE hInstDDraw=LoadLibrary(L"ddraw.dll");
#else
  HINSTANCE hInstDDraw=LoadLibrary("ddraw.dll");
#endif
  if(hInstDDraw!=0)
  {
    DDRAW_MATCH match;
    ZeroMemory(&match,sizeof(DDRAW_MATCH));
    match.hMonitor=hm;

    LPDIRECTDRAWENUMERATEEXA pDirectDrawEnumerateEx=NULL;
    pDirectDrawEnumerateEx=reinterpret_cast<LPDIRECTDRAWENUMERATEEXA>(
      GetProcAddress(hInstDDraw, "DirectDrawEnumerateExA"));

    if(pDirectDrawEnumerateEx!=0)
    {
      pDirectDrawEnumerateEx(DDEnumCallbackEx,static_cast<VOID*>(&match),
                             DDENUM_ATTACHEDSECONDARYDEVICES);
    }

    if(match.bFound)
    {
      LONG iDevice=0;
      DISPLAY_DEVICEA dispdev;

      ZeroMemory(&dispdev,sizeof(dispdev));
      dispdev.cb=sizeof(dispdev);

      bool done=false;
      while(!done && EnumDisplayDevicesA(NULL,iDevice,
                                         static_cast<DISPLAY_DEVICEA *>(&dispdev),0))
      {
        // Skip devices that are monitors that echo another display
        // (DISPLAY_DEVICE_MIRRORING_DRIVER)
        // Skip devices that aren't attached since they cause problems
        // (DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
        done=!(dispdev.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
          && !((dispdev.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)==0)
          && _stricmp( match.strDriverName,dispdev.DeviceName)==0;
        if(done)
        {
          MultiByteToWideChar(CP_ACP,0,dispdev.DeviceID,-1,strDeviceID,
                              cchDeviceID);
          result=true;
        }
        iDevice++;
      }
    }
    FreeLibrary(hInstDDraw);
  }
  return result;
}

// ----------------------------------------------------------------------------
vtkDirectXGPUInfoList::vtkDirectXGPUInfoList()
{
}

// ----------------------------------------------------------------------------
vtkDirectXGPUInfoList::~vtkDirectXGPUInfoList()
{
}

// ----------------------------------------------------------------------------
void vtkDirectXGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
