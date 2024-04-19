// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkChacoGraphReader
 * @brief   Reads chaco graph files.
 *
 *
 * vtkChacoGraphReader reads in files in the Chaco format into a vtkGraph.
 * An example is the following
 * <code>
 * 10 13
 * 2 6 10
 * 1 3
 * 2 4 8
 * 3 5
 * 4 6 10
 * 1 5 7
 * 6 8
 * 3 7 9
 * 8 10
 * 1 5 9
 * </code>
 * The first line specifies the number of vertices and edges
 * in the graph. Each additional line contains the vertices adjacent
 * to a particular vertex.  In this example, vertex 1 is adjacent to
 * 2, 6 and 10, vertex 2 is adjacent to 1 and 3, etc.  Since Chaco ids
 * start at 1 and VTK ids start at 0, the vertex ids in the vtkGraph
 * will be 1 less than the Chaco ids.
 */

#ifndef vtkChacoGraphReader_h
#define vtkChacoGraphReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkUndirectedGraphAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOINFOVIS_EXPORT vtkChacoGraphReader : public vtkUndirectedGraphAlgorithm
{
public:
  static vtkChacoGraphReader* New();
  vtkTypeMacro(vtkChacoGraphReader, vtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The Chaco file name.
   */
  vtkGetFilePathMacro(FileName);
  vtkSetFilePathMacro(FileName);
  ///@}

protected:
  vtkChacoGraphReader();
  ~vtkChacoGraphReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  char* FileName;

  vtkChacoGraphReader(const vtkChacoGraphReader&) = delete;
  void operator=(const vtkChacoGraphReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkChacoGraphReader_h
