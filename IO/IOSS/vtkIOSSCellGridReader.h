// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkIOSSCellGridReader
 * @brief Reader for IOSS (Sierra IO System) that produces cell-grid data.
 *
 * This reader is a subclass of vtkIOSSReader that produces vtkCellGrid
 * objects instead of vtkUnstructuredGrid objects inside each partition
 * of its output partitioned-dataset collection.
 *
 * This reader ignores some of its base-class settings:
 * + RemoveUnusedPoints
 * + MergeExodusEntityBlocks
 *
 * @sa
 * vtkIOSSReader
 */

#ifndef vtkIOSSCellGridReader_h
#define vtkIOSSCellGridReader_h

#include "vtkIOSSReader.h"
#include "vtkNew.h" // for vtkNew

VTK_ABI_NAMESPACE_BEGIN

class VTKIOIOSS_EXPORT vtkIOSSCellGridReader : public vtkIOSSReader
{
public:
  static vtkIOSSCellGridReader* New();
  vtkTypeMacro(vtkIOSSCellGridReader, vtkIOSSReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implementation for vtkReaderAlgorithm API
   */
  int ReadMetaData(vtkInformation* metadata) override;
  int ReadMesh(int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) override;
  int ReadPoints(int, int, int, int, vtkDataObject*) override { return 1; }
  int ReadArrays(int, int, int, int, vtkDataObject*) override { return 1; }
  ///@}

protected:
  vtkIOSSCellGridReader();
  ~vtkIOSSCellGridReader() override;

  // int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkIOSSCellGridReader(const vtkIOSSCellGridReader&) = delete;
  void operator=(const vtkIOSSCellGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
