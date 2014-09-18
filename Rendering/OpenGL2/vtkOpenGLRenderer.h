/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLRenderer - OpenGL renderer
// .SECTION Description
// vtkOpenGLRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkOpenGLRenderer interfaces to the OpenGL graphics library.

#ifndef __vtkOpenGLRenderer_h
#define __vtkOpenGLRenderer_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderer.h"
#include <vector>  // STL Header

class vtkRenderPass;
class vtkOpenGLTexture;
class vtkTextureObject;
class vtkTexturedActor2D;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderer : public vtkRenderer
{
public:
  static vtkOpenGLRenderer *New();
  vtkTypeMacro(vtkOpenGLRenderer, vtkRenderer);
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

  // Description:
  // Set/Get a custom render pass.
  // Initial value is NULL.
  void SetPass(vtkRenderPass *p);
  vtkGetObjectMacro(Pass, vtkRenderPass);

  // Description:
  // get the various texture units used for Depth Peeling
  // the Mappers make use of these
  int GetOpaqueRGBATextureUnit();
  int GetOpaqueZTextureUnit();
  int GetTranslucentRGBATextureUnit();
  int GetTranslucentZTextureUnit();
  int GetCurrentRGBATextureUnit();

protected:
  vtkOpenGLRenderer();
  ~vtkOpenGLRenderer();

  // Description:
  // Check the compilation status of some fragment shader source.
  void CheckCompilation(unsigned int fragmentShader);

  // Internal method to release graphics resources in any derived renderers.
  virtual void ReleaseGraphicsResources(vtkWindow *w);

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

  double PickedZ;

  // Description:
  // Render a peel layer. If there is no more GPU RAM to save the texture,
  // return false otherwise returns true. Also if layer==0 and no prop have
  // been rendered (there is no translucent geometry), it returns false.
  // \pre positive_layer: layer>=0
  int RenderPeel(int layer);

  friend class vtkOpenGLProperty;
  friend class vtkOpenGLTexture;
  friend class vtkOpenGLImageSliceMapper;
  friend class vtkOpenGLImageResliceMapper;

  // Description:
  // This flag is on if the current OpenGL context supports extensions
  // required by the depth peeling technique.
  int DepthPeelingIsSupported;

  // Description:
  // This flag is on once the OpenGL extensions required by the depth peeling
  // technique have been checked.
  int DepthPeelingIsSupportedChecked;
  vtkTexturedActor2D *DepthPeelingActor;
  std::vector<float> *DepthZData;

  vtkTextureObject *OpaqueZTexture;
  vtkTextureObject *OpaqueRGBATexture;
  vtkTextureObject *TranslucentRGBATexture;
  vtkTextureObject *TranslucentZTexture;
  vtkTextureObject *CurrentRGBATexture;


  // Description:
  // Cache viewport values for depth peeling.
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;

  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by vtkOpenGLProperty or vtkOpenGLTexture)
  int DepthPeelingHigherLayer;

  friend class vtkRenderPass;
  vtkRenderPass *Pass;

private:
  vtkOpenGLRenderer(const vtkOpenGLRenderer&);  // Not implemented.
  void operator=(const vtkOpenGLRenderer&);  // Not implemented.
};

#endif
