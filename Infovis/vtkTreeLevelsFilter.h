/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeLevelsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkTreeLevelsFilter - adds level and leaf fields to a vtkTree
//
// .SECTION Description
// The filter currently add two arrays to the incoming vtkTree datastructure.
// 1) "levels" this is the distance from the root of the node. Root = 0
// and you add 1 for each level down from the root
// 2) "leaf" this array simply indicates whether the node is a leaf or not
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for creating this
// class.

#ifndef __vtkTreeLevelsFilter_h
#define __vtkTreeLevelsFilter_h

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTreeLevelsFilter : public vtkTreeAlgorithm 
{
public:
  static vtkTreeLevelsFilter *New();
  vtkTypeRevisionMacro(vtkTreeLevelsFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTreeLevelsFilter();
  ~vtkTreeLevelsFilter() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:  
  vtkTreeLevelsFilter(const vtkTreeLevelsFilter&);  // Not implemented.
  void operator=(const vtkTreeLevelsFilter&);  // Not implemented.
};

#endif
