// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOrientPolyData
 * @brief   Auto detect correct normal orientation and/or enforce consistent polygon ordering.
 *
 * vtkOrientPolyData is a filter that orients the normals of a polygonal mesh,
 * and/or enforces consistent polygon ordering. It is also possible to globally
 * flip the normal orientation.
 *
 * @sa
 * vtkPolyDataNormals vtkSplitPolyData
 */

#ifndef vtkOrientPolyData_h
#define vtkOrientPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;

class VTKFILTERSCORE_EXPORT vtkOrientPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkOrientPolyData* New();
  vtkTypeMacro(vtkOrientPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off the enforcement of consistent polygon ordering.
   *
   * The default is on.
   */
  vtkSetMacro(Consistency, bool);
  vtkGetMacro(Consistency, bool);
  vtkBooleanMacro(Consistency, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the automatic determination of correct normal
   * orientation. NOTE: This assumes a completely closed surface
   * (i.e. no boundary edges) and no non-manifold edges. If these
   * constraints do not hold, all bets are off. This option adds some
   * computational complexity, and is useful if you don't want to have
   * to inspect the rendered image to determine whether to turn on the
   * FlipNormals flag. However, this flag can work with the FlipNormals
   * flag, and if both are set, all the normals in the output will
   * point "inward".
   *
   * The default is off.
   */
  vtkSetMacro(AutoOrientNormals, bool);
  vtkGetMacro(AutoOrientNormals, bool);
  vtkBooleanMacro(AutoOrientNormals, bool);
  ///@}

  ///@{
  /**
   * Turn on/off traversal across non-manifold edges. This will prevent
   * problems where the consistency of polygonal ordering is corrupted due
   * to topological loops.
   *
   * The default is off.
   */
  vtkSetMacro(NonManifoldTraversal, bool);
  vtkGetMacro(NonManifoldTraversal, bool);
  vtkBooleanMacro(NonManifoldTraversal, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the global flipping of normal orientation. Flipping
   * reverves the meaning of front and back for Frontface and Backface
   * culling in vtkProperty.  Flipping modifies both the normal
   * direction and the order of a cell's points.
   *
   * The default is off.
   */
  vtkSetMacro(FlipNormals, bool);
  vtkGetMacro(FlipNormals, bool);
  vtkBooleanMacro(FlipNormals, bool);
  ///@}

protected:
  vtkOrientPolyData();
  ~vtkOrientPolyData() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool Consistency;
  bool AutoOrientNormals;
  bool NonManifoldTraversal;
  bool FlipNormals;

private:
  vtkOrientPolyData(const vtkOrientPolyData&) = delete;
  void operator=(const vtkOrientPolyData&) = delete;

  void TraverseAndOrder(vtkPolyData* input, vtkPolyData* output, vtkIdList* wave, vtkIdList* wave2,
    vtkIdList* cellPointIds, vtkIdList* cellIds, vtkIdList* neighborPointIds,
    std::vector<char>& visited, vtkIdType& numFlips);
};
VTK_ABI_NAMESPACE_END

#endif // vtkOrientPolyData_h
