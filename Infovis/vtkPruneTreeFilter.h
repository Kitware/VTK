/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPruneTreeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPruneTreeFilter - prune a subtree out of a vtkTree
//
// .SECTION Description
//

#ifndef __vtkPruneTreeFilter_h
#define __vtkPruneTreeFilter_h

#include "vtkTreeAlgorithm.h"

class vtkTree;
class vtkPVXMLElement;

class VTK_INFOVIS_EXPORT vtkPruneTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkPruneTreeFilter* New();
  vtkTypeRevisionMacro(vtkPruneTreeFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the parent node of the subtree to remove.
  vtkGetMacro(ParentNode, vtkIdType);
  vtkSetMacro(ParentNode, vtkIdType);

protected:
  vtkPruneTreeFilter();
  ~vtkPruneTreeFilter();

  vtkIdType ParentNode;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkPruneTreeFilter(const vtkPruneTreeFilter&); // Not implemented
  void operator=(const vtkPruneTreeFilter&);   // Not implemented
};

#endif

