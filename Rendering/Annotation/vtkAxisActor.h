/*=========================================================================
Program:   Visualization Toolkit
Module:    vtkAxisActor.h
Language:  C++

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
/**
 * @class   vtkAxisActor
 * @brief   Create an axis with tick marks and labels
 *
 * vtkAxisActor creates an axis with tick marks, labels, and/or a title,
 * depending on the particular instance variable settings. It is assumed that
 * the axes is part of a bounding box and is orthoganal to one of the
 * coordinate axes.  To use this class, you typically specify two points
 * defining the start and end points of the line (xyz definition using
 * vtkCoordinate class), the axis type (X, Y or Z), the axis location in
 * relation to the bounding box, the bounding box, the number of labels, and
 * the data range (min,max). You can also control what parts of the axis are
 * visible including the line, the tick marks, the labels, and the title. It
 * is also possible to control gridlines, and specifiy on which 'side' the
 * tickmarks are drawn (again with respect to the underlying assumed
 * bounding box). You can also specify the label format (a printf style format).
 *
 * This class decides how to locate the labels, and how to create reasonable
 * tick marks and labels.
 *
 * Labels follow the camera so as to be legible from any viewpoint.
 *
 * The instance variables Point1 and Point2 are instances of vtkCoordinate.
 * All calculations and references are in World Coordinates.
 *
 * @par Thanks:
 * This class was written by:
 * Hank Childs, Kathleen Bonnell, Amy Squillacote, Brad Whitlock,
 * Eric Brugger, Claire Guilbaud, Nicolas Dolegieviez, Will Schroeder,
 * Karthik Krishnan, Aashish Chaudhary, Philippe Pebay, David Gobbi,
 * David Partyka, Utkarsh Ayachit David Cole, Francois Bertel, and Mark Olesen
 * Part of this work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France.
 *
 * @sa
 * vtkActor vtkVectorText vtkPolyDataMapper vtkAxisActor2D vtkCoordinate
*/

#ifndef vtkAxisActor_h
#define vtkAxisActor_h

#include "vtkActor.h"
#include "vtkRenderingAnnotationModule.h" // For export macro

class vtkAxisFollower;
class vtkCamera;
class vtkCoordinate;
class vtkFollower;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProp3DAxisFollower;
class vtkProperty2D;
class vtkStringArray;
class vtkTextActor;
class vtkTextActor3D;
class vtkTextProperty;
class vtkVectorText;

