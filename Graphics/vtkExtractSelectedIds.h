/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectedIds - extract a list of cells from a dataset
// .SECTION Description
// vtkExtractSelectedIds extracts a set of cells and points from within a
// vtkDataSet. The set of ids to extract are listed within a vtkSelection.
// This filter adds a scalar array called vtkOriginalCellIds that says what 
// input cell produced each output cell. This is an example of a Pedigree ID 
// which helps to trace back results. If the selection has an ARRAY_NAME
// the filter will use that to decide where each id is. If it doesn't have
// that then it checks if the input data set has a GlobalIds DataSetAttribute
// and uses that array. If neither of those are available it will use the
// offset into the points or cells lists.

// .SECTION See Also
// vtkSelection vtkExtractSelection

#ifndef __vtkExtractSelectedIds_h
#define __vtkExtractSelectedIds_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkSelection;

class VTK_GRAPHICS_EXPORT vtkExtractSelectedIds : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelectedIds,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with NULL extractfilter
  static vtkExtractSelectedIds *New();

protected:
  vtkExtractSelectedIds();
  ~vtkExtractSelectedIds();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);


  int ExtractCells(vtkSelection *sel, vtkDataSet *input, 
                   vtkUnstructuredGrid *output);
  int ExtractPoints(vtkSelection *sel, vtkDataSet *input, 
                    vtkUnstructuredGrid *output);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkExtractSelectedIds(const vtkExtractSelectedIds&);  // Not implemented.
  void operator=(const vtkExtractSelectedIds&);  // Not implemented.
};

#endif
