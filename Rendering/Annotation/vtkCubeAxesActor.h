// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCubeAxesActor
 * @brief   create a plot of a bounding box edges -
 * used for navigation
 *
 * vtkCubeAxesActor is a composite actor that draws axes of the
 * bounding box of an input dataset. The axes include labels and titles
 * for the x-y-z axes. The algorithm selects which axes to draw based
 * on the user-defined 'fly' mode.  (STATIC is default).
 * 'STATIC' constructs axes from all edges of the bounding box.
 * 'CLOSEST_TRIAD' consists of the three axes x-y-z forming a triad that
 * lies closest to the specified camera.
 * 'FURTHEST_TRIAD' consists of the three axes x-y-z forming a triad that
 * lies furthest from the specified camera.
 * 'OUTER_EDGES' is constructed from edges that are on the "exterior" of the
 * bounding box, exterior as determined from examining outer edges of the
 * bounding box in projection (display) space.
 *
 * To use this object you must define a bounding box and the camera used
 * to render the vtkCubeAxesActor. You can optionally turn on/off labels,
 * ticks, gridlines, and set tick location, number of labels, and text to
 * use for axis-titles.  A 'corner offset' can also be set.  This allows
 * the axes to be set partially away from the actual bounding box to perhaps
 * prevent overlap of labels between the various axes.
 *
 * The Bounds instance variable (an array of six doubles) is used to determine
 * the bounding box.
 *
 * @par Thanks:
 * This class was written by:
 * Hank Childs, Kathleen Bonnell, Amy Squillacote, Brad Whitlock, Will Schroeder,
 * Eric Brugger, Daniel Aguilera, Claire Guilbaud, Nicolas Dolegieviez,
 * Aashish Chaudhary, Philippe Pebay, David Gobbi, David Partyka, Utkarsh Ayachit
 * David Cole, Francois Bertel, and Mark Olesen
 * Part of this work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France.
 *
 * @sa
 * vtkActor vtkAxisActor vtkCubeAxesActor2D
 */

#ifndef vtkCubeAxesActor_h
#define vtkCubeAxesActor_h

#include "vtkActor.h"
#include "vtkNew.h"                       // For vtkNew
#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkSmartPointer.h"              // For vtkSmartPointer
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAxisActor;
class vtkCamera;
class vtkTextProperty;
class vtkStringArray;

