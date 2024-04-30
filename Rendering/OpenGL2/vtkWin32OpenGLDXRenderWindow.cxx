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

#include <array>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

struct SharedTexture
{
  GLuint Id = 0;           // OpenGL texture id to be shared with the D3D texture
  HANDLE Handle = nullptr; // OpenGL-D3D shared texture id
};

}

class vtkWin32OpenGLDXRenderWindow::vtkInternals
{
public:
  // D3D resources
  Microsoft::WRL::ComPtr<ID3D11Device> Device = nullptr;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> D3DDeviceContext = nullptr;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> D3DSharedColorTexture = nullptr;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> D3DSharedDepthTexture = nullptr;

  HANDLE DeviceHandle = nullptr;
  LUID AdapterId = { 0, 0 }; // DGXI adapter id

  SharedTexture ColorTexture{};
  SharedTexture DepthTexture{};

  // Specify the required D3D version
  D3D_FEATURE_LEVEL MinFeatureLevel = D3D_FEATURE_LEVEL_11_1;

  UINT ColorTextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
};

vtkStandardNewMacro(vtkWin32OpenGLDXRenderWindow);

//------------------------------------------------------------------------------
vtkWin32OpenGLDXRenderWindow::vtkWin32OpenGLDXRenderWindow()
  : Impl{ new vtkInternals{} }
{
}

//------------------------------------------------------------------------------
vtkWin32OpenGLDXRenderWindow::~vtkWin32OpenGLDXRenderWindow() = default;

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetD3DDeviceContext(ID3D11DeviceContext* context)
{
  context->GetDevice(this->Impl->Device.GetAddressOf());
  this->Impl->D3DDeviceContext = context;
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetD3DDeviceContext(void* context)
{
  this->SetD3DDeviceContext(static_cast<ID3D11DeviceContext*>(context));
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

  if (!this->Impl->Device)
  {
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
      if ((!this->Impl->AdapterId.HighPart && !this->Impl->AdapterId.LowPart) ||
        memcmp(&adapterDesc.AdapterLuid, &this->Impl->AdapterId, sizeof(this->Impl->AdapterId)) ==
          0)
      {
        break;
      }
    }

    // Use unknown driver type with DXGI adapters
    D3D_DRIVER_TYPE driverType =
      DXGIAdapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN;

    // Create the D3D API device object and a corresponding context.
    auto result = D3D11CreateDevice(DXGIAdapter.Get(), driverType, 0,
      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG, &this->Impl->MinFeatureLevel, 1,
      D3D11_SDK_VERSION, this->Impl->Device.GetAddressOf(), nullptr,
      this->Impl->D3DDeviceContext.GetAddressOf());
    if (result != S_OK)
    {
      vtkErrorMacro("D3D11CreateDevice failed in Initialize().");
      return;
    }
  }

  // Acquire a handle to the D3D device for use in OpenGL
  this->Impl->DeviceHandle = wglDXOpenDeviceNV(this->Impl->Device.Get());

  // Create D3D Texture2D
  this->CreateTexture(this->Impl->ColorTextureFormat,
    D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, &this->Impl->D3DSharedColorTexture);
  this->CreateTexture(
    DXGI_FORMAT_D32_FLOAT, D3D11_BIND_DEPTH_STENCIL, &this->Impl->D3DSharedDepthTexture);
}

