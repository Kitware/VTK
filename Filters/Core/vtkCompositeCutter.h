/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeCutter
 * @brief   Cut composite data sets with user-specified implicit function
 *
 * Loop over each data set in the composite input and apply vtkCutter
 * @sa
 * vtkCutter
*/

#ifndef vtkCompositeCutter_h
#define vtkCompositeCutter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkCutter.h"

class VTKFILTERSCORE_EXPORT vtkCompositeCutter : public vtkCutter
{
public:
  vtkTypeMacro(vtkCompositeCutter,vtkCutter);

  static vtkCompositeCutter *New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCompositeCutter(vtkImplicitFunction *cf=nullptr);
  ~vtkCompositeCutter() override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkCompositeCutter(const vtkCompositeCutter&) = delete;
  void operator=(const vtkCompositeCutter&) = delete;
};


#endif
