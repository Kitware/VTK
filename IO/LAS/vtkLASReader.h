// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLASRasterReader
 * @brief   Reads LIDAR data saved using the LAS file format.
 *
 * vtkLASReader is a source object that reads LIDAR data saved using
 * the LAS file format. This reader uses the libLAS library.
 * It produces a vtkPolyData with point data arrays:
 * "intensity": vtkUnsignedShortArray
 * "classification": vtkUnsignedCharArray (optional)
 * "color": vtkUnsignedShortArray (optional)
 *
 *
 * @sa
 * vtkPolyData
 */

#ifndef vtkLASReader_h
#define vtkLASReader_h

#include <vtkIOLASModule.h> // For export macro

#include <vtkPolyDataAlgorithm.h>

namespace liblas
{
class Header;
class Reader;
};

VTK_ABI_NAMESPACE_BEGIN

class VTKIOLAS_EXPORT vtkLASReader : public vtkPolyDataAlgorithm
{
public:
  vtkLASReader(const vtkLASReader&) = delete;
  void operator=(const vtkLASReader&) = delete;
  static vtkLASReader* New();
  vtkTypeMacro(vtkLASReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Accessor for name of the file that will be opened
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);

protected:
  vtkLASReader();
  ~vtkLASReader() override;

  /**
   * Core implementation of the data set reader
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(liblas::Reader& reader, vtkPolyData* pointsPolyData);

  char* FileName;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLASReader_h
