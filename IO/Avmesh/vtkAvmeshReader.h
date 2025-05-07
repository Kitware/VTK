// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAvmeshReader
 * @brief   Read an AVMESH file
 *
 * Read in an AVMESH file as a partitioned dataset collection.  Can optionally read only
 * surface (boundary) collections.
 *
 * AVMESH is the native unstructured mesh format for CREATE-AV Kestrel and
 * Helios.  Formal documentation of the format is included in avmeshlib,
 * which is available from https://github.com/DOD-HPCMP-CREATE/avmeshlib.
 * However, this reader parses AVMESH files without using avmeshlib.
 */

#ifndef vtkAvmeshReader_h
#define vtkAvmeshReader_h

#include "vtkIOAvmeshModule.h" // for export macro
#include <string>
#include <vtkPartitionedDataSetCollectionAlgorithm.h>

VTK_ABI_NAMESPACE_BEGIN
class VTKIOAVMESH_EXPORT vtkAvmeshReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkAvmeshReader* New();
  vtkTypeMacro(vtkAvmeshReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);

  int CanReadFile(VTK_FILEPATH const char* filename);

  vtkSetMacro(SurfaceOnly, bool);
  vtkGetMacro(SurfaceOnly, bool);
  vtkBooleanMacro(SurfaceOnly, bool);

  vtkSetMacro(BuildConnectivityIteratively, bool);
  vtkGetMacro(BuildConnectivityIteratively, bool);
  vtkBooleanMacro(BuildConnectivityIteratively, bool);

protected:
  vtkAvmeshReader();
  ~vtkAvmeshReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAvmeshReader(const vtkAvmeshReader&) = delete;
  void operator=(const vtkAvmeshReader&) = delete;

  std::string FileName;
  bool SurfaceOnly;
  bool BuildConnectivityIteratively;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAvmeshReader_h
