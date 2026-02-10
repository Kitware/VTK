// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkFeatureEdgesDispatcher
 * @brief Feature Edges filter that delegates to type specific implementations
 *
 * This is a meta filter that dispatches to the appropriate feature edges
 * implementation based on input type. It accepts both vtkPolyData and
 * vtkHyperTreeGrid as input.
 *
 * For vtkPolyData input, it delegates to vtkFeatureEdges.
 * For vtkHyperTreeGrid input, it delegates to vtkHyperTreeGridFeatureEdges.
 */

#ifndef vtkFeatureEdgesDispatcher_h
#define vtkFeatureEdgesDispatcher_h

#include "vtkFiltersHyperTreeModule.h" // needed for export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSHYPERTREE_EXPORT vtkFeatureEdgesDispatcher : public vtkPolyDataAlgorithm
{

public:
  static vtkFeatureEdgesDispatcher* New();
  vtkTypeMacro(vtkFeatureEdgesDispatcher, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off the extraction of boundary edges.
   * Default is true.
   */
  vtkSetMacro(BoundaryEdges, bool);
  vtkGetMacro(BoundaryEdges, bool);
  vtkBooleanMacro(BoundaryEdges, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the extraction of feature edges.
   * Default is true.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(FeatureEdges, bool);
  vtkGetMacro(FeatureEdges, bool);
  vtkBooleanMacro(FeatureEdges, bool);
  ///@}

  ///@{
  /**
   * Specify the feature angle for extracting feature edges.
   * Default is 30.0
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  vtkGetMacro(FeatureAngle, double);
  ///@}

  ///@{
  /**
   * Turn on/off the extraction of non-manifold edges.
   * Default is true.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(NonManifoldEdges, bool);
  vtkGetMacro(NonManifoldEdges, bool);
  vtkBooleanMacro(NonManifoldEdges, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the extraction of manifold edges. This typically
   * correspond to interior edges.
   * Default is false.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(ManifoldEdges, bool);
  vtkGetMacro(ManifoldEdges, bool);
  vtkBooleanMacro(ManifoldEdges, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the coloring of edges by type.
   * Default is false.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(Coloring, bool);
  vtkGetMacro(Coloring, bool);
  vtkBooleanMacro(Coloring, bool);
  ///@}

  ///@{
  /**
   * Turn on/off merging of coincident points using a locator.
   * Note that when merging is on, points with different point attributes
   * (e.g., normals) are merged, which may cause rendering artifacts.
   * Default is false.
   *
   * @note: Unused if input is NOT a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  ///@}

protected:
  vtkFeatureEdgesDispatcher() = default;
  ~vtkFeatureEdgesDispatcher() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkFeatureEdgesDispatcher(const vtkFeatureEdgesDispatcher&) = delete;
  void operator=(const vtkFeatureEdgesDispatcher&) = delete;

  // PolyData input options
  double FeatureAngle = 30.0;
  bool BoundaryEdges = true;
  bool FeatureEdges = true;
  bool NonManifoldEdges = true;
  bool ManifoldEdges = false;
  bool Coloring = false;

  // HTG options
  bool MergePoints = false;
};

VTK_ABI_NAMESPACE_END

#endif
