// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredGridReader
 * @brief   read vtk structured grid data file
 *
 * vtkStructuredGridReader is a source object that reads ASCII or binary
 * structured grid data files in vtk format. (see text for format details).
 * The output of this reader is a single vtkStructuredGrid data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkStructuredGrid vtkDataReader
 */

#ifndef vtkStructuredGridReader_h
#define vtkStructuredGridReader_h

#include "vtkDataReader.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkStructuredGrid;

class VTKIOLEGACY_EXPORT vtkStructuredGridReader : public vtkDataReader
{
public:
  static vtkStructuredGridReader* New();
  vtkTypeMacro(vtkStructuredGridReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkStructuredGrid* GetOutput();
  vtkStructuredGrid* GetOutput(int idx);
  void SetOutput(vtkStructuredGrid* output);
  ///@}

  /**
   * Read the meta information from the file (WHOLE_EXTENT).
   */
  int ReadMetaDataSimple(VTK_FILEPATH const std::string& fname, vtkInformation* metadata) override;

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkStructuredGridReader();
  ~vtkStructuredGridReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkStructuredGridReader(const vtkStructuredGridReader&) = delete;
  void operator=(const vtkStructuredGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
