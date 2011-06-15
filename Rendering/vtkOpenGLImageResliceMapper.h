/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageResliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLImageResliceMapper - OpenGL mapper for image slice display
// .SECTION Description
// vtkOpenGLImageResliceMapper is a concrete implementation of the abstract
// class vtkImageResliceMapper that interfaces to the OpenGL library.
// .SECTION Thanks
// Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
// Neurosciences, Foothills Medical Centre, Calgary, for providing this class.

#ifndef __vtkOpenGLImageResliceMapper_h
#define __vtkOpenGLImageResliceMapper_h

#include "vtkImageResliceMapper.h"

class vtkWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkOpenGLRenderWindow;
class vtkImageSlice;
class vtkImageProperty;
class vtkImageData;
class vtkMatrix4x4;

class VTK_RENDERING_EXPORT vtkOpenGLImageResliceMapper :
  public vtkImageResliceMapper
{
public:
  static vtkOpenGLImageResliceMapper *New();
  vtkTypeMacro(vtkOpenGLImageResliceMapper,vtkImageResliceMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.  Perform the render.
  void Render(vtkRenderer *ren, vtkImageSlice *prop);

  // Description:
  // Release any graphics resources that are being consumed by this
  // mapper, the image texture in particular. Using the same texture
  // in multiple render windows is NOT currently supported.
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkOpenGLImageResliceMapper();
  ~vtkOpenGLImageResliceMapper();

  // Description:
  // Call the OpenGL code that does color and lighting.
  void RenderColorAndLighting(
    double red, double green, double blue,
    double alpha, double ambient, double diffuse);

  // Description:
  // Render an opaque polygon behind the image.  This is also used
  // in multi-pass rendering to render into the depth buffer.
  void RenderBackingPolygon();

  // Description:
  // Non-recursive internal method, generate a single texture
  // and its corresponding geometry.
  void RenderTexturedPolygon(
    vtkRenderer *ren, vtkProp3D *prop, vtkImageProperty *property,
    vtkImageData *image, int extent[6], bool recursive);

  // Description:
  // Given an extent that describes a slice (it must have unit thickness
  // in one of the three directions), return the dimension indices that
  // correspond to the texture "x" and "y", provide the x, y image size,
  // and provide the texture size (padded to a power of two if the hardware
  // requires).
  void ComputeTextureSize(
    const int extent[6], int &xdim, int &ydim,
    int imageSize[2], int textureSize[2]);

  // Description:
  // Test whether a given texture size is supported.  This includes a
  // check of whether the texture will fit into texture memory.
  bool TextureSizeOK(const int size[2]);

  // Description:
  // Check various OpenGL capabilities
  void CheckOpenGLCapabilities(vtkOpenGLRenderWindow *renWin);

  vtkTimeStamp LoadTime;
  long Index; // OpenGL ID for texture or display list
  long FragmentShaderIndex; // OpenGL ID for fragment shader
  vtkRenderWindow *RenderWindow; // RenderWindow used for previous render

  int TextureSize[2];
  int TextureBytesPerPixel;

  bool UsePowerOfTwoTextures;
  bool UseClampToEdge;
  bool UseFragmentProgram;

private:
  vtkOpenGLImageResliceMapper(const vtkOpenGLImageResliceMapper&);  // Not implemented.
  void operator=(const vtkOpenGLImageResliceMapper&);  // Not implemented.
};

#endif
