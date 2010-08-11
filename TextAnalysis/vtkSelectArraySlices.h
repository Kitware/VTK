/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectArraySlices.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSelectArraySlices - Produces a selection based on array slice values
//
// Inputs:
//   Input port 0: (required) A vtkTypedArray<double> of any dimension.
//
// Outputs:
//   Output port 0: A vtkSelection containing the indices of each slice (along a
//     user-supplied dimension) that should be selected.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkSelectArraySlices_h
#define __vtkSelectArraySlices_h

#include <vtkSelectionAlgorithm.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkSelectArraySlices :
  public vtkSelectionAlgorithm
{
public:
  static vtkSelectArraySlices* New();
  vtkTypeMacro(vtkSelectArraySlices, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the dimension along which slices will be selected.  Default: 0
  vtkGetMacro(SliceDimension, vtkIdType);
  vtkSetMacro(SliceDimension, vtkIdType);

  // Description:
  // Controls the minimum number of non-zero values that a slice can contain
  // and still be selected.  Default: 1
  vtkGetMacro(MinimumCount, vtkIdType);
  vtkSetMacro(MinimumCount, vtkIdType);

  // Description:
  // Controls the maximum number of non-zero values that a slice can contain
  // and still be selected.  Default: INT_MAX
  vtkGetMacro(MaximumCount, vtkIdType);
  vtkSetMacro(MaximumCount, vtkIdType);

  // Description:
  // Controls the minimum percentage of non-zero values that a slice can contain
  // and still be selected.  Default: 0.0
  vtkGetMacro(MinimumPercent, double);
  vtkSetMacro(MinimumPercent, double);

  // Description:
  // Controls the maximum percentage of non-zero values that a slice can contain
  // and still be selected.  Default: 1.0
  vtkGetMacro(MaximumPercent, double);
  vtkSetMacro(MaximumPercent, double);

//BTX
protected:
  vtkSelectArraySlices();
  ~vtkSelectArraySlices();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkSelectArraySlices(const vtkSelectArraySlices&); // Not implemented
  void operator=(const vtkSelectArraySlices&);   // Not implemented

  vtkIdType SliceDimension;
  vtkIdType MinimumCount;
  vtkIdType MaximumCount;
  double MinimumPercent;
  double MaximumPercent;
//ETX
};

#endif

