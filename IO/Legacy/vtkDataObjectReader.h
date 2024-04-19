// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataObjectReader
 * @brief   read vtk field data file
 *
 * vtkDataObjectReader is a source object that reads ASCII or binary field
 * data files in vtk format. Fields are general matrix structures used
 * represent complex data. (See text for format details).  The output of this
 * reader is a single vtkDataObject.  The superclass of this class,
 * vtkDataReader, provides many methods for controlling the reading of the
 * data file, see vtkDataReader for more information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkFieldData vtkDataObjectWriter
 */

#ifndef vtkDataObjectReader_h
#define vtkDataObjectReader_h

#include "vtkDataReader.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;

class VTKIOLEGACY_EXPORT vtkDataObjectReader : public vtkDataReader
{
public:
  static vtkDataObjectReader* New();
  vtkTypeMacro(vtkDataObjectReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output field of this reader.
   */
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int idx);
  void SetOutput(vtkDataObject*);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkDataObjectReader();
  ~vtkDataObjectReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkDataObjectReader(const vtkDataObjectReader&) = delete;
  void operator=(const vtkDataObjectReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
