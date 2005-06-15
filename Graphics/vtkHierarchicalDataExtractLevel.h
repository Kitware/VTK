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
// vtkHierarchicalDataExtractLevel is a filter to that extracts levels
// between user specified min and max.

#ifndef __vtkHierarchicalDataExtractLevel_h
#define __vtkHierarchicalDataExtractLevel_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataExtractLevel : public vtkHierarchicalDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalDataExtractLevel,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHierarchicalDataExtractLevel *New();

  // Description:
  // Minimum level to be extacted
  vtkSetMacro(MinLevel, unsigned int);
  vtkGetMacro(MinLevel, unsigned int);

  // Description:
  // Maximum level to be extacted
  vtkSetMacro(MaxLevel, unsigned int);
  vtkGetMacro(MaxLevel, unsigned int);

  // Description:
  // Sets the min and max levels
  void SetLevelRange(unsigned int min, unsigned int max)
    {
      this->SetMinLevel(min);
      this->SetMaxLevel(max);
    }

  // Description:
  // Returns input min (always 0) and max levels.
  vtkGetVector2Macro(InputLevels, int);

protected:
  vtkHierarchicalDataExtractLevel();
  ~vtkHierarchicalDataExtractLevel();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation *, 
                                 vtkInformationVector **, 
                                 vtkInformationVector *);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  unsigned int MinLevel;
  unsigned int MaxLevel;
  int InputLevels[2];

private:
  vtkHierarchicalDataExtractLevel(const vtkHierarchicalDataExtractLevel&);  // Not implemented.
  void operator=(const vtkHierarchicalDataExtractLevel&);  // Not implemented.
};

#endif


