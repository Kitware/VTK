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
// .NAME vtkOpenGLImageSliceMapper - OpenGL mapper for image slice display
// .SECTION Description
// vtkOpenGLImageSliceMapper is a concrete implementation of the abstract
// class vtkImageSliceMapper that interfaces to the OpenGL library.
// .SECTION Thanks
// Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
// Neurosciences, Foothills Medical Centre, Calgary, for providing this class.

#ifndef __vtkOpenGLImageSliceMapper_h
#define __vtkOpenGLImageSliceMapper_h

#include "vtkImageSliceMapper.h"

class vtkWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkOpenGLRenderWindow;
class vtkImageSlice;
class vtkImageProperty;
class vtkImageData;

class VTK_RENDERING_EXPORT vtkOpenGLImageSliceMapper :
  public vtkImageSliceMapper
{
public:
  static vtkOpenGLImageSliceMapper *New();
  vtkTypeMacro(vtkOpenGLImageSliceMapper,vtkImageSliceMapper);
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
  vtkOpenGLImageSliceMapper();
  ~vtkOpenGLImageSliceMapper();

  // Description:
  // Load the texture and geometry.
  void Load(vtkRenderer *ren, vtkProp3D *prop, vtkImageProperty *property);

  // Description:
  // Non-recursive internal method, generate a single texture
  // and its corresponding geometry.
  void InternalLoad(
    vtkRenderer *ren, vtkProp3D *prop, vtkImageProperty *property,
    vtkImageData *image, int extent[6], bool recursive);

  // Description:
  // Test whether a given texture size is supported.  This includes a
  // check of whether the texture will fit into texture memory.
  bool TextureSizeOK(const int size[2]);

  // Description:
  // Check various OpenGL capabilities
  void CheckOpenGLCapabilities(vtkOpenGLRenderWindow *renWin);

  // Description:
  // Garbage collection for reference loops.
  void ReportReferences(vtkGarbageCollector*);

  vtkTimeStamp LoadTime;
  long Index; // OpenGL ID for texture or display list
  long FragmentShaderIndex; // OpenGL ID for fragment shader
  vtkRenderWindow *RenderWindow; // RenderWindow used for previous render
  double Coords[12];
  double TCoords[8];
  int TextureSize[2];
  int TextureBytesPerPixel;
  int LastOrientation;
  int LastSliceNumber;

  bool UseClampToEdge;
  bool UseFragmentProgram;

private:
  vtkOpenGLImageSliceMapper(const vtkOpenGLImageSliceMapper&);  // Not implemented.
  void operator=(const vtkOpenGLImageSliceMapper&);  // Not implemented.
};

#endif
