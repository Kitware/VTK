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
// vtkExtractSelection extracts all cells within a vtkSelection from a
// vtkDataSet. Internally, it uses vtkExtractSelectedUGridIds.
// .SECTION See Also
// vtkSelection vtkExtractSelectedUGridIds

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
  // Construct object with NULL extractfilter
  static vtkExtractSelection *New();

protected:
  vtkExtractSelection();
  ~vtkExtractSelection();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);


  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkExtractCells* ExtractFilter;

private:
  vtkExtractSelection(const vtkExtractSelection&);  // Not implemented.
  void operator=(const vtkExtractSelection&);  // Not implemented.
};

#endif
