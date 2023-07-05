// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGraphReader
 * @brief   read vtkGraph data file
 *
 * vtkGraphReader is a source object that reads ASCII or binary
 * vtkGraph data files in vtk format. (see text for format details).
 * The output of this reader is a single vtkGraph data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkGraph vtkDataReader vtkGraphWriter
 */

#ifndef vtkGraphReader_h
#define vtkGraphReader_h

#include "vtkDataReader.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;

class VTKIOLEGACY_EXPORT vtkGraphReader : public vtkDataReader
{
public:
  static vtkGraphReader* New();
  vtkTypeMacro(vtkGraphReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkGraph* GetOutput();
  vtkGraph* GetOutput(int idx);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkGraphReader();
  ~vtkGraphReader() override;

  enum GraphType
  {
    UnknownGraph,
    DirectedGraph,
    UndirectedGraph,
    Molecule
  };

  vtkDataObject* CreateOutput(vtkDataObject* currentOutput) override;

  // Read beginning of file to determine whether the graph is directed.
  virtual int ReadGraphType(const char* fname, GraphType& type);

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkGraphReader(const vtkGraphReader&) = delete;
  void operator=(const vtkGraphReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
