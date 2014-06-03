/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2Glyph3DMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGL2Glyph3DMapper - vtkOpenGLGlyph3D on the GPU.
// .SECTION Description
// Do the same job than vtkGlyph3D but on the GPU. For this reason, it is
// a mapper not a vtkPolyDataAlgorithm. Also, some methods of vtkOpenGLGlyph3D
// don't make sense in vtkOpenGL2Glyph3DMapper: GeneratePointIds, old-style
// SetSource, PointIdsName, IsPointVisible.
// .SECTION Implementation
//
// .SECTION See Also
// vtkOpenGLGlyph3D

#ifndef __vtkOpenGL2Glyph3DMapper_h
#define __vtkOpenGL2Glyph3DMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkGlyph3DMapper.h"
#include "vtkGlyph3D.h" // for the constants (VTK_SCALE_BY_SCALAR, ...).
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include "vtkNew.h" // For vtkNew

class vtkVBOPolyDataMapper;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2Glyph3DMapper
    : public vtkGlyph3DMapper
{
public:
  static vtkOpenGL2Glyph3DMapper* New();
  vtkTypeMacro(vtkOpenGL2Glyph3DMapper, vtkGlyph3DMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

   // Description:
  // Method initiates the mapping process. Generally sent by the actor
  // as each frame is rendered.
  // Its behavior depends on the value of SelectMode.
  virtual void Render(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

protected:
  //BTX
  vtkOpenGL2Glyph3DMapper();
  ~vtkOpenGL2Glyph3DMapper();

  // Description:
  // Send mapper ivars to sub-mapper.
  // \pre mapper_exists: mapper != 0
  void CopyInformationToSubMapper(vtkVBOPolyDataMapper*);

  void SetupColorMapper();

  class vtkColorMapper;
  vtkNew<vtkColorMapper> ColorMapper;

  class vtkOpenGL2Glyph3DMapperEntry;
  class vtkOpenGL2Glyph3DMapperArray;
  vtkOpenGL2Glyph3DMapperArray *GlyphValues; // array of value for datasets

  vtkWeakPointer<vtkWindow> LastWindow; // Window used for previous render.

  vtkVBOPolyDataMapper *Mapper;

  vtkTimeStamp PainterUpdateTime;

private:
  vtkOpenGL2Glyph3DMapper(const vtkOpenGL2Glyph3DMapper&); // Not implemented.
  void operator=(const vtkOpenGL2Glyph3DMapper&); // Not implemented.

  virtual void Render(vtkRenderer*, vtkActor*, vtkDataSet*);

  //ETX
};

#endif
