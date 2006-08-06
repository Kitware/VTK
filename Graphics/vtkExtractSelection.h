/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelection - extract a list of cells from a dataset
// .SECTION Description
// vtkExtractSelection extracts all cells in vtkSelection from a
// vtkDataSet. Internally, it uses vtkExtractCells
// .SECTION See Also
// vtkSelection vtkExtractCells

#ifndef __vtkExtractSelection_h
#define __vtkExtractSelection_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkExtractCells;
class vtkSelection;

class VTK_GRAPHICS_EXPORT vtkExtractSelection : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelection,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with NULL selection.
  static vtkExtractSelection *New();

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
  vtkExtractSelection();
  ~vtkExtractSelection();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  vtkSelection *Selection;

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkExtractCells* ExtractFilter;

private:
  vtkExtractSelection(const vtkExtractSelection&);  // Not implemented.
  void operator=(const vtkExtractSelection&);  // Not implemented.
};

#endif
