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
/**
 * @class   vtkmWarpVector
 * @brief   deform geometry with vector data
 *
 * vtkWarpVector is a filter that modifies point coordinates by moving
 * points along vector times the scale factor. Useful for showing flow
 * profiles or mechanical deformation.
 *
 * The filter passes both its point data and cell data to its output.
 */

#ifndef vtkmWarpVector_h
#define vtkmWarpVector_h

#include "vtkAcceleratorsVTKmModule.h" // required for correct export
#include "vtkWarpVector.h"

class VTKACCELERATORSVTKM_EXPORT vtkmWarpVector : public vtkWarpVector
{
public:
  vtkTypeMacro(vtkmWarpVector, vtkWarpVector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmWarpVector* New();

protected:
  vtkmWarpVector();
  ~vtkmWarpVector() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmWarpVector(const vtkmWarpVector&) = delete;
  void operator=(const vtkmWarpVector&) = delete;
};

#endif // vtkmWarpVector_h

// VTK-HeaderTest-Exclude: vtkmWarpVector.h
