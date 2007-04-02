/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedThresholds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectedThresholds - extract a cells or points from a 
// dataset that have values within a set of thresholds.

// .SECTION Description
// vtkExtractSelectedThresholds extracts all cells and points with attribute 
// values that lie within a vtkSelection's THRESHOLD contents. The selecion
// can specify to threshold a particular array within either the point or cell
// attribute data of the input. This is similar to vtkThreshold
// but allows mutliple thresholds ranges.
// This filter adds a scalar array called vtkOriginalCellIds that says what 
// input cell produced each output cell. This is an example of a Pedigree ID 
// which helps to trace back results.

// .SECTION See Also
// vtkSelection vtkExtractSelection vtkThreshold 

#ifndef __vtkExtractSelectedThresholds_h
#define __vtkExtractSelectedThresholds_h

#include "vtkDataSetAlgorithm.h"

class vtkSelection;
class vtkDataArray;
class vtkDoubleArray;

class VTK_GRAPHICS_EXPORT vtkExtractSelectedThresholds : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelectedThresholds,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkExtractSelectedThresholds *New();

protected:
  vtkExtractSelectedThresholds();
  ~vtkExtractSelectedThresholds();

  //sets up output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  int ExtractCells(vtkSelection *sel, vtkDataSet *input, 
                   vtkDataSet *output,
                   int usePointScalars);
  int ExtractPoints(vtkSelection *sel, vtkDataSet *input, 
                    vtkDataSet *output);

  int EvaluateValue(vtkDataArray *scalars, vtkIdType id, vtkDoubleArray *lims);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkExtractSelectedThresholds(const vtkExtractSelectedThresholds&);  // Not implemented.
  void operator=(const vtkExtractSelectedThresholds&);  // Not implemented.
};

#endif
