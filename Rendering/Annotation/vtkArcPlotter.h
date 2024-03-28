// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArcPlotter
 * @brief   plot data along an arbitrary polyline
 *
 * vtkArcPlotter performs plotting of attribute data along polylines defined
 * with an input vtkPolyData data object. Any type of attribute data can be
 * plotted including scalars, vectors, tensors, normals, texture coordinates,
 * and field data. Either one or multiple data components can be plotted.
 *
 * To use this class you must specify an input data set that contains one or
 * more polylines, and some attribute data including which component of the
 * attribute data. (By default, this class processes the first component of
 * scalar data.) You will also need to set an offset radius (the distance
 * of the polyline to the median line of the plot), a width for the plot
 * (the distance that the minimum and maximum plot values are mapped into),
 * an possibly an offset (used to offset attribute data with multiple
 * components).
 *
 * Normally the filter automatically computes normals for generating the
 * offset arc plot. However, you can specify a default normal and use that
 * instead.
 *
 * @sa
 * vtkXYPlotActor
 */

#ifndef vtkArcPlotter_h
#define vtkArcPlotter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO

#define VTK_PLOT_SCALARS 1
#define VTK_PLOT_VECTORS 2
#define VTK_PLOT_NORMALS 3
#define VTK_PLOT_TCOORDS 4
#define VTK_PLOT_TENSORS 5
#define VTK_PLOT_FIELD_DATA 6

VTK_ABI_NAMESPACE_BEGIN
class vtkCamera;
class vtkDataArray;
class vtkPointData;
class vtkPoints;

class VTKRENDERINGANNOTATION_EXPORT VTK_MARSHALAUTO vtkArcPlotter : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate with no default camera and plot mode set to
   * VTK_SCALARS.
   */
  static vtkArcPlotter* New();

  vtkTypeMacro(vtkArcPlotter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify a camera used to orient the plot along the arc. If no camera
   * is specified, then the orientation of the plot is arbitrary.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera, vtkCamera);
  ///@}

  ///@{
  /**
   * Specify which data to plot: scalars, vectors, normals, texture coords,
   * tensors, or field data. If the data has more than one component, use
   * the method SetPlotComponent to control which component to plot.
   */
  vtkSetMacro(PlotMode, int);
  vtkGetMacro(PlotMode, int);
  void SetPlotModeToPlotScalars() { this->SetPlotMode(VTK_PLOT_SCALARS); }
  void SetPlotModeToPlotVectors() { this->SetPlotMode(VTK_PLOT_VECTORS); }
  void SetPlotModeToPlotNormals() { this->SetPlotMode(VTK_PLOT_NORMALS); }
  void SetPlotModeToPlotTCoords() { this->SetPlotMode(VTK_PLOT_TCOORDS); }
  void SetPlotModeToPlotTensors() { this->SetPlotMode(VTK_PLOT_TENSORS); }
  void SetPlotModeToPlotFieldData() { this->SetPlotMode(VTK_PLOT_FIELD_DATA); }
  ///@}

  ///@{
  /**
   * Set/Get the component number to plot if the data has more than one
   * component. If the value of the plot component is == (-1), then all
   * the components will be plotted.
   */
  vtkSetMacro(PlotComponent, int);
  vtkGetMacro(PlotComponent, int);
  ///@}

  ///@{
  /**
   * Set the radius of the "median" value of the first plotted component.
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Set the height of the plot. (The radius combined with the height
   * define the location of the plot relative to the generating polyline.)
   */
  vtkSetClampMacro(Height, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Height, double);
  ///@}

  ///@{
  /**
   * Specify an offset that translates each subsequent plot (if there is
   * more than one component plotted) from the defining arc (i.e., polyline).
   */
  vtkSetClampMacro(Offset, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Offset, double);
  ///@}

  ///@{
  /**
   * Set a boolean to control whether to use default normals.
   * By default, normals are automatically computed from the generating
   * polyline and camera.
   */
  vtkSetMacro(UseDefaultNormal, vtkTypeBool);
  vtkGetMacro(UseDefaultNormal, vtkTypeBool);
  vtkBooleanMacro(UseDefaultNormal, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the default normal to use if you do not wish automatic normal
   * calculation. The arc plot will be generated using this normal.
   */
  vtkSetVector3Macro(DefaultNormal, float);
  vtkGetVectorMacro(DefaultNormal, float, 3);
  ///@}

  ///@{
  /**
   * Set/Get the field data array to plot. This instance variable is
   * only applicable if field data is plotted.
   */
  vtkSetClampMacro(FieldDataArray, int, 0, VTK_INT_MAX);
  vtkGetMacro(FieldDataArray, int);
  ///@}

  /**
   * New GetMTime because of camera dependency.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkArcPlotter();
  ~vtkArcPlotter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkIdType OffsetPoint(vtkIdType ptId, vtkPoints* inPts, double n[3], vtkPoints* newPts,
    double offset, double* range, double val);
  int ProcessComponents(vtkIdType numPts, vtkPointData* pd);

  vtkCamera* Camera;
  int PlotMode;
  int PlotComponent;
  double Radius;
  double Height;
  double Offset;
  float DefaultNormal[3];
  vtkTypeBool UseDefaultNormal;
  int FieldDataArray;

private:
  vtkDataArray* Data;
  double* DataRange;
  double* Tuple;
  int NumberOfComponents;
  int ActiveComponent;
  int StartComp;
  int EndComp;

  vtkArcPlotter(const vtkArcPlotter&) = delete;
  void operator=(const vtkArcPlotter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
