/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGDALRasterReprojection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGDALRasterReprojection
 *
 */

#ifndef vtkGDALRasterReprojection_h
#define vtkGDALRasterReprojection_h

// VTK Includes
#include "vtkGeovisGDALModule.h" // For export macro
#include "vtkObject.h"

class GDALDataset;

class VTKGEOVISGDAL_EXPORT vtkGDALRasterReprojection : public vtkObject
{
public:
  static vtkGDALRasterReprojection* New();
  vtkTypeMacro(vtkGDALRasterReprojection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The maximum error measured in input pixels that is allowed
   * in approximating the reprojection transformation
   * (0.0 for exact calculations).
   */
  vtkSetClampMacro(MaxError, double, 0.0, VTK_DOUBLE_MAX);
  //@}

  //@{
  /**
   * Pixel resampling algorithm, between 0 and 6
   * 0 = Nearest Neighbor (default)
   * 1 = Bilinear
   * 2 = Cubic
   * 3 = CubicSpline
   * 4 = Lanczos
   * 5 = Average (GDAL 1.10)
   * 6 = Mode    (GDAL 1.10)
   */
  vtkSetClampMacro(ResamplingAlgorithm, int, 0, 6);
  //@}

  /**
   * Suggest image dimensions for specified projection
   * Internally calls GDALSuggestedWarpOutput()
   * The outputProjection parameter can be either the full "well known text"
   * definition, or shorter commonly-used names such as "EPSG:4326" or
   * "WGS84".
   * Returns boolean indicating if computed dimensions are valid.
   */
  bool SuggestOutputDimensions(GDALDataset* inputDataset, const char* outputProjection,
    double geoTransform[6], int* nPixels, int* nLines, double maxError = 0.0);

  /**
   * Compute the reprojection of the input dataset.
   * The output dataset must have its projection initialized to the
   * desired result, as well as its raster dimensions.
   * Returns boolean indicating if the result is valid.
   */
  bool Reproject(GDALDataset* input, GDALDataset* output);

protected:
  vtkGDALRasterReprojection();
  ~vtkGDALRasterReprojection() override;

  double MaxError;
  int ResamplingAlgorithm;

private:
  vtkGDALRasterReprojection(const vtkGDALRasterReprojection&) = delete;
  void operator=(const vtkGDALRasterReprojection&) = delete;
};

#endif // vtkGDALRasterReprojection_h
