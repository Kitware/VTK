/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractFactoredArray.h
  
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

#ifndef __vtkExtractFactoredArray_h
#define __vtkExtractFactoredArray_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkSetGet.h"

// .NAME vtkExtractFactoredArray - Extracts the Nth array stored in a
// vtkFactoredArrayData object into a pipeline array data object.

// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_INFOVIS_EXPORT vtkExtractFactoredArray : public vtkArrayDataAlgorithm
{
public:
  static vtkExtractFactoredArray* New();
  vtkTypeRevisionMacro(vtkExtractFactoredArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls which array will be extracted.
  vtkGetMacro(Index, vtkIdType);
  vtkSetMacro(Index, vtkIdType);

protected:
  vtkExtractFactoredArray();
  ~vtkExtractFactoredArray();

  int FillInputPortInformation(int port, vtkInformation* info);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkExtractFactoredArray(const vtkExtractFactoredArray&); // Not implemented
  void operator=(const vtkExtractFactoredArray&);   // Not implemented

  vtkIdType Index;
};

#endif

