/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2Renderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGL2Renderer - OpenGL renderer
// .SECTION Description
// vtkOpenGL2Renderer is a concrete implementation of the abstract class
// vtkRenderer. vtkOpenGL2Renderer interfaces to the OpenGL graphics library.

#ifndef __vtkOpenGL2Renderer_h
#define __vtkOpenGL2Renderer_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderer.h"

class vtkOpenGL2RendererLayerList; // Pimpl
class vtkRenderPass;
class vtkShaderProgram2;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2Renderer : public vtkRenderer
{
public:
  static vtkOpenGL2Renderer *New();
  vtkTypeMacro(vtkOpenGL2Renderer, vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Concrete open gl render method.
  void DeviceRender(void);

  // Description:
  // Render translucent polygonal geometry. Default implementation just call
  // UpdateTranslucentPolygonalGeometry().
  // Subclasses of vtkRenderer that can deal with depth peeling must
  // override this method.
  virtual void DeviceRenderTranslucentPolygonalGeometry();

  void Clear(void);

  // Description:
  // Ask lights to load themselves into graphics pipeline.
  int UpdateLights(void);

  // Description:
  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by vtkOpenGLProperty or vtkOpenGLTexture)
  int GetDepthPeelingHigherLayer();

  //BTX
  // Description:
  //
  vtkGetObjectMacro(ShaderProgram, vtkShaderProgram2);
  virtual void SetShaderProgram(vtkShaderProgram2 *program);
  //ETX

  // Description:
  // Set/Get a custom render pass.
  // Initial value is NULL.
  void SetPass(vtkRenderPass *p);
  vtkGetObjectMacro(Pass, vtkRenderPass);

protected:
  vtkOpenGL2Renderer();
  ~vtkOpenGL2Renderer();

  // Description:
  // Check the compilation status of some fragment shader source.
  void CheckCompilation(unsigned int fragmentShader);

  // Internal method to release graphics resources in any derived renderers.
  virtual void ReleaseGraphicsResources(vtkWindow *w);

  //BTX
  // Picking functions to be implemented by sub-classes
  virtual void DevicePickRender();
  virtual void StartPick(unsigned int pickFromSize);
  virtual void UpdatePickId();
  virtual void DonePick();
  virtual unsigned int GetPickedId();
  virtual unsigned int GetNumPickedIds();
  virtual int GetPickedIds(unsigned int atMost, unsigned int *callerBuffer);
  virtual double GetPickedZ();

  // Ivars used in picking
  class vtkGLPickInfo* PickInfo;
  //ETX

  double PickedZ;

  // Description:
  // Render a peel layer. If there is no more GPU RAM to save the texture,
  // return false otherwise returns true. Also if layer==0 and no prop have
  // been rendered (there is no translucent geometry), it returns false.
  // \pre positive_layer: layer>=0
  int RenderPeel(int layer);

  //BTX
  friend class vtkOpenGLProperty;
  friend class vtkOpenGLTexture;
  friend class vtkOpenGLImageSliceMapper;
  friend class vtkOpenGLImageResliceMapper;
  //ETX

  // Description:
  // Access to the OpenGL program shader uniform variable "useTexture" from the
  // vtkOpenGLProperty or vtkOpenGLTexture.
  int GetUseTextureUniformVariable();

  // Description:
  // Access to the OpenGL program shader uniform variable "texture" from the
  // vtkOpenGLProperty or vtkOpenGLTexture.
  int GetTextureUniformVariable();

  // Description:
  // This flag is on if the current OpenGL context supports extensions
  // required by the depth peeling technique.
  int DepthPeelingIsSupported;

  // Description:
  // This flag is on once the OpenGL extensions required by the depth peeling
  // technique have been checked.
  int DepthPeelingIsSupportedChecked;

  // Description:
  // Used by the depth peeling technique to store the transparency layers.
  vtkOpenGL2RendererLayerList *LayerList;

  unsigned int OpaqueLayerZ;
  unsigned int TransparentLayerZ;
  unsigned int ProgramShader;

  // Description:
  // Cache viewport values for depth peeling.
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;

  // Description:
  // Actual depth format: vtkgl::DEPTH_COMPONENT16_ARB
  // or vtkgl::DEPTH_COMPONENT24_ARB
  unsigned int DepthFormat;

  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by vtkOpenGLProperty or vtkOpenGLTexture)
  int DepthPeelingHigherLayer;

  vtkShaderProgram2 *ShaderProgram;

  friend class vtkRenderPass;
  vtkRenderPass *Pass;

private:
  vtkOpenGL2Renderer(const vtkOpenGL2Renderer&);  // Not implemented.
  void operator=(const vtkOpenGL2Renderer&);  // Not implemented.
};

#endif
