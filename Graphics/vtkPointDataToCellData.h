/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointDataToCellData - map point data to cell data
// .SECTION Description
// vtkPointDataToCellData is a filter that transforms point data (i.e., data
// specified per point) into cell data (i.e., data specified per cell).
// The method of transformation is based on averaging the data
// values of all points defining a particular cell. Optionally, the input point
// data can be passed through to the output as well.

// .SECTION Caveats
// This filter is an abstract filter, that is, the output is an abstract type
// (i.e., vtkDataSet). Use the convenience methods (e.g.,
// vtkGetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
// of output you want.

// .SECTION See Also
// vtkDataSetToDataSetFilter vtkPointData vtkCellData vtkCellDataToPointData


#ifndef __vtkPointDataToCellData_h
#define __vtkPointDataToCellData_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkPointDataToCellData : public vtkDataSetToDataSetFilter
{
public:
  static vtkPointDataToCellData *New();
  vtkTypeRevisionMacro(vtkPointDataToCellData,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Control whether the input point data is to be passed to the output. If
  // on, then the input point data is passed through to the output; otherwise,
  // only generated point data is placed into the output.
  vtkSetMacro(PassPointData,int);
  vtkGetMacro(PassPointData,int);
  vtkBooleanMacro(PassPointData,int);

protected:
  vtkPointDataToCellData();
  ~vtkPointDataToCellData() {};

  void Execute();

  int PassPointData;
private:
  vtkPointDataToCellData(const vtkPointDataToCellData&);  // Not implemented.
  void operator=(const vtkPointDataToCellData&);  // Not implemented.
};

#endif


