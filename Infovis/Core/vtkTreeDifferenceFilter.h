// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkTreeDifferenceFilter
 * @brief   compare two trees
 *
 *
 * vtkTreeDifferenceFilter compares two trees by analyzing a vtkDoubleArray.
 * Each tree must have a copy of this array.  A user of this filter should
 * call SetComparisonArrayName to specify the array that should be used as
 * the basis of coparison.  This array can either be part of the trees'
 * EdgeData or VertexData.
 *
 */

#ifndef vtkTreeDifferenceFilter_h
#define vtkTreeDifferenceFilter_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro

#include "vtkSmartPointer.h" // For ivars
#include <vector>            // For ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkTree;

class VTKINFOVISCORE_EXPORT vtkTreeDifferenceFilter : public vtkGraphAlgorithm
{
public:
  static vtkTreeDifferenceFilter* New();
  vtkTypeMacro(vtkTreeDifferenceFilter, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the identifier array in the trees' VertexData.
   * This array is used to find corresponding vertices in the two trees.
   * If this array name is not set, then we assume that the vertices in
   * the two trees to compare have corresponding vtkIdTypes.
   * Otherwise, the named array must be a vtkStringArray.
   * The identifier array does not necessarily have to specify a name for
   * each vertex in the tree.  If some vertices are unnamed, then this
   * filter will assign correspondence between ancestors of named vertices.
   */
  vtkSetStringMacro(IdArrayName);
  vtkGetStringMacro(IdArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the name of the array that we're comparing between the two trees.
   * The named array must be a vtkDoubleArray.
   */
  vtkSetStringMacro(ComparisonArrayName);
  vtkGetStringMacro(ComparisonArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the name of a new vtkDoubleArray that will contain the results of
   * the comparison between the two trees.  This new array will be added to
   * the input tree's VertexData or EdgeData, based on the value of
   * ComparisonArrayIsVertexData.  If this method is not called, the new
   * vtkDoubleArray will be named "difference" by default.
   */
  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);
  ///@}

  ///@{
  /**
   * Specify whether the comparison array is within the trees' vertex data or
   * not.  By default, we assume that the array to compare is within the trees'
   * EdgeData().
   */
  vtkSetMacro(ComparisonArrayIsVertexData, bool);
  vtkGetMacro(ComparisonArrayIsVertexData, bool);
  ///@}

protected:
  vtkTreeDifferenceFilter();
  ~vtkTreeDifferenceFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Populate VertexMap and EdgeMap with meaningful values.  These maps
   * allow us to look up the vtkIdType of a vertex or edge in tree #2,
   * given its vtkIdType in tree #1.
   */
  bool GenerateMapping(vtkTree* tree1, vtkTree* tree2);

  /**
   * Compute the differences between tree #1 and tree #2's copies of the
   * comparison array.
   */
  vtkSmartPointer<vtkDoubleArray> ComputeDifference(vtkTree* tree1, vtkTree* tree2);

  char* IdArrayName;
  char* ComparisonArrayName;
  char* OutputArrayName;
  bool ComparisonArrayIsVertexData;

  std::vector<vtkIdType> VertexMap;
  std::vector<vtkIdType> EdgeMap;

private:
  vtkTreeDifferenceFilter(const vtkTreeDifferenceFilter&) = delete;
  void operator=(const vtkTreeDifferenceFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
