/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRepresentationPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaRepresentationPainter - painter handling representation 
// using Mesa.
// .SECTION Description
// This is Mesa implementation of a painter handling representation 
// i.e. Points, Wireframe, Surface.

#ifndef __vtkMesaRepresentationPainter_h
#define __vtkMesaRepresentationPainter_h

#include "vtkRepresentationPainter.h"

class VTK_RENDERING_EXPORT vtkMesaRepresentationPainter : 
  public vtkRepresentationPainter
{
public:
  static vtkMesaRepresentationPainter* New();
  vtkTypeMacro(vtkMesaRepresentationPainter, vtkRepresentationPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMesaRepresentationPainter();
  ~vtkMesaRepresentationPainter();

  // Description:
  // Changes the polygon mode according to the representation.
  void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
    unsigned long typeflags);
private:
  vtkMesaRepresentationPainter(const vtkMesaRepresentationPainter&); // Not implemented.
  void operator=(const vtkMesaRepresentationPainter&); // Not implemented.
};

#endif
