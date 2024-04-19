// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNewickTreeReader
 * @brief   read vtkTree from Newick formatted file
 *
 * vtkNewickTreeReader is a source object that reads Newick tree format
 * files.
 * The output of this reader is a single vtkTree data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @par Thanks:
 * This class is adapted from code originally written by Yu-Wei Wu.
 * @sa
 * vtkTree vtkDataReader
 */

#ifndef vtkNewickTreeReader_h
#define vtkNewickTreeReader_h

#include "vtkDataReader.h"
#include "vtkIOInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkTree;

class VTKIOINFOVIS_EXPORT vtkNewickTreeReader : public vtkDataReader
{
public:
  static vtkNewickTreeReader* New();
  vtkTypeMacro(vtkNewickTreeReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkTree* GetOutput();
  vtkTree* GetOutput(int idx);
  void SetOutput(vtkTree* output);
  int ReadNewickTree(const char* buffer, vtkTree& tree);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkNewickTreeReader();
  ~vtkNewickTreeReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;
  void CountNodes(const char* buffer, vtkIdType* numNodes);
  vtkIdType BuildTree(char* buffer, vtkMutableDirectedGraph* g, vtkDoubleArray* weights,
    vtkStringArray* names, vtkIdType parent);

private:
  vtkNewickTreeReader(const vtkNewickTreeReader&) = delete;
  void operator=(const vtkNewickTreeReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
