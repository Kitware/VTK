// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkBoostDividedEdgeBundling
 * @brief   layout graph edges in directed edge bundles
 *
 *
 * Uses the technique by Selassie, Heller, and Heer to route graph edges into directed
 * bundles, with "lanes" for bundled edges moving in each direction. This technique
 * works best for networks whose vertices have been positioned already (geospatial
 * graphs, for example). Note that this scales to a few thousand edges in a reasonable
 * period of time (~1 minute). The time complexity comes mainly from the doubling
 * of edge control points each cycle and the complex set of forces between many pairs of
 * edge points.
 *
 * The algorithm depends on the Boost graph library for its implementation of all-pairs
 * shortest paths, needed here for determining connectivity compatibility.
 *
 * @par Thanks:
 * This algorithm was developed in the paper:
 *   David Selassie, Brandon Heller, Jeffrey Heer. Divided Edge Bundling for Directional
 *   Network Data. Proceedings of IEEE InfoVis 2011.
 */

#ifndef vtkBoostDividedEdgeBundling_h
#define vtkBoostDividedEdgeBundling_h

#include "vtkDirectedGraphAlgorithm.h"
#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostDividedEdgeBundling
  : public vtkDirectedGraphAlgorithm
{
public:
  static vtkBoostDividedEdgeBundling* New();

  vtkTypeMacro(vtkBoostDividedEdgeBundling, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkBoostDividedEdgeBundling();
  ~vtkBoostDividedEdgeBundling() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkBoostDividedEdgeBundling(const vtkBoostDividedEdgeBundling&) = delete;
  void operator=(const vtkBoostDividedEdgeBundling&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
