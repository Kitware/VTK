/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLineRepresentation
 * @brief   a class defining the representation for a vtkLineWidget2
 *
 * This class is a concrete representation for the vtkLineWidget2. It
 * represents a straight line with three handles: one at the beginning and
 * ending of the line, and one used to translate the line. Through
 * interaction with the widget, the line representation can be arbitrarily
 * placed in the 3D space.
 *
 * To use this representation, you normally specify the position of the two
 * end points (either in world or display coordinates). The PlaceWidget()
 * method is also used to initially position the representation.
 *
 * @warning
 * This class, and vtkLineWidget2, are next generation VTK
 * widgets. An earlier version of this functionality was defined in the
 * class vtkLineWidget.
 *
 * @sa
 * vtkLineWidget2 vtkLineWidget
*/

#ifndef vtkLineRepresentation_h
#define vtkLineRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkConeSource;
class vtkPolyDataMapper;
class vtkLineSource;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataAlgorithm;
class vtkPointHandleRepresentation3D;
class vtkBox;
class vtkFollower;
class vtkVectorText;
class vtkPolyDataMapper;
class vtkCellPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkLineRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkLineRepresentation *New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkLineRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(double pos[3]);
  double* GetPoint1WorldPosition();
  void GetPoint1DisplayPosition(double pos[3]);
  double* GetPoint1DisplayPosition();
  void SetPoint1WorldPosition(double pos[3]);
  void SetPoint1DisplayPosition(double pos[3]);
  void GetPoint2DisplayPosition(double pos[3]);
  double* GetPoint2DisplayPosition();
  void GetPoint2WorldPosition(double pos[3]);
  double* GetPoint2WorldPosition();
  void SetPoint2WorldPosition(double pos[3]);
  void SetPoint2DisplayPosition(double pos[3]);
  //@}

  //@{
  /**
   * This method is used to specify the type of handle representation to
   * use for the three internal vtkHandleWidgets within vtkLineWidget2.
   * To use this method, create a dummy vtkHandleWidget (or subclass),
   * and then invoke this method with this dummy. Then the
   * vtkLineRepresentation uses this dummy to clone three vtkHandleWidgets
   * of the same type. Make sure you set the handle representation before
   * the widget is enabled. (The method InstantiateHandleRepresentation()
   * is invoked by the vtkLineWidget2.)
   */
  void SetHandleRepresentation(vtkPointHandleRepresentation3D *handle);
  void InstantiateHandleRepresentation();
  //@}

  //@{
  /**
   * Get the three handle representations used for the vtkLineWidget2.
   */
  vtkGetObjectMacro(Point1Representation,vtkPointHandleRepresentation3D);
  vtkGetObjectMacro(Point2Representation,vtkPointHandleRepresentation3D);
  vtkGetObjectMacro(LineHandleRepresentation,vtkPointHandleRepresentation3D);
  //@}

  //@{
  /**
   * Get the end-point (sphere) properties. The properties of the end-points
   * when selected and unselected can be manipulated.
   */
  vtkGetObjectMacro(EndPointProperty,vtkProperty);
  vtkGetObjectMacro(SelectedEndPointProperty,vtkProperty);
  //@}

  //@{
  /**
   * Get the end-point (sphere) properties. The properties of the end-points
   * when selected and unselected can be manipulated.
   */
  vtkGetObjectMacro(EndPoint2Property,vtkProperty);
  vtkGetObjectMacro(SelectedEndPoint2Property,vtkProperty);
  //@}

  //@{
  /**
   * Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(LineProperty,vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty,vtkProperty);
  //@}

  //@{
  /**
   * The tolerance representing the distance to the widget (in pixels) in
   * which the cursor is considered near enough to the line or end point
   * to be active.
   */
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);
  //@}

  //@{
  /**
   * Set/Get the resolution (number of subdivisions) of the line. A line with
   * resolution greater than one is useful when points along the line are
   * desired; e.g., generating a rake of streamlines.
   */
  void SetResolution(int res);
  int GetResolution();
  //@}

  /**
   * Retrieve the polydata (including points) that defines the line.  The
   * polydata consists of n+1 points, where n is the resolution of the
   * line. These point values are guaranteed to be up-to-date whenever any
   * one of the three handles are moved. To use this method, the user
   * provides the vtkPolyData as an input argument, and the points and
   * polyline are copied into it.
   */
  void GetPolyData(vtkPolyData *pd);

  //@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual double *GetBounds();
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  virtual void GetActors(vtkPropCollection *pc);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();
  //@}

  // Manage the state of the widget
  enum {Outside=0,OnP1,OnP2,TranslatingP1,TranslatingP2,OnLine,Scaling};

  //@{
  /**
   * The interaction state may be set from a widget (e.g., vtkLineWidget2) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  vtkSetClampMacro(InteractionState,int,Outside,Scaling);
  //@}

  //@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  vtkGetMacro(RepresentationState, int);
  //@}

  //@{
  /**
   * Sets the representation to be a directional line with point 1 represented
   * as a cone.
   */
  void SetDirectionalLine(bool val);
  vtkGetMacro(DirectionalLine, bool);
  vtkBooleanMacro(DirectionalLine, bool);
  //@}

  /**
   * Overload the superclasses' GetMTime() because internal classes
   * are used to keep the state of the representation.
   */
  virtual vtkMTimeType GetMTime();

  /**
   * Overridden to set the rendererer on the internal representations.
   */
  virtual void SetRenderer(vtkRenderer *ren);

  //@{
  /**
   * Show the distance between the points.
   */
  vtkSetMacro( DistanceAnnotationVisibility, int );
  vtkGetMacro( DistanceAnnotationVisibility, int );
  vtkBooleanMacro( DistanceAnnotationVisibility, int );
  //@}

  //@{
  /**
   * Specify the format to use for labelling the line. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the angle value.
   */
  vtkSetStringMacro(DistanceAnnotationFormat);
  vtkGetStringMacro(DistanceAnnotationFormat);
  //@}

  //@{
  /**
   * Scale text (font size along each dimension).
   */
  void SetDistanceAnnotationScale(double x, double y, double z)
  {
    double scale[3];
    scale[0] = x;
    scale[1] = y;
    scale[2] = z;
    this->SetDistanceAnnotationScale(scale);
  }
  virtual void SetDistanceAnnotationScale( double scale[3] );
  virtual double * GetDistanceAnnotationScale();
  //@}

  /**
   * Get the distance between the points.
   */
  double GetDistance();


  /**
   * Convenience method to set the line color.
   * Ideally one should use GetLineProperty()->SetColor().
   */
  void SetLineColor(double r, double g, double b);

  /**
   * Get the distance annotation property
   */
  virtual vtkProperty *GetDistanceAnnotationProperty();

  //@{
  /**
   * Get the text actor
   */
  vtkGetObjectMacro(TextActor, vtkFollower);
  //@}

  enum { RestrictNone = 0, RestrictToX, RestrictToY, RestrictToZ };

  /**
   * Set if translations should be restricted to one of the axes (disabled if
   * RestrictNone is specified).
   */
  vtkSetClampMacro(RestrictFlag, int, RestrictNone, RestrictToZ);