//------------------------------------------------------------------------------
bool vtkWin32OpenGLDXRenderWindow::CreateTexture(
  UINT format, UINT bindFlags, ID3D11Texture2D** output)
{
  D3D11_TEXTURE2D_DESC textureDesc;
  ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
  textureDesc.Width = this->Size[0] > 0 ? this->Size[0] : 300;
  textureDesc.Height = this->Size[1] > 0 ? this->Size[1] : 300;
  textureDesc.MipLevels = 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format = static_cast<DXGI_FORMAT>(format);
  textureDesc.SampleDesc.Count = this->MultiSamples > 1 ? this->MultiSamples : 1;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = bindFlags;
  textureDesc.CPUAccessFlags = 0;
  textureDesc.MiscFlags = 0;

  auto result = this->Impl->Device->CreateTexture2D(&textureDesc, nullptr, output);
  if (result != S_OK)
  {
    vtkErrorMacro("Failed to create D3D texture.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::UpdateTextures()
{
  if (!this->Impl->DeviceHandle || !this->Impl->ColorTexture.Handle)
  {
    return; // not initialized
  }

  // Store native D3D textures description and OpenGL textures ID
  unsigned int colorId = this->Impl->ColorTexture.Id;
  if (colorId == 0)
  {
    return; // not shared
  }

  D3D11_TEXTURE2D_DESC colorDesc;
  this->Impl->D3DSharedColorTexture->GetDesc(&colorDesc);

  unsigned int depthId = 0;
  D3D11_TEXTURE2D_DESC depthDesc;
  if (this->Impl->DepthTexture.Handle)
  {
    depthId = this->Impl->DepthTexture.Id;
    this->Impl->D3DSharedDepthTexture->GetDesc(&depthDesc);
  }

  this->UnregisterSharedTexture();

  this->CreateTexture(this->Impl->ColorTextureFormat,
    D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    this->Impl->D3DSharedColorTexture.ReleaseAndGetAddressOf());

  this->CreateTexture(DXGI_FORMAT_D32_FLOAT, D3D11_BIND_DEPTH_STENCIL,
    this->Impl->D3DSharedDepthTexture.ReleaseAndGetAddressOf());

  this->RegisterSharedTexture(colorId, depthId);
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::Lock()
{
  if (!this->Impl->DeviceHandle)
  {
    vtkErrorMacro("Failed to lock shared texture.");
    return;
  }

  std::array<HANDLE, 2> sharedTextureHandles = { this->Impl->ColorTexture.Handle,
    this->Impl->DepthTexture.Handle };

  if (!wglDXLockObjectsNV(this->Impl->DeviceHandle, this->Impl->DepthTexture.Handle ? 2 : 1,
        sharedTextureHandles.data()))
  {
    vtkErrorMacro("Failed to lock shared texture.");
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::Unlock()
{
  if (!this->Impl->DeviceHandle)
  {
    vtkErrorMacro("Failed to unlock shared texture.");
    return;
  }

  std::array<HANDLE, 2> sharedTextureHandles = { this->Impl->ColorTexture.Handle,
    this->Impl->DepthTexture.Handle };

  if (!wglDXUnlockObjectsNV(this->Impl->DeviceHandle, this->Impl->DepthTexture.Handle ? 2 : 1,
        sharedTextureHandles.data()))
  {
    vtkErrorMacro("Failed to unlock shared texture.");
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::RegisterSharedTexture(unsigned int colorId, unsigned int depthId)
{
  if (colorId == 0)
  {
    vtkErrorMacro("colorId must not be null");
    return;
  }

  if (this->Impl->ColorTexture.Id == colorId && this->Impl->DepthTexture.Id == depthId)
  {
    return; // nothing to do, already registered
  }

  if (this->Impl->ColorTexture.Handle)
  {
    this->UnregisterSharedTexture();
  }

  if (!this->Impl->DeviceHandle)
  {
    vtkWarningMacro("Failed to register shared texture. Initializing window.");
    this->Initialize();
  }

  this->Impl->ColorTexture.Id = colorId;
  this->Impl->DepthTexture.Id = depthId;

  this->Impl->ColorTexture.Handle = wglDXRegisterObjectNV(this->Impl->DeviceHandle,
    this->Impl->D3DSharedColorTexture.Get(), this->Impl->ColorTexture.Id,
    this->MultiSamples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);

  if (!this->Impl->ColorTexture.Handle)
  {
    vtkErrorMacro("wglDXRegisterObjectNV failed in RegisterSharedTexture().");
  }

  if (depthId != 0)
  {
    this->Impl->DepthTexture.Handle = wglDXRegisterObjectNV(this->Impl->DeviceHandle,
      this->Impl->D3DSharedDepthTexture.Get(), this->Impl->DepthTexture.Id,
      this->MultiSamples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);

    if (!this->Impl->DepthTexture.Handle)
    {
      vtkErrorMacro("wglDXRegisterObjectNV failed in RegisterSharedTexture().");
    }
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::RegisterSharedTexture()
{
  this->RegisterSharedTexture(
    this->GetRenderFramebuffer()->GetColorAttachmentAsTextureObject(0)->GetHandle());
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::UnregisterSharedTexture()
{
  if (!this->Impl->DeviceHandle || !this->Impl->ColorTexture.Handle)
  {
    return;
  }

  wglDXUnregisterObjectNV(this->Impl->DeviceHandle, this->Impl->ColorTexture.Handle);
  this->Impl->ColorTexture.Id = 0;
  this->Impl->ColorTexture.Handle = nullptr;

  if (this->Impl->DepthTexture.Handle)
  {
    wglDXUnregisterObjectNV(this->Impl->DeviceHandle, this->Impl->DepthTexture.Handle);
    this->Impl->DepthTexture.Id = 0;
    this->Impl->DepthTexture.Handle = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetSize(int width, int height)
{
  if (this->Size[0] != width || this->Size[1] != height)
  {
    this->Superclass::SetSize(width, height);
    this->UpdateTextures();
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetMultiSamples(int samples)
{
  if (this->MultiSamples != samples)
  {
    this->Superclass::SetMultiSamples(samples);
    this->UpdateTextures();
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::BlitToTexture(ID3D11Texture2D* color, ID3D11Texture2D* depth)
{
  if (!this->Impl->D3DDeviceContext || !color || !this->Impl->D3DSharedColorTexture)
  {
    return;
  }
  D3D11_TEXTURE2D_DESC desc;
  color->GetDesc(&desc);

  // Resolve texture if needed
  if (this->MultiSamples > 1 && desc.SampleDesc.Count <= 1)
  {
    this->Impl->D3DDeviceContext->ResolveSubresource(color, // destination
      0,                                                    // destination subresource id
      this->Impl->D3DSharedColorTexture.Get(),              // source
      0,                                                    // source subresource id
      static_cast<DXGI_FORMAT>(this->Impl->ColorTextureFormat));
  }
  else
  {
    this->Impl->D3DDeviceContext->CopySubresourceRegion(color, // destination
      0,                                                       // destination subresource id
      0, 0, 0,                                                 // destination origin x,y,z
      this->Impl->D3DSharedColorTexture.Get(),                 // source
      0,                                                       // source subresource id
      nullptr); // source clip box (nullptr == full extent)
  }

  if (depth)
  {
    this->Impl->D3DDeviceContext->CopySubresourceRegion(depth, // destination
      0,                                                       // destination subresource id
      0, 0, 0,                                                 // destination origin x,y,z
      this->Impl->D3DSharedDepthTexture.Get(),                 // source
      0,                                                       // source subresource id
      nullptr); // source clip box (nullptr == full extent)
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::BlitToTexture(void* color, void* depth)
{
  this->BlitToTexture(static_cast<ID3D11Texture2D*>(color), static_cast<ID3D11Texture2D*>(depth));
}

//------------------------------------------------------------------------------
ID3D11Device* vtkWin32OpenGLDXRenderWindow::GetDevice()
{
  return this->Impl->Device.Get();
}

//------------------------------------------------------------------------------
ID3D11Texture2D* vtkWin32OpenGLDXRenderWindow::GetD3DSharedTexture()
{
  return this->Impl->D3DSharedColorTexture.Get();
}
//------------------------------------------------------------------------------
ID3D11Texture2D* vtkWin32OpenGLDXRenderWindow::GetD3DSharedDepthTexture()
{
  return this->Impl->D3DSharedDepthTexture.Get();
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetAdapterId(LUID uid)
{
  this->Impl->AdapterId = uid;
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::SetColorTextureFormat(UINT format)
{
  if (format != this->Impl->ColorTextureFormat)
  {
    this->Impl->ColorTextureFormat = format;
    this->UpdateTextures();
  }
}

//------------------------------------------------------------------------------
void vtkWin32OpenGLDXRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Shared Color Texture Handle: " << this->Impl->ColorTexture.Handle << "\n";
  os << indent << "Registered GL Color Texture: " << this->Impl->ColorTexture.Id << "\n";
  os << indent << "Shared Depth Texture Handle: " << this->Impl->DepthTexture.Handle << "\n";
  os << indent << "Registered GL Depth Texture: " << this->Impl->DepthTexture.Id << "\n";
}
VTK_ABI_NAMESPACE_END