class VTKRENDERINGANNOTATION_EXPORT VTK_MARSHALAUTO vtkCubeAxesActor : public vtkActor
{
public:
  vtkTypeMacro(vtkCubeAxesActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate object with label format "6.3g" and the number of labels
   * per axis set to 3.
   */
  static vtkCubeAxesActor* New();

  ///@{
  /**
   * Draw the axes as per the vtkProp superclass' API.
   */
  int RenderOpaqueGeometry(vtkViewport*) override;
  virtual int RenderTranslucentGeometry(vtkViewport*);
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  int RenderOverlay(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  ///@{
  /**
   * Gets/Sets the RebuildAxes flag.
   * Default: true.
   */
  vtkSetMacro(RebuildAxes, bool);
  vtkGetMacro(RebuildAxes, bool);
  ///@}

  ///@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds is used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   */
  vtkSetVector6Macro(Bounds, double);
  using Superclass::GetBounds;
  double* GetBounds() VTK_SIZEHINT(6) override { return this->Bounds; }
  ///@}

  ///@{
  /**
   * Method used to properly return the bounds of the cube axis itself with all
   * its labels.
   */
  virtual void GetRenderedBounds(double rBounds[6]);
  virtual double* GetRenderedBounds();
  ///@}

  ///@{
  /**
   * Explicitly specify the range of each axes that's used to define the prop.
   * The default, (if you do not use these methods) is to use the bounds
   * specified, or use the bounds of the Input Prop if one is specified. This
   * method allows you to separate the notion of extent of the axes in physical
   * space (bounds) and the extent of the values it represents. In other words,
   * you can have the ticks and labels show a different range.
   */
  vtkSetVector2Macro(XAxisRange, double);
  vtkSetVector2Macro(YAxisRange, double);
  vtkSetVector2Macro(ZAxisRange, double);
  vtkGetVector2Macro(XAxisRange, double);
  vtkGetVector2Macro(YAxisRange, double);
  ///@}
  ///@{
  /**
   * Explicitly specify the axis labels along an axis as an array of strings
   * instead of using the values.
   */
  vtkStringArray* GetAxisLabels(int axis);
  void SetAxisLabels(int axis, vtkStringArray* value);
  ///@}

  vtkGetVector2Macro(ZAxisRange, double);

  ///@{
  /**
   * Explicitly specify the screen size of title and label text.
   * ScreenSize determines the size of the text in terms of screen
   * pixels.
   * Default: 10.0.
   */
  void SetScreenSize(double screenSize);
  vtkGetMacro(ScreenSize, double);
  ///@}

  ///@{
  /**
   * Explicitly specify the offset between labels and the axis.
   * Default: 20.0.
   */
  void SetLabelOffset(double offset);
  vtkGetMacro(LabelOffset, double);
  ///@}

  ///@{
  /**
   * Explicitly specify the offset between title and labels.
   * Default: (20.0, 20.0).
   */
  void SetTitleOffset(double titleOffset[2]);
  vtkGetVector2Macro(TitleOffset, double);
  ///@}

  ///@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * vtkCubeAxesActor.
   */
  virtual void SetCamera(vtkCamera*);
  vtkCamera* GetCamera();
  ///@}

  enum FlyMode
  {
    VTK_FLY_OUTER_EDGES = 0,
    VTK_FLY_CLOSEST_TRIAD = 1,
    VTK_FLY_FURTHEST_TRIAD = 2,
    VTK_FLY_STATIC_TRIAD = 3,
    VTK_FLY_STATIC_EDGES = 4
  };

  ///@{
  /**
   * Specify a mode to control how the axes are drawn: either static,
   * closest triad, furthest triad or outer edges in relation to the
   * camera position.
   * Default: VTK_FLY_CLOSEST_TRIAD.
   */
  vtkSetClampMacro(FlyMode, int, VTK_FLY_OUTER_EDGES, VTK_FLY_STATIC_EDGES);
  vtkGetMacro(FlyMode, int);
  void SetFlyModeToOuterEdges() { this->SetFlyMode(VTK_FLY_OUTER_EDGES); }
  void SetFlyModeToClosestTriad() { this->SetFlyMode(VTK_FLY_CLOSEST_TRIAD); }
  void SetFlyModeToFurthestTriad() { this->SetFlyMode(VTK_FLY_FURTHEST_TRIAD); }
  void SetFlyModeToStaticTriad() { this->SetFlyMode(VTK_FLY_STATIC_TRIAD); }
  void SetFlyModeToStaticEdges() { this->SetFlyMode(VTK_FLY_STATIC_EDGES); }
  ///@}

  ///@{
  /**
   * Set/Get the labels for the x, y, and z axes. By default,
   * use "X-Axis", "Y-Axis" and "Z-Axis".
   */
  vtkSetStringMacro(XTitle);
  vtkGetStringMacro(XTitle);
  vtkSetStringMacro(XUnits);
  vtkGetStringMacro(XUnits);
  vtkSetStringMacro(YTitle);
  vtkGetStringMacro(YTitle);
  vtkSetStringMacro(YUnits);
  vtkGetStringMacro(YUnits);
  vtkSetStringMacro(ZTitle);
  vtkGetStringMacro(ZTitle);
  vtkSetStringMacro(ZUnits);
  vtkGetStringMacro(ZUnits);
  ///@}

  ///@{
  /**
   * Set/Get the format with which to print the labels on each of the
   * x-y-z axes.
   */
  vtkSetStringMacro(XLabelFormat);
  vtkGetStringMacro(XLabelFormat);
  vtkSetStringMacro(YLabelFormat);
  vtkGetStringMacro(YLabelFormat);
  vtkSetStringMacro(ZLabelFormat);
  vtkGetStringMacro(ZLabelFormat);
  ///@}

  ///@{
  /**
   * Set/Get the inertial factor that controls how often (i.e, how
   * many renders) the axes can switch position (jump from one axes
   * to another).
   * Default: 1.
   */
  vtkSetClampMacro(Inertia, int, 1, VTK_INT_MAX);
  vtkGetMacro(Inertia, int);
  ///@}

  ///@{
  /**
   * Specify an offset value to "pull back" the axes from the corner at
   * which they are joined to avoid overlap of axes labels. The
   * "CornerOffset" is the fraction of the axis length to pull back.
   * Default: 0.0.
   */
  vtkSetMacro(CornerOffset, double);
  vtkGetMacro(CornerOffset, double);
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
   * Default: 0.8.
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
   * Default: 0.2.
   */
  vtkSetClampMacro(ViewAngleLODThreshold, double, 0., 1.);
  vtkGetMacro(ViewAngleLODThreshold, double);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of each axis.
   * Default: true.
   */
  vtkSetMacro(XAxisVisibility, bool);
  vtkGetMacro(XAxisVisibility, bool);
  vtkBooleanMacro(XAxisVisibility, bool);

  vtkSetMacro(YAxisVisibility, bool);
  vtkGetMacro(YAxisVisibility, bool);
  vtkBooleanMacro(YAxisVisibility, bool);

  vtkSetMacro(ZAxisVisibility, bool);
  vtkGetMacro(ZAxisVisibility, bool);
  vtkBooleanMacro(ZAxisVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of labels for each axis.
   * Default: true.
   */
  vtkSetMacro(XAxisLabelVisibility, bool);
  vtkGetMacro(XAxisLabelVisibility, bool);
  vtkBooleanMacro(XAxisLabelVisibility, bool);

  vtkSetMacro(YAxisLabelVisibility, bool);
  vtkGetMacro(YAxisLabelVisibility, bool);
  vtkBooleanMacro(YAxisLabelVisibility, bool);

  vtkSetMacro(ZAxisLabelVisibility, bool);
  vtkGetMacro(ZAxisLabelVisibility, bool);
  vtkBooleanMacro(ZAxisLabelVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of ticks for each axis.
   * Default: true.
   */
  vtkSetMacro(XAxisTickVisibility, bool);
  vtkGetMacro(XAxisTickVisibility, bool);
  vtkBooleanMacro(XAxisTickVisibility, bool);

  vtkSetMacro(YAxisTickVisibility, bool);
  vtkGetMacro(YAxisTickVisibility, bool);
  vtkBooleanMacro(YAxisTickVisibility, bool);

  vtkSetMacro(ZAxisTickVisibility, bool);
  vtkGetMacro(ZAxisTickVisibility, bool);
  vtkBooleanMacro(ZAxisTickVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of minor ticks for each axis.
   * Default: true.
   */
  vtkSetMacro(XAxisMinorTickVisibility, bool);
  vtkGetMacro(XAxisMinorTickVisibility, bool);
  vtkBooleanMacro(XAxisMinorTickVisibility, bool);

  vtkSetMacro(YAxisMinorTickVisibility, bool);
  vtkGetMacro(YAxisMinorTickVisibility, bool);
  vtkBooleanMacro(YAxisMinorTickVisibility, bool);

  vtkSetMacro(ZAxisMinorTickVisibility, bool);
  vtkGetMacro(ZAxisMinorTickVisibility, bool);
  vtkBooleanMacro(ZAxisMinorTickVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of grid lines for each axis.
   * Default: false.
   */
  vtkSetMacro(DrawXGridlines, bool);
  vtkGetMacro(DrawXGridlines, bool);
  vtkBooleanMacro(DrawXGridlines, bool);

  vtkSetMacro(DrawYGridlines, bool);
  vtkGetMacro(DrawYGridlines, bool);
  vtkBooleanMacro(DrawYGridlines, bool);

  vtkSetMacro(DrawZGridlines, bool);
  vtkGetMacro(DrawZGridlines, bool);
  vtkBooleanMacro(DrawZGridlines, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of inner grid lines for each axis.
   * Default: false.
   */
  vtkSetMacro(DrawXInnerGridlines, bool);
  vtkGetMacro(DrawXInnerGridlines, bool);
  vtkBooleanMacro(DrawXInnerGridlines, bool);

  vtkSetMacro(DrawYInnerGridlines, bool);
  vtkGetMacro(DrawYInnerGridlines, bool);
  vtkBooleanMacro(DrawYInnerGridlines, bool);

  vtkSetMacro(DrawZInnerGridlines, bool);
  vtkGetMacro(DrawZInnerGridlines, bool);
  vtkBooleanMacro(DrawZInnerGridlines, bool);
  ///@}

  ///@{
  /**
   * Turn on and off the visibility of grid polys for each axis.
   * Default: false.
   */
  vtkSetMacro(DrawXGridpolys, bool);
  vtkGetMacro(DrawXGridpolys, bool);
  vtkBooleanMacro(DrawXGridpolys, bool);

  vtkSetMacro(DrawYGridpolys, bool);
  vtkGetMacro(DrawYGridpolys, bool);
  vtkBooleanMacro(DrawYGridpolys, bool);

  vtkSetMacro(DrawZGridpolys, bool);
  vtkGetMacro(DrawZGridpolys, bool);
  vtkBooleanMacro(DrawZGridpolys, bool);
  ///@}

  ///@{
  /**
   * Returns the text property for the title on an axis.
   */
  vtkTextProperty* GetTitleTextProperty(int);
  void SetXAxesTitleProperty(vtkTextProperty*);
  vtkTextProperty* GetXAxesTitleProperty();
  void SetYAxesTitleProperty(vtkTextProperty*);
  vtkTextProperty* GetYAxesTitleProperty();
  void SetZAxesTitleProperty(vtkTextProperty*);
  vtkTextProperty* GetZAxesTitleProperty();
  ///@}

  ///@{
  /**
   * Returns the text property for the labels on an axis.
   */
  vtkTextProperty* GetLabelTextProperty(int);
  void SetXAxesLabelProperty(vtkTextProperty*);
  vtkTextProperty* GetXAxesLabelProperty();
  void SetYAxesLabelProperty(vtkTextProperty*);
  vtkTextProperty* GetYAxesLabelProperty();
  void SetZAxesLabelProperty(vtkTextProperty*);
  vtkTextProperty* GetZAxesLabelProperty();
  ///@}

  ///@{
  /**
   * Get/Set axes actors properties.
   */
  void SetXAxesLinesProperty(vtkProperty*);
  vtkProperty* GetXAxesLinesProperty();
  void SetYAxesLinesProperty(vtkProperty*);
  vtkProperty* GetYAxesLinesProperty();
  void SetZAxesLinesProperty(vtkProperty*);
  vtkProperty* GetZAxesLinesProperty();
  ///@}

  ///@{
  /**
   * Get/Set axes (outer) gridlines actors properties.
   */
  void SetXAxesGridlinesProperty(vtkProperty*);
  vtkProperty* GetXAxesGridlinesProperty();
  void SetYAxesGridlinesProperty(vtkProperty*);
  vtkProperty* GetYAxesGridlinesProperty();
  void SetZAxesGridlinesProperty(vtkProperty*);
  vtkProperty* GetZAxesGridlinesProperty();
  ///@}

  ///@{
  /**
   * Get/Set axes inner gridlines actors properties.
   */
  void SetXAxesInnerGridlinesProperty(vtkProperty*);
  vtkProperty* GetXAxesInnerGridlinesProperty();
  void SetYAxesInnerGridlinesProperty(vtkProperty*);
  vtkProperty* GetYAxesInnerGridlinesProperty();
  void SetZAxesInnerGridlinesProperty(vtkProperty*);
  vtkProperty* GetZAxesInnerGridlinesProperty();
  ///@}

  ///@{
  /**
   * Get/Set axes gridPolys actors properties.
   */
  void SetXAxesGridpolysProperty(vtkProperty*);
  vtkProperty* GetXAxesGridpolysProperty();
  void SetYAxesGridpolysProperty(vtkProperty*);
  vtkProperty* GetYAxesGridpolysProperty();
  void SetZAxesGridpolysProperty(vtkProperty*);
  vtkProperty* GetZAxesGridpolysProperty();
  ///@}

  enum TickLocation
  {
    VTK_TICKS_INSIDE = 0,
    VTK_TICKS_OUTSIDE = 1,
    VTK_TICKS_BOTH = 2
  };

  ///@{
  /**
   * Set/Get the location of ticks marks.
   * Default: VTK_TICKS_INSIDE.
   */
  vtkSetClampMacro(TickLocation, int, VTK_TICKS_INSIDE, VTK_TICKS_BOTH);
  vtkGetMacro(TickLocation, int);
  ///@}

  void SetTickLocationToInside() { this->SetTickLocation(VTK_TICKS_INSIDE); }
  void SetTickLocationToOutside() { this->SetTickLocation(VTK_TICKS_OUTSIDE); }
  void SetTickLocationToBoth() { this->SetTickLocation(VTK_TICKS_BOTH); }

  void SetLabelScaling(bool, int, int, int);

  ///@{
  /**
   * Use or not vtkTextActor3D for titles and labels.
   * See Also:
   * vtkAxisActor::SetUseTextActor3D(), vtkAxisActor::GetUseTextActor3D()
   */
  void SetUseTextActor3D(bool enable);
  bool GetUseTextActor3D();
  ///@}

  ///@{
  /**
   * Get/Set 2D mode
   * NB: Use vtkTextActor for titles in 2D instead of vtkAxisFollower
   */
  void SetUse2DMode(bool enable);
  bool GetUse2DMode();
  ///@}

  /**
   * For 2D mode only: save axis title positions for later use
   */
  void SetSaveTitlePosition(int val);

  ///@{
  /**
   * Provide an oriented bounded box when using AxisBaseFor.
   * Default: (-1, 1, -1, 1, -1, 1).
   */
  vtkSetVector6Macro(OrientedBounds, double);
  vtkGetVector6Macro(OrientedBounds, double);
  ///@}

  ///@{
  /**
   * Enable/Disable the usage of the OrientedBounds.
   * Default: false.
   */
  vtkSetMacro(UseOrientedBounds, bool);
  vtkGetMacro(UseOrientedBounds, bool);
  ///@}

  ///@{
  /**
   * Vector that should be use as the base for X.
   * Default: (1.0, 0.0, 0.0).
   */
  vtkSetVector3Macro(AxisBaseForX, double);
  vtkGetVector3Macro(AxisBaseForX, double);
  ///@}

  ///@{
  /**
   * Vector that should be use as the base for Y.
   * Default: (0.0, 1.0, 0.0).
   */
  vtkSetVector3Macro(AxisBaseForY, double);
  vtkGetVector3Macro(AxisBaseForY, double);
  ///@}

  ///@{
  /**
   * Vector that should be use as the base for Z.
   * Default: (0.0, 0.0, 1.0).
   */
  vtkSetVector3Macro(AxisBaseForZ, double);
  vtkGetVector3Macro(AxisBaseForZ, double);
  ///@}

  ///@{
  /**
   * Provide a custom AxisOrigin. This point must be inside the bounding box and
   * will represent the point where the 3 axes will intersect.
   * Default: (0.0, 0.0, 0.0).
   */
  vtkSetVector3Macro(AxisOrigin, double);
  vtkGetVector3Macro(AxisOrigin, double);
  ///@}

  ///@{
  /**
   * Enable/Disable the usage of the AxisOrigin.
   * Default: false.
   */
  vtkSetMacro(UseAxisOrigin, bool);
  vtkGetMacro(UseAxisOrigin, bool);
  ///@}

  ///@{
  /**
   * Specify the mode in which the cube axes should render its gridLines.
   * Default: VTK_GRID_LINES_ALL.
   */
  vtkSetMacro(GridLineLocation, int);
  vtkGetMacro(GridLineLocation, int);
  ///@}

  ///@{
  /**
   * Enable/Disable axis stickiness. When on, the axes will be adjusted to always
   * be visible in the viewport unless the original bounds of the axes are entirely
   * outside the viewport.
   * Default: false.
   */
  vtkSetMacro(StickyAxes, bool);
  vtkGetMacro(StickyAxes, bool);
  vtkBooleanMacro(StickyAxes, bool);
  ///@}

  ///@{
  /**
   * Enable/Disable centering of axes when the Sticky option is
   * on. If on, the axes bounds will be centered in the
   * viewport. Otherwise, the axes can move about the longer of the
   * horizontal or vertical directions of the viewport to follow the
   * data.
   * Default: true.
   */
  vtkSetMacro(CenterStickyAxes, bool);
  vtkGetMacro(CenterStickyAxes, bool);
  vtkBooleanMacro(CenterStickyAxes, bool);
  ///@}

  enum GridVisibility
  {
    VTK_GRID_LINES_ALL = 0,
    VTK_GRID_LINES_CLOSEST = 1,
    VTK_GRID_LINES_FURTHEST = 2
  };

protected:
  vtkCubeAxesActor();
  ~vtkCubeAxesActor() override;

  /**
   * Computes a bounding sphere used to determine the sticky bounding box.
   * Sphere center and sphere radius are return parameters and can remain uninitialized
   * prior to calling this method.
   */
  void ComputeStickyAxesBoundingSphere(
    vtkViewport* viewport, const double bounds[6], double sphereCenter[3], double& sphereRadius);

  /**
   * Get bounds such that the axes are entirely within a viewport
   */
  void GetViewportLimitedBounds(vtkViewport* viewport, double bounds[6]);

  /**
   * Get the bits for a bounds point. 0 means the lower side for a
   * coordinate, 1 means the higher side.
   */
  static void GetBoundsPointBits(
    unsigned int pointIndex, unsigned int& xBit, unsigned int& yBit, unsigned int& zBit);

  /**
   * Get a point on the bounding box by point index
   */
  static void GetBoundsPoint(unsigned int pointIndex, const double bounds[6], double point[3]);

  int LabelExponent(double min, double max);

  int Digits(double min, double max);

  double MaxOf(double, double);
  double MaxOf(double, double, double, double);

  double FFix(double);
  double FSign(double, double);
  int FRound(double fnt);
  int GetNumTicks(double range, double fxt);

  void UpdateLabels(vtkAxisActor** axis, int index);

  vtkSmartPointer<vtkCamera> Camera;

  int FlyMode = VTK_FLY_CLOSEST_TRIAD;

  // Expose internally closest axis index computation
  int FindClosestAxisIndex(double pts[8][3]);

  // Expose internally furthest axis index computation
  int FindFurtherstAxisIndex(double pts[8][3]);

  // Expose internally the boundary edge fly mode axis index computation
  void FindBoundaryEdge(int& indexOfAxisX, int& indexOfAxisY, int& indexOfAxisZ, double pts[8][3]);

  /**
   * This will Update AxisActors with GridVisibility when those should be
   * dynamaic regarding the viewport.
   * GridLineLocation = [VTK_CLOSEST_GRID_LINES, VTK_FURTHEST_GRID_LINES]
   */
  void UpdateGridLineVisibility(int axisIndex);

  // VTK_ALL_GRID_LINES      0
  // VTK_CLOSEST_GRID_LINES  1
  // VTK_FURTHEST_GRID_LINES 2
  int GridLineLocation = VTK_GRID_LINES_ALL;

  /**
   * Flag for axes stickiness
   */
  bool StickyAxes = false;

  /**
   * Flag for centering sticky axes
   */
  bool CenterStickyAxes = true;

  /**
   * If enabled the actor will not be visible at a certain distance from the camera.
   * Default is true
   */
  bool EnableDistanceLOD = true;

  /**
   * Default is 0.80
   * This determines at what fraction of camera far clip range, actor is not visible.
   */
  double DistanceLODThreshold = 0.8;

  /**
   * If enabled the actor will not be visible at a certain view angle.
   * Default is true.
   */
  bool EnableViewAngleLOD = true;

  /**
   * This determines at what view angle to geometry will make the geometry not visible.
   * Default is 0.3.
   */
  double ViewAngleLODThreshold = 0.2;

  enum NumberOfAlignedAxis
  {
    NUMBER_OF_ALIGNED_AXIS = 4
  };

  ///@{
  /**
   * Control variables for all axes
   * NB: [0] always for 'Major' axis during non-static fly modes.
   */
  vtkAxisActor* XAxes[NUMBER_OF_ALIGNED_AXIS];
  vtkAxisActor* YAxes[NUMBER_OF_ALIGNED_AXIS];
  vtkAxisActor* ZAxes[NUMBER_OF_ALIGNED_AXIS];
  ///@}

  bool RebuildAxes = true;

  char* XTitle = nullptr;
  char* XUnits = nullptr;
  char* YTitle = nullptr;
  char* YUnits = nullptr;
  char* ZTitle = nullptr;
  char* ZUnits = nullptr;

  char* ActualXLabel = nullptr;
  char* ActualYLabel = nullptr;
  char* ActualZLabel = nullptr;

  int TickLocation = VTK_TICKS_INSIDE;

  bool XAxisVisibility = true;
  bool YAxisVisibility = true;
  bool ZAxisVisibility = true;

  bool XAxisTickVisibility = true;
  bool YAxisTickVisibility = true;
  bool ZAxisTickVisibility = true;

  bool XAxisMinorTickVisibility = true;
  bool YAxisMinorTickVisibility = true;
  bool ZAxisMinorTickVisibility = true;

  bool XAxisLabelVisibility = true;
  bool YAxisLabelVisibility = true;
  bool ZAxisLabelVisibility = true;

  bool DrawXGridlines = false;
  bool DrawYGridlines = false;
  bool DrawZGridlines = false;

  bool DrawXInnerGridlines = false;
  bool DrawYInnerGridlines = false;
  bool DrawZInnerGridlines = false;

  bool DrawXGridpolys = false;
  bool DrawYGridpolys = false;
  bool DrawZGridpolys = false;

  char* XLabelFormat = nullptr;
  char* YLabelFormat = nullptr;
  char* ZLabelFormat = nullptr;

  double CornerOffset = 0.0;

  int Inertia = 1;

  int RenderCount = 0;

  int InertiaLocs[3] = { -1, -1, -1 };

  bool RenderSomething = false;

  vtkNew<vtkTextProperty> TitleTextProperty[3];
  vtkSmartPointer<vtkStringArray> AxisLabels[3];
  vtkNew<vtkTextProperty> LabelTextProperty[3];

  vtkNew<vtkProperty> XAxesLinesProperty;
  vtkNew<vtkProperty> YAxesLinesProperty;
  vtkNew<vtkProperty> ZAxesLinesProperty;
  vtkNew<vtkProperty> XAxesGridlinesProperty;
  vtkNew<vtkProperty> YAxesGridlinesProperty;
  vtkNew<vtkProperty> ZAxesGridlinesProperty;
  vtkNew<vtkProperty> XAxesInnerGridlinesProperty;
  vtkNew<vtkProperty> YAxesInnerGridlinesProperty;
  vtkNew<vtkProperty> ZAxesInnerGridlinesProperty;
  vtkNew<vtkProperty> XAxesGridpolysProperty;
  vtkNew<vtkProperty> YAxesGridpolysProperty;
  vtkNew<vtkProperty> ZAxesGridpolysProperty;

  double RenderedBounds[6] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  double OrientedBounds[6] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  bool UseOrientedBounds = false;

  double AxisOrigin[3] = { 0.0, 0.0, 0.0 };
  bool UseAxisOrigin = false;

  double AxisBaseForX[3] = { 1.0, 0.0, 0.0 };
  double AxisBaseForY[3] = { 0.0, 1.0, 0.0 };
  double AxisBaseForZ[3] = { 0.0, 0.0, 1.0 };

private:
  vtkCubeAxesActor(const vtkCubeAxesActor&) = delete;
  void operator=(const vtkCubeAxesActor&) = delete;

  vtkSetStringMacro(ActualXLabel);
  vtkSetStringMacro(ActualYLabel);
  vtkSetStringMacro(ActualZLabel);

  vtkTimeStamp BuildTime;
  bool LastUseOrientedBounds = false;
  int LastXPow = 0;
  int LastYPow = 0;
  int LastZPow = 0;

  int UserXPow = 0;
  int UserYPow = 0;
  int UserZPow = 0;

  bool AutoLabelScaling = true;

  int LastXAxisDigits = 3;
  int LastYAxisDigits = 3;
  int LastZAxisDigits = 3;

  double LastXRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
  double LastYRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
  double LastZRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
  double LastBounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };

  int LastFlyMode = -1;

  int RenderAxesX[NUMBER_OF_ALIGNED_AXIS] = { 0, 1, 2, 3 };
  int RenderAxesY[NUMBER_OF_ALIGNED_AXIS] = { 0, 1, 2, 3 };
  int RenderAxesZ[NUMBER_OF_ALIGNED_AXIS] = { 0, 1, 2, 3 };

  int NumberOfAxesX = 1;
  int NumberOfAxesY = 1;
  int NumberOfAxesZ = 1;

  bool MustAdjustXValue = false;
  bool MustAdjustYValue = false;
  bool MustAdjustZValue = false;

  bool ForceXLabelReset = false;
  bool ForceYLabelReset = false;
  bool ForceZLabelReset = false;

  double XAxisRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
  double YAxisRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
  double ZAxisRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };

  double LabelScale = -1.0;
  double TitleScale = -1.0;

  double ScreenSize = 10.0;
  double LabelOffset = 20.0;
  double TitleOffset[2] = { 20.0, 20.0 };

  ///@{
  /**
   * Major start and delta values, in each direction.
   * These values are needed for inner grid lines generation
   */
  double MajorStart[3] = { 0.0, 0.0, 0.0 };
  double DeltaMajor[3] = { 0.0, 0.0, 0.0 };
  ///@}

  int RenderGeometry(bool& initialRender, vtkViewport* viewport, bool checkAxisVisibility,
    int (vtkAxisActor::*renderMethod)(vtkViewport*));

  void TransformBounds(vtkViewport* viewport, const double bounds[6], double pts[8][3]);
  void AdjustAxes(double bounds[6], double xCoords[NUMBER_OF_ALIGNED_AXIS][6],
    double yCoords[NUMBER_OF_ALIGNED_AXIS][6], double zCoords[NUMBER_OF_ALIGNED_AXIS][6],
    double xRange[2], double yRange[2], double zRange[2]);

  bool ComputeTickSize(double bounds[6]);
  void AdjustValues(const double xRange[2], const double yRange[2], const double zRange[2]);
  void AdjustRange(const double bounds[6]);
  void BuildAxes(vtkViewport*);
  void DetermineRenderAxes(vtkViewport*);
  void SetNonDependentAttributes();
  void BuildLabels(vtkAxisActor* axes[NUMBER_OF_ALIGNED_AXIS]);
  void AdjustTicksComputeRange(
    vtkAxisActor* axes[NUMBER_OF_ALIGNED_AXIS], double rangeMin, double rangeMax);

  void AutoScale(vtkViewport* viewport);
  void AutoScale(vtkViewport* viewport, vtkAxisActor* axes[NUMBER_OF_ALIGNED_AXIS]);
  double AutoScale(vtkViewport* viewport, double screenSize, double position[3]);
};

VTK_ABI_NAMESPACE_END
#endif
