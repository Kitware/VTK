// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkBoostBrandesCentrality
 * @brief   Compute Brandes betweenness centrality
 * on a vtkGraph
 *
 *
 *
 * This vtk class uses the Boost brandes_betweeness_centrality
 * generic algorithm to compute betweenness centrality on
 * the input graph (a vtkGraph).
 *
 * @sa
 * vtkGraph vtkBoostGraphAdapter
 */

#ifndef vtkBoostBrandesCentrality_h
#define vtkBoostBrandesCentrality_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkVariant.h"                           // For variant type

#include "vtkGraphAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostBrandesCentrality : public vtkGraphAlgorithm
{
public:
  static vtkBoostBrandesCentrality* New();
  vtkTypeMacro(vtkBoostBrandesCentrality, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the flag that sets the rule whether or not to use the
   * edge weight array as set using \c SetEdgeWeightArrayName.
   */
  vtkSetMacro(UseEdgeWeightArray, bool);
  vtkBooleanMacro(UseEdgeWeightArray, bool);
  ///@}

  vtkSetMacro(InvertEdgeWeightArray, bool);
  vtkBooleanMacro(InvertEdgeWeightArray, bool);

  ///@{
  /**
   * Get/Set the name of the array that needs to be used as the edge weight.
   * The array should be a vtkDataArray.
   */
  vtkGetStringMacro(EdgeWeightArrayName);
  vtkSetStringMacro(EdgeWeightArrayName);
  ///@}

protected:
  vtkBoostBrandesCentrality();
  ~vtkBoostBrandesCentrality() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  bool UseEdgeWeightArray;
  bool InvertEdgeWeightArray;
  char* EdgeWeightArrayName;

  vtkBoostBrandesCentrality(const vtkBoostBrandesCentrality&) = delete;
  void operator=(const vtkBoostBrandesCentrality&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
