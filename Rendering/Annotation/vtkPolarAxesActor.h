// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolarAxesActor
 * @brief   create an actor of a polar axes -
 *
 *
 * vtkPolarAxesActor is a composite actor that draws polar axes in a
 * specified plane for a give pole.
 * Currently the plane has to be the xy plane.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2011.
 * This work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France.
 *
 * @sa
 * vtkActor vtkAxisActor vtkPolarAxesActor
 */

#ifndef vtkPolarAxesActor_h
#define vtkPolarAxesActor_h

#include "vtkActor.h"
#include "vtkAxisActor.h"                 // access to enum values
#include "vtkNew.h"                       // used for vtkNew
#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkSmartPointer.h"              // used for vtkSmartPointer
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO
#include <list>                           // To process exponent list as reference
#include <string>                         // used for ivar
#include <vector>                         // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkCamera;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;
class vtkStringArray;
class vtkTextProperty;

class VTKRENDERINGANNOTATION_EXPORT VTK_MARSHALAUTO vtkPolarAxesActor : public vtkActor
{
public:
  vtkTypeMacro(vtkPolarAxesActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate object with label format "6.3g" and the number of labels
   * per axis set to 3.
   */
  static vtkPolarAxesActor* New();

  ///@{
  /**
   * Draw the polar axes
   */
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override { return 0; }
  ///@}

  ///@{
  /**
   * Explicitly specify the coordinate of the pole.
   */
  virtual void SetPole(double[3]);
  virtual void SetPole(double, double, double);
  vtkGetVector3Macro(Pole, double);
  ///@}

  ///@{
  /**
   * Enable/Disable log scale.
   * Default: false.
   */
  vtkSetMacro(Log, bool);
  vtkGetMacro(Log, bool);
  vtkBooleanMacro(Log, bool);
  ///@}

  ///@{
  /**
   * Gets/Sets the number of radial axes.
   * Default: 0.
   */
  vtkSetClampMacro(RequestedNumberOfRadialAxes, vtkIdType, 0, VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES);
  vtkGetMacro(RequestedNumberOfRadialAxes, vtkIdType);
  ///@}

  ///@{
  /**
   * Gets/Sets the number of polar axes.
   * Default: 5.
   */
  vtkSetClampMacro(RequestedNumberOfPolarAxes, vtkIdType, 0, VTK_MAXIMUM_NUMBER_OF_POLAR_AXES);
  vtkGetMacro(RequestedNumberOfPolarAxes, vtkIdType);
  ///@}

  ///@{
  /**
   * Define the range values displayed on the polar Axis.
   * Default: (0, 10).
   */
  vtkSetVector2Macro(Range, double);
  vtkGetVectorMacro(Range, double, 2);
  ///@}

  ///@{
  /**
   * Set/Get the minimal radius of the polar coordinates.
   * Default: 0.
   */
  virtual void SetMinimumRadius(double);
  vtkGetMacro(MinimumRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the maximum radius of the polar coordinates.
   * Default: 1.
   */
  virtual void SetMaximumRadius(double);
  vtkGetMacro(MaximumRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the minimum radius of the polar coordinates (in degrees).
   * Default: 0.
   */
  virtual void SetMinimumAngle(double);
  vtkGetMacro(MinimumAngle, double);
  ///@}

  ///@{
  /**
   * Set/Get the maximum radius of the polar coordinates (in degrees).
   * Default: 90.
   */
  virtual void SetMaximumAngle(double);
  vtkGetMacro(MaximumAngle, double);
  ///@}

  ///@{
  /**
   * Set/Get the minimum radial angle distinguishable from polar axis.
   * NB: This is used only when polar axis is visible.
   * Default: 0.5.
   */
  vtkSetClampMacro(SmallestVisiblePolarAngle, double, 0., 5.);
  vtkGetMacro(SmallestVisiblePolarAngle, double);
  ///@}

  ///@{
  /**
   * Set/Get the location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   * Default: VTK_TICKS_BOTH.
   */
  vtkSetClampMacro(TickLocation, int, vtkAxisActor::VTK_TICKS_INSIDE, vtkAxisActor::VTK_TICKS_BOTH);
  vtkGetMacro(TickLocation, int);
  ///@}

  ///@{
  /**
   * Default: true
   */
  vtkSetMacro(RadialUnits, bool);
  vtkGetMacro(RadialUnits, bool);
  ///@}

  ///@{
  /**
   * Explicitly specify the screen size of title and label text.
   * ScreenSize determines the size of the text in terms of screen
   * pixels.
   * Default: 10.0.
   */
  vtkSetMacro(ScreenSize, double);
  vtkGetMacro(ScreenSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the polar title offset.
   * X-component is used only if text is not aligned to center.
   * Default: (20, 10).
   */
  vtkSetVector2Macro(PolarTitleOffset, double);
  vtkGetVectorMacro(PolarTitleOffset, double, 2);
  ///@}

  ///@{
  /**
   * Set/Get the radial title offset.
   * X-component is used only if text is not aligned to center.
   * Default: (20, 0).
   */
  vtkSetVector2Macro(RadialTitleOffset, double);
  vtkGetVectorMacro(RadialTitleOffset, double, 2);
  ///@}

  ///@{
  /**
   * Set/Get the polar label Y-offset.
   * Default: 10.
   */
  vtkGetMacro(PolarLabelOffset, double);
  vtkSetMacro(PolarLabelOffset, double);
  ///@}

  ///@{
  /**
   * Set/Get the polar exponent Y-offset.
   * Default: 5.
   */
  vtkGetMacro(PolarExponentOffset, double);
  vtkSetMacro(PolarExponentOffset, double);
  ///@}

  ///@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * vtkPolarAxesActor.
   */
  virtual void SetCamera(vtkCamera*);
  vtkCamera* GetCamera();
  ///@}

  ///@{
  /**
   * Set/Get the labels for the polar axis.
   * Default: "Radial Distance".
   */
  vtkGetMacro(PolarAxisTitle, std::string);
  vtkSetMacro(PolarAxisTitle, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the format with which to print the polar axis labels.
   */
  vtkSetStringMacro(PolarLabelFormat);
  vtkGetStringMacro(PolarLabelFormat);
  ///@}

  enum ExponentLocation
  {
    VTK_EXPONENT_BOTTOM = 0,
    VTK_EXPONENT_EXTERN = 1,
    VTK_EXPONENT_LABELS = 2
  };

  ///@{
  /**
   * Get/Set the location of the exponent (if any) of the polar axis values.
   * Possible location: VTK_EXPONENT_BOTTOM, VTK_EXPONENT_EXTERN,
   * VTK_EXPONENT_LABELS
   */
  vtkSetClampMacro(ExponentLocation, int, VTK_EXPONENT_BOTTOM, VTK_EXPONENT_LABELS);
  vtkGetMacro(ExponentLocation, int);
  ///@}

  ///@{
  /**
   * String to format angle values displayed on the radial axes.
   */
  vtkSetStringMacro(RadialAngleFormat);
  vtkGetStringMacro(RadialAngleFormat);
  ///@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  ///@{
  /**
   * Enable and disable the use of distance based LOD for titles and labels.
   * Default: true.
   */
  vtkSetMacro(EnableDistanceLOD, bool);
  vtkGetMacro(EnableDistanceLOD, bool);
  ///@}

  ///@{
  /**
   * Set distance LOD threshold [0.0 - 1.0] for titles and labels.
   * Default: 0.7.
   */
  vtkSetClampMacro(DistanceLODThreshold, double, 0.0, 1.0);
  vtkGetMacro(DistanceLODThreshold, double);
  ///@}

  ///@{
  /**
   * Enable and disable the use of view angle based LOD for titles and labels.
   * Default: true.
   */
  vtkSetMacro(EnableViewAngleLOD, bool);
  vtkGetMacro(EnableViewAngleLOD, bool);
  ///@}

  ///@{
  /**
   * Set view angle LOD threshold [0.0 - 1.0] for titles and labels.
   * Default: 0.3.
   */
  vtkSetClampMacro(ViewAngleLODThreshold, double, 0., 1.);
  vtkGetMacro(ViewAngleLODThreshold, double);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of the polar axis.
   * Default: true.
   */
  vtkSetMacro(PolarAxisVisibility, bool);
  vtkGetMacro(PolarAxisVisibility, bool);
  vtkBooleanMacro(PolarAxisVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of inner radial grid lines
   * Default: true.
   */
  vtkSetMacro(DrawRadialGridlines, bool);
  vtkGetMacro(DrawRadialGridlines, bool);
  vtkBooleanMacro(DrawRadialGridlines, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of inner polar arcs grid lines
   * Default: true.
   */
  vtkSetMacro(DrawPolarArcsGridlines, bool);
  vtkGetMacro(DrawPolarArcsGridlines, bool);
  vtkBooleanMacro(DrawPolarArcsGridlines, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of titles for polar axis.
   * Default: true.
   */
  vtkSetMacro(PolarTitleVisibility, bool);
  vtkGetMacro(PolarTitleVisibility, bool);
  vtkBooleanMacro(PolarTitleVisibility, bool);
  ///@}

  enum TitleLocation
  {
    VTK_TITLE_BOTTOM = 0,
    VTK_TITLE_EXTERN = 1
  };

  ///@{
  /**
   * Get/Set the alignment of the radial axes title related to the axis.
   * Possible Alignment: VTK_TITLE_BOTTOM, VTK_TITLE_EXTERN.
   * Default: VTK_TITLE_BOTTOM.
   */
  vtkSetClampMacro(RadialAxisTitleLocation, int, VTK_TITLE_BOTTOM, VTK_TITLE_EXTERN);
  vtkGetMacro(RadialAxisTitleLocation, int);
  ///@}

  ///@{
  /**
   * Get/Set the alignment of the polar axes title related to the axis.
   * Possible Alignment: VTKTITLE_BOTTOM, VTK_TITLE_EXTERN.
   * Default: VTK_TITLE_BOTTOM.
   */
  vtkSetClampMacro(PolarAxisTitleLocation, int, VTK_TITLE_BOTTOM, VTK_TITLE_EXTERN);
  vtkGetMacro(PolarAxisTitleLocation, int);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of labels for polar axis.
   * Default: true.
   */
  vtkSetMacro(PolarLabelVisibility, bool);
  vtkGetMacro(PolarLabelVisibility, bool);
  vtkBooleanMacro(PolarLabelVisibility, bool);
  ///@}

  ///@{
  /**
   * If On, the ticks are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngle Major/Minor. if Off, the start angle is
   * 0.0, i.e. the angle on the major radius of the ellipse.
   * Default: true.
   */
  vtkSetMacro(ArcTicksOriginToPolarAxis, bool);
  vtkGetMacro(ArcTicksOriginToPolarAxis, bool);
  vtkBooleanMacro(ArcTicksOriginToPolarAxis, bool);
  ///@}

  ///@{
  /**
   * If On, the radial axes are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngleRadialAxes. if Off, the start angle is
   * 0.0, i.e. the angle on the major radius of the ellipse.
   * Default: true.
   */
  vtkSetMacro(RadialAxesOriginToPolarAxis, bool);
  vtkGetMacro(RadialAxesOriginToPolarAxis, bool);
  vtkBooleanMacro(RadialAxesOriginToPolarAxis, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the overall visibility of ticks.
   * Default: true.
   */
  vtkSetMacro(PolarTickVisibility, bool);
  vtkGetMacro(PolarTickVisibility, bool);
  vtkBooleanMacro(PolarTickVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of major ticks on polar axis and last radial axis.
   * Default: true.
   */
  vtkSetMacro(AxisTickVisibility, bool);
  vtkGetMacro(AxisTickVisibility, bool);
  vtkBooleanMacro(AxisTickVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of minor ticks on polar axis and last radial axis.
   * Default: false.
   */
  vtkSetMacro(AxisMinorTickVisibility, bool);
  vtkGetMacro(AxisMinorTickVisibility, bool);
  vtkBooleanMacro(AxisMinorTickVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the use of polar axes range for axis major ticks.
   * Default: true.
   */
  vtkSetMacro(AxisTickMatchesPolarAxes, bool);
  vtkGetMacro(AxisTickMatchesPolarAxes, bool);
  vtkBooleanMacro(AxisTickMatchesPolarAxes, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of major ticks on the last arc.
   * Default: true.
   */
  vtkSetMacro(ArcTickVisibility, bool);
  vtkGetMacro(ArcTickVisibility, bool);
  vtkBooleanMacro(ArcTickVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of minor ticks on the last arc.
   * Default: false.
   */
  vtkSetMacro(ArcMinorTickVisibility, bool);
  vtkGetMacro(ArcMinorTickVisibility, bool);
  vtkBooleanMacro(ArcMinorTickVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the use of radial axes angle for arc major ticks.
   * Default: true.
   */
  vtkSetMacro(ArcTickMatchesRadialAxes, bool);
  vtkGetMacro(ArcTickMatchesRadialAxes, bool);
  vtkBooleanMacro(ArcTickMatchesRadialAxes, bool);
  ///@}

  ///@{
  /**
   * Set/Get the size of the major ticks on the last arc.
   * If set to 0, compute it as a ratio of maximum radius.
   * Default 0.
   */
  vtkSetMacro(ArcMajorTickSize, double);
  vtkGetMacro(ArcMajorTickSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the size of the major ticks on the polar axis.
   * If set to 0, compute it as a ratio of maximum radius.
   * Default 0.
   */
  vtkSetMacro(PolarAxisMajorTickSize, double);
  vtkGetMacro(PolarAxisMajorTickSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the size of the major ticks on the last radial axis.
   * If set to 0, compute it as a ratio of maximum radius.
   * Default 0.
   */
  vtkSetMacro(LastRadialAxisMajorTickSize, double);
  vtkGetMacro(LastRadialAxisMajorTickSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between major and minor Polar Axis ticks size.
   * Default: 0.3.
   */
  vtkSetMacro(PolarAxisTickRatioSize, double);
  vtkGetMacro(PolarAxisTickRatioSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between major and minor Last Radial axis ticks size.
   * Default: 0.3.
   */
  vtkSetMacro(LastAxisTickRatioSize, double);
  vtkGetMacro(LastAxisTickRatioSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between major and minor Arc ticks size.
   * Default: 0.3.
   */
  vtkSetMacro(ArcTickRatioSize, double);
  vtkGetMacro(ArcTickRatioSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between maximum radius and major ticks size.
   * Default: 0.02.
   */
  vtkSetMacro(TickRatioRadiusSize, double);
  vtkGetMacro(TickRatioRadiusSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the size of the thickness of polar axis ticks.
   * Default: 1.
   */
  vtkSetMacro(PolarAxisMajorTickThickness, double);
  vtkGetMacro(PolarAxisMajorTickThickness, double);
  ///@}

  ///@{
  /**
   * Set/Get the size of the thickness of last radial axis ticks.
   * Default: 1.
   */
  vtkSetMacro(LastRadialAxisMajorTickThickness, double);
  vtkGetMacro(LastRadialAxisMajorTickThickness, double);
  ///@}

  ///@{
  /**
   * Set/Get the size of the thickness of the last arc ticks.
   * Default: 1.
   */
  vtkSetMacro(ArcMajorTickThickness, double);
  vtkGetMacro(ArcMajorTickThickness, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between major and minor Polar Axis ticks thickness.
   * Default: 0.5.
   */
  vtkSetMacro(PolarAxisTickRatioThickness, double);
  vtkGetMacro(PolarAxisTickRatioThickness, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between major and minor Last Radial axis ticks thickness.
   * Default: 0.5.
   */
  vtkSetMacro(LastAxisTickRatioThickness, double);
  vtkGetMacro(LastAxisTickRatioThickness, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between major and minor Arc ticks thickness.
   * Default: 0.5.
   */
  vtkSetMacro(ArcTickRatioThickness, double);
  vtkGetMacro(ArcTickRatioThickness, double);
  ///@}

  ///@{
  /**
   * Set/Get the range between 2 major ticks (values displayed on the axis).
   * Default: 1.
   */
  vtkSetMacro(DeltaRangeMajor, double);
  vtkGetMacro(DeltaRangeMajor, double);
  ///@}

  ///@{
  /**
   * Set/Get the range between 2 minor ticks.
   * Default: 0.5.
   */
  vtkSetMacro(DeltaRangeMinor, double);
  vtkGetMacro(DeltaRangeMinor, double);
  ///@}

  ///@{
  /**
   * Set/Get requested delta range for polar axes.
   * If set to 0, compute it depending on count.
   * Default: 0.
   */
  vtkSetMacro(RequestedDeltaRangePolarAxes, double);
  vtkGetMacro(RequestedDeltaRangePolarAxes, double);
  ///@}

  ///@{
  /**
   * Set/Get the angle between 2 major ticks on the last arc.
   * Default: 45.
   */
  vtkSetMacro(DeltaAngleMajor, double);
  vtkGetMacro(DeltaAngleMajor, double);
  ///@}

  ///@{
  /**
   * Set/Get the angle between 2 minor ticks on the last arc.
   * Default: 22.5.
   */
  vtkSetMacro(DeltaAngleMinor, double);
  vtkGetMacro(DeltaAngleMinor, double);
  ///@}

  ///@{
  /**
   * Set/Get requested delta angle for radial axes.
   * If set to 0, compute it depending on count.
   * Default: 45.
   */
  vtkSetMacro(RequestedDeltaAngleRadialAxes, double);
  vtkGetMacro(RequestedDeltaAngleRadialAxes, double);
  ///@}

  //------------------------------------------------

  ///@{
  /**
   * Turn on and off the visibility of non-polar radial axes.
   * Default: true.
   */
  vtkSetMacro(RadialAxesVisibility, bool);
  vtkGetMacro(RadialAxesVisibility, bool);
  vtkBooleanMacro(RadialAxesVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of titles for non-polar radial axes.
   * Default: true.
   */
  vtkSetMacro(RadialTitleVisibility, bool);
  vtkGetMacro(RadialTitleVisibility, bool);
  vtkBooleanMacro(RadialTitleVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of arcs for polar axis.
   * Default: true.
   */
  vtkSetMacro(PolarArcsVisibility, bool);
  vtkGetMacro(PolarArcsVisibility, bool);
  vtkBooleanMacro(PolarArcsVisibility, bool);
  ///@}

  ///@{
  /**
   * Enable/Disable labels 2D mode (always facing the camera).
   */
  void SetUse2DMode(bool enable);
  bool GetUse2DMode();
  ///@}

  ///@{
  /**
   * Set/Get the polar axis title text property.
   */
  virtual void SetPolarAxisTitleTextProperty(vtkTextProperty* p);
  vtkTextProperty* GetPolarAxisTitleTextProperty();
  ///@}

  ///@{
  /**
   * Set/Get the polar axis labels text property.
   */
  virtual void SetPolarAxisLabelTextProperty(vtkTextProperty* p);
  vtkTextProperty* GetPolarAxisLabelTextProperty();
  ///@}

  ///@{
  /**
   * Set/Get the last radial axis text property.
   */
  virtual void SetLastRadialAxisTextProperty(vtkTextProperty* p);
  vtkTextProperty* GetLastRadialAxisTextProperty();
  ///@}

  ///@{
  /**
   * Set/Get the secondary radial axes text property.
   */
  virtual void SetSecondaryRadialAxesTextProperty(vtkTextProperty* p);
  vtkTextProperty* GetSecondaryRadialAxesTextProperty();
  ///@}

  ///@{
  /**
   * Get/Set polar axis actor properties.
   */
  virtual void SetPolarAxisProperty(vtkProperty*);
  vtkProperty* GetPolarAxisProperty();
  ///@}

  ///@{
  /**
   * Get/Set last radial axis actor properties.
   */
  virtual void SetLastRadialAxisProperty(vtkProperty* p);
  vtkProperty* GetLastRadialAxisProperty();
  ///@}

  ///@{
  /**
   * Get/Set secondary radial axes actors properties.
   */
  virtual void SetSecondaryRadialAxesProperty(vtkProperty* p);
  vtkProperty* GetSecondaryRadialAxesProperty();
  ///@}

  ///@{
  /**
   * Get/Set principal polar arc actor property.
   */
  virtual void SetPolarArcsProperty(vtkProperty* p);
  vtkProperty* GetPolarArcsProperty();
  ///@}

  ///@{
  /**
   * Get/Set secondary polar arcs actors property.
   */
  virtual void SetSecondaryPolarArcsProperty(vtkProperty* p);
  vtkProperty* GetSecondaryPolarArcsProperty();
  ///@}

  ///@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds are used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   * Default: (-1, 1, -1, 1, -1, 1).
   */
  vtkSetVector6Macro(Bounds, double);
  double* GetBounds() override;
  void GetBounds(
    double& xmin, double& xmax, double& ymin, double& ymax, double& zmin, double& zmax);
  void GetBounds(double bounds[6]);
  ///@}

  ///@{
  /**
   * Ratio.
   * Default: 1.
   */
  vtkSetClampMacro(Ratio, double, 0.001, 100.0);
  vtkGetMacro(Ratio, double);
  ///@}

  ///@{
  /**
   * Set/Get the number of line per degree to draw polar arc.
   * Default: 0.2.
   */
  vtkSetClampMacro(PolarArcResolutionPerDegree, double, VTK_MINIMUM_POLAR_ARC_RESOLUTION_PER_DEG,
    VTK_MAXIMUM_POLAR_ARC_RESOLUTION_PER_DEG);
  vtkGetMacro(PolarArcResolutionPerDegree, double);
  ///@}

protected:
  vtkPolarAxesActor();
  ~vtkPolarAxesActor() override;

  /**
   * Check consistency of vtkPolarAxesActor members.
   */
  bool CheckMembersConsistency();

  /**
   * Build the axes.
   * Determine coordinates, position, etc.
   */
  void BuildAxes(vtkViewport*);

  /**
   * Calculate bounds based on maximum radius and angular sector
   */
  void CalculateBounds();

  /**
   * Send attributes which are common to all axes, both polar and radial
   */
  void SetCommonAxisAttributes(vtkAxisActor*);

  /**
   * Set properties specific to PolarAxis
   */
  void SetPolarAxisAttributes(vtkAxisActor*);

  /**
   * Create requested number of type X axes.
   */
  void CreateRadialAxes(int axisCount);

  /**
   * Build requested number of radial axes with respect to specified pole.
   * Call without viewport will delay some parameter correct initialization to next frame update
   * (scale for example).
   */
  void BuildRadialAxes(vtkViewport* viewport = nullptr);

  /**
   * return a step attempting to be as rounded as possible according to input parameters
   */
  double ComputeIdealStep(int subDivsRequired, double rangeLength, int maxSubDivs = 1000);

  /**
   * Build Arc ticks
   */
  void BuildArcTicks();

  /**
   * Init tick point located on an ellipse at angleEllipseRad angle and according to "a" major
   * radius
   */
  void StoreTicksPtsFromParamEllipse(
    double a, double angleEllipseRad, double tickSize, vtkPoints* tickPts);

  /**
   * Build polar axis labels and arcs with respect to specified pole.
   */
  void BuildPolarAxisLabelsArcs();

  /**
   * Build labels and arcs with log scale axis
   */
  void BuildPolarAxisLabelsArcsLog();

  /**
   * Define label values
   */
  void BuildLabelsLog();

  void BuildPolarArcsLog();

  /**
   * Find a common exponent for label values.
   */
  std::string FindExponentAndAdjustValues(std::list<double>& valuesList);

  /**
   * Yield a string array with the float part of each values. 0.01e-2 -> 0.0001
   */
  void GetSignificantPartFromValues(vtkStringArray* valuesStr, std::list<double>& valuesList);

  ///@{
  /**
   * Convenience methods
   */
  double FFix(double);
  double FSign(double, double);
  ///@}

  /**
   * Automatically rescale titles and labels
   * NB: Current implementation only for perspective projections.
   */
  void AutoScale(vtkViewport* viewport);

  /**
   * convert section angle to an angle applied to ellipse equation.
   * the result point with ellipse angle, is the point located on section angle
   */
  static double ComputeEllipseAngle(double angleInDegrees, double ratio);

  /**
   * Compute delta range of polar axes.
   */
  virtual void ComputeDeltaRangePolarAxes(vtkIdType);

  /**
   * Compute delta angle of radial axes.
   */
  virtual void ComputeDeltaAngleRadialAxes(vtkIdType);

private:
  /**
   * Coordinates of the pole
   */
  double Pole[3] = { 0.0, 0.0, 0.0 };

  /**
   * Number of radial axes
   */
  int NumberOfRadialAxes = 0;

  /**
   * Requested Number of radial axes
   * If set to 0, compute it depending on angle.
   */
  int RequestedNumberOfRadialAxes = 0;

  /**
   * Number of polar axes
   */
  int NumberOfPolarAxes = 5;

  /**
   * Requested Number of polar axes
   * If set to 0, compute it depending on range.
   */
  int RequestedNumberOfPolarAxes = 5;

  /**
   * Ratio for elliptical representation of the polar axes actor.
   */
  double Ratio = 1.0;

  /**
   * Polar arc resolution (number of line) per degree.
   */
  double PolarArcResolutionPerDegree = 0.2;

  /**
   * Define the range values displayed on the polar Axis.
   */
  double Range[2] = { 0.0, 10.0 };

  /**
   * Range between 2 minor ticks.
   */
  double DeltaRangeMinor = 0.5;

  /**
   * Range between 2 major ticks (values displayed on the axis).
   */
  double DeltaRangeMajor = 1.0;

  /**
   * Range between 2 polar axes.
   */
  double DeltaRangePolarAxes = 0.0;

  /**
   * Requested delta range for polar axes.
   * If set to 0, compute it depending on count.
   */
  double RequestedDeltaRangePolarAxes = 0.0;

  /**
   * Angle between 2 minor ticks on the last arc.
   */
  double DeltaAngleMinor = 22.5;

  /**
   * Angle between 2 major ticks on the last arc.
   */
  double DeltaAngleMajor = 45.0;

  /**
   * Angle between 2 radial Axes.
   */
  double DeltaAngleRadialAxes = 45.0;

  /**
   * Requested delta angle for radial axes.
   * If set to 0, compute it depending on count.
   */
  double RequestedDeltaAngleRadialAxes = 45.0;

  /**
   * Minimum polar radius.
   */
  double MinimumRadius = 0.0;

  /**
   * Maximum polar radius.
   */
  double MaximumRadius = 1.0;

  /**
   * Enable/Disable log scale
   */
  bool Log = false;

  /**
   * Minimum polar angle
   */
  double MinimumAngle = 0.0;

  /**
   * Maximum polar angle
   */
  double MaximumAngle = 90.0;

  /**
   * Smallest radial angle distinguishable from polar axis
   */
  double SmallestVisiblePolarAngle = 0.5;

  // Structures for principal polar arc
  vtkNew<vtkPolyData> PolarArcs;
  vtkNew<vtkPolyDataMapper> PolarArcsMapper;
  vtkNew<vtkActor> PolarArcsActor;

  ///@{
  /**
   * Structures for secondary polar arcs
   */
  vtkNew<vtkPolyData> SecondaryPolarArcs;
  vtkNew<vtkPolyDataMapper> SecondaryPolarArcsMapper;
  vtkNew<vtkActor> SecondaryPolarArcsActor;
  ///@}

  /**
   * Camera attached to the polar axes system
   */
  vtkSmartPointer<vtkCamera> Camera;

  /**
   * Control variables for polar axis
   */
  vtkNew<vtkAxisActor> PolarAxis;

  /**
   * Control variables for non-polar radial axes
   */
  std::vector<vtkSmartPointer<vtkAxisActor>> RadialAxes;

  ///@{
  /**
   * Title to be used for the polar axis
   * NB: Non-polar radial axes use the polar angle as title and have no labels
   */
  std::string PolarAxisTitle = "Radial Distance";
  char* PolarLabelFormat = nullptr;
  ///@}

  /**
   * String to format angle values displayed on the radial axes.
   */
  char* RadialAngleFormat = nullptr;

  /**
   * Display angle units (degrees) to label radial axes
   */
  bool RadialUnits = true;

  /**
   * If enabled the actor will not be visible at a certain distance from the camera.
   */
  bool EnableDistanceLOD = true;

  /**
   * This determines at what fraction of camera far clip range, actor is not visible.
   */
  double DistanceLODThreshold = 0.7;

  /**
   * If enabled the actor will not be visible at a certain view angle.
   */
  bool EnableViewAngleLOD = true;

  /**
   * This determines at what view angle to geometry will make the geometry not visible.
   * Default is 0.3.
   */
  double ViewAngleLODThreshold = 0.3;

  ///@{
  /**
   * Visibility of polar axis and its title, labels, ticks (major only)
   */
  bool PolarAxisVisibility = true;
  bool PolarTitleVisibility = true;
  bool PolarLabelVisibility = true;
  ///@}

  /**
   * Describes the tick orientation for the graph elements involved by this property.
   * The ticks are drawn according to the direction of the 2 orthogonal axes, of the axisBase
   * defined for a vtkAxisActor.
   * For an ellipse, tick directions are defined from ellipse center to tick origin and
   * the orthogonal direction of the ellipse plane.
   */
  int TickLocation = vtkAxisActor::VTK_TICKS_BOTH;

  /**
   * Hold visibility for all present ticks
   */
  bool PolarTickVisibility = true;

  /**
   * If On, the ticks are drawn from the angle of the polarAxis (i.e. this->MinimumAngle)
   * and continue counterclockwise with the step DeltaAngle Major/Minor. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  bool ArcTicksOriginToPolarAxis = true;

  /**
   * If On, the radial axes are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngleRadialAxes. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  bool RadialAxesOriginToPolarAxis = true;

  /**
   * Hold visibility of major/minor ticks for the polar axis and the last radial axis
   */
  bool AxisTickVisibility = true, AxisMinorTickVisibility = false;

  /**
   * Enable / Disable major ticks matching polar axes range (and minor half angle)
   */
  bool AxisTickMatchesPolarAxes = true;

  /**
   * Enable / Disable major/minor tick visibility on the last arc displayed
   */
  bool ArcTickVisibility = true, ArcMinorTickVisibility = false;

  /**
   * Enable / Disable major ticks matching radial axes angle (and minor half angle)
   */
  bool ArcTickMatchesRadialAxes = true;

  /**
   * Defines the length of the ticks located on the last arc
   */
  double PolarAxisMajorTickSize = 0.0, LastRadialAxisMajorTickSize = 0.0, ArcMajorTickSize = 0.0;

  /**
   * Set the ratios between major and minor tick Size for each ticks location
   */
  double PolarAxisTickRatioSize = 0.3, LastAxisTickRatioSize = 0.3, ArcTickRatioSize = 0.3;

  /**
   * Set the ratio between maximum radius and major tick size
   */
  double TickRatioRadiusSize = 0.02;

  /**
   * Defines the thickness of the major ticks.
   */
  double PolarAxisMajorTickThickness = 1.0, LastRadialAxisMajorTickThickness = 1.0,
         ArcMajorTickThickness = 1.0;

  /**
   * Set the ratios between major tick thickness for each ticks location
   */
  double PolarAxisTickRatioThickness = 0.5, LastAxisTickRatioThickness = 0.5,
         ArcTickRatioThickness = 0.5;

  ///@{
  /**
   * Visibility of radial axes and their titles
   */
  bool RadialAxesVisibility = true;
  bool RadialTitleVisibility = true;
  ///@}

  /**
   * Define the alignment of the title related to the radial axis. (BOTTOM or EXTERN)
   */
  int RadialAxisTitleLocation = VTK_TITLE_BOTTOM;

  /**
   * Define the alignment of the title related to the polar axis. (BOTTOM or EXTERN)
   */
  int PolarAxisTitleLocation = VTK_TITLE_BOTTOM;

  /**
   * Define the location of the exponent of the labels values, located on the polar axis.
   * it could be: LABEL, EXTERN, BOTTOM
   */
  int ExponentLocation = VTK_EXPONENT_LABELS;

  /**
   * Visibility of polar arcs
   */
  bool PolarArcsVisibility = true;

  /**
   * Visibility of the inner axes (overridden to 0 if RadialAxesVisibility is set to 0)
   */
  bool DrawRadialGridlines = true;

  /**
   * Visibility of the inner arcs (overridden to 0 if PolarArcsVisibility is set to 0)
   */
  bool DrawPolarArcsGridlines = true;

  /**
   * Keep the arc major ticks vtkPoints instances
   */
  vtkNew<vtkPoints> ArcMajorTickPts;

  /**
   * Keep the arc minor ticks vtkPoints instances
   */
  vtkNew<vtkPoints> ArcMinorTickPts;

  ///@{
  /**
   * vtk object for arc Ticks
   */
  vtkNew<vtkPolyData> ArcTickPolyData;
  vtkNew<vtkPolyData> ArcMinorTickPolyData;
  vtkNew<vtkPolyDataMapper> ArcTickPolyDataMapper;
  vtkNew<vtkPolyDataMapper> ArcMinorTickPolyDataMapper;
  vtkNew<vtkActor> ArcTickActor;
  vtkNew<vtkActor> ArcMinorTickActor;
  ///@}

  ///@{
  /**
   * Text properties of polar axis title and labels
   */
  vtkSmartPointer<vtkTextProperty> PolarAxisTitleTextProperty;
  vtkSmartPointer<vtkTextProperty> PolarAxisLabelTextProperty;
  ///@}

  /**
   * Text properties of last radial axis
   */
  vtkSmartPointer<vtkTextProperty> LastRadialAxisTextProperty;

  /**
   * Text properties of secondary radial axes
   */
  vtkSmartPointer<vtkTextProperty> SecondaryRadialAxesTextProperty;

  /**
   * General properties of polar axis
   * Behavior may be override by polar axis ticks 's actor property.
   */
  vtkSmartPointer<vtkProperty> PolarAxisProperty;

  /**
   * General properties of last radial axis
   */
  vtkSmartPointer<vtkProperty> LastRadialAxisProperty;

  /**
   * General properties of radial axes
   */
  vtkSmartPointer<vtkProperty> SecondaryRadialAxesProperty;

  vtkTimeStamp BuildTime;

  /**
   * Text screen size
   */
  double ScreenSize = 10.0;

  ///@{
  /**
   * Handles title (x,y), label (y) and exponent (y) available offsets.
   */
  double PolarTitleOffset[2] = { 20.0, 10.0 }, RadialTitleOffset[2] = { 20.0, 0.0 };
  double PolarLabelOffset = 10.0, PolarExponentOffset = 5.0;
  ///@}

  static constexpr int VTK_MAXIMUM_NUMBER_OF_POLAR_AXES = 20;
  static constexpr int VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES = 50;
  static constexpr double VTK_MINIMUM_POLAR_ARC_RESOLUTION_PER_DEG = 0.05;
  static constexpr double VTK_MAXIMUM_POLAR_ARC_RESOLUTION_PER_DEG = 100.0;

  vtkPolarAxesActor(const vtkPolarAxesActor&) = delete;
  void operator=(const vtkPolarAxesActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
