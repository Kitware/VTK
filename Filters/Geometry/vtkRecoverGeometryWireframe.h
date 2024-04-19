// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkRecoverGeometryWireframe
 * @brief   Get corrected wireframe from tessellated facets
 *
 * This filter create an edge mask that is used at render time to ignore the
 * rendering of specific edges in wireframe mode. For that it checks a cell attribute
 * so that each adjacent cells having the same attribute value will not display an
 * edge between them.
 *
 * The main usage of this filter is at the output of vtkDataSetSurfaceFilter or
 * vtkGeometryFilter, when we are subdividing non-linear cells but we still want to
 * visualize the edges of the original cells. In this case the cell attribute
 * will usually be the original cell id values.
 *
 * @warning As the edge flag mechanism does not allow to specify a single edge
 * from a point the filter might duplicate some points, so topology
 * is not preserved.
 *
 * @sa
 * vtkDataSetSurfaceFilter
 */

#ifndef vtkRecoverGeometryWireframe_h
#define vtkRecoverGeometryWireframe_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include <string> // std::string

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRY_EXPORT vtkRecoverGeometryWireframe : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkRecoverGeometryWireframe, vtkPolyDataAlgorithm);
  static vtkRecoverGeometryWireframe* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the cell attribute name that will be used to discriminate edges that
   * should be kept from edges that shouldn't. This array should be a vtkIdType
   * array.
   *
   * Default is empty.
   */
  vtkSetMacro(CellIdsAttribute, std::string);
  vtkGetMacro(CellIdsAttribute, std::string);
  ///@}

protected:
  vtkRecoverGeometryWireframe();
  ~vtkRecoverGeometryWireframe() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkRecoverGeometryWireframe(const vtkRecoverGeometryWireframe&) = delete;
  void operator=(const vtkRecoverGeometryWireframe&) = delete;

  std::string CellIdsAttribute;
};

VTK_ABI_NAMESPACE_END
#endif // vtkRecoverGeometryWireframe_h
