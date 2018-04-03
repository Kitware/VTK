/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRasterReprojectionFilter -
// .SECTION Description
// Applies map reprojection to vtkUniformGrid or vtkImageData.
// Internally uses GDAL/Proj4 for the reprojection calculations.

#ifndef __vtkRasterReprojectionFilter_h
#define __vtkRasterReprojectionFilter_h

#include "vtkImageAlgorithm.h"
#include "vtkmapgdal_export.h"

class VTKMAPGDAL_EXPORT vtkRasterReprojectionFilter : public vtkImageAlgorithm
{
public:
  static vtkRasterReprojectionFilter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkRasterReprojectionFilter, vtkImageAlgorithm);

  // Description:
  // Set the map-projection string for the input image data.
  // This should *only* be used for nonstandard image inputs,
  // when the MAP_PROJECTION is not embedded as field data.
  // Can be specified using any string formats supported by GDAL,
  // such as "well known text" (WKT) formats (GEOGS[]),
  // or shorter "user string" formats, such as EPSG:3857.
  vtkSetStringMacro(InputProjection);
  vtkGetStringMacro(InputProjection);

  // Description:
  // Set the map-projection string for the output image data.
  vtkSetStringMacro(OutputProjection);
  vtkGetStringMacro(OutputProjection);

  // Description:
  // Set the width and height of the output image.
  // It is recommended to leave this variable unset, in which case,
  // the filter will use the GDAL suggested dimensions to construct
  // the output image. This method can be used to override this, and
  // impose specific output image dimensions.
  vtkSetVector2Macro(OutputDimensions, int);
  vtkGetVector2Macro(OutputDimensions, int);

  // Description:
  // The data value to use internally to represent blank points in GDAL datasets.
  // By default, this will be set to the minimum value for the input data type.
  vtkSetMacro(NoDataValue, double);
  vtkGetMacro(NoDataValue, double);

  // Description:
  // Set the maximum error, measured in input pixels, that is allowed
  // in approximating the GDAL reprojection transformation.
  // The default is 0.0, for exact calculations.
  vtkSetClampMacro(MaxError, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaxError, double);

  // Description:
  // Set the pixel resampling algorithm. Choices range between 0 and 6:
  //   0 = Nearest Neighbor (default)
  //   1 = Bilinear
  //   2 = Cubic
  //   3 = CubicSpline
  //   4 = Lanczos
  //   5 = Average
  //   6 = Mode
  vtkSetClampMacro(ResamplingAlgorithm, int, 0, 6);

protected:
  vtkRasterReprojectionFilter();
  ~vtkRasterReprojectionFilter();

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestUpdateExtent(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  char* InputProjection;
  char* OutputProjection;
  int OutputDimensions[2];
  double NoDataValue;
  double MaxError;
  int ResamplingAlgorithm;

  class vtkRasterReprojectionFilterInternal;
  vtkRasterReprojectionFilterInternal* Internal;

private:
  // Not implemented:
  vtkRasterReprojectionFilter(const vtkRasterReprojectionFilter&);
  vtkRasterReprojectionFilter& operator=(const vtkRasterReprojectionFilter&);
};

#endif // __vtkRasterReprojectionFilter_h
