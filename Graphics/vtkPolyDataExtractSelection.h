/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataExtractSelection - extract a list of cells from a polydata
// .SECTION Description
// vtkPolyDataExtractSelection extracts all cells in vtkSelection from a
// vtkPolyData.
// .SECTION See Also
// vtkSelection

#ifndef __vtkPolyDataExtractSelection_h
#define __vtkPolyDataExtractSelection_h

#include "vtkPolyDataAlgorithm.h"

class vtkSelection;

class VTK_GRAPHICS_EXPORT vtkPolyDataExtractSelection : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkPolyDataExtractSelection,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with NULL selection.
  static vtkPolyDataExtractSelection *New();

  // Description:
  // Return the MTime taking into account changes to the selection
  unsigned long GetMTime();

  // Description:
  // Specify the implicit function for inside/outside checks. The selection
  // must have a CONTENT_TYPE of CELL_IDS and have a vtkIdTypeArray
  // containing the cell id list.
  virtual void SetSelection(vtkSelection*);
  vtkGetObjectMacro(Selection,vtkSelection);

protected:
  vtkPolyDataExtractSelection();
  ~vtkPolyDataExtractSelection();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  vtkSelection *Selection;

private:
  vtkPolyDataExtractSelection(const vtkPolyDataExtractSelection&);  // Not implemented.
  void operator=(const vtkPolyDataExtractSelection&);  // Not implemented.
};

#endif
