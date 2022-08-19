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
 * @class   vtkmContour
 * @brief   generate isosurface(s) from volume
 *
 * vtkmContour is a filter that takes as input a volume (e.g., 3D
 * structured point set) and generates on output one or more isosurfaces.
 * One or more contour values must be specified to generate the isosurfaces.
 * Alternatively, you can specify a min/max scalar range and the number of
 * contours to generate a series of evenly spaced contour values.
 *
 * @warning
 * This filter is currently only supports 3D volumes. If you are interested in
 * contouring other types of data, use the general vtkContourFilter. If you
 * want to contour an image (i.e., a volume slice), use vtkMarchingSquares.
 *
 */

#ifndef vtkmContour_h
#define vtkmContour_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkContourFilter.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmContour : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkmContour, vtkContourFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmContour* New();

protected:
  /// \brief Check if the input dataset and parameters combination is supported by this filter
  ///
  /// Certain input and parameters combinations are not currently supported by vtkm.
  /// This information is internally used to determine if this filter should fall back to
  /// Superclass implementaion.
  bool CanProcessInput(vtkDataSet* input);

  vtkmContour();
  ~vtkmContour() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmContour(const vtkmContour&) = delete;
  void operator=(const vtkmContour&) = delete;
  vtkmInitializer Initializer;
};

#endif // vtkmContour_h
