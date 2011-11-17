/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageSliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaImageSliceMapper - OpenGL mapper for image slice display
// .SECTION Description
// vtkMesaImageSliceMapper is a concrete implementation of the abstract
// class vtkImageSliceMapper that interfaces to the OpenGL library.
// .SECTION Thanks
// Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
// Neurosciences, Foothills Medical Centre, Calgary, for providing this class.

#ifndef __vtkMesaImageSliceMapper_h
#define __vtkMesaImageSliceMapper_h

#include "vtkImageSliceMapper.h"

class vtkWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkMesaRenderWindow;
class vtkImageSlice;
class vtkImageProperty;
class vtkImageData;

class VTK_RENDERING_EXPORT vtkMesaImageSliceMapper :
  public vtkImageSliceMapper
{
public:
  static vtkMesaImageSliceMapper *New();
  vtkTypeMacro(vtkMesaImageSliceMapper,vtkImageSliceMapper);
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
  vtkMesaImageSliceMapper();
  ~vtkMesaImageSliceMapper();

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
  // Recursive internal method, will call the non-recursive method
  // as many times as necessary if the texture must be broken up into
  // pieces that are small enough for the GPU to render
  void RecursiveRenderTexturedPolygon(
    vtkRenderer *ren, vtkImageProperty *property,
    vtkImageData *image, int extent[6], bool recursive);

  // Description:
  // Non-recursive internal method, generate a single texture
  // and its corresponding geometry.
  void RenderTexturedPolygon(
    vtkRenderer *ren, vtkImageProperty *property,
    vtkImageData *image, int extent[6], bool recursive);

  // Description:
  // Build the fragment program to use with the texture.
  vtkStdString BuildFragmentProgram(vtkImageProperty *property);

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
  void CheckOpenGLCapabilities(vtkMesaRenderWindow *renWin);

  long Index; // OpenGL ID for texture or display list
  long FragmentShaderIndex; // OpenGL ID for fragment shader
  vtkRenderWindow *RenderWindow; // RenderWindow used for previous render
  int TextureSize[2];
  int TextureBytesPerPixel;
  int LastOrientation;
  int LastSliceNumber;

  vtkTimeStamp LoadTime;
  int LoadCount;

  bool UsePowerOfTwoTextures;
  bool UseClampToEdge;
  bool UseFragmentProgram;

private:
  vtkMesaImageSliceMapper(const vtkMesaImageSliceMapper&);  // Not implemented.
  void operator=(const vtkMesaImageSliceMapper&);  // Not implemented.
};

#endif
