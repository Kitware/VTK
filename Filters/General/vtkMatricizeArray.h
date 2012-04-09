/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatricizeArray.h
  
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

// .NAME vtkMatricizeArray - Convert an array of arbitrary dimensions to a
// matrix.
//
// .SECTION Description
// Given a sparse input array of arbitrary dimension, creates a sparse output
// matrix (vtkSparseArray<double>) where each column is a slice along an
// arbitrary dimension from the source.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkMatricizeArray_h
#define __vtkMatricizeArray_h

#include "vtkArrayDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkMatricizeArray : public vtkArrayDataAlgorithm
{
public:
  static vtkMatricizeArray* New();
  vtkTypeMacro(vtkMatricizeArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the 0-numbered dimension that will be mapped to columns in the output
  vtkGetMacro(SliceDimension, vtkIdType);
  
  // Description:
  // Sets the 0-numbered dimension that will be mapped to columns in the output
  vtkSetMacro(SliceDimension, vtkIdType);

protected:
  vtkMatricizeArray();
  ~vtkMatricizeArray();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkMatricizeArray(const vtkMatricizeArray&); // Not implemented
  void operator=(const vtkMatricizeArray&);   // Not implemented

//BTX
  class Generator;
//ETX

  vtkIdType SliceDimension;
};

#endif

