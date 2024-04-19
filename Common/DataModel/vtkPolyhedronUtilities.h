// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyhedronUtilities
 * @brief   vtkPolyhedron utilities
 *
 * This class contains specific methods used to process vtkPolyhedron.
 * These methods are intended to improve filters behavior over bad-shaped or degenerated
 * polyhedrons (for example, non-planar ones).
 *
 * @sa
 * vtkPolyhedron
 */

#ifndef vtkPolyhedronUtilities_h
#define vtkPolyhedronUtilities_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSetGet.h"       // For vtkTypeMacro
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkType.h"         // For vtkIdType

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkPointData;
class vtkPolyhedron;
class vtkUnstructuredGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkPolyhedronUtilities
{
public:
  vtkPolyhedronUtilities(vtkPolyhedronUtilities const&) = default;
  vtkPolyhedronUtilities& operator=(vtkPolyhedronUtilities const&) = default;

  /**
   * Decompose the input polyhedron into tetrahedrons.
   * This method will generate new points on each faces (faces barycenters) and another that is the
   * barycenter of the cell. These new points are used to create the tetrahedrons and will lead to
   * better result when applying filters (for example contours) on the output if the input
   * polyhedron contains concave faces. The user can give point data and cell data to be passed
   * through the decomposition. The point data on the new points (barycenters) correspond to the
   * mean value of the respective data on the faces points. The point data on the barycenter of the
   * cell correspond to the mean value of the respective data on all points. The cell data at given
   * cellId will simply be copied to each output tetrahedron.
   */
  static vtkSmartPointer<vtkUnstructuredGrid> Decompose(
    vtkPolyhedron* polyhedron, vtkPointData* inPd, vtkIdType cellId, vtkCellData* inCd);

private:
  vtkPolyhedronUtilities() = default;
  ~vtkPolyhedronUtilities() = default;
};

VTK_ABI_NAMESPACE_END
#endif
