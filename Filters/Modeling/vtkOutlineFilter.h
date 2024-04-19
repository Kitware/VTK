// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOutlineFilter
 * @brief   create wireframe outline for an arbitrary data set or composite dataset
 *
 * vtkOutlineFilter is a filter that generates a wireframe outline of any
 * dataset or composite dataset. An outline consists of the twelve edges of
 * the dataset bounding box. An option exists for generating faces instead of
 * a wireframe outline.
 *
 * @warning
 * When an input composite dataset is provided, options exist for producing
 * different styles of outline(s). Also, if the composite dataset has
 * non-geometric members (like tables) the result is unpredictable.
 *
 * @warning
 * Specialized versions of the outline filter are also available. For example
 * see vtkStructuredGridOutlineFilter, vtkRectilinearGridOutlineFilter, and
 * vtkImageDataOutlineFilter.
 */

#ifndef vtkOutlineFilter_h
#define vtkOutlineFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSMODELING_EXPORT vtkOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation. type information, and printing.
   */
  static vtkOutlineFilter* New();
  vtkTypeMacro(vtkOutlineFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Generate solid faces for the box. This is off by default.
   */
  vtkSetMacro(GenerateFaces, vtkTypeBool);
  vtkBooleanMacro(GenerateFaces, vtkTypeBool);
  vtkGetMacro(GenerateFaces, vtkTypeBool);
  ///@}

  enum CompositeOutlineStyle
  {
    ROOT_LEVEL = 0,
    LEAF_DATASETS = 1,
    ROOT_AND_LEAFS = 2,
    SPECIFIED_INDEX = 3
  };

  ///@{
  /**
   * Specify a style for creating bounding boxes around input composite
   * datasets. (If the filter input is a vtkDataSet type these options have
   * no effect.) There are four choices: 1) place a bounding box around the
   * root of the vtkCompositeDataSet (i.e., all of the data); 2) place
   * separate bounding boxes around each vtkDataSet leaf of the composite
   * dataset; 3) place a bounding box around the root and all dataset leaves;
   * and 4) place a bounding box around each (flat) index of the composite
   * dataset. The default behavior is both root and leafs.
   */
  vtkSetMacro(CompositeStyle, int);
  vtkGetMacro(CompositeStyle, int);
  void SetCompositeStyleToRoot() { this->SetCompositeStyle(ROOT_LEVEL); }
  void SetCompositeStyleToLeafs() { this->SetCompositeStyle(LEAF_DATASETS); }
  void SetCompositeStyleToRootAndLeafs() { this->SetCompositeStyle(ROOT_AND_LEAFS); }
  void SetCompositeStyleToSpecifiedIndex() { this->SetCompositeStyle(SPECIFIED_INDEX); }
  ///@}

  ///@{
  /**
   * If the composite style is set to SpecifiedIndex, then one or more flat
   * indices can be specified, and bounding boxes will be drawn around those
   * pieces of the composite dataset. (Recall that the flat index is a
   * non-negative integer, with root index=0, increasing in perorder
   * (depth-first) traversal order.
   */
  void AddIndex(unsigned int index);
  void RemoveIndex(unsigned int index);
  void RemoveAllIndices();
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkOutlineFilter();
  ~vtkOutlineFilter() override;

  vtkTypeBool GenerateFaces;
  int CompositeStyle;
  int OutputPointsPrecision;

  class vtkIndexSet;
  vtkIndexSet* Indices;

  void AppendOutline(vtkPoints* pts, vtkCellArray* lines, vtkCellArray* faces, double bds[6]);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkOutlineFilter(const vtkOutlineFilter&) = delete;
  void operator=(const vtkOutlineFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
