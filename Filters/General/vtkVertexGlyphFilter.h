// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVertexGlyphFilter
 * @brief   Make a vtkPolyData with a vertex on each point.
 *
 *
 *
 * This filter throws away all of the cells in the input and replaces them with
 * a vertex on each point.  The intended use of this filter is roughly
 * equivalent to the vtkGlyph3D filter, except this filter is specifically for
 * data that has many vertices, making the rendered result faster and less
 * cluttered than the glyph filter. This filter may take a graph or point set
 * as input.
 *
 */

#ifndef vtkVertexGlyphFilter_h
#define vtkVertexGlyphFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkVertexGlyphFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkVertexGlyphFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVertexGlyphFilter* New();

protected:
  vtkVertexGlyphFilter();
  ~vtkVertexGlyphFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkVertexGlyphFilter(const vtkVertexGlyphFilter&) = delete;
  void operator=(const vtkVertexGlyphFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif //_vtkVertexGlyphFilter_h
