// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test interoperability between Direct3D and a VTK OpenGL render window.
// The regression test image background is cleared using a D3D11 context,
// then VTK renders its OpenGL scene on the top of it.

#include "vtkOpenGLFramebufferObject.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTextureObject.h"
#include "vtkWin32OpenGLDXRenderWindow.h"

#include <d3d11.h>
#include <system_error>
#include <wrl/client.h> // For Microsoft::WRL::ComPtr

namespace
{ // anonymous namespace
// Clear background using Direct3D and render the VTK scene in the same shared texture.
void Render(Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dDeviceContext,
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> d3DRenderTargetView,
  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dFramebufferTexture,
  Microsoft::WRL::ComPtr<IDXGISwapChain> d3dSwapChain, vtkWin32OpenGLDXRenderWindow* renderWindow)
{
  // Clear background color
  float backgroundColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
  d3dDeviceContext->ClearRenderTargetView(d3DRenderTargetView.Get(), backgroundColor);

  // Copy D3D framebuffer into VTK's shared texture
  d3dDeviceContext->CopySubresourceRegion(renderWindow->GetD3DSharedTexture(), // destination
    0,                           // destination subresource id
    0, 0, 0,                     // destination origin x,y,z
    d3dFramebufferTexture.Get(), // source
    0,                           // source subresource id
    nullptr);                    // source clip box (nullptr == full extent)

  d3dDeviceContext->OMSetRenderTargets(1, d3DRenderTargetView.GetAddressOf(), nullptr);

  // Render VTK scene
  renderWindow->Lock();
  renderWindow->Render();
  renderWindow->Unlock();

  // Copy the VTK-D3D texture to the back buffer to present the swapchain on screen
  renderWindow->BlitToTexture(d3dFramebufferTexture.Get());

  d3dSwapChain->Present(1, 0);
}
} // anonymous namespace

int TestWin32OpenGLDXRenderWindow(int argc, char* argv[])
{
  // Use VTK to create a window handle for the D3D swapchain.
  vtkNew<vtkWin32OpenGLRenderWindow> d3dWindow;
  d3dWindow->SetSize(400, 400);
  d3dWindow->Initialize();

  // Create an hidden OpenGL-D3D render window to render in shared texture.
  vtkNew<vtkWin32OpenGLDXRenderWindow> renderWindow;
  renderWindow->ShowWindowOff();
  // Make sure VTK framebuffers are created to register shared texture
  renderWindow->Render();
  // Register VTK's render framebuffer as shared OpenGL-D3D texture.
  renderWindow->RegisterSharedTexture(
    renderWindow->GetRenderFramebuffer()->GetColorAttachmentAsTextureObject(0)->GetHandle());

  // VTK scene
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  // We expect the following background color to be overridden by D3D if
  // preserving the color buffer
  renderer->SetBackground(1, 0, 0);
  renderer->PreserveColorBufferOn();

  vtkNew<vtkSphereSource> source;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Initialize D3D resources
  Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dDeviceContext;
  Microsoft::WRL::ComPtr<IDXGISwapChain> d3dSwapChain;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> d3DRenderTargetView;
  HRESULT err;

  // Use the Direct3D context initialized by the VTK render window.
  // This makes sure that the Direct3D resources allocated in this test exist
  // in the same context as the OpenGL-Direct3D shared texture, which is
  // required to blit one texture into the other.
  d3dDevice = renderWindow->GetDevice();

  // Obtain DXGI factory from device

  // Get IDXGIDevice from ID3D11Device
  Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
  err = d3dDevice->QueryInterface(
    __uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf()));
  if (err < 0 || !dxgiDevice)
  {
    cerr << "Unable to get IDXGIDevice from ID3D11Device: " << std::system_category().message(err);
  }

  // Get IDXGIAdapter from IDXGIDevice
  Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
  err = dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
  if (err < 0 || !dxgiAdapter)
  {
    cerr << "Unable to get IDXGIAdapter from IDXGIDevice: " << std::system_category().message(err);
  }

  // Get IDXGIFactory from IDXGIAdapter
  Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
  err = dxgiAdapter->GetParent(
    __uuidof(IDXGIFactory), reinterpret_cast<void**>(dxgiFactory.GetAddressOf()));
  if (err < 0 || !dxgiAdapter)
  {
    cerr << "Unable to get IDXGIFactory from IDXGIAdapter: " << std::system_category().message(err);
  }

  // SwapChain descriptor.
  DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1;
  swapChainDesc.OutputWindow = d3dWindow->GetWindowId();
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.SampleDesc.Count =
    renderWindow->GetMultiSamples() > 0 ? renderWindow->GetMultiSamples() : 1;
  swapChainDesc.Windowed = true;

  // Create IDXGISwapChain from IDXGIFactory
  Microsoft::WRL::ComPtr<IDXGISwapChain> dxgiSwapChain;
  err = dxgiFactory->CreateSwapChain(d3dDevice.Get(), &swapChainDesc, d3dSwapChain.GetAddressOf());
  if (err < 0 || !d3dSwapChain)
  {
    cerr << "Unable to get IDXGISwapChain from IDXGIFactory: "
         << std::system_category().message(err);
  }

  // Get ID3D11DeviceContext from ID3D11Device
  d3dDevice->GetImmediateContext(d3dDeviceContext.GetAddressOf());

  // Get the swapchain framebuffer and create the associated render target view
  // to present it on screen.
  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dFramebufferTexture;
  err = d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3dFramebufferTexture);
  if (err < 0 || !d3dFramebufferTexture)
  {
    cerr << "Unable to get ID3D11Texture2D from IDXGISwapChain: "
         << std::system_category().message(err);
  }

  err = d3dDevice->CreateRenderTargetView(
    d3dFramebufferTexture.Get(), 0, d3DRenderTargetView.GetAddressOf());
  if (err < 0 || !d3DRenderTargetView)
  {
    cerr << "Unable to create ID3D11RenderTargetView: " << std::system_category().message(err);
  }

  // Make sure our OpenGL-D3D texture has the same size as the test window.
  D3D11_TEXTURE2D_DESC d3dFramebufferDesc;
  d3dFramebufferTexture->GetDesc(&d3dFramebufferDesc);
  renderWindow->SetSize(d3dFramebufferDesc.Width, d3dFramebufferDesc.Height);

  // Render using D3D and OpenGL
  Render(d3dDeviceContext, d3DRenderTargetView, d3dFramebufferTexture, d3dSwapChain, renderWindow);

  renderWindow->Lock();
  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);
  renderWindow->Unlock();

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    // Start D3D window event loop
    while (d3dWindow->GetWindowId())
    {
      // Dispatch events
      MSG msg;
      while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
      {
        DispatchMessageA(&msg);
      }

      // Present swapchain
      Render(
        d3dDeviceContext, d3DRenderTargetView, d3dFramebufferTexture, d3dSwapChain, renderWindow);
    }
  }

  return !retVal;
}
