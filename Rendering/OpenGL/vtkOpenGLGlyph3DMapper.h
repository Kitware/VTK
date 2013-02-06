/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGlyph3DMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLGlyph3DMapper - vtkOpenGLGlyph3D on the GPU.
// .SECTION Description
// Do the same job than vtkGlyph3D but on the GPU. For this reason, it is
// a mapper not a vtkPolyDataAlgorithm. Also, some methods of vtkOpenGLGlyph3D
// don't make sense in vtkOpenGLGlyph3DMapper: GeneratePointIds, old-style
// SetSource, PointIdsName, IsPointVisible.
// .SECTION Implementation
//
// .SECTION See Also
// vtkOpenGLGlyph3D

#ifndef __vtkOpenGLGlyph3DMapper_h
#define __vtkOpenGLGlyph3DMapper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkGlyph3DMapper.h"
#include "vtkGlyph3D.h" // for the constants (VTK_SCALE_BY_SCALAR, ...).
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkOpenGLGlyph3DMapperArray; // pimp
class vtkPainterPolyDataMapper;
class vtkScalarsToColorsPainter;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLGlyph3DMapper : public vtkGlyph3DMapper
{
public:
  static vtkOpenGLGlyph3DMapper* New();
  vtkTypeMacro(vtkOpenGLGlyph3DMapper, vtkGlyph3DMapper);
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
  vtkOpenGLGlyph3DMapper();
  ~vtkOpenGLGlyph3DMapper();

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector*);

  // Description:
  // Send mapper ivars to sub-mapper.
  // \pre mapper_exists: mapper!=0
  void CopyInformationToSubMapper(vtkPainterPolyDataMapper*);

  // Description:
  // Release display list used for matrices and color.
  void ReleaseList();

  // Description:
  // Called when the PainterInformation becomes obsolete.
  // It is called before the Render is initiated on the Painter.
  virtual void UpdatePainterInformation();

  vtkOpenGLGlyph3DMapperArray *SourceMappers; // array of mappers

  vtkWeakPointer<vtkWindow> LastWindow; // Window used for previous render.

  unsigned int DisplayListId; // GLuint

  vtkScalarsToColorsPainter* ScalarsToColorsPainter;
  vtkInformation* PainterInformation;
  vtkTimeStamp PainterUpdateTime;

private:
  vtkOpenGLGlyph3DMapper(const vtkOpenGLGlyph3DMapper&); // Not implemented.
  void operator=(const vtkOpenGLGlyph3DMapper&); // Not implemented.

  virtual void Render(vtkRenderer*, vtkActor*, vtkDataSet*);
  //ETX
};

#endif
