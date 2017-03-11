/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor.h
  Language:  C++

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen
All rights reserve
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
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

#define VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES 50
#define VTK_DEFAULT_NUMBER_OF_RADIAL_AXES 5
#define VTK_MAXIMUM_NUMBER_OF_POLAR_AXIS_TICKS 200
#define VTK_MAXIMUM_RATIO 1000.0
#define VTK_POLAR_ARC_RESOLUTION_PER_DEG 0.2

#include "vtkActor.h"
#include "vtkAxisActor.h"                 // access to enum values
#include "vtkRenderingAnnotationModule.h" // For export macro
#include <list>                           // To process exponent list as reference

class vtkCamera;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;
class vtkStringArray;
class vtkTextProperty;

class VTKRENDERINGANNOTATION_EXPORT vtkPolarAxesActor : public vtkActor
{
public:
  vtkTypeMacro(vtkPolarAxesActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Instantiate object with label format "6.3g" and the number of labels
   * per axis set to 3.
   */
  static vtkPolarAxesActor* New();

  //@{
  /**
   * Draw the polar axes
   */
  int RenderOpaqueGeometry(vtkViewport*) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport*) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) VTK_OVERRIDE { return 0; };
  //@}

  //@{
  /**
   * Explicitly specify the coordinate of the pole.
   */
  virtual void SetPole(double[3]);
  virtual void SetPole(double, double, double);
  vtkGetVector3Macro(Pole, double);
  //@}

  //@{
  /**
   * Enable/Disable log scale
   * Default: false
   */
  vtkSetMacro(Log, bool);
  vtkGetMacro(Log, bool);
  vtkBooleanMacro(Log, bool);
  //@}

  //@{
  /**
   * Gets/Sets the number of radial axes
   */
  vtkSetClampMacro(RequestedNumberOfRadialAxes, vtkIdType, 0, VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES);
  vtkGetMacro(RequestedNumberOfRadialAxes, vtkIdType);
  //@}

  //@{
  /**
   * Set/Get a number of ticks that one would like to display along polar axis
   * NB: it modifies DeltaRangeMajor to correspond to this number
   */
  virtual void SetNumberOfPolarAxisTicks(int);
  int GetNumberOfPolarAxisTicks();
  //@}

  //@{
  /**
   * Set/Get whether the number of polar axis ticks and arcs should be automatically calculated
   * Default: true
   */
  vtkSetMacro(AutoSubdividePolarAxis, bool);
  vtkGetMacro(AutoSubdividePolarAxis, bool);
  vtkBooleanMacro(AutoSubdividePolarAxis, bool);
  //@}

  //@{
  /**
   * Define the range values displayed on the polar Axis.
   */
  vtkSetVector2Macro(Range, double);
  vtkGetVectorMacro(Range, double, 2);
  //@}

  //@{
  /**
   * Set/Get the minimal radius of the polar coordinates.
   */
  virtual void SetMinimumRadius(double);
  vtkGetMacro(MinimumRadius, double);
  //@}

  //@{
  /**
   * Set/Get the maximum radius of the polar coordinates.
   */
  virtual void SetMaximumRadius(double);
  vtkGetMacro(MaximumRadius, double);
  //@}

  //@{
  /**
   * Set/Get the minimum radius of the polar coordinates (in degrees).
   */
  virtual void SetMinimumAngle(double);
  vtkGetMacro(MinimumAngle, double);
  //@}

  //@{
  /**
   * Set/Get the maximum radius of the polar coordinates (in degrees).
   */
  virtual void SetMaximumAngle(double);
  vtkGetMacro(MaximumAngle, double);
  //@}

  //@{
  /**
   * Set/Get the minimum radial angle distinguishable from polar axis
   * NB: This is used only when polar axis is visible
   * Default: 0.5
   */
  vtkSetClampMacro(SmallestVisiblePolarAngle, double, 0., 5.);
  vtkGetMacro(SmallestVisiblePolarAngle, double);
  //@}

  //@{
  /**
   * Set/Get the location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   */
  vtkSetClampMacro(TickLocation, int, vtkAxisActor::VTK_TICKS_INSIDE, vtkAxisActor::VTK_TICKS_BOTH);
  vtkGetMacro(TickLocation, int);
  //@}

  //@{
  /**
   * Default: true
   */
  vtkSetMacro(RadialUnits, bool);
  vtkGetMacro(RadialUnits, bool);
  //@}

  //@{
  /**
   * Explicitly specify the screen size of title and label text.
   * ScreenSize detemines the size of the text in terms of screen
   * pixels.
   * Default: 10.0.
   */
  vtkSetMacro(ScreenSize, double);
  vtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * vtkPolarAxesActor.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera, vtkCamera);
  //@}

  //@{
  /**
   * Set/Get the labels for the polar axis.
   * Default: "Radial Distance".
   */
  vtkSetStringMacro(PolarAxisTitle);
  vtkGetStringMacro(PolarAxisTitle);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the polar axis labels.
   */
  vtkSetStringMacro(PolarLabelFormat);
  vtkGetStringMacro(PolarLabelFormat);
  //@}

  enum ExponentLocation
  {
    VTK_EXPONENT_BOTTOM = 0,
    VTK_EXPONENT_EXTERN = 1,
    VTK_EXPONENT_LABELS = 2
  };

  //@{
  /**
   * Get/Set the location of the exponent (if any) of the polar axis values.
   * Possible location: VTK_EXPONENT_BOTTOM, VTK_EXPONENT_EXTERN,
   * VTK_EXPONENT_LABELS
   */
  vtkSetClampMacro(ExponentLocation, int, VTK_EXPONENT_BOTTOM, VTK_EXPONENT_LABELS);
  vtkGetMacro(ExponentLocation, int);
  //@}

  //@{
  /**
   * String to format angle values displayed on the radial axes.
   */
  vtkSetStringMacro(RadialAngleFormat);
  vtkGetStringMacro(RadialAngleFormat);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;

  //@{
  /**
   * Enable and disable the use of distance based LOD for titles and labels.
   */
  vtkSetMacro(EnableDistanceLOD, int);
  vtkGetMacro(EnableDistanceLOD, int);
  //@}

  //@{
  /**
   * Set distance LOD threshold [0.0 - 1.0] for titles and labels.
   */
  vtkSetClampMacro(DistanceLODThreshold, double, 0.0, 1.0);
  vtkGetMacro(DistanceLODThreshold, double);
  //@}

  //@{
  /**
   * Enable and disable the use of view angle based LOD for titles and labels.
   */
  vtkSetMacro(EnableViewAngleLOD, int);
  vtkGetMacro(EnableViewAngleLOD, int);
  //@}

  //@{
  /**
   * Set view angle LOD threshold [0.0 - 1.0] for titles and labels.
   */
  vtkSetClampMacro(ViewAngleLODThreshold, double, 0., 1.);
  vtkGetMacro(ViewAngleLODThreshold, double);
  //@}

  //@{
  /**
   * Turn on and off the visibility of the polar axis.
   */
  vtkSetMacro(PolarAxisVisibility, int);
  vtkGetMacro(PolarAxisVisibility, int);
  vtkBooleanMacro(PolarAxisVisibility, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of inner radial grid lines
   */
  vtkSetMacro(DrawRadialGridlines, int);
  vtkGetMacro(DrawRadialGridlines, int);
  vtkBooleanMacro(DrawRadialGridlines, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of inner polar arcs grid lines
   */
  vtkSetMacro(DrawPolarArcsGridlines, int);
  vtkGetMacro(DrawPolarArcsGridlines, int);
  vtkBooleanMacro(DrawPolarArcsGridlines, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of titles for polar axis.
   */
  vtkSetMacro(PolarTitleVisibility, int);
  vtkGetMacro(PolarTitleVisibility, int);
  vtkBooleanMacro(PolarTitleVisibility, int);
  //@}

  enum TitleLocation
  {
    VTK_TITLE_BOTTOM = 0,
    VTK_TITLE_EXTERN = 1
  };

  //@{
  /**
   * Get/Set the alignement of the radial axes title related to the axis.
   * Possible Alignment: VTK_TITLE_BOTTOM, VTK_TITLE_EXTERN
   */
  vtkSetClampMacro(RadialAxisTitleLocation, int, VTK_TITLE_BOTTOM, VTK_TITLE_EXTERN);
  vtkGetMacro(RadialAxisTitleLocation, int);
  //@}

  //@{
  /**
   * Get/Set the alignement of the polar axes title related to the axis.
   * Possible Alignment: VTKTITLE_BOTTOM, VTK_TITLE_EXTERN
   */
  vtkSetClampMacro(PolarAxisTitleLocation, int, VTK_TITLE_BOTTOM, VTK_TITLE_EXTERN);
  vtkGetMacro(PolarAxisTitleLocation, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of labels for polar axis.
   */
  vtkSetMacro(PolarLabelVisibility, int);
  vtkGetMacro(PolarLabelVisibility, int);
  vtkBooleanMacro(PolarLabelVisibility, int);
  //@}

  //@{
  /**
   * If On, the ticks are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngle Major/Minor. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  vtkSetMacro(ArcTicksOriginToPolarAxis, int);
  vtkGetMacro(ArcTicksOriginToPolarAxis, int);
  vtkBooleanMacro(ArcTicksOriginToPolarAxis, int);
  //@}

  //@{
  /**
   * If On, the radial axes are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngleRadialAxes. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  vtkSetMacro(RadialAxesOriginToPolarAxis, int);
  vtkGetMacro(RadialAxesOriginToPolarAxis, int);
  vtkBooleanMacro(RadialAxesOriginToPolarAxis, int);
  //@}

  //@{
  /**
   * Turn on and off the overall visibility of ticks.
   */
  vtkSetMacro(PolarTickVisibility, int);
  vtkGetMacro(PolarTickVisibility, int);
  vtkBooleanMacro(PolarTickVisibility, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of major ticks on polar axis and last radial axis.
   */
  vtkSetMacro(AxisTickVisibility, int);
  vtkGetMacro(AxisTickVisibility, int);
  vtkBooleanMacro(AxisTickVisibility, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of minor ticks on polar axis and last radial axis.
   */
  vtkSetMacro(AxisMinorTickVisibility, int);
  vtkGetMacro(AxisMinorTickVisibility, int);
  vtkBooleanMacro(AxisMinorTickVisibility, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of major ticks on the last arc.
   */
  vtkSetMacro(ArcTickVisibility, int);
  vtkGetMacro(ArcTickVisibility, int);
  vtkBooleanMacro(ArcTickVisibility, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of minor ticks on the last arc.
   */
  vtkSetMacro(ArcMinorTickVisibility, int);
  vtkGetMacro(ArcMinorTickVisibility, int);
  vtkBooleanMacro(ArcMinorTickVisibility, int);
  //@}

  //@{
  /**
   * Set/Get the size of the major ticks on the last arc.
   */
  vtkSetMacro(ArcMajorTickSize, double);
  vtkGetMacro(ArcMajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the major ticks on the polar axis.
   */
  vtkSetMacro(PolarAxisMajorTickSize, double);
  vtkGetMacro(PolarAxisMajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the major ticks on the last radial axis.
   */
  vtkSetMacro(LastRadialAxisMajorTickSize, double);
  vtkGetMacro(LastRadialAxisMajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Polar Axis ticks size
   */
  vtkSetMacro(PolarAxisTickRatioSize, double);
  vtkGetMacro(PolarAxisTickRatioSize, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Last Radial axis ticks size
   */
  vtkSetMacro(LastAxisTickRatioSize, double);
  vtkGetMacro(LastAxisTickRatioSize, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Arc ticks size
   */
  vtkSetMacro(ArcTickRatioSize, double);
  vtkGetMacro(ArcTickRatioSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the thickness of polar axis ticks
   */
  vtkSetMacro(PolarAxisMajorTickThickness, double);
  vtkGetMacro(PolarAxisMajorTickThickness, double);
  //@}

  //@{
  /**
   * Set/Get the size of the thickness of last radial axis ticks
   */
  vtkSetMacro(LastRadialAxisMajorTickThickness, double);
  vtkGetMacro(LastRadialAxisMajorTickThickness, double);
  //@}

  //@{
  /**
   * Set/Get the size of the thickness of the last arc ticks
   */
  vtkSetMacro(ArcMajorTickThickness, double);
  vtkGetMacro(ArcMajorTickThickness, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Polar Axis ticks thickness
   */
  vtkSetMacro(PolarAxisTickRatioThickness, double);
  vtkGetMacro(PolarAxisTickRatioThickness, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Last Radial axis ticks thickness
   */
  vtkSetMacro(LastAxisTickRatioThickness, double);
  vtkGetMacro(LastAxisTickRatioThickness, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Arc ticks thickness
   */
  vtkSetMacro(ArcTickRatioThickness, double);
  vtkGetMacro(ArcTickRatioThickness, double);
  //@}

  //@{
  /**
   * Set/Get the step between 2 major ticks, in range value (values displayed on the axis).
   */
  vtkSetMacro(DeltaRangeMajor, double);
  vtkGetMacro(DeltaRangeMajor, double);
  //@}

  //@{
  /**
   * Set/Get the step between 2 minor ticks, in range value (values displayed on the axis).
   */
  vtkSetMacro(DeltaRangeMinor, double);
  vtkGetMacro(DeltaRangeMinor, double);
  //@}

  //@{
  /**
   * Set/Get the angle between 2 major ticks on the last arc.
   */
  vtkSetMacro(DeltaAngleMajor, double);
  vtkGetMacro(DeltaAngleMajor, double);
  //@}

  //@{
  /**
   * Set/Get the angle between 2 minor ticks on the last arc.
   */
  vtkSetMacro(DeltaAngleMinor, double);
  vtkGetMacro(DeltaAngleMinor, double);
  //@}

  //@{
  /**
   * Set/Get the angle between 2 radial axes.
   */
  vtkSetMacro(DeltaAngleRadialAxes, double);
  vtkGetMacro(DeltaAngleRadialAxes, double);
  //@}

  //------------------------------------------------

  //@{
  /**
   * Turn on and off the visibility of non-polar radial axes.
   */
  vtkSetMacro(RadialAxesVisibility, int);
  vtkGetMacro(RadialAxesVisibility, int);
  vtkBooleanMacro(RadialAxesVisibility, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of titles for non-polar radial axes.
   */
  vtkSetMacro(RadialTitleVisibility, int);
  vtkGetMacro(RadialTitleVisibility, int);
  vtkBooleanMacro(RadialTitleVisibility, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of arcs for polar axis.
   */
  vtkSetMacro(PolarArcsVisibility, int);
  vtkGetMacro(PolarArcsVisibility, int);
  vtkBooleanMacro(PolarArcsVisibility, int);
  //@}

  //@{
  /**
   * Enable/Disable labels 2D mode (always facing the camera).
   */
  void SetUse2DMode(int val);
  int GetUse2DMode();
  //@}

  //@{
  /**
   * Set/Get the polar axis title text property.
   */
  virtual void SetPolarAxisTitleTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(PolarAxisTitleTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the polar axis labels text property.
   */
  virtual void SetPolarAxisLabelTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(PolarAxisLabelTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the last radial axis text property.
   */
  virtual void SetLastRadialAxisTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(LastRadialAxisTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the secondary radial axes text property.
   */
  virtual void SetSecondaryRadialAxesTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(SecondaryRadialAxesTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Get/Set polar axis actor properties.
   */
  virtual void SetPolarAxisProperty(vtkProperty*);
  vtkGetObjectMacro(PolarAxisProperty, vtkProperty);
  //@}

  //@{
  /**
   * Get/Set last radial axis actor properties.
   */
  virtual void SetLastRadialAxisProperty(vtkProperty* p);
  vtkGetObjectMacro(LastRadialAxisProperty, vtkProperty);
  //@}

  //@{
  /**
   * Get/Set secondary radial axes actors properties.
   */
  virtual void SetSecondaryRadialAxesProperty(vtkProperty* p);
  vtkGetObjectMacro(SecondaryRadialAxesProperty, vtkProperty);
  //@}

  //@{
  /**
   * Get/Set principal polar arc actor property.
   */
  virtual void SetPolarArcsProperty(vtkProperty* p);
  vtkProperty* GetPolarArcsProperty();
  //@}

  //@{
  /**
   * Get/Set secondary polar arcs actors property.
   */
  virtual void SetSecondaryPolarArcsProperty(vtkProperty* p);
  vtkProperty* GetSecondaryPolarArcsProperty();
  //@}

  //@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds are used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   */
  vtkSetVector6Macro(Bounds, double);
  double* GetBounds() VTK_OVERRIDE;
  void GetBounds(
    double& xmin, double& xmax, double& ymin, double& ymax, double& zmin, double& zmax);
  void GetBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Ratio
   */
  vtkSetClampMacro(Ratio, double, 0.001, 100.0);
  vtkGetMacro(Ratio, double);
  //@}

protected:
  vtkPolarAxesActor();
  ~vtkPolarAxesActor() VTK_OVERRIDE;

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
   */
  void BuildRadialAxes();

  /**
   * Set Range and PolarAxis members value to build axis ticks
   * this fonction doesn't actually build PolarAxis ticks, it set the DeltaRangeMajor and DeltaMajor
   * attributes
   * then PolarAxis itself is in charge of ticks drawing
   */
  void AutoComputeTicksProperties();

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

  //@{
  /**
   * Convenience methods
   */
  double FFix(double);
  double FSign(double, double);
  //@}

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
   * Compute delta angle of radial axes.
   */
  virtual void ComputeDeltaAngleRadialAxes(vtkIdType);
  /**
   * Coordinates of the pole
   * Default: (0,0,0).
   */
  double Pole[3];

  /**
   * Number of radial axes
   */
  int NumberOfRadialAxes;

  /**
   * Requested Number of radial axes
   */
  int RequestedNumberOfRadialAxes;

  /**
   * Whether the number of polar axis ticks and arcs should be automatically calculated.
   * Default: TRUE
   */
  bool AutoSubdividePolarAxis;

  /**
   * Ratio for elliptical representation of the polar axes actor.
   */
  double Ratio;

  /**
   * Define the range values displayed on the polar Axis.
   */
  double Range[2];

  /**
   * Step between 2 minor ticks, in range value (values displayed on the axis).
   */
  double DeltaRangeMinor;

  /**
   * Step between 2 major ticks, in range value (values displayed on the axis).
   */
  double DeltaRangeMajor;

  /**
   * Angle between 2 minor ticks on the last arc.
   */
  double DeltaAngleMinor;

  /**
   * Angle between 2 major ticks on the last arc.
   */
  double DeltaAngleMajor;

  /**
   * Angle between 2 radial Axes.
   */
  double DeltaAngleRadialAxes;

  /**
   * Minimum polar radius.
   * Default: 0.0
   */
  double MinimumRadius;

  /**
   * Maximum polar radius.
   * Default: 1
   */
  double MaximumRadius;

  /**
   * Enable/Disable log scale
   * Default: 0
   */
  bool Log;

  /**
   * Auto-scale polar radius (with respect to average length scale of x-y bounding box).
   */
  bool AutoScaleRadius;

  /**
   * Minimum polar angle
   * Default: 0.
   */
  double MinimumAngle;

  /**
   * Maximum polar angle
   * Default: 90.
   */
  double MaximumAngle;

  /**
   * Smallest radial angle distinguishable from polar axis
   */
  double SmallestVisiblePolarAngle;

  /**
   * Explicit actor bounds
   */
  double Bounds[6];

  // Structures for principal polar arc
  vtkPolyData* PolarArcs;
  vtkPolyDataMapper* PolarArcsMapper;
  vtkActor* PolarArcsActor;

  //@{
  /**
   * Structures for secondary polar arcs
   */
  vtkPolyData* SecondaryPolarArcs;
  vtkPolyDataMapper* SecondaryPolarArcsMapper;
  vtkActor* SecondaryPolarArcsActor;
  //@}

  /**
   * Camera attached to the polar axes system
   */
  vtkCamera* Camera;

  /**
   * Control variables for polar axis
   */
  vtkAxisActor* PolarAxis;

  /**
   * Control variables for non-polar radial axes
   */
  vtkAxisActor** RadialAxes;

  //@{
  /**
   * Title to be used for the polar axis
   * NB: Non-polar radial axes use the polar angle as title and have no labels
   */
  char* PolarAxisTitle;
  char* PolarLabelFormat;
  //@}

  /**
   * String to format angle values displayed on the radial axes.
   */
  char* RadialAngleFormat;

  /**
   * Display angle units (degrees) to label radial axes
   * Default is true
   */
  bool RadialUnits;

  /**
   * If enabled the actor will not be visible at a certain distance from the camera.
   * Default is true
   */
  int EnableDistanceLOD;

  /**
   * Default is 0.80
   * This determines at what fraction of camera far clip range, actor is not visible.
   */
  double DistanceLODThreshold;

  /**
   * If enabled the actor will not be visible at a certain view angle.
   * Default is true.
   */
  int EnableViewAngleLOD;

  /**
   * This determines at what view angle to geometry will make the geometry not visible.
   * Default is 0.3.
   */
  double ViewAngleLODThreshold;

  //@{
  /**
   * Visibility of polar axis and its title, labels, ticks (major only)
   */
  int PolarAxisVisibility;
  int PolarTitleVisibility;
  int PolarLabelVisibility;
  //@}

  /**
   * Describes the tick orientation for the graph elements involved by this property.
   * The ticks are drawn according to the direction of the 2 orthogonal axes, of the axisBase
   * defined for a vtkAxisActor.
   * For an ellipse, tick directions are defined from ellipse center to tick origin and
   * the orthogonal direction of the ellipse plane.
   */
  int TickLocation;

  /**
   * Hold visibility for all present ticks
   */
  int PolarTickVisibility;

  /**
   * If On, the ticks are drawn from the angle of the polarAxis (i.e. this->MinimumAngle)
   * and continue counterclockwise with the step DeltaAngle Major/Minor. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  int ArcTicksOriginToPolarAxis;

  /**
   * If On, the radial axes are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngleRadialAxes. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  int RadialAxesOriginToPolarAxis;

  /**
   * Hold visibility of major/minor ticks for the polar axis and the last radial axis
   */
  int AxisTickVisibility, AxisMinorTickVisibility;

  /**
   * Enable / Disable major/minor tick visibility on the last arc displayed
   */
  int ArcTickVisibility, ArcMinorTickVisibility;

  /**
   * Defines the length of the ticks located on the last arc
   */
  double PolarAxisMajorTickSize, LastRadialAxisMajorTickSize, ArcMajorTickSize;

  /**
   * Set the ratios between major tick Size for each ticks location
   */
  double PolarAxisTickRatioSize, LastAxisTickRatioSize, ArcTickRatioSize;

  /**
   * Defines the tickness of the major ticks.
   */
  double PolarAxisMajorTickThickness, LastRadialAxisMajorTickThickness, ArcMajorTickThickness;

  /**
   * Set the ratios between major tick thickness for each ticks location
   */
  double PolarAxisTickRatioThickness, LastAxisTickRatioThickness, ArcTickRatioThickness;

  //@{
  /**
   * Visibility of radial axes and their titles
   */
  int RadialAxesVisibility;
  int RadialTitleVisibility;
  //@}

  /**
   * Define the alignement of the title related to the radial axis. (BOTTOM or EXTERN)
   */
  int RadialAxisTitleLocation;

  /**
   * Define the alignement of the title related to the polar axis. (BOTTOM or EXTERN)
   */
  int PolarAxisTitleLocation;

  /**
   * Define the location of the exponent of the labels values, located on the polar axis.
   * it could be: LABEL, EXTERN, BOTTOM
   */
  int ExponentLocation;

  /**
   * Visibility of polar arcs
   */
  int PolarArcsVisibility;

  /**
   * Visibility of the inner axes (overridden to 0 if RadialAxesVisibility is set to 0)
   */
  int DrawRadialGridlines;

  /**
   * Visibility of the inner arcs (overridden to 0 if PolarArcsVisibility is set to 0)
   */
  int DrawPolarArcsGridlines;

  /**
   * Keep the arc major ticks vtkPoints instances
   */
  vtkPoints* ArcMajorTickPts;

  /**
   * Keep the arc minor ticks vtkPoints instances
   */
  vtkPoints* ArcMinorTickPts;

  //@{
  /**
   * vtk object for arc Ticks
   */
  vtkPolyData* ArcTickPolyData;
  vtkPolyData* ArcMinorTickPolyData;
  vtkPolyDataMapper* ArcTickPolyDataMapper;
  vtkPolyDataMapper* ArcMinorTickPolyDataMapper;
  vtkActor* ArcTickActor;
  vtkActor* ArcMinorTickActor;
  //@}

  //@{
  /**
   * Text properties of polar axis title and labels
   */
  vtkTextProperty* PolarAxisTitleTextProperty;
  vtkTextProperty* PolarAxisLabelTextProperty;
  //@}

  /**
   * Text properties of last radial axis
   */
  vtkTextProperty* LastRadialAxisTextProperty;

  /**
   * Text properties of secondary radial axes
   */
  vtkTextProperty* SecondaryRadialAxesTextProperty;

  /**
   * General properties of polar axis
   * Behavior may be override by polar axis ticks 's actor property.
   */
  vtkProperty* PolarAxisProperty;

  /**
   * General properties of last radial axis
   */
  vtkProperty* LastRadialAxisProperty;

  /**
   * General properties of radial axes
   */
  vtkProperty* SecondaryRadialAxesProperty;

  vtkTimeStamp BuildTime;

  /**
   * Title scale factor
   */
  double TitleScale;

  /**
   * Label scale factor
   */
  double LabelScale;

  /**
   * Text screen size
   */
  double ScreenSize;

private:
  vtkPolarAxesActor(const vtkPolarAxesActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolarAxesActor&) VTK_DELETE_FUNCTION;
};

#endif
