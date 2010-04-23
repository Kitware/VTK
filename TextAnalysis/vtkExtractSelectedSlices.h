/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedSlices.h
  
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

#ifndef __vtkExtractSelectedSlices_h
#define __vtkExtractSelectedSlices_h

#include <vtkArrayDataAlgorithm.h>

// .NAME vtkExtractSelectedSlices - Extract selected slices from a vtkArray
//
// Inputs:
//   Input port 0: (required) A vtkSparseArray<double> of any dimension.
//   Input port 1: (required) A vtkSelection containing indices.
//
// Outputs:
//   Output port 0: A vtkSparseArray<double> containing only the selected slices.
//
// Note that the indices in the input selection must be less-than the maximum
// extent of the input array along the slice dimension.
//
// .SECTION Caveats
// Only works with vtkSparseArray<double>, this needs to be generalized.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkExtractSelectedSlices :
  public vtkArrayDataAlgorithm
{
public:
  static vtkExtractSelectedSlices* New();
  vtkTypeMacro(vtkExtractSelectedSlices, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the dimension along which slices will be extracted.  Default: 0
  vtkGetMacro(SliceDimension, int);
  vtkSetMacro(SliceDimension, int);

//BTX
protected:
  vtkExtractSelectedSlices();
  ~vtkExtractSelectedSlices();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkExtractSelectedSlices(const vtkExtractSelectedSlices&); // Not implemented
  void operator=(const vtkExtractSelectedSlices&);   // Not implemented

  int SliceDimension;
//ETX
};

#endif

