// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkApplyColors
 * @brief   apply colors to a data set.
 *
 *
 * vtkApplyColors performs a coloring of the dataset using default colors,
 * lookup tables, annotations, and/or a selection. The output is a
 * four-component vtkUnsignedCharArray containing RGBA tuples for each
 * element in the dataset. The first input is the dataset to be colored, which
 * may be a vtkTable, vtkGraph subclass, or vtkDataSet subclass. The API
 * of this algorithm refers to "points" and "cells". For vtkGraph, the
 * "points" refer to the graph vertices and "cells" refer to graph edges.
 * For vtkTable, "points" refer to table rows. For vtkDataSet subclasses, the
 * meaning is obvious.
 *
 * The second (optional) input is a vtkAnnotationLayers object, which stores
 * a list of annotation layers, with each layer holding a list of
 * vtkAnnotation objects. The annotation specifies a subset of data along with
 * other properties, including color. For annotations with color properties,
 * this algorithm will use the color to color elements, using a "top one wins"
 * strategy.
 *
 * The third (optional) input is a vtkSelection object, meant for specifying
 * the current selection. You can control the color of the selection.
 *
 * The algorithm takes two input arrays, specified with
 * SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name)
 * and
 * SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, name).
 * These set the point and cell data arrays to use to color the data with
 * the associated lookup table. For vtkGraph, vtkTable inputs, you would use
 * FIELD_ASSOCIATION_VERTICES, FIELD_ASSOCIATION_EDGES, or
 * FIELD_ASSOCIATION_ROWS as appropriate.
 *
 * To use the color array generated here, you should do the following:
 *
 *  mapper->SetScalarModeToUseCellFieldData();
 *  mapper->SelectColorArray("vtkApplyColors color");
 *  mapper->SetScalarVisibility(true);
 *
 * Colors are assigned with the following priorities:
 * <ol>
 * <li> If an item is part of the selection, it is colored with that color.
 * <li> Otherwise, if the item is part of an annotation, it is colored
 *      with the color of the final (top) annotation in the set of layers.
 * <li> Otherwise, if the lookup table is used, it is colored using the
 *      lookup table color for the data value of the element.
 * <li> Otherwise it will be colored with the default color.
 * </ol>
 *
 * Note: The opacity of an unselected item is defined by the multiplication
 * of default opacity, lookup table opacity, and annotation opacity, where
 * opacity is taken as a number from 0 to 1. So items will never be more opaque
 * than any of these three opacities. Selected items are always given the
 * selection opacity directly.
 */

#ifndef vtkApplyColors_h
#define vtkApplyColors_h

