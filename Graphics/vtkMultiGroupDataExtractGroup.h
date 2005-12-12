/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataExtractGroup.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataExtractGroup - extact groups between min and max
// .SECTION Description
// vtkMultiGroupDataExtractGroup is a filter that extracts groups
// between user specified min and max.

#ifndef __vtkMultiGroupDataExtractGroup_h
#define __vtkMultiGroupDataExtractGroup_h

#include "vtkMultiGroupDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkMultiGroupDataExtractGroup : public vtkMultiGroupDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkMultiGroupDataExtractGroup,vtkMultiGroupDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkMultiGroupDataExtractGroup *New();

  // Description:
  // Minimum group to be extacted
  vtkSetMacro(MinGroup, unsigned int);
  vtkGetMacro(MinGroup, unsigned int);

  // Description:
  // Maximum group to be extacted
  vtkSetMacro(MaxGroup, unsigned int);
  vtkGetMacro(MaxGroup, unsigned int);

  // Description:
  // Sets the min and max groups
  void SetGroupRange(unsigned int min, unsigned int max)
    {
      this->SetMinGroup(min);
      this->SetMaxGroup(max);
    }

protected:
  vtkMultiGroupDataExtractGroup();
  ~vtkMultiGroupDataExtractGroup();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation *, 
                                 vtkInformationVector **, 
                                 vtkInformationVector *);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  unsigned int MinGroup;
  unsigned int MaxGroup;

private:
  vtkMultiGroupDataExtractGroup(const vtkMultiGroupDataExtractGroup&);  // Not implemented.
  void operator=(const vtkMultiGroupDataExtractGroup&);  // Not implemented.
};

#endif


