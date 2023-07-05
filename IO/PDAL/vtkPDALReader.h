// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPDALReader
 * @brief   Reads LIDAR data using the PDAL library.
 *
 * vtkPDALReader reads LIDAR data using the PDAL library.  See the
 * readers section on www.pdal.io for the supported formats. It produces a
 * vtkPolyData with point data arrays for attributes such as Intensity,
 * Classification, Color, ...
 *
 *
 * @sa
 * vtkPolyData
 */

#ifndef vtkPDALReader_h
#define vtkPDALReader_h

#include <vtkIOPDALModule.h> // For export macro

#include <vtkPolyDataAlgorithm.h>

namespace pdal
{
class Stage;
};

VTK_ABI_NAMESPACE_BEGIN

class VTKIOPDAL_EXPORT vtkPDALReader : public vtkPolyDataAlgorithm
{
public:
  vtkPDALReader(const vtkPDALReader&) = delete;
  void operator=(const vtkPDALReader&) = delete;
  static vtkPDALReader* New();
  vtkTypeMacro(vtkPDALReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Name of the file that will be opened
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);

protected:
  vtkPDALReader();
  ~vtkPDALReader() override;

  /**
   * Core implementation of the data set reader
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(pdal::Stage& reader, vtkPolyData* pointsPolyData);

  char* FileName;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPDALReader_h
