/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataExtractLevel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataExtractLevel - extact levels between min and max
// .SECTION Description
// Legacy class. Use vtkMultiGroupDataExtractGroup instead.
// .SECTION See Also
// vtkMultiGroupDataExtractGroup

#ifndef __vtkHierarchicalDataExtractLevel_h
#define __vtkHierarchicalDataExtractLevel_h

#include "vtkMultiGroupDataExtractGroup.h"

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataExtractLevel : public vtkMultiGroupDataExtractGroup 
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalDataExtractLevel,vtkMultiGroupDataExtractGroup);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHierarchicalDataExtractLevel *New();

  // Description:
  // Minimum level to be extacted
  void SetMinLevel(unsigned int level)
    {
      this->Superclass::SetMinGroup(level);
    }
  unsigned int GetMinLevel()
    {
      return this->Superclass::GetMinGroup();
    }

  // Description:
  // Maximum level to be extacted
  void SetMaxLevel(unsigned int level)
    {
      this->Superclass::SetMaxGroup(level);
    }
  unsigned int GetMaxLevel()
    {
      return this->Superclass::GetMaxGroup();
    }

  // Description:
  // Sets the min and max levels
  void SetLevelRange(unsigned int min, unsigned int max)
    {
      this->SetMinLevel(min);
      this->SetMaxLevel(max);
    }

protected:
  vtkHierarchicalDataExtractLevel();
  ~vtkHierarchicalDataExtractLevel();

private:
  vtkHierarchicalDataExtractLevel(const vtkHierarchicalDataExtractLevel&);  // Not implemented.
  void operator=(const vtkHierarchicalDataExtractLevel&);  // Not implemented.
};

#endif


