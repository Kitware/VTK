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

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor.h"

class vtkAxisActor;
class vtkCamera;
class vtkTextProperty;
class vtkStringArray;

class VTKRENDERINGANNOTATION_EXPORT vtkCubeAxesActor : public vtkActor
{
public:
  vtkTypeMacro(vtkCubeAxesActor,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate object with label format "6.3g" and the number of labels
   * per axis set to 3.
   */
  static vtkCubeAxesActor *New();

  //@{
  /**
   * Draw the axes as per the vtkProp superclass' API.
   */
  int RenderOpaqueGeometry(vtkViewport*) override;
  virtual int RenderTranslucentGeometry(vtkViewport*);
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  int RenderOverlay(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Gets/Sets the RebuildAxes flag
   */
  vtkSetMacro( RebuildAxes, bool );
  vtkGetMacro( RebuildAxes, bool );
  //@}

  //@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds is used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   */
  vtkSetVector6Macro(Bounds,double);
  using Superclass::GetBounds;
  double *GetBounds() VTK_SIZEHINT(6) override { return this->Bounds; }
  //@}

  //@{
  /**
   * Method used to properly return the bounds of the cube axis itself with all
   * its labels.
   */
  virtual void GetRenderedBounds(double rBounds[6]);
  virtual double* GetRenderedBounds();
  //@}

  //@{
  /**
   * Explicitly specify the range of each axes that's used to define the prop.
   * The default, (if you do not use these methods) is to use the bounds
   * specified, or use the bounds of the Input Prop if one is specified. This
   * method allows you to separate the notion of extent of the axes in physical
   * space (bounds) and the extent of the values it represents. In other words,
   * you can have the ticks and labels show a different range.
   */
  vtkSetVector2Macro( XAxisRange, double );
  vtkSetVector2Macro( YAxisRange, double );
  vtkSetVector2Macro( ZAxisRange, double );
  vtkGetVector2Macro( XAxisRange, double );
  vtkGetVector2Macro( YAxisRange, double );
  //@}
  //@{
  /**
   * Explicitly specify the axis labels along an axis as an array of strings
   * instead of using the values.
   */
  vtkStringArray* GetAxisLabels(int axis);
  void SetAxisLabels(int axis, vtkStringArray* value);
  //@}

  vtkGetVector2Macro( ZAxisRange, double );

  //@{
  /**
   * Explicitly specify the screen size of title and label text.
   * ScreenSize determines the size of the text in terms of screen
   * pixels. Default is 10.0.
   */
  void SetScreenSize(double screenSize);
  vtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * Explicitly specify the distance between labels and the axis.
   * Default is 20.0.
   */
  void SetLabelOffset(double offset);
  vtkGetMacro(LabelOffset, double);
  //@}

  //@{
  /**
   * Explicitly specify the distance between title and labels.
   * Default is 20.0.
   */
  void SetTitleOffset(double offset);
  vtkGetMacro(TitleOffset, double);
  //@}

  //@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * vtkCubeAxesActor.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera,vtkCamera);
  //@}

  enum FlyMode
  {
    VTK_FLY_OUTER_EDGES = 0,
    VTK_FLY_CLOSEST_TRIAD = 1,
    VTK_FLY_FURTHEST_TRIAD = 2,
    VTK_FLY_STATIC_TRIAD = 3,
    VTK_FLY_STATIC_EDGES = 4
  };

  //@{
  /**
   * Specify a mode to control how the axes are drawn: either static,
   * closest triad, furthest triad or outer edges in relation to the
   * camera position.
   */
  vtkSetClampMacro(FlyMode, int, VTK_FLY_OUTER_EDGES, VTK_FLY_STATIC_EDGES);
  vtkGetMacro(FlyMode, int);
  void SetFlyModeToOuterEdges()
    {this->SetFlyMode(VTK_FLY_OUTER_EDGES);};
  void SetFlyModeToClosestTriad()
    {this->SetFlyMode(VTK_FLY_CLOSEST_TRIAD);};
  void SetFlyModeToFurthestTriad()
    {this->SetFlyMode(VTK_FLY_FURTHEST_TRIAD);};
  void SetFlyModeToStaticTriad()
    {this->SetFlyMode(VTK_FLY_STATIC_TRIAD);};
  void SetFlyModeToStaticEdges()
    {this->SetFlyMode(VTK_FLY_STATIC_EDGES);};
  //@}

  //@{
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
  //@}

  //@{
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
  //@}

  //@{
  /**
   * Set/Get the inertial factor that controls how often (i.e, how
   * many renders) the axes can switch position (jump from one axes
   * to another).
   */
  vtkSetClampMacro(Inertia, int, 1, VTK_INT_MAX);
  vtkGetMacro(Inertia, int);
  //@}

  //@{
  /**
   * Specify an offset value to "pull back" the axes from the corner at
   * which they are joined to avoid overlap of axes labels. The
   * "CornerOffset" is the fraction of the axis length to pull back.
   */
  vtkSetMacro(CornerOffset, double);
  vtkGetMacro(CornerOffset, double);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override;

  //@{
  /**
   * Enable and disable the use of distance based LOD for titles and labels.
   */
  vtkSetMacro( EnableDistanceLOD, int );
  vtkGetMacro( EnableDistanceLOD, int );
  //@}

  //@{
  /**
   * Set distance LOD threshold [0.0 - 1.0] for titles and labels.
   */
  vtkSetClampMacro( DistanceLODThreshold, double, 0.0, 1.0 );
  vtkGetMacro( DistanceLODThreshold, double);
  //@}

  //@{
  /**
   * Enable and disable the use of view angle based LOD for titles and labels.
   */
  vtkSetMacro( EnableViewAngleLOD, int );
  vtkGetMacro( EnableViewAngleLOD, int );
  //@}

  //@{
  /**
   * Set view angle LOD threshold [0.0 - 1.0] for titles and labels.
   */
  vtkSetClampMacro( ViewAngleLODThreshold, double, 0., 1. );
  vtkGetMacro( ViewAngleLODThreshold, double );
  //@}

  //@{
  /**
   * Turn on and off the visibility of each axis.
   */
  vtkSetMacro(XAxisVisibility,vtkTypeBool);
  vtkGetMacro(XAxisVisibility,vtkTypeBool);
  vtkBooleanMacro(XAxisVisibility,vtkTypeBool);
  vtkSetMacro(YAxisVisibility,vtkTypeBool);
  vtkGetMacro(YAxisVisibility,vtkTypeBool);
  vtkBooleanMacro(YAxisVisibility,vtkTypeBool);
  vtkSetMacro(ZAxisVisibility,vtkTypeBool);
  vtkGetMacro(ZAxisVisibility,vtkTypeBool);
  vtkBooleanMacro(ZAxisVisibility,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of labels for each axis.
   */
  vtkSetMacro(XAxisLabelVisibility,vtkTypeBool);
  vtkGetMacro(XAxisLabelVisibility,vtkTypeBool);
  vtkBooleanMacro(XAxisLabelVisibility,vtkTypeBool);
  //@}

  vtkSetMacro(YAxisLabelVisibility,vtkTypeBool);
  vtkGetMacro(YAxisLabelVisibility,vtkTypeBool);
  vtkBooleanMacro(YAxisLabelVisibility,vtkTypeBool);

  vtkSetMacro(ZAxisLabelVisibility,vtkTypeBool);
  vtkGetMacro(ZAxisLabelVisibility,vtkTypeBool);
  vtkBooleanMacro(ZAxisLabelVisibility,vtkTypeBool);

  //@{
  /**
   * Turn on and off the visibility of ticks for each axis.
   */
  vtkSetMacro(XAxisTickVisibility,vtkTypeBool);
  vtkGetMacro(XAxisTickVisibility,vtkTypeBool);
  vtkBooleanMacro(XAxisTickVisibility,vtkTypeBool);
  //@}

  vtkSetMacro(YAxisTickVisibility,vtkTypeBool);
  vtkGetMacro(YAxisTickVisibility,vtkTypeBool);
  vtkBooleanMacro(YAxisTickVisibility,vtkTypeBool);

  vtkSetMacro(ZAxisTickVisibility,vtkTypeBool);
  vtkGetMacro(ZAxisTickVisibility,vtkTypeBool);
  vtkBooleanMacro(ZAxisTickVisibility,vtkTypeBool);

  //@{
  /**
   * Turn on and off the visibility of minor ticks for each axis.
   */
  vtkSetMacro(XAxisMinorTickVisibility,vtkTypeBool);
  vtkGetMacro(XAxisMinorTickVisibility,vtkTypeBool);
  vtkBooleanMacro(XAxisMinorTickVisibility,vtkTypeBool);
  //@}

  vtkSetMacro(YAxisMinorTickVisibility,vtkTypeBool);
  vtkGetMacro(YAxisMinorTickVisibility,vtkTypeBool);
  vtkBooleanMacro(YAxisMinorTickVisibility,vtkTypeBool);

  vtkSetMacro(ZAxisMinorTickVisibility,vtkTypeBool);
  vtkGetMacro(ZAxisMinorTickVisibility,vtkTypeBool);
  vtkBooleanMacro(ZAxisMinorTickVisibility,vtkTypeBool);

  vtkSetMacro(DrawXGridlines,vtkTypeBool);
  vtkGetMacro(DrawXGridlines,vtkTypeBool);
  vtkBooleanMacro(DrawXGridlines,vtkTypeBool);

  vtkSetMacro(DrawYGridlines,vtkTypeBool);
  vtkGetMacro(DrawYGridlines,vtkTypeBool);
  vtkBooleanMacro(DrawYGridlines,vtkTypeBool);

  vtkSetMacro(DrawZGridlines,vtkTypeBool);
  vtkGetMacro(DrawZGridlines,vtkTypeBool);
  vtkBooleanMacro(DrawZGridlines,vtkTypeBool);

  vtkSetMacro(DrawXInnerGridlines,vtkTypeBool);
  vtkGetMacro(DrawXInnerGridlines,vtkTypeBool);
  vtkBooleanMacro(DrawXInnerGridlines,vtkTypeBool);

  vtkSetMacro(DrawYInnerGridlines,vtkTypeBool);
  vtkGetMacro(DrawYInnerGridlines,vtkTypeBool);
  vtkBooleanMacro(DrawYInnerGridlines,vtkTypeBool);

  vtkSetMacro(DrawZInnerGridlines,vtkTypeBool);
  vtkGetMacro(DrawZInnerGridlines,vtkTypeBool);
  vtkBooleanMacro(DrawZInnerGridlines,vtkTypeBool);

  vtkSetMacro(DrawXGridpolys,vtkTypeBool);
  vtkGetMacro(DrawXGridpolys,vtkTypeBool);
  vtkBooleanMacro(DrawXGridpolys,vtkTypeBool);

  vtkSetMacro(DrawYGridpolys,vtkTypeBool);
  vtkGetMacro(DrawYGridpolys,vtkTypeBool);
  vtkBooleanMacro(DrawYGridpolys,vtkTypeBool);

  vtkSetMacro(DrawZGridpolys,vtkTypeBool);
  vtkGetMacro(DrawZGridpolys,vtkTypeBool);
  vtkBooleanMacro(DrawZGridpolys,vtkTypeBool);

  /**
   * Returns the text property for the title on an axis.
   */
  vtkTextProperty *GetTitleTextProperty(int);

  /**
   * Returns the text property for the labels on an axis.
   */
  vtkTextProperty *GetLabelTextProperty(int);

  //@{
  /**
   * Get/Set axes actors properties.
   */
  void SetXAxesLinesProperty(vtkProperty *);
  vtkProperty* GetXAxesLinesProperty();
  void SetYAxesLinesProperty(vtkProperty *);
  vtkProperty* GetYAxesLinesProperty();
  void SetZAxesLinesProperty(vtkProperty *);
  vtkProperty* GetZAxesLinesProperty();
  //@}

  //@{
  /**
   * Get/Set axes (outer) gridlines actors properties.
   */
  void SetXAxesGridlinesProperty(vtkProperty *);
  vtkProperty* GetXAxesGridlinesProperty();
  void SetYAxesGridlinesProperty(vtkProperty *);
  vtkProperty* GetYAxesGridlinesProperty();
  void SetZAxesGridlinesProperty(vtkProperty *);
  vtkProperty* GetZAxesGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set axes inner gridlines actors properties.
   */
  void SetXAxesInnerGridlinesProperty(vtkProperty *);
  vtkProperty* GetXAxesInnerGridlinesProperty();
  void SetYAxesInnerGridlinesProperty(vtkProperty *);
  vtkProperty* GetYAxesInnerGridlinesProperty();
  void SetZAxesInnerGridlinesProperty(vtkProperty *);
  vtkProperty* GetZAxesInnerGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set axes gridPolys actors properties.
   */
  void SetXAxesGridpolysProperty(vtkProperty *);
  vtkProperty* GetXAxesGridpolysProperty();
  void SetYAxesGridpolysProperty(vtkProperty *);
  vtkProperty* GetYAxesGridpolysProperty();
  void SetZAxesGridpolysProperty(vtkProperty *);
  vtkProperty* GetZAxesGridpolysProperty();
  //@}

  enum TickLocation
  {
    VTK_TICKS_INSIDE = 0,
    VTK_TICKS_OUTSIDE = 1,
    VTK_TICKS_BOTH = 2
  };

  //@{
  /**
   * Set/Get the location of ticks marks.
   */
  vtkSetClampMacro(TickLocation, int, VTK_TICKS_INSIDE, VTK_TICKS_BOTH);
  vtkGetMacro(TickLocation, int);
  //@}

  void SetTickLocationToInside(void)
    { this->SetTickLocation(VTK_TICKS_INSIDE); };
  void SetTickLocationToOutside(void)
    { this->SetTickLocation(VTK_TICKS_OUTSIDE); };
  void SetTickLocationToBoth(void)
    { this->SetTickLocation(VTK_TICKS_BOTH); };

  void SetLabelScaling(bool, int, int, int);

  //@{
  /**
   * Use or not vtkTextActor3D for titles and labels.
   * See Also:
   * vtkAxisActor::SetUseTextActor3D(), vtkAxisActor::GetUseTextActor3D()
   */
  void SetUseTextActor3D( int val );
  int GetUseTextActor3D();
  //@}

  //@{
  /**
   * Get/Set 2D mode
   * NB: Use vtkTextActor for titles in 2D instead of vtkAxisFollower
   */
  void SetUse2DMode( int val );
  int GetUse2DMode();
  //@}

  /**
   * For 2D mode only: save axis title positions for later use
   */
  void SetSaveTitlePosition( int val );

  //@{
  /**
   * Provide an oriented bounded box when using AxisBaseFor.
   */
  vtkSetVector6Macro(OrientedBounds,double);
  vtkGetVector6Macro(OrientedBounds, double);
  //@}

  //@{
  /**
   * Enable/Disable the usage of the OrientedBounds
   */
  vtkSetMacro(UseOrientedBounds, int);
  vtkGetMacro(UseOrientedBounds, int);
  //@}

  //@{
  /**
   * Vector that should be use as the base for X
   */
  vtkSetVector3Macro(AxisBaseForX,double);
  vtkGetVector3Macro(AxisBaseForX, double);
  //@}

  //@{
  /**
   * Vector that should be use as the base for Y
   */
  vtkSetVector3Macro(AxisBaseForY,double);
  vtkGetVector3Macro(AxisBaseForY, double);
  //@}

  //@{
  /**
   * Vector that should be use as the base for Z
   */
  vtkSetVector3Macro(AxisBaseForZ,double);
  vtkGetVector3Macro(AxisBaseForZ, double);
  //@}

  //@{
  /**
   * Provide a custom AxisOrigin. This point must be inside the bounding box and
   * will represent the point where the 3 axes will intersect
   */
  vtkSetVector3Macro(AxisOrigin,double);
  vtkGetVector3Macro(AxisOrigin, double);
  //@}

  //@{
  /**
   * Enable/Disable the usage of the AxisOrigin
   */
  vtkSetMacro(UseAxisOrigin, int);
  vtkGetMacro(UseAxisOrigin, int);
  //@}

  //@{
  /**
   * Specify the mode in which the cube axes should render its gridLines
   */
  vtkSetMacro(GridLineLocation,int);
  vtkGetMacro(GridLineLocation,int);
  //@}

  //@{
  /**
   * Enable/Disable axis stickiness. When on, the axes will be adjusted to always
   * be visible in the viewport unless the original bounds of the axes are entirely
   * outside the viewport. Defaults to off.
   */
  vtkSetMacro(StickyAxes,vtkTypeBool);
  vtkGetMacro(StickyAxes,vtkTypeBool);
  vtkBooleanMacro(StickyAxes,vtkTypeBool);
  //@}

  //@{
  /**
   * Enable/Disable centering of axes when the Sticky option is
   * on. If on, the axes bounds will be centered in the
   * viewport. Otherwise, the axes can move about the longer of the
   * horizontal or verical directions of the viewport to follow the
   * data. Defaults to on.
   */
  vtkSetMacro(CenterStickyAxes,vtkTypeBool);
  vtkGetMacro(CenterStickyAxes,vtkTypeBool);
  vtkBooleanMacro(CenterStickyAxes,vtkTypeBool);
  //@}

  enum GridVisibility
  {
    VTK_GRID_LINES_ALL = 0,
    VTK_GRID_LINES_CLOSEST = 1,
    VTK_GRID_LINES_FURTHEST =  2
  };

protected:
  vtkCubeAxesActor();
  ~vtkCubeAxesActor() override;

  /**
   * Computes a bounding sphere used to determine the sticky bounding box.
   * Sphere center and sphere radius are return parameters and can remain uninitialized
   * prior to calling this method.
   */
  void ComputeStickyAxesBoundingSphere(vtkViewport* viewport, const double bounds[6],
                                       double sphereCenter[3], double & sphereRadius);

  /**
   * Get bounds such that the axes are entirely within a viewport
   */
  void GetViewportLimitedBounds(vtkViewport* viewport, double bounds[6]);

  /**
   * Get the bits for a bounds point. 0 means the lower side for a
   * coordinate, 1 means the higher side.
   */
  static void GetBoundsPointBits(unsigned int pointIndex,
                                 unsigned int & xBit,
                                 unsigned int & yBit,
                                 unsigned int & zBit);

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
  int FRound( double fnt );
  int GetNumTicks( double range, double fxt);

  void UpdateLabels(vtkAxisActor **axis, int index);

  vtkCamera *Camera;

  int FlyMode;

  // Expose internally closest axis index computation
  int FindClosestAxisIndex(double pts[8][3]);

  // Expose internally furthest axis index computation
  int FindFurtherstAxisIndex(double pts[8][3]);

  // Expose internally the boundary edge fly mode axis index computation
  void FindBoundaryEdge(int &indexOfAxisX, int &indexOfAxisY, int &indexOfAxisZ,
                        double pts[8][3]);

  /**
   * This will Update AxisActors with GridVisibility when those should be
   * dynamaic regarding the viewport.
   * GridLineLocation = [VTK_CLOSEST_GRID_LINES, VTK_FURTHEST_GRID_LINES]
   */
  void UpdateGridLineVisibility(int axisIndex);

  // VTK_ALL_GRID_LINES      0
  // VTK_CLOSEST_GRID_LINES  1
  // VTK_FURTHEST_GRID_LINES 2
  int GridLineLocation;

  /**
   * Flag for axes stickiness
   */
  vtkTypeBool StickyAxes;

  /**
   * Flag for centering sticky axes
   */
  vtkTypeBool CenterStickyAxes;

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

  enum NumberOfAlignedAxis
  {
    NUMBER_OF_ALIGNED_AXIS = 4
  };

  //@{
  /**
   * Control variables for all axes
   * NB: [0] always for 'Major' axis during non-static fly modes.
   */
  vtkAxisActor *XAxes[NUMBER_OF_ALIGNED_AXIS];
  vtkAxisActor *YAxes[NUMBER_OF_ALIGNED_AXIS];
  vtkAxisActor *ZAxes[NUMBER_OF_ALIGNED_AXIS];
  //@}

  bool RebuildAxes;

  char *XTitle;
  char *XUnits;
  char *YTitle;
  char *YUnits;
  char *ZTitle;
  char *ZUnits;

  char *ActualXLabel;
  char *ActualYLabel;
  char *ActualZLabel;

  int TickLocation;

  vtkTypeBool XAxisVisibility;
  vtkTypeBool YAxisVisibility;
  vtkTypeBool ZAxisVisibility;

  vtkTypeBool XAxisTickVisibility;
  vtkTypeBool YAxisTickVisibility;
  vtkTypeBool ZAxisTickVisibility;

  vtkTypeBool XAxisMinorTickVisibility;
  vtkTypeBool YAxisMinorTickVisibility;
  vtkTypeBool ZAxisMinorTickVisibility;

  vtkTypeBool XAxisLabelVisibility;
  vtkTypeBool YAxisLabelVisibility;
  vtkTypeBool ZAxisLabelVisibility;

  vtkTypeBool DrawXGridlines;
  vtkTypeBool DrawYGridlines;
  vtkTypeBool DrawZGridlines;

  vtkTypeBool DrawXInnerGridlines;
  vtkTypeBool DrawYInnerGridlines;
  vtkTypeBool DrawZInnerGridlines;

  vtkTypeBool DrawXGridpolys;
  vtkTypeBool DrawYGridpolys;
  vtkTypeBool DrawZGridpolys;

  char  *XLabelFormat;
  char  *YLabelFormat;
  char  *ZLabelFormat;

  double CornerOffset;

  int   Inertia;

  int   RenderCount;

  int   InertiaLocs[3];

  int RenderSomething;

  vtkTextProperty* TitleTextProperty[3];
  vtkStringArray* AxisLabels[3];

  vtkTextProperty* LabelTextProperty[3];

  vtkProperty  *XAxesLinesProperty;
  vtkProperty  *YAxesLinesProperty;
  vtkProperty  *ZAxesLinesProperty;
  vtkProperty  *XAxesGridlinesProperty;
  vtkProperty  *YAxesGridlinesProperty;
  vtkProperty  *ZAxesGridlinesProperty;
  vtkProperty  *XAxesInnerGridlinesProperty;
  vtkProperty  *YAxesInnerGridlinesProperty;
  vtkProperty  *ZAxesInnerGridlinesProperty;
  vtkProperty  *XAxesGridpolysProperty;
  vtkProperty  *YAxesGridpolysProperty;
  vtkProperty  *ZAxesGridpolysProperty;

  double RenderedBounds[6];
  double OrientedBounds[6];
  int UseOrientedBounds;

  double AxisOrigin[3];
  int UseAxisOrigin;

  double AxisBaseForX[3];
  double AxisBaseForY[3];
  double AxisBaseForZ[3];

private:
  vtkCubeAxesActor(const vtkCubeAxesActor&) = delete;
  void operator=(const vtkCubeAxesActor&) = delete;

  vtkSetStringMacro(ActualXLabel);
  vtkSetStringMacro(ActualYLabel);
  vtkSetStringMacro(ActualZLabel);

  vtkTimeStamp BuildTime;
  int LastUseOrientedBounds;
  int LastXPow;
  int LastYPow;
  int LastZPow;

  int UserXPow;
  int UserYPow;
  int UserZPow;

  bool AutoLabelScaling;

  int LastXAxisDigits;
  int LastYAxisDigits;
  int LastZAxisDigits;

  double LastXRange[2];
  double LastYRange[2];
  double LastZRange[2];
  double LastBounds[6];

  int    LastFlyMode;

  int   RenderAxesX[NUMBER_OF_ALIGNED_AXIS];
  int   RenderAxesY[NUMBER_OF_ALIGNED_AXIS];
  int   RenderAxesZ[NUMBER_OF_ALIGNED_AXIS];

  int   NumberOfAxesX;
  int   NumberOfAxesY;
  int   NumberOfAxesZ;

  bool MustAdjustXValue;
  bool MustAdjustYValue;
  bool MustAdjustZValue;

  bool ForceXLabelReset;
  bool ForceYLabelReset;
  bool ForceZLabelReset;

  double XAxisRange[2];
  double YAxisRange[2];
  double ZAxisRange[2];

  double LabelScale;
  double TitleScale;

  double ScreenSize;
  double LabelOffset;
  double TitleOffset;

  //@{
  /**
   * Major start and delta values, in each direction.
   * These values are needed for inner grid lines generation
   */
  double MajorStart[3];
  double DeltaMajor[3];
  //@}

  int RenderGeometry(bool &initialRender, vtkViewport *viewport, bool checkAxisVisibility,int (vtkAxisActor::*renderMethod)(vtkViewport*));

  void  TransformBounds(vtkViewport *viewport, const double bounds[6],
                        double pts[8][3]);
  void  AdjustAxes(double bounds[6],
                   double xCoords[NUMBER_OF_ALIGNED_AXIS][6],
                   double yCoords[NUMBER_OF_ALIGNED_AXIS][6],
                   double zCoords[NUMBER_OF_ALIGNED_AXIS][6],
                   double xRange[2], double yRange[2], double zRange[2]);

  bool  ComputeTickSize(double bounds[6]);
  void  AdjustValues(const double xRange[2],
                     const double yRange[2],
                     const double zRange[2]);
  void  AdjustRange(const double bounds[6]);
  void  BuildAxes(vtkViewport *);
  void  DetermineRenderAxes(vtkViewport *);
  void  SetNonDependentAttributes(void);
  void  BuildLabels(vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS]);
  void  AdjustTicksComputeRange(vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS],
      double rangeMin, double rangeMax);

  void    AutoScale(vtkViewport *viewport);
  void    AutoScale(vtkViewport *viewport, vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS]);
  double  AutoScale(vtkViewport *viewport, double screenSize, double position[3]);
};


#endif
