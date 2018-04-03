/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGDALRasterConverter -
// .SECTION Description
// Converts between VTK image representations and GDAL datasets.
// This class is intended to be used internally

#ifndef __vtkGDALRasterConverter_h
#define __vtkGDALRasterConverter_h

#include "vtkObject.h"
#include "vtkmapgdal_export.h"
class vtkImageData;
class vtkUniformGrid;
class GDALDataset;

class VTKMAPGDAL_EXPORT vtkGDALRasterConverter : public vtkObject
{
public:
  static vtkGDALRasterConverter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkGDALRasterConverter, vtkObject);

  // Description
  // No-data value for pixels in the source image
  // Default is NaN (not used).
  vtkSetMacro(NoDataValue, double);
  vtkGetMacro(NoDataValue, double);

  // Description:
  // Create GDAL dataset in memory.
  // This dataset must be released by the calling code,
  // using GDALClose().
  GDALDataset* CreateGDALDataset(
    int xDim, int yDim, int vtkDataType, int numberOfBands);

  // Description:
  // Create GDALDataset to match vtkImageData.
  // This dataset must be released by the calling code,
  // using GDALClose().
  GDALDataset* CreateGDALDataset(vtkImageData* data, const char* mapProjection);

  // Description:
  // Copies color interpretation and color tables
  void CopyBandInfo(GDALDataset* src, GDALDataset* dest);

  // Description
  // Create vtkUniformGrid to match GDALDataset.
  // The calling code must call the Delete() method
  // to release the returned instance.
  vtkUniformGrid* CreateVTKUniformGrid(GDALDataset* input);

  // Description:
  // Set projection on GDAL dataset, using any projection string
  // recognized by GDAL.
  void SetGDALProjection(GDALDataset* dataset, const char* projectionString);

  // Description:
  // Set geo-transform on GDAL dataset.
  void SetGDALGeoTransform(
    GDALDataset* dataset, double origin[2], double spacing[2]);

  // Description:
  // Copies NoDataValue info from 1st to 2nd dataset
  void CopyNoDataValues(GDALDataset* src, GDALDataset* dest);

  // Description:
  // Write GDALDataset to tiff file
  void WriteTifFile(GDALDataset* dataset, const char* filename);

  // Description:
  // Traverse values in specified band to find min/max.
  // Note that the bandId starts at 1, not zero.
  // Returns boolean indicating success.
  bool FindDataRange(
    GDALDataset* dataset, int bandId, double* minValue, double* maxValue);

protected:
  vtkGDALRasterConverter();
  ~vtkGDALRasterConverter();

  double NoDataValue;

  // Description
  // Copies vtkImageData contents to GDALDataset
  // GDALDataset must be initialized to same dimensions as vtk image.
  bool CopyToGDAL(vtkImageData* input, GDALDataset* output);

  class vtkGDALRasterConverterInternal;
  vtkGDALRasterConverterInternal* Internal;

private:
  // Not implemented:
  vtkGDALRasterConverter(const vtkGDALRasterConverter&);
  vtkGDALRasterConverter& operator=(const vtkGDALRasterConverter&);
};

#endif // __vtkGDALRasterConverter_h
