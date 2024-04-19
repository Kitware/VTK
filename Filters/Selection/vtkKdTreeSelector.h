// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkKdTreeSelector
 * @brief   Selects point ids using a kd-tree.
 *
 *
 * If SetKdTree is used, the filter ignores the input and selects based on that
 * kd-tree.  If SetKdTree is not used, the filter builds a kd-tree using the
 * input point set and uses that tree for selection.  The output is a
 * vtkSelection containing the ids found in the kd-tree using the specified
 * bounds.
 */

#ifndef vtkKdTreeSelector_h
#define vtkKdTreeSelector_h

#include "vtkFiltersSelectionModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkKdTree;

class VTKFILTERSSELECTION_EXPORT vtkKdTreeSelector : public vtkSelectionAlgorithm
{
public:
  static vtkKdTreeSelector* New();
  vtkTypeMacro(vtkKdTreeSelector, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The kd-tree to use to find selected ids.
   * The kd-tree must be initialized with the desired set of points.
   * When this is set, the optional input is ignored.
   */
  void SetKdTree(vtkKdTree* tree);
  vtkGetObjectMacro(KdTree, vtkKdTree);
  ///@}

  ///@{
  /**
   * The bounds of the form (xmin,xmax,ymin,ymax,zmin,zmax).
   * To perform a search in 2D, use the bounds
   * (xmin,xmax,ymin,ymax,VTK_DOUBLE_MIN,VTK_DOUBLE_MAX).
   */
  vtkSetVector6Macro(SelectionBounds, double);
  vtkGetVector6Macro(SelectionBounds, double);
  ///@}

  ///@{
  /**
   * The field name to use when generating the selection.
   * If set, creates a VALUES selection.
   * If not set (or is set to nullptr), creates a INDICES selection.
   * By default this is not set.
   */
  vtkSetStringMacro(SelectionFieldName);
  vtkGetStringMacro(SelectionFieldName);
  ///@}

  ///@{
  /**
   * The field attribute to use when generating the selection.
   * If set, creates a PEDIGREEIDS or GLOBALIDS selection.
   * If not set (or is set to -1), creates a INDICES selection.
   * By default this is not set.
   * NOTE: This should be set a constant in vtkDataSetAttributes,
   * not vtkSelection.
   */
  vtkSetMacro(SelectionAttribute, int);
  vtkGetMacro(SelectionAttribute, int);
  ///@}

  ///@{
  /**
   * Whether to only allow up to one value in the result.
   * The item selected is closest to the center of the bounds,
   * if there are any points within the selection threshold.
   * Default is off.
   */
  vtkSetMacro(SingleSelection, bool);
  vtkGetMacro(SingleSelection, bool);
  vtkBooleanMacro(SingleSelection, bool);
  ///@}

  ///@{
  /**
   * The threshold for the single selection.
   * A single point is added to the selection if it is within
   * this threshold from the bounds center.
   * Default is 1.
   */
  vtkSetMacro(SingleSelectionThreshold, double);
  vtkGetMacro(SingleSelectionThreshold, double);
  ///@}

  vtkMTimeType GetMTime() override;

protected:
  vtkKdTreeSelector();
  ~vtkKdTreeSelector() override;

  vtkKdTree* KdTree;
  double SelectionBounds[6];
  char* SelectionFieldName;
  bool BuildKdTreeFromInput;
  bool SingleSelection;
  double SingleSelectionThreshold;
  int SelectionAttribute;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkKdTreeSelector(const vtkKdTreeSelector&) = delete;
  void operator=(const vtkKdTreeSelector&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
