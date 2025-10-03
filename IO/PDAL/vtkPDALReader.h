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
 * Supports applying LAS header offsets and provide access to that offset as a string
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
   * Whether the reader detected an offset in the file, set during RequestInformation pass.
   */
  vtkGetMacro(HasOffset, vtkTypeBool);

  /**
   * Get a string representation of the point cloud offsets. set during RequestInformation pass.
   */
  vtkGetMacro(OffsetAsString, std::string);

  ///@{
  /**
   * Name of the file that will be opened
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Whether to apply an automatic offset to the point coordinates
   * if provided in the file metadata, default is false.
   */
  vtkSetMacro(ApplyOffset, bool);
  vtkGetMacro(ApplyOffset, bool);
  vtkBooleanMacro(ApplyOffset, bool);
  ///@}

protected:
  vtkPDALReader();
  ~vtkPDALReader() override;

  /**
   * Provide metadata (offsets) for preview in the pipeline
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Core implementation of the data set reader
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(pdal::Stage& reader, vtkPolyData* pointsPolyData);

  /**
   * Get LAS file offsets from PDAL metadata
   * returns 0,0,0 if offsets are not available
   */
  std::array<double, 3> GetLasOffsets(pdal::Stage* reader);

  char* FileName = nullptr;
  bool ApplyOffset = false;
  bool HasOffset = false;
  std::string OffsetAsString;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPDALReader_h
