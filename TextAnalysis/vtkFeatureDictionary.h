/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureDictionary.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef __vtkFeatureDictionary_h
#define __vtkFeatureDictionary_h

#include <vtkTableAlgorithm.h>

// .NAME vtkFeatureDictionary - Generates a dictionary of unique terms.
//
// .SECTION Description
// vtkFeatureDictionary reorganizes a table containing (potentially duplicated) terms into
// a dictionary where every term appears exactly once.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing "type" and "text" columns.
//
// Outputs:
//   Output port 0: A vtkTable containing "type" and "text" columns where each
//     input term appears exactly once.
//
// Use SetInputArrayToProcess(0, ...) to specify the input "type" array.
// Use SetInputArrayToProcess(1, ...) to specify the input "text" array.
//
// .SECTION See Also
// vtkPTermDictionary
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkFeatureDictionary :
  public vtkTableAlgorithm
{
public:
  static vtkFeatureDictionary* New();
  vtkTypeMacro(vtkFeatureDictionary, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkFeatureDictionary();
  ~vtkFeatureDictionary();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkFeatureDictionary(const vtkFeatureDictionary&); // Not implemented
  void operator=(const vtkFeatureDictionary&);   // Not implemented
};

#endif

