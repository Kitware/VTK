// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiNewickTreeReader
 * @brief   read multiple vtkTrees from Newick formatted file
 *
 * vtkMultiNewickTreeReader is a source object that reads Newick tree format
 * files.
 * The output of this reader is a single vtkMultiPieceDataSet that contains multiple vtkTree
 * objects. The superclass of this class, vtkDataReader, provides many methods for controlling the
 * reading of the data file, see vtkDataReader for more information.
 * @sa
 * vtkTree vtkDataReader
 */

#ifndef vtkMultiNewickTreeReader_h
#define vtkMultiNewickTreeReader_h

#include "vtkDataReader.h"
#include "vtkIOInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiPieceDataSet;
class vtkNewickTreeReader;
class VTKIOINFOVIS_EXPORT vtkMultiNewickTreeReader : public vtkDataReader
{
public:
  static vtkMultiNewickTreeReader* New();
  vtkTypeMacro(vtkMultiNewickTreeReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkMultiPieceDataSet* GetOutput();
  vtkMultiPieceDataSet* GetOutput(int idx);
  void SetOutput(vtkMultiPieceDataSet* output);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkMultiNewickTreeReader();
  ~vtkMultiNewickTreeReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkMultiNewickTreeReader(const vtkMultiNewickTreeReader&) = delete;
  void operator=(const vtkMultiNewickTreeReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
