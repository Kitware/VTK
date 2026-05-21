// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotRadViz
 * @brief   Class for drawing a RadViz diagram.
 *
 *
 */

#ifndef vtkPlotRadViz_h
#define vtkPlotRadViz_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPen.h"              // For SetSmartPointerMacro
#include "vtkPlot.h"             // For the base class
#include "vtkTextProperty.h"     // For SetSmartPointerMacro
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkColorSeries;
class vtkDataObject;
class vtkDataSet;
class vtkFloatArray;
class vtkPoints2D;
class vtkTextProperty;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotRadViz : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotRadViz, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPlotRadViz* New();

  void SetInputData(vtkDataSet* dataset);
  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Set the dimensions of the pie, arguments 1 and 2 are the x and y coordinate
   * of the bottom corner. Arguments 3 and 4 are the width and height.
   */
  void SetDimensions(int arg1, int arg2, int arg3, int arg4);

  /**
   * Set the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  void SetDimensions(const int arg[4]);

  ///@{
  /**
   * Get the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  vtkGetVector4Macro(Dimensions, int);
  ///@}

  ///@{
  /**
   * Get the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  vtkSetSmartPointerMacro(VerticesPen, vtkPen);
  vtkGetSmartPointerMacro(VerticesPen, vtkPen);
  ///@}

  ///@{
  /**
   * Get the vtkTextProperty that governs how the vertices labels are displayed
   */
  vtkSetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  vtkGetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  ///@}

protected:
  vtkPlotRadViz();
  ~vtkPlotRadViz() override = default;
  void DrawMarkers(vtkContext2D* painter, vtkFloatArray* gbc, vtkPen* pen);
  void DrawLabels(vtkContext2D* painter, vtkFloatArray* vertices);
  void ComputeAutoLabels(vtkDataObject* input, vtkDataArray* array);
  vtkSmartPointer<vtkPoints2D> Transform(vtkFloatArray* points);

  int Dimensions[4];
  /**
   * This object stores the vtkPen that controls how the vertices is drawn.
   * These are the markers on the circle for the variables.
   */
  vtkSmartPointer<vtkPen> VerticesPen;
  /**
   * The text properties associated with the vertices labels
   */
  vtkSmartPointer<vtkTextProperty> LabelsProperties;

private:
  vtkPlotRadViz(const vtkPlotRadViz&) = delete;
  void operator=(const vtkPlotRadViz&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotRadViz_h
