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

#ifndef vtkDaxMarchingCubes_H
#define vtkDaxMarchingCubes_H

#include <vtkMarchingCubes.h>
#include "vtkAcceleratorsDaxModule.h"

class VTKACCELERATORSDAX_EXPORT vtkDaxMarchingCubes : public vtkMarchingCubes
{
public:
  vtkTypeMacro(vtkDaxMarchingCubes,vtkMarchingCubes)
  static vtkDaxMarchingCubes* New();

protected:
  vtkDaxMarchingCubes();
  ~vtkDaxMarchingCubes();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkDaxMarchingCubes(const vtkDaxMarchingCubes&);
  void operator=(const vtkDaxMarchingCubes&);
};

#endif // vtkDaxMarchingCubes_H
