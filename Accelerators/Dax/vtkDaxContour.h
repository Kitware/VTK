//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkDaxContour_h
#define vtkDaxContour_h

#include "vtkContourFilter.h"
#include "vtkAcceleratorsDaxModule.h" //required for correct implementation

class VTKACCELERATORSDAX_EXPORT vtkDaxContour : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkDaxContour,vtkContourFilter)
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDaxContour* New();

protected:
  vtkDaxContour();
  ~vtkDaxContour();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkDaxContour(const vtkDaxContour&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDaxContour&) VTK_DELETE_FUNCTION;
};

#endif // vtkDaxContour_H
