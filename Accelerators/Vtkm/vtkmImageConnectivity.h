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
 * @class   vtkmImageConnectivity
 * @brief   Label regions inside an image by connectivity
 *
 * vtkmImageConnectivity will identify connected regions within an
 * image and label them.
 * The filter finds groups of points that have the same field value and are
 * connected together through their topology. Any point is considered to be
 * connected to its Moore neighborhood:
 * - 8 neighboring points for 2D
 * - 27 neighboring points for 3D
 *
 * The active field passed to the filter must be associated with the points.
 * The result of the filter is a point field of type vtkIdType.
 * Each entry in the point field will be a number that identifies to which
 * region it belongs. By default, this output point field is named “component”.
 *
 * @sa
 * vtkConnectivityFilter, vtkImageConnectivityFilter
 */

#ifndef vtkmImageConnectivity_h
#define vtkmImageConnectivity_h

#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkImageAlgorithm.h"

class VTKACCELERATORSVTKM_EXPORT vtkmImageConnectivity : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkmImageConnectivity, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmImageConnectivity* New();

protected:
  vtkmImageConnectivity();
  ~vtkmImageConnectivity() override;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmImageConnectivity(const vtkmImageConnectivity&) = delete;
  void operator=(const vtkmImageConnectivity&) = delete;
};

#endif // vtkmImageConnectivity_h
// VTK-HeaderTest-Exclude: vtkmImageConnectivity.h
