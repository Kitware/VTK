// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractEdges
 * @brief   extract cell edges from any type of dataset
 *
 * vtkExtractEdges is a filter to extract edges from a dataset. Edges
 * are extracted as lines in an output vtkPolyData.
 *
 * There are two modes of extraction depending on the data member
 * UseAllPoints, If UseAllPoints is enabled, then the output points contain
 * all of the input points, and the point ids of the output lines (i.e.,
 * edges) remain unchanged from the input point numbering.  If UseAllPoints
 * is disabled (which is the default), then the numbering of the output points
 * may change, and any unused points are omitted from the filter output.
 *
 * @warning
 * If present in the filter input, output cell data is produced for the
 * output edges. Since an edge may be used by more than one cell, this is
 * potentially an undefined behavior. To ensure deterministic output, the
 * cell data from the cell with smallest cell id is copied to the output
 * edge.
 *
 * @sa
 * vtkFeatureEdges
 */

#ifndef vtkExtractEdges_h
#define vtkExtractEdges_h

#include "vtkFiltersCoreModule.h"       // For export macro
#include "vtkIncrementalPointLocator.h" // Support vtkSmartPointer<>
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkExtractEdges : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing the state of an instance.
   * default an instance of vtkMergePoints is used.
   */
  static vtkExtractEdges* New();
  vtkTypeMacro(vtkExtractEdges, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set / get a spatial locator for merging points. By default an instance
   * of vtkMergePoints is used.
   */
  vtkSetSmartPointerMacro(Locator, vtkIncrementalPointLocator);
  vtkGetSmartPointerMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  ///@{
  /**
   * Indicates whether all of the points of the input mesh should exist in
   * the output, i.e., whether point renumbering is permitted. By default,
   * UseAllPoints is disabled, so that unused points are omitted from the
   * output.
   */
  vtkSetMacro(UseAllPoints, bool);
  vtkGetMacro(UseAllPoints, bool);
  vtkBooleanMacro(UseAllPoints, bool);
  ///@}

  /**
   * Return the modified time also considering the locator since it may be
   * modified independent of this filter.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkExtractEdges();
  ~vtkExtractEdges() override = default;

  vtkSmartPointer<vtkIncrementalPointLocator> Locator;
  bool UseAllPoints;

  // Usual pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkExtractEdges(const vtkExtractEdges&) = delete;
  void operator=(const vtkExtractEdges&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
