// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWin32OpenGLDXRenderWindow
 * @brief   VTK render window providing OpenGL-DirectX interop
 *
 * This vtkWin32OpenGLRenderWindow subclass allows for rendering in a texture
 * that is shared between an OpenGL and a D3D context, using the NVidia
 * NV_DX_interop extension.
 */

#ifndef vtkWin32OpenGLDXRenderWindow_h
#define vtkWin32OpenGLDXRenderWindow_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWin32OpenGLRenderWindow.h"

#include <memory> // For std::unique_ptr

struct ID3D11Device;
struct ID3D11Texture2D;

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT vtkWin32OpenGLDXRenderWindow : public vtkWin32OpenGLRenderWindow
{
public:
  static vtkWin32OpenGLDXRenderWindow* New();
  vtkTypeMacro(vtkWin32OpenGLDXRenderWindow, vtkWin32OpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to create the D3D device, context and texture.
   */
  void Initialize() override;

  ///@{
  /**
   * Lock/Unlock the shared texture.
   * The texture must be locked before rendering into it.
   */
  void Lock();
  void Unlock();
  ///@}

  ///@{
  /**
   * Register/Unregister the OpenGL textures designated by \p colorId  and \p depthId with
   * this render window internal D3D shared textures. depthId is optional
   */
  void RegisterSharedTexture(unsigned int colorId, unsigned int depthId = 0);
  void UnregisterSharedTexture();
  ///@}

  ///@{
  /**
   * Overridden to resize the internal D3D shared texture
   */
  void SetSize(int width, int height) override;
  ///@}

  ///@{
  /**
   * Overridden to update the internal D3D shared texture
   */
  void SetMultiSamples(int samples) override;
  ///@}

  ///@{
  /**
   * Blits the internal D3D shared texture into \p color and optionally \p depth.
   */
  void BlitToTexture(ID3D11Texture2D* color, ID3D11Texture2D* depth = nullptr);
  ///@}

  ///@{
  /**
   * Returns the D3D device associated to this render window
   */
  ID3D11Device* GetDevice();
  ///@}

  ///@{
  /**
   * Returns the D3D texture shared with this render window
   */
  ID3D11Texture2D* GetD3DSharedTexture();
  ID3D11Texture2D* GetD3DSharedDepthTexture();
  ///@}

  ///@{
  /**
   * Specify the DGXI adapter to be used for initialization.
   * If left unspecified, the first available adapter is used.
   */
  void SetAdapterId(LUID uid);
  ///@}

protected:
  vtkWin32OpenGLDXRenderWindow();
  ~vtkWin32OpenGLDXRenderWindow() override;

  /**
   * @brief Initialize D3D adapter, device and shared texture
   */
  void InitializeDX();

private:
  vtkWin32OpenGLDXRenderWindow(const vtkWin32OpenGLDXRenderWindow&) = delete;
  void operator=(const vtkWin32OpenGLDXRenderWindow&) = delete;

  bool CreateTexture(UINT format, UINT bindFlags, ID3D11Texture2D** output);
  void UpdateTextures();

  class vtkInternals;
  std::unique_ptr<vtkInternals> Impl;
};
VTK_ABI_NAMESPACE_END
#endif
