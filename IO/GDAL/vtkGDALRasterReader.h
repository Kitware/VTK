/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGDALRasterReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGDALRasterReader
 * @brief   Read raster file formats using GDAL.
 *
 * vtkGDALRasterReader is a source object that reads raster files and uses
 * GDAL as the underlying library for the task. GDAL is required for this
 * reader. The output of the reader is a vtkUniformGrid instead of vtkImageData
 * to support blanking.
 *
 *
 * @sa
 * vtkUniformGrid, vtkImageData
*/

#ifndef vtkGDALRasterReader_h
#define vtkGDALRasterReader_h

#include <vtkImageReader2.h>
#include <vtkIOGDALModule.h> // For export macro

// C++ includes
#include <string> // string is required
#include <vector> // vector is required

class VTKIOGDAL_EXPORT vtkGDALRasterReader : public vtkImageReader2
{
public:
  static vtkGDALRasterReader* New();
  vtkTypeMacro(vtkGDALRasterReader, vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGDALRasterReader();
  virtual ~vtkGDALRasterReader();

  //@{
  /**
   * Set input file name
   */
  vtkSetStringMacro(FileName);
  // Get input file name
  vtkGetStringMacro(FileName);
  //@}

  /**
   * Return proj4 spatial reference
   */
  const char*  GetProjectionString() const;

  /**
   * Return geo-referenced corner points (Upper left,
   * lower left, lower right, upper right)
   */
  const double* GetGeoCornerPoints();

  //@{
  /**
   * Set desired width and height of the image
   */
  vtkSetVector2Macro(TargetDimensions, int);
  vtkGetVector2Macro(TargetDimensions, int);
  //@}

  //@{
  /**
   * Get raster width and heigth
   */
  vtkGetVector2Macro(RasterDimensions, int);
  //@}

  /**
   * Return metadata as reported by GDAL
   */
  const std::vector<std::string>& GetMetaData();

  /**
   * Return the invalid value for a pixel (for blanking purposes)
   */
  double GetInvalidValue();

  /**
   * Return domain metadata
   */
  std::vector<std::string> GetDomainMetaData(const std::string& domain);

  //@{
  /**
   * Return driver name which was used to read the current data
   */
  const std::string& GetDriverShortName();
  const std::string& GetDriverLongName();
  //@}

  vtkIdType GetNumberOfPoints();

protected:

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  virtual int FillOutputPortInformation(int port,
                                        vtkInformation* info);

protected:
  int TargetDimensions[2];
  int RasterDimensions[2];
  std::string Projection;
  std::string DomainMetaData;
  std::string DriverShortName;
  std::string DriverLongName;
  std::vector<std::string> Domains;
  std::vector<std::string> MetaData;

  class vtkGDALRasterReaderInternal;
  vtkGDALRasterReaderInternal* Implementation;

private:
  vtkGDALRasterReader(const vtkGDALRasterReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGDALRasterReader&) VTK_DELETE_FUNCTION;
};

#endif // vtkGDALRasterReader_h
