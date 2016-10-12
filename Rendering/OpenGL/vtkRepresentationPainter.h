/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRepresentationPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRepresentationPainter
 * @brief   painter that handles representation.
 *
 * This painter merely defines the interface.
 * Subclasses will change the polygon rendering mode dependent on
 * the graphics library.
*/

#ifndef vtkRepresentationPainter_h
#define vtkRepresentationPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataPainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkRepresentationPainter : public vtkPolyDataPainter
{
public:
  static vtkRepresentationPainter* New();
  vtkTypeMacro(vtkRepresentationPainter, vtkPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkRepresentationPainter();
  ~vtkRepresentationPainter();

private:
  vtkRepresentationPainter(const vtkRepresentationPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRepresentationPainter&) VTK_DELETE_FUNCTION;
};

#endif
