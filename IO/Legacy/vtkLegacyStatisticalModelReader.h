// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegacyStatisticalModelReader
 * @brief   read vtk unstructured grid data file
 *
 * vtkLegacyStatisticalModelReader is a source object that reads ASCII or binary
 * statistical model data files in VTK format (see text for format details).
 * The output of this reader is a single vtkStatisticalModel data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkStatisticalModel vtkDataReader
 */

#ifndef vtkLegacyStatisticalModelReader_h
#define vtkLegacyStatisticalModelReader_h

#include "vtkDataReader.h"
#include "vtkIOLegacyModule.h" // For export macro
#include "vtkNew.h"            // For ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkStatisticalModel;

class VTKIOLEGACY_EXPORT vtkLegacyStatisticalModelReader : public vtkDataReader
{
public:
  static vtkLegacyStatisticalModelReader* New();
  vtkTypeMacro(vtkLegacyStatisticalModelReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkStatisticalModel* GetOutput();
  vtkStatisticalModel* GetOutput(int idx);
  void SetOutput(vtkStatisticalModel* output);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkLegacyStatisticalModelReader();
  ~vtkLegacyStatisticalModelReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkLegacyStatisticalModelReader(const vtkLegacyStatisticalModelReader&) = delete;
  void operator=(const vtkLegacyStatisticalModelReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
