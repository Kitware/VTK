/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLDXRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include <d3d11.h> // For D3D11 interface

class VTKRENDERINGOPENGL2_EXPORT vtkWin32OpenGLDXRenderWindow : public vtkWin32OpenGLRenderWindow
{
public:
  static vtkWin32OpenGLDXRenderWindow* New();
  vtkTypeMacro(vtkWin32OpenGLDXRenderWindow, vtkWin32OpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overriden to create the D3D device, context and texture.
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
   * Register/Unregister the OpenGL texture designated by \p textureHandle with
   * this render window internal D3D shared texture.
   */
  void RegisterSharedTexture(unsigned int textureHandle);
  void UnregisterSharedTexture();
  ///@}

  ///@{
  /**
   * Overriden to resize the internal D3D shared texture
   */
  void SetSize(int width, int height) override;
  ///@}

  ///@{
  /**
   * Overriden to update the internal D3D shared texture
   */
  void SetMultiSamples(int samples) override;
  ///@}

  ///@{
  /**
   * Blits the internal D3D shared texture into \p texture.
   */
  void BlitToTexture(ID3D11Texture2D* texture);
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
  ///@}

  ///@{
  /**
   * Specify the DGXI adapter to be used for initialization.
   * If left unspecified, the first available adapter is used.
   */
  void SetAdapterId(LUID uid) { this->AdapterId = uid; }
  ///@}

protected:
  vtkWin32OpenGLDXRenderWindow();
  ~vtkWin32OpenGLDXRenderWindow() override;

private:
  vtkWin32OpenGLDXRenderWindow(const vtkWin32OpenGLDXRenderWindow&) = delete;
  void operator=(const vtkWin32OpenGLDXRenderWindow&) = delete;

  // Specify the required D3D version
  D3D_FEATURE_LEVEL MinFeatureLevel = D3D_FEATURE_LEVEL_11_1;

  // Hide D3D resources managed by Microsoft::WRL::ComPtr
  class PIMPL;
  PIMPL* Private;

  HANDLE DeviceHandle = 0;

  unsigned int TextureId = 0; // OpenGL texture id to be shared with the D3D texture

  HANDLE GLSharedTextureHandle = 0; // OpenGL-D3D shared texture id

  LUID AdapterId = { 0, 0 }; // DGXI adapter id
};
#endif
