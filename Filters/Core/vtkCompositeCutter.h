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
// .NAME vtkCompositeCutter - Cut composite data sets with user-specified implicit function
// .SECTION Description
// Loop over each data set in the composite input and apply vtkCutter
// .SECTION See Also
// vtkCutter

#ifndef __vtkCompositeCutter_h
#define __vtkCompositeCutter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkCutter.h"

class VTKFILTERSCORE_EXPORT vtkCompositeCutter : public vtkCutter
{
public:
  vtkTypeMacro(vtkCompositeCutter,vtkCutter);

  static vtkCompositeCutter *New();

  void PrintSelf(ostream& os, vtkIndent indent);;

protected:
  vtkCompositeCutter(vtkImplicitFunction *cf=NULL);
  virtual ~vtkCompositeCutter();
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkCompositeCutter(const vtkCompositeCutter&);  // Not implemented.
  void operator=(const vtkCompositeCutter&);  // Not implemented.
};


#endif
