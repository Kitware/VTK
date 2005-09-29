/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataInformation - hierarchical information collection
// .SECTION Description
// Legacy class. Use vtkMultiGroupDataInformation instead.
// .SECTION See Also
// vtkMultiGroupDataInformation

#ifndef __vtkHierarchicalDataInformation_h
#define __vtkHierarchicalDataInformation_h

#include "vtkMultiGroupDataInformation.h"

class VTK_FILTERING_EXPORT vtkHierarchicalDataInformation : public vtkMultiGroupDataInformation
{
public:
  static vtkHierarchicalDataInformation *New();
  vtkTypeRevisionMacro(vtkHierarchicalDataInformation,vtkMultiGroupDataInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the number of hierarchy levels.
  unsigned int GetNumberOfLevels()
    {
      return this->GetNumberOfGroups();
    }

  // Description:
  // Set the number of hierarchy levels.
  void SetNumberOfLevels(unsigned int numLevels)
    {
      this->SetNumberOfGroups(numLevels);
    }

protected:
  vtkHierarchicalDataInformation();
  ~vtkHierarchicalDataInformation();

private:
  vtkHierarchicalDataInformation(const vtkHierarchicalDataInformation&);  // Not implemented.
  void operator=(const vtkHierarchicalDataInformation&);  // Not implemented.
};

#endif
