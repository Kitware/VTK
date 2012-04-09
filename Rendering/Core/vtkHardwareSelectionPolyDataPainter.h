/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwareSelectionPolyDataPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHardwareSelectionPolyDataPainter - painter used to render polydata
// for selection passes.
// .SECTION Description
// vtkHardwareSelectionPolyDataPainter is a painter for polydata used when
// rendering hardware selection passes.
// .SECTION See Also
// vtkHardwareSelector

#ifndef __vtkHardwareSelectionPolyDataPainter_h
#define __vtkHardwareSelectionPolyDataPainter_h

#include "vtkStandardPolyDataPainter.h"

class VTK_RENDERING_EXPORT vtkHardwareSelectionPolyDataPainter :
  public vtkStandardPolyDataPainter
{
public:
  static vtkHardwareSelectionPolyDataPainter* New();
  vtkTypeMacro(vtkHardwareSelectionPolyDataPainter, vtkStandardPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/Disable vtkHardwareSelector class. Useful when using this painter as
  // an internal painter. Default is enabled.
  vtkSetMacro(EnableSelection, int);
  vtkGetMacro(EnableSelection, int);
  vtkBooleanMacro(EnableSelection, int);

//BTX
protected:
  vtkHardwareSelectionPolyDataPainter();
  ~vtkHardwareSelectionPolyDataPainter();

  // Description:
  // Generates rendering primitives of appropriate type(s). Multiple types 
  // of primitives can be requested by or-ring the primitive flags. 
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
    unsigned long typeflags, bool forceCompileOnly);

  void DrawCells(int mode, vtkCellArray *connectivity,
    vtkIdType startCellId, vtkRenderer *renderer);

  int EnableSelection;
  vtkIdType TotalCells;
private:
  vtkHardwareSelectionPolyDataPainter(const vtkHardwareSelectionPolyDataPainter&); // Not implemented.
  void operator=(const vtkHardwareSelectionPolyDataPainter&); // Not implemented.
//ETX
};

#endif


