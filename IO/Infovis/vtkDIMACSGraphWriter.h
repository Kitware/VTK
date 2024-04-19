// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkDIMACSGraphWriter
 * @brief   write vtkGraph data to a DIMACS
 * formatted file
 *
 *
 * vtkDIMACSGraphWriter is a sink object that writes
 * vtkGraph data files into a generic DIMACS (.gr) format.
 *
 * Output files contain a problem statement line:
 *
 * p graph \em num_verts \em num_edges
 *
 * Followed by |E| edge descriptor lines that are formatted as:
 *
 * e \em source \em target \em weight
 *
 * Vertices are numbered from 1..n in DIMACS formatted files.
 *
 * See webpage for format details.
 * http://prolland.free.fr/works/research/dsat/dimacs.html
 *
 * @sa
 * vtkDIMACSGraphReader
 *
 */

#ifndef vtkDIMACSGraphWriter_h
#define vtkDIMACSGraphWriter_h

#include "vtkDataWriter.h"
#include "vtkIOInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;

class VTKIOINFOVIS_EXPORT vtkDIMACSGraphWriter : public vtkDataWriter
{
public:
  static vtkDIMACSGraphWriter* New();
  vtkTypeMacro(vtkDIMACSGraphWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkGraph* GetInput();
  vtkGraph* GetInput(int port);
  ///@}

protected:
  vtkDIMACSGraphWriter() = default;
  ~vtkDIMACSGraphWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkDIMACSGraphWriter(const vtkDIMACSGraphWriter&) = delete;
  void operator=(const vtkDIMACSGraphWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
