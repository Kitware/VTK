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
 * @class   vtkmThreshold
 * @brief   extracts cells where scalar value in cell satisfies threshold criterion
 *
 * vtkmThreshold is a filter that extracts cells from any dataset type that
 * satisfy a threshold criterion. A cell satisfies the criterion if the
 * scalar value of every point or cell satisfies the criterion. The
 * criterion takes the form of between two values. The output of this
 * filter is an unstructured grid.
 *
 * Note that scalar values are available from the point and cell attribute
 * data. By default, point data is used to obtain scalars, but you can
 * control this behavior. See the AttributeMode ivar below.
 *
 */
#ifndef vtkmThreshold_h
#define vtkmThreshold_h

#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkThreshold.h"

class VTKACCELERATORSVTKM_EXPORT vtkmThreshold : public vtkThreshold
{
public:
  vtkTypeMacro(vtkmThreshold, vtkThreshold);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmThreshold* New();

protected:
  vtkmThreshold();
  ~vtkmThreshold() override;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmThreshold(const vtkmThreshold&) = delete;
  void operator=(const vtkmThreshold&) = delete;
};

#endif // vtkmThreshold_h
// VTK-HeaderTest-Exclude: vtkmThreshold.h
