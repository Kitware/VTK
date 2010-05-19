/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArray.h
  
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

#ifndef __vtkExtractArray_h
#define __vtkExtractArray_h

#include "vtkArrayDataAlgorithm.h"

// .NAME vtkExtractArray - Given a vtkArrayData object containing one-or-more
// vtkArray instances, produces a vtkArrayData containing just one vtkArray,
// indentified by index.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_INFOVIS_EXPORT vtkExtractArray : public vtkArrayDataAlgorithm
{
public:
  static vtkExtractArray* New();
  vtkTypeMacro(vtkExtractArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls which array will be extracted.
  vtkGetMacro(Index, vtkIdType);
  vtkSetMacro(Index, vtkIdType);

protected:
  vtkExtractArray();
  ~vtkExtractArray();

  int FillInputPortInformation(int port, vtkInformation* info);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkExtractArray(const vtkExtractArray&); // Not implemented
  void operator=(const vtkExtractArray&);   // Not implemented

  vtkIdType Index;
};

#endif

