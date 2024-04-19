// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkConvertToPolyhedra
 * @brief   convert 3D linear cells to vtkPolyhedra
 *
 * vtkConvertToPolyhedra is a filter that takes a vtkUnstructuredGrid as
 * input and produces a vtkUnstructuredGrid on output, converting 3D linear
 * cells such as tetrahedra, hexahedra, wedges, and pyramids into
 * vtkPolyhedron.
 *
 * @warning
 * Certain cells are skipped and not converted, this includes cells of dimension
 * two or less (e.g., triangles, quads, lines, verts, and so on); and higher
 * order cells that cannot easily be converted to vtkPolyhedra. (TODO: tessellate
 * high-order 3D cell faces and then use these to form the polyhedra.)
 *
 * @warning
 * This filter is typically used for testing. In general, processing linear cells
 * is preferable to processing polyhedra due to differences in speed of processing,
 * and memory requirements.
 *
 * @sa
 * vtkUnstructuredGrid vtkPolyhedron
 */

#ifndef vtkConvertToPolyhedra_h
#define vtkConvertToPolyhedra_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkConvertToPolyhedra : public vtkUnstructuredGridAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing the state of the object.
   */
  static vtkConvertToPolyhedra* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkConvertToPolyhedra, vtkUnstructuredGridAlgorithm);
  ///@}

  ///@{
  /**
   * Indicate whether to include non-polyhedral cells in the filter output.
   * Non-polyhedral cells are cells which cannot be converted to polyhedra,
   * for example cells of dimension < 3, and higher-order cells.
   * If enabled, the output will contain a mix of polyhedra and non-polyhedra
   * cells. If disabled, only polyhedra cells will be output. By default, this
   * is disabled.
   */
  vtkSetMacro(OutputAllCells, bool);
  vtkGetMacro(OutputAllCells, bool);
  vtkBooleanMacro(OutputAllCells, bool);
  ///@}

protected:
  vtkConvertToPolyhedra();
  ~vtkConvertToPolyhedra() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Control output of cells
  bool OutputAllCells;

private:
  vtkConvertToPolyhedra(const vtkConvertToPolyhedra&) = delete;
  void operator=(const vtkConvertToPolyhedra&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