#include "vtkPassInputTypeAlgorithm.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTKVIEWSINFOVIS_EXPORT vtkApplyColors : public vtkPassInputTypeAlgorithm
{
public:
  static vtkApplyColors* New();
  vtkTypeMacro(vtkApplyColors, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The lookup table to use for point colors. This is only used if
   * input array 0 is set and UsePointLookupTable is on.
   */
  virtual void SetPointLookupTable(vtkScalarsToColors* lut);
  vtkGetObjectMacro(PointLookupTable, vtkScalarsToColors);
  ///@}

  ///@{
  /**
   * If on, uses the point lookup table to set the colors of unannotated,
   * unselected elements of the data.
   */
  vtkSetMacro(UsePointLookupTable, bool);
  vtkGetMacro(UsePointLookupTable, bool);
  vtkBooleanMacro(UsePointLookupTable, bool);
  ///@}

  ///@{
  /**
   * If on, uses the range of the data to scale the lookup table range.
   * Otherwise, uses the range defined in the lookup table.
   */
  vtkSetMacro(ScalePointLookupTable, bool);
  vtkGetMacro(ScalePointLookupTable, bool);
  vtkBooleanMacro(ScalePointLookupTable, bool);
  ///@}

  ///@{
  /**
   * The default point color for all unannotated, unselected elements
   * of the data. This is used if UsePointLookupTable is off.
   */
  vtkSetVector3Macro(DefaultPointColor, double);
  vtkGetVector3Macro(DefaultPointColor, double);
  ///@}

  ///@{
  /**
   * The default point opacity for all unannotated, unselected elements
   * of the data. This is used if UsePointLookupTable is off.
   */
  vtkSetMacro(DefaultPointOpacity, double);
  vtkGetMacro(DefaultPointOpacity, double);
  ///@}

  ///@{
  /**
   * The point color for all selected elements of the data.
   * This is used if the selection input is available.
   */
  vtkSetVector3Macro(SelectedPointColor, double);
  vtkGetVector3Macro(SelectedPointColor, double);
  ///@}

  ///@{
  /**
   * The point opacity for all selected elements of the data.
   * This is used if the selection input is available.
   */
  vtkSetMacro(SelectedPointOpacity, double);
  vtkGetMacro(SelectedPointOpacity, double);
  ///@}

  ///@{
  /**
   * The output array name for the point color RGBA array.
   * Default is "vtkApplyColors color".
   */
  vtkSetStringMacro(PointColorOutputArrayName);
  vtkGetStringMacro(PointColorOutputArrayName);
  ///@}

  ///@{
  /**
   * The lookup table to use for cell colors. This is only used if
   * input array 1 is set and UseCellLookupTable is on.
   */
  virtual void SetCellLookupTable(vtkScalarsToColors* lut);
  vtkGetObjectMacro(CellLookupTable, vtkScalarsToColors);
  ///@}

  ///@{
  /**
   * If on, uses the cell lookup table to set the colors of unannotated,
   * unselected elements of the data.
   */
  vtkSetMacro(UseCellLookupTable, bool);
  vtkGetMacro(UseCellLookupTable, bool);
  vtkBooleanMacro(UseCellLookupTable, bool);
  ///@}

  ///@{
  /**
   * If on, uses the range of the data to scale the lookup table range.
   * Otherwise, uses the range defined in the lookup table.
   */
  vtkSetMacro(ScaleCellLookupTable, bool);
  vtkGetMacro(ScaleCellLookupTable, bool);
  vtkBooleanMacro(ScaleCellLookupTable, bool);
  ///@}

  ///@{
  /**
   * The default cell color for all unannotated, unselected elements
   * of the data. This is used if UseCellLookupTable is off.
   */
  vtkSetVector3Macro(DefaultCellColor, double);
  vtkGetVector3Macro(DefaultCellColor, double);
  ///@}

  ///@{
  /**
   * The default cell opacity for all unannotated, unselected elements
   * of the data. This is used if UseCellLookupTable is off.
   */
  vtkSetMacro(DefaultCellOpacity, double);
  vtkGetMacro(DefaultCellOpacity, double);
  ///@}

  ///@{
  /**
   * The cell color for all selected elements of the data.
   * This is used if the selection input is available.
   */
  vtkSetVector3Macro(SelectedCellColor, double);
  vtkGetVector3Macro(SelectedCellColor, double);
  ///@}

  ///@{
  /**
   * The cell opacity for all selected elements of the data.
   * This is used if the selection input is available.
   */
  vtkSetMacro(SelectedCellOpacity, double);
  vtkGetMacro(SelectedCellOpacity, double);
  ///@}

  ///@{
  /**
   * The output array name for the cell color RGBA array.
   * Default is "vtkApplyColors color".
   */
  vtkSetStringMacro(CellColorOutputArrayName);
  vtkGetStringMacro(CellColorOutputArrayName);
  ///@}

  ///@{
  /**
   * Use the annotation to color the current annotation
   * (i.e. the current selection). Otherwise use the selection
   * color attributes of this filter.
   */
  vtkSetMacro(UseCurrentAnnotationColor, bool);
  vtkGetMacro(UseCurrentAnnotationColor, bool);
  vtkBooleanMacro(UseCurrentAnnotationColor, bool);
  ///@}

  /**
   * Retrieve the modified time for this filter.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkApplyColors();
  ~vtkApplyColors() override;

  /**
   * Convert the vtkGraph into vtkPolyData.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to vtkGraph.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  void ProcessColorArray(vtkUnsignedCharArray* colorArr, vtkScalarsToColors* lut,
    vtkAbstractArray* arr, unsigned char color[4], bool scale);

  vtkScalarsToColors* PointLookupTable;
  vtkScalarsToColors* CellLookupTable;
  double DefaultPointColor[3];
  double DefaultPointOpacity;
  double DefaultCellColor[3];
  double DefaultCellOpacity;
  double SelectedPointColor[3];
  double SelectedPointOpacity;
  double SelectedCellColor[3];
  double SelectedCellOpacity;
  bool ScalePointLookupTable;
  bool ScaleCellLookupTable;
  bool UsePointLookupTable;
  bool UseCellLookupTable;
  char* PointColorOutputArrayName;
  char* CellColorOutputArrayName;
  bool UseCurrentAnnotationColor;

private:
  vtkApplyColors(const vtkApplyColors&) = delete;
  void operator=(const vtkApplyColors&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
