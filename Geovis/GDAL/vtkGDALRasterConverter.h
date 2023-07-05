// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkGDALRasterConverter
 * @brief Convert between VTK image representation and GDAL datasets
 *
 * vtkGDALRasterConverter is an internal implementation class used to convert
 * between VTK and GDAL data formats.
 *
 * @sa vtkRasterReprojectionFilter
 */

#ifndef vtkGDALRasterConverter_h
#define vtkGDALRasterConverter_h

#include "vtkGeovisGDALModule.h" // For export macro
#include "vtkObject.h"

// GDAL Forward declarations
class GDALDataset;

VTK_ABI_NAMESPACE_BEGIN
// VTK Forward declarations
class vtkImageData;
class vtkUniformGrid;

class VTKGEOVISGDAL_EXPORT vtkGDALRasterConverter : public vtkObject
{
public:
  static vtkGDALRasterConverter* New();
  vtkTypeMacro(vtkGDALRasterConverter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * No-data value for pixels in the source image
   * Default is NaN (not used).
   */
  vtkSetMacro(NoDataValue, double);
  vtkGetMacro(NoDataValue, double);
  ///@}

  /**
   * Create GDAL dataset in memory.
   * This dataset must be released by the calling code,
   * using GDALClose().
   */
  GDALDataset* CreateGDALDataset(int xDim, int yDim, int vtkDataType, int numberOfBands);

  /**
   * Create GDALDataset to match vtkImageData.
   * This dataset must be released by the calling code,
   * using GDALClose().
   */
  GDALDataset* CreateGDALDataset(vtkImageData* data, const char* mapProjection, int flipAxis[3]);

  /**
   * Copies color interpretation and color tables
   */
  void CopyBandInfo(GDALDataset* src, GDALDataset* dest);

  /**
   * Create vtkUniformGrid to match GDALDataset.
   * The calling code must call the Delete() method
   * to release the returned instance.
   */
  vtkUniformGrid* CreateVTKUniformGrid(GDALDataset* input);

  /**
   * Set projection on GDAL dataset, using any projection string
   * recognized by GDAL.
   */
  void SetGDALProjection(GDALDataset* dataset, const char* projectionString);

  /**
   * Set geo-transform on GDAL dataset.
   */
  void SetGDALGeoTransform(
    GDALDataset* dataset, double origin[2], double spacing[2], int flipAxis[2]);

  /**
   * Copies NoDataValue info from 1st to 2nd dataset
   */
  void CopyNoDataValues(GDALDataset* src, GDALDataset* dest);

  /**
   * Write GDALDataset to tiff file
   */
  void WriteTifFile(GDALDataset* dataset, VTK_FILEPATH const char* filename);

  /**
   * Traverse values in specified band to find min/max.
   * Note that the bandId starts at 1, not zero.
   * Returns boolean indicating success.
   */
  bool FindDataRange(GDALDataset* dataset, int bandId, double* minValue, double* maxValue);

protected:
  vtkGDALRasterConverter();
  ~vtkGDALRasterConverter() override;

  double NoDataValue;

  /**
   * Copies vtkImageData contents to GDALDataset
   * GDALDataset must be initialized to same dimensions as vtk image.
   */
  bool CopyToGDAL(vtkImageData* input, GDALDataset* output, int flipAxis[3]);

  class vtkGDALRasterConverterInternal;
  vtkGDALRasterConverterInternal* Internal;

private:
  vtkGDALRasterConverter(const vtkGDALRasterConverter&) = delete;
  void operator=(const vtkGDALRasterConverter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkGDALRasterConverter_h
