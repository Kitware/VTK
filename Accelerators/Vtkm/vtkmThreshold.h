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

#ifndef vtkmThreshold_h
#define vtkmThreshold_h

#include "vtkThreshold.h"
#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation

class VTKACCELERATORSVTKM_EXPORT vtkmThreshold : public vtkThreshold
{
public:
  vtkTypeMacro(vtkmThreshold,vtkThreshold)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkmThreshold* New();

protected:
  vtkmThreshold();
  ~vtkmThreshold();

  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkmThreshold(const vtkmThreshold&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmThreshold&) VTK_DELETE_FUNCTION;
};

#endif // vtkmThreshold_h
// VTK-HeaderTest-Exclude: vtkmThreshold.h
