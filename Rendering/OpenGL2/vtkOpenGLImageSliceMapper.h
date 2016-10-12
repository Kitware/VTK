/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageSliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLImageSliceMapper
 * @brief   OpenGL mapper for image slice display
 *
 * vtkOpenGLImageSliceMapper is a concrete implementation of the abstract
 * class vtkImageSliceMapper that interfaces to the OpenGL library.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
*/

#ifndef vtkOpenGLImageSliceMapper_h
#define vtkOpenGLImageSliceMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkImageSliceMapper.h"

class vtkRenderWindow;
class vtkOpenGLRenderWindow;
class vtkActor;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLImageSliceMapper :
  public vtkImageSliceMapper
{
public:
  static vtkOpenGLImageSliceMapper *New();
  vtkTypeMacro(vtkOpenGLImageSliceMapper, vtkImageSliceMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implement base class method.  Perform the render.
   */
  void Render(vtkRenderer *ren, vtkImageSlice *prop);

  /**
   * Release any graphics resources that are being consumed by this
   * mapper, the image texture in particular. Using the same texture
   * in multiple render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkOpenGLImageSliceMapper();
  ~vtkOpenGLImageSliceMapper();

  /**
   * Recursive internal method, will call the non-recursive method
   * as many times as necessary if the texture must be broken up into
   * pieces that are small enough for the GPU to render
   */
  void RecursiveRenderTexturedPolygon(
    vtkRenderer *ren, vtkImageProperty *property,
    vtkImageData *image, int extent[6], bool recursive);

  /**
   * Non-recursive internal method, generate a single texture
   * and its corresponding geometry.
   */
  void RenderTexturedPolygon(
    vtkRenderer *ren, vtkImageProperty *property,
    vtkImageData *image, int extent[6], bool recursive);

  /**
   * Basic polygon rendering, if the textured parameter is set the tcoords
   * are included, otherwise they aren't.
   */
  void RenderPolygon(vtkActor *actor, vtkPoints *points, const int extent[6], vtkRenderer *ren);

  /**
   * Render the background, which means rendering everything within the
   * plane of the image except for the polygon that displays the image data.
   */
  void RenderBackground(
    vtkActor *actor, vtkPoints *points, const int extent[6], vtkRenderer *ren);

  /**
   * Bind the fragment program, and generate it first if necessary.
   */
  void BindFragmentProgram(vtkRenderer *ren, vtkImageProperty *property);

  /**
   * Build the fragment program to use with the texture.
   */
  vtkStdString BuildFragmentProgram(vtkImageProperty *property);

  /**
   * Given an extent that describes a slice (it must have unit thickness
   * in one of the three directions), return the dimension indices that
   * correspond to the texture "x" and "y", provide the x, y image size,
   * and provide the texture size (padded to a power of two if the hardware
   * requires).
   */
  void ComputeTextureSize(
    const int extent[6], int &xdim, int &ydim,
    int imageSize[2], int textureSize[2]);

  /**
   * Test whether a given texture size is supported.  This includes a
   * check of whether the texture will fit into texture memory.
   */
  bool TextureSizeOK(const int size[2]);

  /**
   * Check various OpenGL capabilities
   */
  void CheckOpenGLCapabilities(vtkOpenGLRenderWindow *renWin);

  long FragmentShaderIndex; // OpenGL ID for fragment shader
  vtkRenderWindow *RenderWindow; // RenderWindow used for previous render
  int TextureSize[2];
  int TextureBytesPerPixel;
  int LastOrientation;
  int LastSliceNumber;

  vtkActor *PolyDataActor;
  vtkActor *BackingPolyDataActor;
  vtkActor *BackgroundPolyDataActor;

  vtkTimeStamp LoadTime;
  int LoadCount;

  bool UseClampToEdge;
  bool UseFragmentProgram;

private:
  vtkOpenGLImageSliceMapper(const vtkOpenGLImageSliceMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLImageSliceMapper&) VTK_DELETE_FUNCTION;
};

#endif
