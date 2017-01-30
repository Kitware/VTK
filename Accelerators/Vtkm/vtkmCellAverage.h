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

#ifndef vtkmCellAverage_h
#define vtkmCellAverage_h

#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkDataSetAlgorithm.h"

class VTKACCELERATORSVTKM_EXPORT vtkmCellAverage : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkmCellAverage, vtkDataSetAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmCellAverage* New();

protected:
  vtkmCellAverage();
  ~vtkmCellAverage();

  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkmCellAverage(const vtkmCellAverage&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmCellAverage&) VTK_DELETE_FUNCTION;
};

#endif // vtkmCellAverage_h
// VTK-HeaderTest-Exclude: vtkmCellAverage.h
