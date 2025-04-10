// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAxisActor
 * @brief   Create an axis with tick marks and labels
 *
 * vtkAxisActor creates an axis with tick marks, labels, and/or a title,
 * depending on the particular instance variable settings. It is assumed that
 * the axes is part of a bounding box and is orthogonal to one of the
 * coordinate axes.  To use this class, you typically specify two points
 * defining the start and end points of the line (xyz definition using
 * vtkCoordinate class), the axis type (X, Y or Z), the axis location in
 * relation to the bounding box, the bounding box, the number of labels, and
 * the data range (min,max). You can also control what parts of the axis are
 * visible including the line, the tick marks, the labels, and the title. It
 * is also possible to control gridlines, and specify on which 'side' the
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
#include "vtkDeprecation.h"               // for Deprecation macro
#include "vtkNew.h"                       // For vtkNew
#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkSmartPointer.h"              // For vtkSmartPointer
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO

#include <memory>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkAxisFollower;
class vtkCamera;
class vtkCoordinate;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProp3DAxisFollower;
class vtkStringArray;
class vtkTextActor;
class vtkTextActorInterfaceInternal;
class vtkTextActor3D;
class vtkTextProperty;
class vtkVectorText;

class VTKRENDERINGANNOTATION_EXPORT VTK_MARSHALAUTO vtkAxisActor : public vtkActor
{
public:
  vtkTypeMacro(vtkAxisActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate object.
   */
  static vtkAxisActor* New();

  ///@{
  /**
   * Specify the position of the first point defining the axis.
   */
  virtual vtkCoordinate* GetPoint1Coordinate();
  virtual void SetPoint1(double x[3]) { this->SetPoint1(x[0], x[1], x[2]); }
  virtual void SetPoint1(double x, double y, double z);
  virtual double* GetPoint1();
  ///@}

  ///@{
  /**
   * Specify the position of the second point defining the axis.
   */
  virtual vtkCoordinate* GetPoint2Coordinate();
  virtual void SetPoint2(double x[3]) { this->SetPoint2(x[0], x[1], x[2]); }
  virtual void SetPoint2(double x, double y, double z);
  virtual double* GetPoint2();
  ///@}

  ///@{
  /**
   * Specify the (min,max) axis range. This will be used in the generation
   * of labels, if labels are visible.
   * Default: (0.0, 1.0).
   */
  vtkSetVector2Macro(Range, double);
  vtkGetVectorMacro(Range, double, 2);
  ///@}

  ///@{
  /**
   * Set or get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   * Default: (-1, 1, -1, 1, -1, 1).
   */
  void SetBounds(const double bounds[6]);
  void SetBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  double* GetBounds() VTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]);
  ///@}

  ///@{
  /**
   * Set/Get the format with which to print the labels on the axis.
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  ///@}

  ///@{
  /**
   * Render text as polygons (vtkVectorText) or as sprites (vtkTextActor3D).
   * In 2D mode, the value is ignored and text is rendered as vtkTextActor.
   * False(0) by default.
   * See Also:
   * GetUse2DMode(), SetUse2DMode
   */
  vtkSetMacro(UseTextActor3D, bool);
  vtkGetMacro(UseTextActor3D, bool);
  ///@}

  ///@{
  /**
   * Set/Get the flag that controls whether the minor ticks are visible.
   * Default: true.
   */
  vtkSetMacro(MinorTicksVisible, bool);
  vtkGetMacro(MinorTicksVisible, bool);
  vtkBooleanMacro(MinorTicksVisible, bool);
  ///@}

  ///@{
  /**
   * Set/Get the title of the axis actor.
   */
  void SetTitle(const std::string& title);
  vtkGetMacro(Title, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the common exponent of the labels values.
   */
  void SetExponent(const std::string& exp);
  vtkGetMacro(Exponent, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the size of major tick marks.
   * Default: 1.0.
   */
  vtkSetMacro(MajorTickSize, double);
  vtkGetMacro(MajorTickSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the size of minor tick marks.
   * Default: 0.5.
   */
  vtkSetMacro(MinorTickSize, double);
  vtkGetMacro(MinorTickSize, double);
  ///@}

  enum TickLocation
  {
    VTK_TICKS_INSIDE = 0,
    VTK_TICKS_OUTSIDE = 1,
    VTK_TICKS_BOTH = 2
  };

  ///@{
  /**
   * Set/Get the location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   * Default: VTK_TICKS_INSIDE.
   */
  vtkSetClampMacro(TickLocation, int, VTK_TICKS_INSIDE, VTK_TICKS_BOTH);
  vtkGetMacro(TickLocation, int);
  ///@}

  void SetTickLocationToInside() { this->SetTickLocation(VTK_TICKS_INSIDE); }
  void SetTickLocationToOutside() { this->SetTickLocation(VTK_TICKS_OUTSIDE); }
  void SetTickLocationToBoth() { this->SetTickLocation(VTK_TICKS_BOTH); }

  ///@{
  /**
   * Set/Get visibility of the axis line.
   * Default: true.
   */
  vtkSetMacro(AxisVisibility, bool);
  vtkGetMacro(AxisVisibility, bool);
  vtkBooleanMacro(AxisVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/Get visibility of the axis major tick marks.
   * Default: true.
   */
  vtkSetMacro(TickVisibility, bool);
  vtkGetMacro(TickVisibility, bool);
  vtkBooleanMacro(TickVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/Get visibility of the axis labels.
   * Default: true.
   */
  vtkSetMacro(LabelVisibility, bool);
  vtkGetMacro(LabelVisibility, bool);
  vtkBooleanMacro(LabelVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/Get visibility of the axis title.
   * Default: true.
   */
  vtkSetMacro(TitleVisibility, bool);
  vtkGetMacro(TitleVisibility, bool);
  vtkBooleanMacro(TitleVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/Get visibility of the axis detached exponent.
   * Default: false.
   */
  vtkSetMacro(ExponentVisibility, bool);
  vtkGetMacro(ExponentVisibility, bool);
  vtkBooleanMacro(ExponentVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/Get visibility of the axis detached exponent.
   * Default: false.
   */
  vtkSetMacro(LastMajorTickPointCorrection, bool);
  vtkGetMacro(LastMajorTickPointCorrection, bool);
  vtkBooleanMacro(LastMajorTickPointCorrection, bool);
  ///@}

  enum AlignLocation
  {
    VTK_ALIGN_TOP = 0,
    VTK_ALIGN_BOTTOM = 1,
    VTK_ALIGN_POINT1 = 2,
    VTK_ALIGN_POINT2 = 3
  };

  ///@{
  /**
   * Get/Set the alignment of the title related to the axis.
   * Possible Alignment: VTK_ALIGN_TOP, VTK_ALIGN_BOTTOM, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2.
   * Default: VTK_ALIGN_BOTTOM.
   */
  virtual void SetTitleAlignLocation(int location);
  vtkGetMacro(TitleAlignLocation, int);
  ///@}

  ///@{
  /**
   * Get/Set the location of the Detached Exponent related to the axis.
   * Possible Location: VTK_ALIGN_TOP, VTK_ALIGN_BOTTOM, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2.
   * Default: VTK_ALIGN_POINT2.
   */
  virtual void SetExponentLocation(int location);
  vtkGetMacro(ExponentLocation, int);
  ///@}

  ///@{
  /**
   * Set/Get the axis title text property.
   */
  virtual void SetTitleTextProperty(vtkTextProperty* p);
  vtkTextProperty* GetTitleTextProperty();
  ///@}

  ///@{
  /**
   * Set/Get the axis labels text property.
   */
  virtual void SetLabelTextProperty(vtkTextProperty* p);
  vtkTextProperty* GetLabelTextProperty();
  ///@}

  ///@{
  /**
   * Get/Set axis actor property (axis and its ticks) (kept for compatibility)
   */
  void SetAxisLinesProperty(vtkProperty*);
  vtkProperty* GetAxisLinesProperty();
  ///@}

  ///@{
  /**
   * Get/Set main line axis actor property
   */
  void SetAxisMainLineProperty(vtkProperty*);
  vtkProperty* GetAxisMainLineProperty();
  ///@}

  ///@{
  /**
   * Get/Set axis actor property (axis and its ticks)
   */
  void SetAxisMajorTicksProperty(vtkProperty*);
  vtkProperty* GetAxisMajorTicksProperty();
  ///@}

  ///@{
  /**
   * Get/Set axis actor property (axis and its ticks)
   */
  void SetAxisMinorTicksProperty(vtkProperty*);
  vtkProperty* GetAxisMinorTicksProperty();
  ///@}

  ///@{
  /**
   * Get/Set gridlines actor property (outer grid lines)
   */
  void SetGridlinesProperty(vtkProperty*);
  vtkProperty* GetGridlinesProperty();
  ///@}

  ///@{
  /**
   * Get/Set inner gridlines actor property
   */
  void SetInnerGridlinesProperty(vtkProperty*);
  vtkProperty* GetInnerGridlinesProperty();
  ///@}

  ///@{
  /**
   * Get/Set gridPolys actor property (grid quads)
   */
  void SetGridpolysProperty(vtkProperty*);
  vtkProperty* GetGridpolysProperty();
  ///@}

  ///@{
  /**
   * Set/Get whether gridlines should be drawn.
   * Default: false.
   */
  vtkSetMacro(DrawGridlines, bool);
  vtkGetMacro(DrawGridlines, bool);
  vtkBooleanMacro(DrawGridlines, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether ONLY the gridlines should be drawn.
   * This will only draw GridLines and will skip any other part of the rendering
   * such as Axis/Tick/Title/...
   * Default: false.
   */
  vtkSetMacro(DrawGridlinesOnly, bool);
  vtkGetMacro(DrawGridlinesOnly, bool);
  vtkBooleanMacro(DrawGridlinesOnly, bool);
  ///@}

  vtkSetMacro(DrawGridlinesLocation, int);
  vtkGetMacro(DrawGridlinesLocation, int);

  ///@{
  /**
   * Set/Get whether inner gridlines should be drawn.
   * Default: false.
   */
  vtkSetMacro(DrawInnerGridlines, bool);
  vtkGetMacro(DrawInnerGridlines, bool);
  vtkBooleanMacro(DrawInnerGridlines, bool);
  ///@}

  ///@{
  /**
   * Set/Get the length to use when drawing gridlines.
   * Default: 1.0.
   */
  vtkSetMacro(GridlineXLength, double);
  vtkGetMacro(GridlineXLength, double);
  vtkSetMacro(GridlineYLength, double);
  vtkGetMacro(GridlineYLength, double);
  vtkSetMacro(GridlineZLength, double);
  vtkGetMacro(GridlineZLength, double);
  ///@}

  ///@{
  /**
   * Set/Get whether gridpolys should be drawn.
   * Default: false.
   */
  vtkSetMacro(DrawGridpolys, bool);
  vtkGetMacro(DrawGridpolys, bool);
  vtkBooleanMacro(DrawGridpolys, bool);
  ///@}

  enum AxisType
  {
    VTK_AXIS_TYPE_X = 0,
    VTK_AXIS_TYPE_Y = 1,
    VTK_AXIS_TYPE_Z = 2
  };

  ///@{
  /**
   * Set/Get the type of this axis.
   * Default: VTK_AXIS_TYPE_X.
   */
  vtkSetClampMacro(AxisType, int, VTK_AXIS_TYPE_X, VTK_AXIS_TYPE_Z);
  vtkGetMacro(AxisType, int);
  void SetAxisTypeToX() { this->SetAxisType(VTK_AXIS_TYPE_X); }
  void SetAxisTypeToY() { this->SetAxisType(VTK_AXIS_TYPE_Y); }
  void SetAxisTypeToZ() { this->SetAxisType(VTK_AXIS_TYPE_Z); }
  ///@}

  enum AxisPosition
  {
    VTK_AXIS_POS_MINMIN = 0,
    VTK_AXIS_POS_MINMAX = 1,
    VTK_AXIS_POS_MAXMAX = 2,
    VTK_AXIS_POS_MAXMIN = 3
  };

  ///@{
  /**
   * Set/Get The type of scale, enable logarithmic scale or linear by default.
   * Default: false.
   */
  vtkSetMacro(Log, bool);
  vtkGetMacro(Log, bool);
  vtkBooleanMacro(Log, bool);
  ///@}

  ///@{
  /**
   * Set/Get the position of this axis (in relation to an an
   * assumed bounding box).  For an x-type axis, MINMIN corresponds
   * to the x-edge in the bounding box where Y values are minimum and
   * Z values are minimum. For a y-type axis, MAXMIN corresponds to the
   * y-edge where X values are maximum and Z values are minimum.
   * Default: VTK_AXIS_POS_MINMIN.
   */
  vtkSetClampMacro(AxisPosition, int, VTK_AXIS_POS_MINMIN, VTK_AXIS_POS_MAXMIN);
  vtkGetMacro(AxisPosition, int);
  ///@}

  void SetAxisPositionToMinMin() { this->SetAxisPosition(VTK_AXIS_POS_MINMIN); }
  void SetAxisPositionToMinMax() { this->SetAxisPosition(VTK_AXIS_POS_MINMAX); }
  void SetAxisPositionToMaxMax() { this->SetAxisPosition(VTK_AXIS_POS_MAXMAX); }
  void SetAxisPositionToMaxMin() { this->SetAxisPosition(VTK_AXIS_POS_MAXMIN); }

  ///@{
  /**
   * Set/Get the camera for this axis.  The camera is used by the
   * labels to 'follow' the camera and be legible from any viewpoint.
   */
  virtual void SetCamera(vtkCamera*);
  vtkCamera* GetCamera();
  ///@}

  ///@{
  /**
   * Draw the axis.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  virtual int RenderTranslucentGeometry(vtkViewport* viewport);
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  int RenderOverlay(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  ///@{
  /**
   * Compute the max diagonal lentgh of the labels.
   * Camera and labels should have been set before.
   */
  VTK_DEPRECATED_IN_9_5_0("Argument is not used anymore, please use the variant without argument")
  double ComputeMaxLabelLength(const double[3]) { return this->ComputeMaxLabelLength(); }
  double ComputeMaxLabelLength();
  ///@}

  ///@{
  /**
   * Compute the diagonal length of the Title text.
   * Camera and title should have been set before.
   */
  VTK_DEPRECATED_IN_9_5_0("Argument is not used anymore, please use the variant without argument")
  double ComputeTitleLength(const double[3]) { return this->ComputeTitleLength(); }
  double ComputeTitleLength();
  ///@}

  ///@{
  /**
   * Set scale on underlying actor.
   */
  void SetLabelScale(double scale);
  void SetLabelScale(int labelIndex, double scale);
  void SetTitleScale(double scale);
  void SetExponentScale(double scale);
  ///@}

  ///@{
  /**
   * Set/Get the starting position for minor and major tick points,
   * and the delta values that determine their spacing.
   *
   * The "Minor" versions are not used anymore, will return 0.
   */
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") void SetMinorStart(double){};
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") double GetMinorStart() { return 0.; };

  double GetMajorStart(int axis);
  void SetMajorStart(int axis, double value);

  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") void SetDeltaMinor(double){};
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") double GetDeltaMinor() { return 1.; };

  double GetDeltaMajor(int axis);
  void SetDeltaMajor(int axis, double value);
  ///@}

  ///@{
  /**
   * Set/Get the starting position for minor and major tick points on
   * the range. The range and the position need not be identical. ie the displayed
   * values need not match the actual positions in 3D space.
   * Default: 0.0.
   */
  vtkSetMacro(MinorRangeStart, double);
  vtkGetMacro(MinorRangeStart, double);
  vtkSetMacro(MajorRangeStart, double);
  vtkGetMacro(MajorRangeStart, double);
  ///@}

  ///@{
  /**
   * Set/Get the delta range for minor and major tick points that determine their spacing.
   * Default: 1.0.
   */
  vtkSetMacro(DeltaRangeMinor, double);
  vtkGetMacro(DeltaRangeMinor, double);
  vtkSetMacro(DeltaRangeMajor, double);
  vtkGetMacro(DeltaRangeMajor, double);
  ///@}

  void SetLabels(vtkStringArray* labels);

  void BuildAxis(vtkViewport* viewport, bool);

  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  vtkAxisFollower* GetTitleActor();

  /**
   * Get exponent follower actor
   */
  vtkAxisFollower* GetExponentActor();

  ///@{
  /**
   * Get label actors responsigle for drawing label text.
   */
  VTK_DEPRECATED_IN_9_5_0("This is not safe. Use GetLabelFollower instead.")
  vtkAxisFollower** GetLabelActors();
  vtkAxisFollower* GetLabelFollower(int index);
  int GetNumberOfLabelFollowers() { return this->GetNumberOfLabelsBuilt(); }
  ///@}

  ///@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  vtkProp3DAxisFollower* GetTitleProp3D();
  ///@}

  ///@{
  /**
   * Get label actors responsigle for drawing label text.
   */
  VTK_DEPRECATED_IN_9_5_0("This is not safe. Use GetLabelFollower3D instead.")
  vtkProp3DAxisFollower** GetLabelProps3D();
  vtkProp3DAxisFollower* GetLabelFollower3D(int index);
  int GetNumberOfLabelFollower3D() { return this->GetNumberOfLabelsBuilt(); }
  ///@}

  ///@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  vtkProp3DAxisFollower* GetExponentProp3D();
  ///@}

  ///@{
  /**
   * Get total number of labels built. Once built
   * this count does not change.
   */
  vtkGetMacro(NumberOfLabelsBuilt, int);
  ///@}

  ///@{
  /**
   * Set/Get flag whether to calculate title offset.
   * Default: false.
   */
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") vtkSetMacro(CalculateTitleOffset, bool);
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") vtkGetMacro(CalculateTitleOffset, bool);
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") void CalculateTitleOffsetOn() {}
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") void CalculateTitleOffsetOff() {}
  ///@}

  ///@{
  /**
   * Set/Get flag whether to calculate label offset.
   * Default: false.
   */
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") vtkSetMacro(CalculateLabelOffset, bool);
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") vtkGetMacro(CalculateLabelOffset, bool);
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") void CalculateLabelOffsetOn() {}
  VTK_DEPRECATED_IN_9_5_0("Member is not used anymore") void CalculateLabelOffsetOff() {}
  ///@}

  ///@{
  /**
   * Set/Get the 2D mode.
   * Default: false.
   */
  vtkSetMacro(Use2DMode, bool);
  vtkGetMacro(Use2DMode, bool);
  ///@}

  ///@{
  /**
   * Set/Get the 2D mode the vertical offset for X title in 2D mode.
   * Default: -40.0.
   */
  vtkSetMacro(VerticalOffsetXTitle2D, double);
  vtkGetMacro(VerticalOffsetXTitle2D, double);
  ///@}

  ///@{
  /**
   * Set/Get the 2D mode the horizontal offset for Y title in 2D mode.
   * Default: -50.0.
   */
  vtkSetMacro(HorizontalOffsetYTitle2D, double);
  vtkGetMacro(HorizontalOffsetYTitle2D, double);
  ///@}

  ///@{
  /**
   * Set/Get whether title position must be saved in 2D mode.
   * Default: 0.
   */
  vtkSetMacro(SaveTitlePosition, int);
  vtkGetMacro(SaveTitlePosition, int);
  ///@}

  ///@{
  /**
   * Provide real vector for non aligned axis.
   * Default: (1.0, 0.0, 0.0).
   */
  vtkSetVector3Macro(AxisBaseForX, double);
  vtkGetVector3Macro(AxisBaseForX, double);
  ///@}

  ///@{
  /**
   * Provide real vector for non aligned axis.
   * Default: (0.0, 1.0, 0.0).
   */
  vtkSetVector3Macro(AxisBaseForY, double);
  vtkGetVector3Macro(AxisBaseForY, double);
  ///@}

  ///@{
  /**
   * Provide real vector for non aligned axis.
   * Default: (0.0, 0.0, 1.0).
   */
  vtkSetVector3Macro(AxisBaseForZ, double);
  vtkGetVector3Macro(AxisBaseForZ, double);
  ///@}

  ///@{
  /**
   * Notify the axes that is not part of a cube anymore.
   * Default: false.
   */
  vtkSetMacro(AxisOnOrigin, bool);
  vtkGetMacro(AxisOnOrigin, bool);
  ///@}

  ///@{
  /**
   * Set/Get size factor for text, used for offsets.
   * Default: 10.0.
   */
  vtkSetMacro(ScreenSize, double);
  vtkGetMacro(ScreenSize, double);
  ///@}

  ///@{
  /**
   * Set/Get the Y-offset used to position label.
   * Default: 30.0.
   */
  vtkSetMacro(LabelOffset, double);
  vtkGetMacro(LabelOffset, double);
  ///@}

  ///@{
  /**
   * Set/Get the Y-offset used to position exponent.
   * Default: 20.0.
   */
  vtkSetMacro(ExponentOffset, double);
  vtkGetMacro(ExponentOffset, double);
  ///@}

  ///@{
  /**
   * Set/Get the 2D-offsets used to position title texts.
   * X-component is applied only when not center aligned
   * Center aligned <=> VTK_ALIGN_TOP and VTK_ALIGN_BOTTOM
   * Y-component is the same than other offsets.
   * Default: (20.0, 20.0).
   */
  vtkSetVector2Macro(TitleOffset, double);
  vtkGetVector2Macro(TitleOffset, double);
  ///@}

protected:
  vtkAxisActor();
  ~vtkAxisActor() override;

private:
  vtkAxisActor(const vtkAxisActor&) = delete;
  void operator=(const vtkAxisActor&) = delete;

  void TransformBounds(vtkViewport*, double bnds[6]);

  void BuildLabels(vtkViewport*, bool);
  void BuildLabels2D(vtkViewport*, bool);
  void SetLabelPositions(vtkViewport*, bool);
  void SetLabelPositions2D(vtkViewport*, bool);

  /**
   * This method set the text and set the base position of the follower from the axis
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

  /**
   * Get scenepos in display coordinates, using viewport.
   */
  void Get2DPosition(
    vtkViewport* viewport, double multiplier, double scenepos[3], double displayPos[2]);

  void SetAxisPointsAndLines();

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

  bool TickVisibilityChanged();

  vtkProperty* NewTitleProperty();
  vtkProperty* NewLabelProperty();

  bool BoundsDisplayCoordinateChanged(vtkViewport* viewport);

  ///@{
  /**
   * Return the correct actor depending on current mode (Use2DMode and UseTextActor3D)
   */
  vtkProp* GetTitleActorInternal();
  vtkProp* GetLabelActorInternal(int index);
  vtkProp* GetExponentActorInternal();
  ///@}

  /**
   * Update the actor text property.
   * Mainly set TitleTextProperty/LabelTextProperty on the sub actor.
   * If the underlying actor does not have a vtkTextProperty,
   * use Property and override Color and Opacity from the
   * corresponding vtkTextProperty.
   * Note that exponent uses TitleTextProperty.
   */
  ///@{
  void UpdateTitleActorProperty();
  void UpdateLabelActorProperty(int idx);
  void UpdateExponentActorProperty();
  ///@}

  std::string Title;
  std::string Exponent;
  char* LabelFormat = nullptr;
  double Range[2] = { 0.0, 1.0 };
  double LastRange[2] = { -1.0, -1.0 };
  bool UseTextActor3D = false;
  int NumberOfLabelsBuilt = 0;
  bool MinorTicksVisible = true;
  bool LastMinorTicksVisible = true;

  /**
   * The location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   */
  int TickLocation = VTK_TICKS_INSIDE;

  /**
   * Hold the alignment property of the title related to the axis.
   * Possible Alignment: VTK_ALIGN_BOTTOM, VTK_ALIGN_TOP, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2.
   */
  int TitleAlignLocation = VTK_ALIGN_BOTTOM;

  /**
   * Hold the alignment property of the exponent coming from the label values.
   * Possible Alignment: VTK_ALIGN_BOTTOM, VTK_ALIGN_TOP, VTK_ALIGN_POINT1, VTK_ALIGN_POINT2.
   */
  int ExponentLocation = VTK_ALIGN_POINT2;

  bool DrawGridlines = false;
  bool DrawGridlinesOnly = false;
  bool LastDrawGridlines = false;
  int DrawGridlinesLocation = 0;     // 0: all | 1: closest | 2: furthest
  int LastDrawGridlinesLocation = 0; // 0: all | 1: closest | 2: furthest
  double GridlineXLength = 1.0;
  double GridlineYLength = 1.0;
  double GridlineZLength = 1.0;

  bool DrawInnerGridlines = false;

  bool DrawGridpolys = false;

  bool AxisVisibility = true;
  bool TickVisibility = true;
  bool LastTickVisibility = true;
  bool LabelVisibility = true;
  bool TitleVisibility = true;
  bool ExponentVisibility = false;
  bool LastMajorTickPointCorrection = false;

  bool Log = false;
  int AxisType = VTK_AXIS_TYPE_X;
  int AxisPosition = VTK_AXIS_POS_MINMIN;

  // coordinate system for axisAxtor, relative to world coordinates
  double AxisBaseForX[3] = { 1.0, 0.0, 0.0 };
  double AxisBaseForY[3] = { 0.0, 1.0, 0.0 };
  double AxisBaseForZ[3] = { 0.0, 0.0, 1.0 };

  vtkNew<vtkCoordinate> Point1Coordinate;
  vtkNew<vtkCoordinate> Point2Coordinate;

  double MajorTickSize = 1.0;
  double MinorTickSize = 0.5;

  // For each axis (for the inner gridline generation)
  double MajorStart[3] = { 0.0, 0.0, 0.0 };
  double DeltaMajor[3] = { 1.0, 1.0, 1.0 };

  // For the ticks, w.r.t to the set range
  double MajorRangeStart = 0.0;
  double MinorRangeStart = 0.0;

  /**
   * step between 2 minor ticks, in range value (values displayed on the axis)
   */
  double DeltaRangeMinor = 1.0;

  /**
   * step between 2 major ticks, in range value (values displayed on the axis)
   */
  double DeltaRangeMajor = 1.0;

  int LastAxisPosition = -1;
  int LastTickLocation = -1;

  vtkNew<vtkPoints> MinorTickPts;
  vtkNew<vtkPoints> MajorTickPts;
  vtkNew<vtkPoints> GridlinePts;
  vtkNew<vtkPoints> InnerGridlinePts;
  vtkNew<vtkPoints> GridpolyPts;

  std::unique_ptr<vtkTextActorInterfaceInternal> TitleProp;
  vtkSmartPointer<vtkTextProperty> TitleTextProperty;

  ///@{
  /**
   * Mapper/Actor used to display a common exponent of the label values
   */
  std::unique_ptr<vtkTextActorInterfaceInternal> ExponentProp;
  ///@}

  std::vector<std::shared_ptr<vtkTextActorInterfaceInternal>> LabelProps;
  vtkSmartPointer<vtkTextProperty> LabelTextProperty;

  // VTK_DEPRECATED_IN_9_5_0
  std::vector<vtkAxisFollower*> LabelActors;
  // VTK_DEPRECATED_IN_9_5_0
  std::vector<vtkProp3DAxisFollower*> LabelProps3D;

  // Main line axis
  vtkNew<vtkPolyData> AxisLines;
  vtkNew<vtkActor> AxisLinesActor;

  // Ticks of the axis
  vtkNew<vtkPolyData> AxisMajorTicks, AxisMinorTicks;
  vtkNew<vtkActor> AxisMajorTicksActor, AxisMinorTicksActor;

  vtkNew<vtkPolyData> Gridlines;
  vtkNew<vtkActor> GridlinesActor;
  vtkNew<vtkPolyData> InnerGridlines;
  vtkNew<vtkActor> InnerGridlinesActor;
  vtkNew<vtkPolyData> Gridpolys;
  vtkNew<vtkActor> GridpolysActor;

  vtkSmartPointer<vtkCamera> Camera;
  vtkTimeStamp BuildTime;
  vtkTimeStamp BuildTickPointsTime;
  vtkTimeStamp BoundsTime;
  vtkTimeStamp LabelBuildTime;
  vtkTimeStamp TitleTextTime;
  vtkTimeStamp ExponentTextTime;

  bool AxisOnOrigin = false;

  bool AxisHasZeroLength = false;

  bool CalculateTitleOffset = false; // VTK_DEPRECATED_IN_9_5_0
  bool CalculateLabelOffset = false; // VTK_DEPRECATED_IN_9_5_0

  /**
   * Use xy-axis only when Use2DMode=1:
   */
  bool Use2DMode = false;

  /**
   * Vertical offset in display coordinates for X axis title (used in 2D mode only)
   * Default: -40
   */
  double VerticalOffsetXTitle2D = -40;

  /**
   * Vertical offset in display coordinates for X axis title (used in 2D mode only)
   * Default: -50
   */
  double HorizontalOffsetYTitle2D = -50;

  /**
   * Save title position (used in 2D mode only):
   * val = 0 : no need to save position (doesn't stick actors in a position)
   * val = 1 : positions have to be saved during the next render pass
   * val = 2 : positions are saved; use them
   */
  int SaveTitlePosition = 0;

  /**
   * Constant position for the title (used in 2D mode only)
   */
  double TitleConstantPosition[2] = { 0.0, 0.0 };

  /**
   * True if the 2D title has to be built, false otherwise
   */
  bool NeedBuild2D = false;

  double LastMinDisplayCoordinate[3] = { 0.0, 0.0, 0.0 };
  double LastMaxDisplayCoordinate[3] = { 0.0, 0.0, 0.0 };
  double TickVector[3] = { 0.0, 0.0, 0.0 };

  /**
   * Size factor for text.
   */
  double ScreenSize = 10.0;

  ///@{
  /**
   * Offsets used to position text.
   */
  double LabelOffset = 30.0;
  double TitleOffset[2] = { 20.0, 20.0 };
  double ExponentOffset = 20.0;
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
