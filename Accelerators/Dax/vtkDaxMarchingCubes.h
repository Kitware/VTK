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

#ifndef __vtkDaxMarchingCubes_h
#define __vtkDaxMarchingCubes_h

#include "vtkMarchingCubes.h"
#include "vtkAcceleratorsDaxModule.h" //required for correct implementation

class VTKACCELERATORSDAX_EXPORT vtkDaxMarchingCubes : public vtkMarchingCubes
{
public:
  vtkTypeMacro(vtkDaxMarchingCubes,vtkMarchingCubes)
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDaxMarchingCubes* New();

protected:
  vtkDaxMarchingCubes();
  ~vtkDaxMarchingCubes();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkDaxMarchingCubes(const vtkDaxMarchingCubes&); //Not implemented
  void operator=(const vtkDaxMarchingCubes&); // Not implemented
};

#endif // vtkDaxMarchingCubes_H
