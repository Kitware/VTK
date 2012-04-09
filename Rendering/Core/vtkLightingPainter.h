/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightingPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLightingPainter - abstract class defining interface for painter
// that can handle lightin.

#ifndef __vtkLightingPainter_h
#define __vtkLightingPainter_h

#include "vtkPolyDataPainter.h"

class VTK_RENDERING_EXPORT vtkLightingPainter : public vtkPolyDataPainter
{
public:
  static vtkLightingPainter* New();
  vtkTypeMacro(vtkLightingPainter, vtkPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkLightingPainter();
  ~vtkLightingPainter();
private:
  vtkLightingPainter(const vtkLightingPainter&); // Not implemented.
  void operator=(const vtkLightingPainter&); // Not implemented.
};

#endif

