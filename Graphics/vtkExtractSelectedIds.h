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
// which helps to trace back results. Depending on whether the selection has
// GLOBALIDS, VALUES or OFFSETS, the selection will use the contents of the
// array named in the GLOBALIDS DataSetAttribute, and arbitrary array, or the
// position (tuple id or number) within the cell or point array.
// .SECTION See Also
// vtkSelection vtkExtractSelection

#ifndef __vtkExtractSelectedIds_h
#define __vtkExtractSelectedIds_h

#include "vtkDataSetAlgorithm.h"

class vtkSelection;

class VTK_GRAPHICS_EXPORT vtkExtractSelectedIds : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelectedIds,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with NULL extractfilter
  static vtkExtractSelectedIds *New();

protected:
  vtkExtractSelectedIds();
  ~vtkExtractSelectedIds();

  //sets up output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);
 

  // Usual data generation method
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);


  int ExtractCells(vtkSelection *sel, vtkDataSet *input, 
                   vtkDataSet *output);
  int ExtractPoints(vtkSelection *sel, vtkDataSet *input, 
                    vtkDataSet *output);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkExtractSelectedIds(const vtkExtractSelectedIds&);  // Not implemented.
  void operator=(const vtkExtractSelectedIds&);  // Not implemented.
};

#endif