class VTKRENDERINGANNOTATION_EXPORT vtkAxisActor : public vtkActor
{
public:
  vtkTypeMacro(vtkAxisActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Instantiate object.
   */
  static vtkAxisActor* New();

  //@{
  /**
   * Specify the position of the first point defining the axis.
   */
  virtual vtkCoordinate* GetPoint1Coordinate();
  virtual void SetPoint1(double x[3]) { this->SetPoint1(x[0], x[1], x[2]); }
  virtual void SetPoint1(double x, double y, double z);
  virtual double* GetPoint1();
  //@}

  //@{
  /**
   * Specify the position of the second point defining the axis.
   */
  virtual vtkCoordinate* GetPoint2Coordinate();
  virtual void SetPoint2(double x[3]) { this->SetPoint2(x[0], x[1], x[2]); }
  virtual void SetPoint2(double x, double y, double z);
  virtual double* GetPoint2();
  //@}

  //@{
  /**
   * Specify the (min,max) axis range. This will be used in the generation
   * of labels, if labels are visible.
   */
  vtkSetVector2Macro(Range, double);
  vtkGetVectorMacro(Range, double, 2);
  //@}

  //@{
  /**
   * Set or get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  void SetBounds(const double bounds[6]);
  void SetBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  double* GetBounds(void) VTK_OVERRIDE;
  void GetBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on the axis.
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Render text as polygons (vtkVectorText) or as sprites (vtkTextActor3D).
   * In 2D mode, the value is ignored and text is rendered as vtkTextActor.
   * False(0) by default.
   * See Also:
   * GetUse2DMode(), SetUse2DMode
   */
  vtkSetMacro(UseTextActor3D, int);
  vtkGetMacro(UseTextActor3D, int);
  //@}

  //@{
  /**
   * Set/Get the flag that controls whether the minor ticks are visible.
   */
  vtkSetMacro(MinorTicksVisible, int);
  vtkGetMacro(MinorTicksVisible, int);
  vtkBooleanMacro(MinorTicksVisible, int);
  //@}

  //@{
  /**
   * Set/Get the title of the axis actor,
   */
  void SetTitle(const char* t);
  vtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the common exponent of the labels values
   */
  void SetExponent(const char* t);
  vtkGetStringMacro(Exponent);
  //@}

  //@{
  /**
   * Set/Get the size of the major tick marks
   */
  vtkSetMacro(MajorTickSize, double);
  vtkGetMacro(MajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the major tick marks
   */
  vtkSetMacro(MinorTickSize, double);
  vtkGetMacro(MinorTickSize, double);
  //@}

  enum TickLocation
  {
    VTK_TICKS_INSIDE = 0,
    VTK_TICKS_OUTSIDE = 1,
    VTK_TICKS_BOTH = 2
  };

  //@{
  /**
   * Set/Get the location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   */
  vtkSetClampMacro(TickLocation, int, VTK_TICKS_INSIDE, VTK_TICKS_BOTH);
  vtkGetMacro(TickLocation, int);
  //@}

  void SetTickLocationToInside(void) { this->SetTickLocation(VTK_TICKS_INSIDE); };
  void SetTickLocationToOutside(void) { this->SetTickLocation(VTK_TICKS_OUTSIDE); };
  void SetTickLocationToBoth(void) { this->SetTickLocation(VTK_TICKS_BOTH); };

  //@{
  /**
   * Set/Get visibility of the axis line.
   */
  vtkSetMacro(AxisVisibility, int);
  vtkGetMacro(AxisVisibility, int);
  vtkBooleanMacro(AxisVisibility, int);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis major tick marks.
   */
  vtkSetMacro(TickVisibility, int);
  vtkGetMacro(TickVisibility, int);
  vtkBooleanMacro(TickVisibility, int);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis labels.
   */
  vtkSetMacro(LabelVisibility, int);
  vtkGetMacro(LabelVisibility, int);
  vtkBooleanMacro(LabelVisibility, int);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis title.
   */
  vtkSetMacro(TitleVisibility, int);
  vtkGetMacro(TitleVisibility, int);
  vtkBooleanMacro(TitleVisibility, int);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis detached exponent.
   */
  vtkSetMacro(ExponentVisibility, bool);
  vtkGetMacro(ExponentVisibility, bool);
  vtkBooleanMacro(ExponentVisibility, bool);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis detached exponent.
   */
  vtkSetMacro(LastMajorTickPointCorrection, bool);
  vtkGetMacro(LastMajorTickPointCorrection, bool);
  vtkBooleanMacro(LastMajorTickPointCorrection, bool);
  //@}

  enum AlignLocation
  {
    VTK_ALIGN_TOP = 0,
    VTK_ALIGN_BOTTOM = 1,
    VTK_ALIGN_POINT1 = 2,
    VTK_ALIGN_POINT2 = 3
  };

  //@{
  /**
   * Get/Set the alignement of the title related to the axis.
   * Possible Alignment: VTK_ALIGN_TOP, VTK_ALIGN_BOTTOM, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2
   */
  virtual void SetTitleAlignLocation(int location);
  vtkGetMacro(TitleAlignLocation, int);
  //@}

  //@{
  /**
   * Get/Set the location of the Detached Exponent related to the axis.
   * Possible Location: VTK_ALIGN_TOP, VTK_ALIGN_BOTTOM, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2
   */
  virtual void SetExponentLocation(int location);
  vtkGetMacro(ExponentLocation, int);
  //@}

  //@{
  /**
   * Set/Get the axis title text property.
   */
  virtual void SetTitleTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(TitleTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the axis labels text property.
   */
  virtual void SetLabelTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(LabelTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Get/Set axis actor property (axis and its ticks) (kept for compatibility)
   */
  void SetAxisLinesProperty(vtkProperty*);
  vtkProperty* GetAxisLinesProperty();
  //@}

  //@{
  /**
   * Get/Set main line axis actor property
   */
  void SetAxisMainLineProperty(vtkProperty*);
  vtkProperty* GetAxisMainLineProperty();
  //@}

  //@{
  /**
   * Get/Set axis actor property (axis and its ticks)
   */
  void SetAxisMajorTicksProperty(vtkProperty*);
  vtkProperty* GetAxisMajorTicksProperty();
  //@}

  //@{
  /**
   * Get/Set axis actor property (axis and its ticks)
   */
  void SetAxisMinorTicksProperty(vtkProperty*);
  vtkProperty* GetAxisMinorTicksProperty();
  //@}

  //@{
  /**
   * Get/Set gridlines actor property (outer grid lines)
   */
  void SetGridlinesProperty(vtkProperty*);
  vtkProperty* GetGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set inner gridlines actor property
   */
  void SetInnerGridlinesProperty(vtkProperty*);
  vtkProperty* GetInnerGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set gridPolys actor property (grid quads)
   */
  void SetGridpolysProperty(vtkProperty*);
  vtkProperty* GetGridpolysProperty();
  //@}

  //@{
  /**
   * Set/Get whether gridlines should be drawn.
   */
  vtkSetMacro(DrawGridlines, int);
  vtkGetMacro(DrawGridlines, int);
  vtkBooleanMacro(DrawGridlines, int);
  //@}

  //@{
  /**
   * Set/Get whether ONLY the gridlines should be drawn.
   * This will only draw GridLines and will skip any other part of the rendering
   * such as Axis/Tick/Title/...
   */
  vtkSetMacro(DrawGridlinesOnly, int);
  vtkGetMacro(DrawGridlinesOnly, int);
  vtkBooleanMacro(DrawGridlinesOnly, int);
  //@}

  vtkSetMacro(DrawGridlinesLocation, int);
  vtkGetMacro(DrawGridlinesLocation, int);

  //@{
  /**
   * Set/Get whether inner gridlines should be drawn.
   */
  vtkSetMacro(DrawInnerGridlines, int);
  vtkGetMacro(DrawInnerGridlines, int);
  vtkBooleanMacro(DrawInnerGridlines, int);
  //@}

  //@{
  /**
   * Set/Get the length to use when drawing gridlines.
   */
  vtkSetMacro(GridlineXLength, double);
  vtkGetMacro(GridlineXLength, double);
  vtkSetMacro(GridlineYLength, double);
  vtkGetMacro(GridlineYLength, double);
  vtkSetMacro(GridlineZLength, double);
  vtkGetMacro(GridlineZLength, double);
  //@}

  //@{
  /**
   * Set/Get whether gridpolys should be drawn.
   */
  vtkSetMacro(DrawGridpolys, int);
  vtkGetMacro(DrawGridpolys, int);
  vtkBooleanMacro(DrawGridpolys, int);
  //@}

  enum AxisType
  {
    VTK_AXIS_TYPE_X = 0,
    VTK_AXIS_TYPE_Y = 1,
    VTK_AXIS_TYPE_Z = 2
  };

  //@{
  /**
   * Set/Get the type of this axis.
   */
  vtkSetClampMacro(AxisType, int, VTK_AXIS_TYPE_X, VTK_AXIS_TYPE_Z);
  vtkGetMacro(AxisType, int);
  void SetAxisTypeToX(void) { this->SetAxisType(VTK_AXIS_TYPE_X); };
  void SetAxisTypeToY(void) { this->SetAxisType(VTK_AXIS_TYPE_Y); };
  void SetAxisTypeToZ(void) { this->SetAxisType(VTK_AXIS_TYPE_Z); };
  //@}

  enum AxisPosition
  {
    VTK_AXIS_POS_MINMIN = 0,
    VTK_AXIS_POS_MINMAX = 1,
    VTK_AXIS_POS_MAXMAX = 2,
    VTK_AXIS_POS_MAXMIN = 3
  };

  //@{
  /**
   * Set/Get The type of scale, enable logarithmic scale or linear by default
   */
  vtkSetMacro(Log, bool);
  vtkGetMacro(Log, bool);
  vtkBooleanMacro(Log, bool);
  //@}

  //@{
  /**
   * Set/Get the position of this axis (in relation to an an
   * assumed bounding box).  For an x-type axis, MINMIN corresponds
   * to the x-edge in the bounding box where Y values are minimum and
   * Z values are minimum. For a y-type axis, MAXMIN corresponds to the
   * y-edge where X values are maximum and Z values are minimum.
   */
  vtkSetClampMacro(AxisPosition, int, VTK_AXIS_POS_MINMIN, VTK_AXIS_POS_MAXMIN);
  vtkGetMacro(AxisPosition, int);
  //@}

  void SetAxisPositionToMinMin(void) { this->SetAxisPosition(VTK_AXIS_POS_MINMIN); };
  void SetAxisPositionToMinMax(void) { this->SetAxisPosition(VTK_AXIS_POS_MINMAX); };
  void SetAxisPositionToMaxMax(void) { this->SetAxisPosition(VTK_AXIS_POS_MAXMAX); };
  void SetAxisPositionToMaxMin(void) { this->SetAxisPosition(VTK_AXIS_POS_MAXMIN); };

  //@{
  /**
   * Set/Get the camera for this axis.  The camera is used by the
   * labels to 'follow' the camera and be legible from any viewpoint.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera, vtkCamera);
  //@}

  //@{
  /**
   * Draw the axis.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) VTK_OVERRIDE;
  virtual int RenderTranslucentGeometry(vtkViewport* viewport);
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport* viewport) VTK_OVERRIDE;
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;

  double ComputeMaxLabelLength(const double[3]);
  double ComputeTitleLength(const double[3]);

  void SetLabelScale(const double scale);
  void SetLabelScale(int labelIndex, const double scale);
  void SetTitleScale(const double scale);

  //@{
  /**
   * Set/Get the starting position for minor and major tick points,
   * and the delta values that determine their spacing.
   */
  vtkSetMacro(MinorStart, double);
  vtkGetMacro(MinorStart, double);
  double GetMajorStart(int axis);
  void SetMajorStart(int axis, double value);
  // vtkSetMacro(MajorStart, double);
  // vtkGetMacro(MajorStart, double);
  vtkSetMacro(DeltaMinor, double);
  vtkGetMacro(DeltaMinor, double);
  double GetDeltaMajor(int axis);
  void SetDeltaMajor(int axis, double value);
  // vtkSetMacro(DeltaMajor, double);
  // vtkGetMacro(DeltaMajor, double);
  //@}

  //@{
  /**
   * Set/Get the starting position for minor and major tick points on
   * the range and the delta values that determine their spacing. The
   * range and the position need not be identical. ie the displayed
   * values need not match the actual positions in 3D space.
   */
  vtkSetMacro(MinorRangeStart, double);
  vtkGetMacro(MinorRangeStart, double);
  vtkSetMacro(MajorRangeStart, double);
  vtkGetMacro(MajorRangeStart, double);
  vtkSetMacro(DeltaRangeMinor, double);
  vtkGetMacro(DeltaRangeMinor, double);
  vtkSetMacro(DeltaRangeMajor, double);
  vtkGetMacro(DeltaRangeMajor, double);
  //@}

  void SetLabels(vtkStringArray* labels);

  void BuildAxis(vtkViewport* viewport, bool);

  //@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  vtkGetObjectMacro(TitleActor, vtkAxisFollower);
  //@}

  //@{
  /**
   * Get exponent follower actor
   */
  vtkGetObjectMacro(ExponentActor, vtkAxisFollower);
  //@}

  /**
   * Get label actors responsigle for drawing label text.
   */
  inline vtkAxisFollower** GetLabelActors() { return this->LabelActors; }

  //@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  vtkGetObjectMacro(TitleProp3D, vtkProp3DAxisFollower);
  //@}

  /**
   * Get label actors responsigle for drawing label text.
   */
  inline vtkProp3DAxisFollower** GetLabelProps3D() { return this->LabelProps3D; }

  //@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  vtkGetObjectMacro(ExponentProp3D, vtkProp3DAxisFollower);
  //@}

  //@{
  /**
   * Get total number of labels built. Once built
   * this count does not change.
   */
  vtkGetMacro(NumberOfLabelsBuilt, int);
  //@}

  //@{
  /**
   * Set/Get flag whether to calculate title offset.
   * Default is true.
   */
  vtkSetMacro(CalculateTitleOffset, int);
  vtkGetMacro(CalculateTitleOffset, int);
  vtkBooleanMacro(CalculateTitleOffset, int);
  //@}

  //@{
  /**
   * Set/Get flag whether to calculate label offset.
   * Default is true.
   */
  vtkSetMacro(CalculateLabelOffset, int);
  vtkGetMacro(CalculateLabelOffset, int);
  vtkBooleanMacro(CalculateLabelOffset, int);
  //@}

  //@{
  /**
   * Set/Get the 2D mode
   */
  vtkSetMacro(Use2DMode, int);
  vtkGetMacro(Use2DMode, int);
  //@}

  //@{
  /**
   * Set/Get the 2D mode the vertical offset for X title in 2D mode
   */
  vtkSetMacro(VerticalOffsetXTitle2D, double);
  vtkGetMacro(VerticalOffsetXTitle2D, double);
  //@}

  //@{
  /**
   * Set/Get the 2D mode the horizontal offset for Y title in 2D mode
   */
  vtkSetMacro(HorizontalOffsetYTitle2D, double);
  vtkGetMacro(HorizontalOffsetYTitle2D, double);
  //@}

  //@{
  /**
   * Set/Get whether title position must be saved in 2D mode
   */
  vtkSetMacro(SaveTitlePosition, int);
  vtkGetMacro(SaveTitlePosition, int);
  //@}

  //@{
  /**
   * Provide real vector for non aligned axis
   */
  vtkSetVector3Macro(AxisBaseForX, double);
  vtkGetVector3Macro(AxisBaseForX, double);
  //@}

  //@{
  /**
   * Provide real vector for non aligned axis
   */
  vtkSetVector3Macro(AxisBaseForY, double);
  vtkGetVector3Macro(AxisBaseForY, double);
  //@}

  //@{
  /**
   * Provide real vector for non aligned axis
   */
  vtkSetVector3Macro(AxisBaseForZ, double);
  vtkGetVector3Macro(AxisBaseForZ, double);
  //@}

  //@{
  /**
   * Notify the axes that is not part of a cube anymore
   */
  vtkSetMacro(AxisOnOrigin, int);
  vtkGetMacro(AxisOnOrigin, int);
  //@}

  //@{
  /**
   * Set/Get the offsets used to position texts.
   */
  vtkSetMacro(LabelOffset, double);
  vtkGetMacro(LabelOffset, double);
  vtkSetMacro(TitleOffset, double);
  vtkGetMacro(TitleOffset, double);
  vtkSetMacro(ExponentOffset, double);
  vtkGetMacro(ExponentOffset, double);
  vtkSetMacro(ScreenSize, double);
  vtkGetMacro(ScreenSize, double);
  //@}

protected:
  vtkAxisActor();
  ~vtkAxisActor() VTK_OVERRIDE;

  char* Title;
  char* Exponent;
  double Range[2];
  double LastRange[2];
  char* LabelFormat;
  int UseTextActor3D;
  int NumberOfLabelsBuilt;
  int MinorTicksVisible;
  int LastMinorTicksVisible;

  /**
   * The location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   */
  int TickLocation;

  /**
   * Hold the alignement property of the title related to the axis.
   * Possible Alignment: VTK_ALIGN_BOTTOM, VTK_ALIGN_TOP, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2.
   */
  int TitleAlignLocation;

  /**
   * Hold the alignement property of the exponent coming from the label values.
   * Possible Alignment: VTK_ALIGN_BOTTOM, VTK_ALIGN_TOP, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2.
   */
  int ExponentLocation;

  int DrawGridlines;
  int DrawGridlinesOnly;
  int LastDrawGridlines;
  int DrawGridlinesLocation;     // 0: all | 1: closest | 2: farest
  int LastDrawGridlinesLocation; // 0: all | 1: closest | 2: farest
  double GridlineXLength;
  double GridlineYLength;
  double GridlineZLength;

  int DrawInnerGridlines;
  int LastDrawInnerGridlines;

  int DrawGridpolys;
  int LastDrawGridpolys;

  int AxisVisibility;
  int TickVisibility;
  int LastTickVisibility;
  int LabelVisibility;
  int TitleVisibility;
  bool ExponentVisibility;
  bool LastMajorTickPointCorrection;

  bool Log;
  int AxisType;
  int AxisPosition;
  double Bounds[6];

  // coordinate system for axisAxtor, relative to world coordinates
  double AxisBaseForX[3];
  double AxisBaseForY[3];
  double AxisBaseForZ[3];

private:
  vtkAxisActor(const vtkAxisActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAxisActor&) VTK_DELETE_FUNCTION;

  void TransformBounds(vtkViewport*, double bnds[6]);

  void BuildLabels(vtkViewport*, bool);
  void BuildLabels2D(vtkViewport*, bool);
  void SetLabelPositions(vtkViewport*, bool);
  void SetLabelPositions2D(vtkViewport*, bool);

  /**
   * Set orientation of the actor 2D (follower) to keep the axis orientation and stay on the right
   * size
   */
  void RotateActor2DFromAxisProjection(vtkTextActor* pActor2D);

  /**
   * Init the geometry of the title. (no positioning or orientation)
   */
  void InitTitle();

  /**
   * Init the geometry of the common exponent of the labels values. (no positioning or orientation)
   */
  void InitExponent();

  /**
   * This methdod set the text and set the base position of the follower from the axis
   * The position will be modified in vtkAxisFollower::Render() sub-functions according to the
   * camera position
   * for convenience purpose.
   */
  void BuildTitle(bool);

  /**
   * Build the actor to display the exponent in case it should appear next to the title or next to
   * p2 coordinate.
   */
  void BuildExponent(bool force);

  void BuildExponent2D(vtkViewport* viewport, bool force);

  void BuildTitle2D(vtkViewport* viewport, bool);

  void SetAxisPointsAndLines(void);

  bool BuildTickPoints(double p1[3], double p2[3], bool force);

  // Build major ticks for linear scale.
  void BuildMajorTicks(double p1[3], double p2[3], double localCoordSys[3][3]);

  // Build major ticks for logarithmic scale.
  void BuildMajorTicksLog(double p1[3], double p2[3], double localCoordSys[3][3]);

  // Build minor ticks for linear scale.
  void BuildMinorTicks(double p1[3], double p2[3], double localCoordSys[3][3]);

  // Build minor ticks for logarithmic scale enabled
  void BuildMinorTicksLog(double p1[3], double p2[3], double localCoordSys[3][3]);

  void BuildAxisGridLines(double p1[3], double p2[3], double localCoordSys[3][3]);

  bool TickVisibilityChanged(void);
  vtkProperty* NewTitleProperty();
  vtkProperty2D* NewTitleProperty2D();
  vtkProperty* NewLabelProperty();

  bool BoundsDisplayCoordinateChanged(vtkViewport* viewport);

  vtkCoordinate* Point1Coordinate;
  vtkCoordinate* Point2Coordinate;

  double MajorTickSize;
  double MinorTickSize;

  // For each axis (for the inner gridline generation)
  double MajorStart[3];
  double DeltaMajor[3];
  double MinorStart;
  double DeltaMinor;

  // For the ticks, w.r.t to the set range
  double MajorRangeStart;
  double MinorRangeStart;

  /**
   * step between 2 minor ticks, in range value (values displayed on the axis)
   */
  double DeltaRangeMinor;

  /**
   * step between 2 major ticks, in range value (values displayed on the axis)
   */
  double DeltaRangeMajor;

  int LastAxisPosition;
  int LastAxisType;
  int LastTickLocation;
  double LastLabelStart;

  vtkPoints* MinorTickPts;
  vtkPoints* MajorTickPts;
  vtkPoints* GridlinePts;
  vtkPoints* InnerGridlinePts;
  vtkPoints* GridpolyPts;

  vtkVectorText* TitleVector;
  vtkPolyDataMapper* TitleMapper;
  vtkAxisFollower* TitleActor;
  vtkTextActor* TitleActor2D;
  vtkProp3DAxisFollower* TitleProp3D;
  vtkTextActor3D* TitleActor3D;
  vtkTextProperty* TitleTextProperty;

  //@{
  /**
   * Mapper/Actor used to display a common exponent of the label values
   */
  vtkVectorText* ExponentVector;
  vtkPolyDataMapper* ExponentMapper;
  vtkAxisFollower* ExponentActor;
  vtkTextActor* ExponentActor2D;
  vtkProp3DAxisFollower* ExponentProp3D;
  vtkTextActor3D* ExponentActor3D;
  //@}

  vtkVectorText** LabelVectors;
  vtkPolyDataMapper** LabelMappers;
  vtkAxisFollower** LabelActors;
  vtkProp3DAxisFollower** LabelProps3D;
  vtkTextActor** LabelActors2D;
  vtkTextActor3D** LabelActors3D;
  vtkTextProperty* LabelTextProperty;

  // Main line axis
  vtkPolyData* AxisLines;
  vtkPolyDataMapper* AxisLinesMapper;
  vtkActor* AxisLinesActor;

  // Ticks of the axis
  vtkPolyData *AxisMajorTicks, *AxisMinorTicks;
  vtkPolyDataMapper *AxisMajorTicksMapper, *AxisMinorTicksMapper;
  vtkActor *AxisMajorTicksActor, *AxisMinorTicksActor;

  vtkPolyData* Gridlines;
  vtkPolyDataMapper* GridlinesMapper;
  vtkActor* GridlinesActor;
  vtkPolyData* InnerGridlines;
  vtkPolyDataMapper* InnerGridlinesMapper;
  vtkActor* InnerGridlinesActor;
  vtkPolyData* Gridpolys;
  vtkPolyDataMapper* GridpolysMapper;
  vtkActor* GridpolysActor;

  vtkCamera* Camera;
  vtkTimeStamp BuildTime;
  vtkTimeStamp BuildTickPointsTime;
  vtkTimeStamp BoundsTime;
  vtkTimeStamp LabelBuildTime;
  vtkTimeStamp TitleTextTime;
  vtkTimeStamp ExponentTextTime;

  int AxisOnOrigin;

  int AxisHasZeroLength;

  int CalculateTitleOffset;
  int CalculateLabelOffset;

  /**
   * Use xy-axis only when Use2DMode=1:
   */
  int Use2DMode;

  /**
   * Vertical offset in display coordinates for X axis title (used in 2D mode only)
   * Default: -40
   */
  double VerticalOffsetXTitle2D;

  /**
   * Vertical offset in display coordinates for X axis title (used in 2D mode only)
   * Default: -50
   */
  double HorizontalOffsetYTitle2D;

  /**
   * Save title position (used in 2D mode only):
   * val = 0 : no need to save position (doesn't stick actors in a position)
   * val = 1 : positions have to be saved during the next render pass
   * val = 2 : positions are saved; use them
   */
  int SaveTitlePosition;

  /**
   * Constant position for the title (used in 2D mode only)
   */
  double TitleConstantPosition[2];

  /**
   * True if the 2D title has to be built, false otherwise
   */
  bool NeedBuild2D;

  double LastMinDisplayCoordinate[3];
  double LastMaxDisplayCoordinate[3];
  double TickVector[3];

  //@{
  /**
   * Offsets used to position text.
   */
  double ScreenSize;
  double LabelOffset;
  double TitleOffset;
  double ExponentOffset;
};
//@}

#endif
