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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkStandardPolyDataPainter.h"

class VTKRENDERINGCORE_EXPORT vtkHardwareSelectionPolyDataPainter :
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

  // Description:
  // By default, this painters uses the dataset's point and cell ids during
  // rendering. However, one can override those by specifying cell and point
  // data arrays to use instead. Currently, only vtkIdType array is supported.
  // Set to NULL string (default) to use the point ids instead.
  vtkSetStringMacro(PointIdArrayName);
  vtkGetStringMacro(PointIdArrayName);
  vtkSetStringMacro(CellIdArrayName);
  vtkGetStringMacro(CellIdArrayName);

  // Description:
  // If the painter should override the process id using a data-array,
  // set this variable to the name of the array to use. It must be a
  // point-array.
  vtkSetStringMacro(ProcessIdArrayName);
  vtkGetStringMacro(ProcessIdArrayName);

  // Description:
  // Generally, vtkCompositePainter can render the composite id when iterating
  // over composite datasets. However in some cases (as in AMR), the rendered
  // structure may not correspond to the input data, in which case we need
  // to provide a cell array that can be used to render in the composite id in
  // selection passes. Set to NULL (default) to not override the composite id
  // color set by vtkCompositePainter if any.
  // The array *MUST* be a cell array and of type vtkUnsignedIntArray.
  vtkSetStringMacro(CompositeIdArrayName);
  vtkGetStringMacro(CompositeIdArrayName);

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
  char* PointIdArrayName;
  char* CellIdArrayName;
  char* ProcessIdArrayName;
  char* CompositeIdArrayName;

private:
  vtkHardwareSelectionPolyDataPainter(const vtkHardwareSelectionPolyDataPainter&); // Not implemented.
  void operator=(const vtkHardwareSelectionPolyDataPainter&); // Not implemented.
//ETX
};

#endif


