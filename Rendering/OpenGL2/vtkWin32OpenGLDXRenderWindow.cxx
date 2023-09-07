// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWin32OpenGLDXRenderWindow.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkTextureObject.h"
#include "vtk_glew.h"

#include <d3d11.h> // For D3D11 interface
#include <dxgi.h>
#include <wrl/client.h> // For Microsoft::WRL::ComPtr

VTK_ABI_NAMESPACE_BEGIN
class vtkWin32OpenGLDXRenderWindow::PIMPL
{
public:
  // D3D resources
  Microsoft::WRL::ComPtr<ID3D11Device> Device = nullptr;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> D3DDeviceContext = nullptr;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> D3DSharedTexture = nullptr;

  // Specify the required D3D version
  D3D_FEATURE_LEVEL MinFeatureLevel = D3D_FEATURE_LEVEL_11_1;
};

vtkStandardNewMacro(vtkWin32OpenGLDXRenderWindow);

//------------------------------------------------------------------------------
vtkWin32OpenGLDXRenderWindow::vtkWin32OpenGLDXRenderWindow()
{
  this->Private = new PIMPL;
}

//------------------------------------------------------------------------------
vtkWin32OpenGLDXRenderWindow::~vtkWin32OpenGLDXRenderWindow()
{
  delete this->Private;
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::Initialize()
{
  this->Superclass::Initialize();
  this->InitializeDX();
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::InitializeDX()
{
  // Require NV_DX_interop OpenGL extension
  if (!WGLEW_NV_DX_interop)
  {
    vtkErrorMacro("OpenGL extension WGLEW_NV_DX_interop unsupported.");
    return;
  }

  // Create the DXGI adapter.
  Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
  CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgiFactory));
  Microsoft::WRL::ComPtr<IDXGIAdapter1> DXGIAdapter;
  for (UINT adapterIndex = 0;; adapterIndex++)
  {
    // Return when there are no more adapters to enumerate
    HRESULT hr = dxgiFactory->EnumAdapters1(adapterIndex, DXGIAdapter.GetAddressOf());
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
      vtkWarningMacro("No DXGI adapter found");
      break;
    }

    DXGI_ADAPTER_DESC1 adapterDesc;
    DXGIAdapter->GetDesc1(&adapterDesc);
    // Choose the adapter matching the internal adapter id or
    // return the first adapter that is available if AdapterId is not set.
    if ((!this->AdapterId.HighPart && !this->AdapterId.LowPart) ||
      memcmp(&adapterDesc.AdapterLuid, &this->AdapterId, sizeof(this->AdapterId)) == 0)
    {
      break;
    }
  }

  // Use unknown driver type with DXGI adapters
  D3D_DRIVER_TYPE driverType =
    DXGIAdapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN;

  // Create the D3D API device object and a corresponding context.
  D3D11CreateDevice(DXGIAdapter.Get(), driverType, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
    &this->Private->MinFeatureLevel, 1, D3D11_SDK_VERSION, this->Private->Device.GetAddressOf(),
    nullptr, this->Private->D3DDeviceContext.GetAddressOf());

  if (!this->Private->Device)
  {
    vtkErrorMacro("D3D11CreateDevice failed in Initialize().");
    return;
  }

  // Acquire a handle to the D3D device for use in OpenGL
  this->DeviceHandle = wglDXOpenDeviceNV(this->Private->Device.Get());

  // Create D3D Texture2D
  D3D11_TEXTURE2D_DESC textureDesc;
  ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
  textureDesc.Width = this->Size[0] > 0 ? this->Size[0] : 300;
  textureDesc.Height = this->Size[1] > 0 ? this->Size[1] : 300;
  textureDesc.MipLevels = 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.SampleDesc.Count = this->MultiSamples > 1 ? this->MultiSamples : 1;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  textureDesc.CPUAccessFlags = 0;
  textureDesc.MiscFlags = 0;
  this->Private->Device->CreateTexture2D(&textureDesc, nullptr, &this->Private->D3DSharedTexture);

  if (!this->Private->D3DSharedTexture)
  {
    vtkErrorMacro("Failed to create D3D shared texture.");
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::Lock()
{
  if (!this->DeviceHandle || !this->GLSharedTextureHandle)
  {
    vtkWarningMacro("Failed to lock shared texture.");
    return;
  }

  wglDXLockObjectsNV(this->DeviceHandle, 1, &this->GLSharedTextureHandle);
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::Unlock()
{
  if (!this->DeviceHandle || !this->GLSharedTextureHandle)
  {
    vtkWarningMacro("Failed to unlock shared texture.");
    return;
  }

  wglDXUnlockObjectsNV(this->DeviceHandle, 1, &this->GLSharedTextureHandle);
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::RegisterSharedTexture(unsigned int textureHandle)
{
  if (this->TextureId == textureHandle)
  {
    return;
  }

  if (!this->DeviceHandle || !this->Private->D3DSharedTexture)
  {
    vtkWarningMacro("Failed to register shared texture. Initializing window.");
    this->Initialize();
  }

  this->TextureId = textureHandle;

  this->GLSharedTextureHandle = wglDXRegisterObjectNV(this->DeviceHandle, // D3D device handle
    this->Private->D3DSharedTexture.Get(),                                // D3D texture
    this->TextureId,                                                      // OpenGL texture id
    this->MultiSamples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);

  if (!this->GLSharedTextureHandle)
  {
    vtkErrorMacro("wglDXRegisterObjectNV failed in RegisterSharedTexture().");
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::UnregisterSharedTexture()
{
  if (!this->DeviceHandle || !this->GLSharedTextureHandle)
  {
    return;
  }

  wglDXUnregisterObjectNV(this->DeviceHandle, this->GLSharedTextureHandle);
  this->TextureId = 0;
  this->GLSharedTextureHandle = 0;
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Superclass::SetSize(width, height);

    if (!this->DeviceHandle || !this->Private->D3DSharedTexture)
    {
      return;
    }

    D3D11_TEXTURE2D_DESC textureDesc;
    this->Private->D3DSharedTexture->GetDesc(&textureDesc);

    unsigned int tId = this->TextureId;
    this->UnregisterSharedTexture();

    textureDesc.Width = this->Size[0];
    textureDesc.Height = this->Size[1];
    this->Private->Device->CreateTexture2D(
      &textureDesc, nullptr, this->Private->D3DSharedTexture.ReleaseAndGetAddressOf());

    this->RegisterSharedTexture(tId);
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetMultiSamples(int samples)
{
  if (this->MultiSamples != samples)
  {
    this->Superclass::SetMultiSamples(samples);

    if (!this->DeviceHandle || !this->Private->D3DSharedTexture)
    {
      return;
    }

    D3D11_TEXTURE2D_DESC textureDesc;
    this->Private->D3DSharedTexture->GetDesc(&textureDesc);

    unsigned int tId = this->TextureId;
    this->UnregisterSharedTexture();

    textureDesc.SampleDesc.Count = this->MultiSamples > 1 ? this->MultiSamples : 1;
    this->Private->Device->CreateTexture2D(
      &textureDesc, nullptr, this->Private->D3DSharedTexture.ReleaseAndGetAddressOf());

    this->RegisterSharedTexture(tId);
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::BlitToTexture(ID3D11Texture2D* target)
{
  if (!this->Private->D3DDeviceContext || !target || !this->Private->D3DSharedTexture)
  {
    return;
  }

  this->Private->D3DDeviceContext->CopySubresourceRegion(target, // destination
    0,                                                           // destination subresource id
    0, 0, 0,                                                     // destination origin x,y,z
    this->Private->D3DSharedTexture.Get(),                       // source
    0,                                                           // source subresource id
    nullptr); // source clip box (nullptr == full extent)
}

//------------------------------------------------------------------------------
ID3D11Device* vtkWin32OpenGLDXRenderWindow::GetDevice()
{
  return this->Private->Device.Get();
}

//------------------------------------------------------------------------------
ID3D11Texture2D* vtkWin32OpenGLDXRenderWindow::GetD3DSharedTexture()
{
  return this->Private->D3DSharedTexture.Get();
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Shared Texture Handle: " << this->GLSharedTextureHandle << "\n";
  os << indent << "Registered GL texture: " << this->TextureId << "\n";
}
VTK_ABI_NAMESPACE_END
