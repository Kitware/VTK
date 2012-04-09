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
// .NAME vtkRepresentationPainter - painter that handles representation.
// .SECTION Description
// This painter merely defines the interface.
// Subclasses will change the polygon rendering mode dependent on
// the graphics library.

#ifndef __vtkRepresentationPainter_h
#define __vtkRepresentationPainter_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPolyDataPainter.h"

class VTKRENDERINGCORE_EXPORT vtkRepresentationPainter : public vtkPolyDataPainter
{
public:
  static vtkRepresentationPainter* New();
  vtkTypeMacro(vtkRepresentationPainter, vtkPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkRepresentationPainter();
  ~vtkRepresentationPainter();

private:
  vtkRepresentationPainter(const vtkRepresentationPainter&); // Not implemented.
  void operator=(const vtkRepresentationPainter&); // Not implemented.
};


#endif