protected:
  vtkLineRepresentation();
  ~vtkLineRepresentation();

  // The handle and the rep used to close the handles
  vtkPointHandleRepresentation3D *HandleRepresentation;
  vtkPointHandleRepresentation3D *Point1Representation;
  vtkPointHandleRepresentation3D *Point2Representation;
  vtkPointHandleRepresentation3D *LineHandleRepresentation;

  // Manage how the representation appears
  int RepresentationState;
  bool DirectionalLine;

  // the line
  vtkActor          *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkLineSource     *LineSource;

  // glyphs representing hot spots (e.g., handles)
  vtkActor              **Handle;
  vtkPolyDataMapper     **HandleMapper;
  vtkPolyDataAlgorithm  **HandleGeometry;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *EndPointProperty;
  vtkProperty *SelectedEndPointProperty;
  vtkProperty *EndPoint2Property;
  vtkProperty *SelectedEndPoint2Property;
  vtkProperty *LineProperty;
  vtkProperty *SelectedLineProperty;
  void         CreateDefaultProperties();

  // Selection tolerance for the handles and the line
  int Tolerance;

  // Helper members
  int  ClampToBounds;
  void ClampPosition(double x[3]);
  void HighlightPoint(int ptId, int highlight);
  void HighlightLine(int highlight);
  int  InBounds(double x[3]);
  void SizeHandles();

  // Ivars used during widget interaction to hold initial positions
  double StartP1[3];
  double StartP2[3];
  double StartLineHandle[3];
  double Length;
  double LastEventPosition[3];

  // Support GetBounds() method
  vtkBox *BoundingBox;

  // Need to keep track if we have successfully initialized the display position.
  // The widget tends to do stuff in world coordinates, put if the renderer has
  // not been assigned, then certain operations do not properly update the display
  // position.
  int InitializedDisplayPosition;

  // Format for the label
  int DistanceAnnotationVisibility;
  char *DistanceAnnotationFormat;

  vtkFollower       *TextActor;
  vtkPolyDataMapper *TextMapper;
  vtkVectorText     *TextInput;
  double             Distance;
  bool               AnnotationTextScaleInitialized;

  vtkCellPicker     *LinePicker;

  int RestrictFlag;

private:
  vtkLineRepresentation(const vtkLineRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLineRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
