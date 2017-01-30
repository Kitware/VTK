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

#ifndef vtkmContour_h
#define vtkmContour_h

#include "vtkContourFilter.h"
#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation

class VTKACCELERATORSVTKM_EXPORT vtkmContour : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkmContour,vtkContourFilter)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmContour* New();

protected:
  vtkmContour();
  ~vtkmContour();

  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkmContour(const vtkmContour&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmContour&) VTK_DELETE_FUNCTION;
};

#endif // vtkmContour_h
// VTK-HeaderTest-Exclude: vtkmContour.h
