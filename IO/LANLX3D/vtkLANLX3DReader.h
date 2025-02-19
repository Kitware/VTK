// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2021, Los Alamos National Laboratory
// SPDX-FileCopyrightText: Copyright (c) 2021. Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
/**
 *
 * @class vtkLANLX3DReader
 * @brief class for reading LANL X3D format files
 *
 * @section caveats Caveats
 * The LANL X3D file format is not to be confused with the X3D file format that
 * is the successor to VRML. The LANL X3D format is designed to store geometry
 * for LANL physics codes.
 *
 * @par Thanks:
 * Developed by Jonathan Woodering at Los Alamos National Laboratory
 */

#ifndef vtkLANLX3DReader_h
#define vtkLANLX3DReader_h

#include "vtkIOLANLX3DModule.h" // for export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkMultiBlockDataSet;

VTK_ABI_NAMESPACE_BEGIN
class VTKIOLANLX3D_EXPORT vtkLANLX3DReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkLANLX3DReader* New();
  vtkTypeMacro(vtkLANLX3DReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetMacro(ReadAllPieces, bool);
  vtkGetMacro(ReadAllPieces, bool);

protected:
  vtkLANLX3DReader();
  ~vtkLANLX3DReader() override;

  char* FileName = nullptr;
  bool ReadAllPieces = true;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkLANLX3DReader(const vtkLANLX3DReader&) = delete;
  void operator=(const vtkLANLX3DReader&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
