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

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkThreshold.h"

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmThreshold : public vtkThreshold
{
public:
  vtkTypeMacro(vtkmThreshold, vtkThreshold);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmThreshold* New();

  ///@{
  /**
   * When this flag is off (the default), then the computation will fall back
   * to the serial VTK version if VTK-m fails to run. When the flag is on,
   * the filter will generate an error if VTK-m fails to run. This is mostly
   * useful in testing to make sure the expected algorithm is run.
   */
  vtkGetMacro(ForceVTKm, vtkTypeBool);
  vtkSetMacro(ForceVTKm, vtkTypeBool);
  vtkBooleanMacro(ForceVTKm, vtkTypeBool);
  ///@}

protected:
  vtkmThreshold();
  ~vtkmThreshold() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ForceVTKm = false;

private:
  vtkmThreshold(const vtkmThreshold&) = delete;
  void operator=(const vtkmThreshold&) = delete;
};

#endif // vtkmThreshold_h
