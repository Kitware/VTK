/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputeQuartiles.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkComputeQuartiles_h
#define vtkComputeQuartiles_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class vtkDataSet;
class vtkDoubleArray;
class vtkFieldData;
class vtkTable;

// .NAME vtkComputeQuartiles - Extract quartiles and extremum values
// of all columns of a table or all fields of a dataset.
//
// .SECTION Description
// vtkComputeQuartiles accepts any vtkDataObject as input and produces a
// vtkTable data as output.
// This filter can be used to generate a table to create box plots
// using a vtkPlotBox instance.
// The filter internally uses vtkOrderStatistics to compute quartiles.
//
// .SECTION See also
// vtkTableAlgorithm vtkOrderStatistics vtkPlotBox vtkChartBox
//
// .SECTION Thanks
// This class was written by Kitware SAS and supported by EDF - www.edf.fr

class VTKFILTERSSTATISTICS_EXPORT vtkComputeQuartiles : public vtkTableAlgorithm
{
public:
  static vtkComputeQuartiles* New();
  vtkTypeMacro(vtkComputeQuartiles, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkComputeQuartiles();
  ~vtkComputeQuartiles();

  virtual int FillInputPortInformation (int port, vtkInformation *info);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  void ComputeTable(vtkDataObject*, vtkTable*, vtkIdType);

  int FieldAssociation;

private:
  void operator=(const vtkComputeQuartiles&); // Not implemented
  vtkComputeQuartiles(const vtkComputeQuartiles&); // Not implemented

  int GetInputFieldAssociation();
  vtkFieldData* GetInputFieldData(vtkDataObject* input);
};

#endif
