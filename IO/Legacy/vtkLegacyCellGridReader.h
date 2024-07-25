// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegacyCellGridReader
 * @brief   read vtk unstructured grid data file
 *
 * vtkLegacyCellGridReader is a source object that reads ASCII or binary
 * unstructured grid data files in vtk format. (see text for format details).
 * The output of this reader is a single vtkCellGrid data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkCellGrid vtkDataReader
 */

#ifndef vtkLegacyCellGridReader_h
#define vtkLegacyCellGridReader_h

#include "vtkCellGridReader.h" // For ivar
#include "vtkDataReader.h"
#include "vtkIOLegacyModule.h" // For export macro
#include "vtkNew.h"            // For ivar

VTK_ABI_NAMESPACE_BEGIN

class VTKIOLEGACY_EXPORT vtkLegacyCellGridReader : public vtkDataReader
{
public:
  static vtkLegacyCellGridReader* New();
  vtkTypeMacro(vtkLegacyCellGridReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkCellGrid* GetOutput();
  vtkCellGrid* GetOutput(int idx);
  void SetOutput(vtkCellGrid* output);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkLegacyCellGridReader();
  ~vtkLegacyCellGridReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;

  vtkNew<vtkCellGridReader> Subreader;

private:
  vtkLegacyCellGridReader(const vtkLegacyCellGridReader&) = delete;
  void operator=(const vtkLegacyCellGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
