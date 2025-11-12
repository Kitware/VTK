// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiLineRepresentation
 * @brief   a class defining the representation for a vtkMultiLineWidget
 *
 * This class is a concrete representation for the vtkMultiLineWidget. It
 * represents multiple straight lines with three handles: one at the beginning and
 * ending of each line, and one used to translate each line.
 *
 *
 * @sa
 * vtkMultiLineWidget
 */

#ifndef vtkMultiLineRepresentation_h
#define vtkMultiLineRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkBox;
class vtkDoubleArray;
class vtkLineRepresentation;
class vtkPolyData;
class vtkProperty;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkMultiLineRepresentation
  : public vtkWidgetRepresentation
{
public:
  static vtkMultiLineRepresentation* New();

  vtkTypeMacro(vtkMultiLineRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * a line representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(int index, double pos[3]) VTK_FUTURE_CONST;
  double* GetPoint1WorldPosition(int index) VTK_SIZEHINT(3);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void GetPoint1DisplayPosition(int index, double pos[3]) VTK_FUTURE_CONST;
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  double* GetPoint1DisplayPosition(int index) VTK_SIZEHINT(3);
  void SetPoint1WorldPosition(int index, double pos[3]);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetPoint1DisplayPosition(int index, double pos[3]);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void GetPoint2DisplayPosition(int index, double pos[3]) VTK_FUTURE_CONST;
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  double* GetPoint2DisplayPosition(int index) VTK_SIZEHINT(3);
  void GetPoint2WorldPosition(int index, double pos[3]) VTK_FUTURE_CONST;
  double* GetPoint2WorldPosition(int index) VTK_SIZEHINT(3);
  void SetPoint2WorldPosition(int index, double pos[3]);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetPoint2DisplayPosition(int index, double pos[3]);

  void SetPoint1WorldPosition(int index, double x, double y, double z);
  void SetPoint1DisplayPosition(int index, double x, double y, double z);
  void SetPoint2WorldPosition(int index, double x, double y, double z);
  void SetPoint2DisplayPosition(int index, double x, double y, double z);

  vtkDoubleArray* GetPoint1WorldPositions();
  vtkDoubleArray* GetPoint2WorldPositions();
  vtkDoubleArray* GetPoint1DisplayPositions();
  vtkDoubleArray* GetPoint2DisplayPositions();
  ///@}

  ///@{
  /**
   * Set/Get the number of vtkLineRepresentation in this widget.
   */
  void SetLineCount(int n);
  vtkGetMacro(LineCount, int)
  ///@}

  ///@{
  /**
   * Get the end-point properties. The properties of the end-points
   * when selected and unselected can be manipulated.
   */
  vtkGetObjectMacro(EndPointProperty, vtkProperty);
  vtkGetObjectMacro(SelectedEndPointProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the end-point properties. The properties of the end-points
   * when selected and unselected can be manipulated.
   */
  vtkGetObjectMacro(EndPoint2Property, vtkProperty);
  vtkGetObjectMacro(SelectedEndPoint2Property, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the lines properties. The properties of the lines when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(LineProperty, vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * The tolerance representing the distance to a line (in pixels) in
   * which the cursor is considered near enough to a line or end point
   * to be active. The value is clamped between 1 and 100.
   */
  void SetTolerance(int tol);
  vtkGetMacro(Tolerance, int);
  ///@}

  ///@{
  /**
   * Set/Get the resolution (number of subdivisions, minimum 1) of the line. A line with
   * resolution greater than one is useful when points along the line are
   * desired; e.g., generating a rake of streamlines.
   */
  void SetResolution(int res);
  vtkGetMacro(Resolution, int);
  ///@}

  /**
   * Retrieve the polydata (including points) that defines the line.  The
   * polydata consists of n+1 points, where n is the resolution of the
   * line. These point values are guaranteed to be up-to-date whenever any
   * one of the three handles are moved. To use this method, the user
   * provides the vtkPolyData as an input argument, and the points and
   * polyline are copied into it.
   */
  void GetPolyData(int index, vtkPolyData* pd);

  ///@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  double* GetBounds() VTK_SIZEHINT(6) override;
  ///@}

  // Manage the state of the widget (with his correspondent in vtkLineRepresentation next to him)
  enum
  {
    MOUSE_OUTSIDE_LINES = 0, // Outside
    MOUSE_ON_P1,             // OnP1
    MOUSE_ON_P2,             // OnP2
    TRANSLATING_P1,          // TranslatingP1
    TRANSLATING_P2,          // TranslatingP2
    MOUSE_ON_LINE,           // OnLine
    SCALING                  // Scaling
  };

  ///@{
  /**
   * The interaction state may be set from a widget (e.g., vtkMultiLineRepresentation) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  vtkSetClampMacro(InteractionState, int, MOUSE_OUTSIDE_LINES, MOUSE_ON_LINE);
  ///@}

  ///@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  vtkGetMacro(RepresentationState, int);
  ///@}

  ///@{
  /**
   * Methods supporting the rendering process.
   */
  void GetActors(vtkPropCollection* pc) override;
  void ReleaseGraphicsResources(vtkWindow* window) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  ///@{
  /**
   * Sets the representation of each line to be a directional line with point 1 represented
   * as a cone.
   */
  void SetDirectionalLine(bool val);
  vtkGetMacro(DirectionalLine, bool);
  vtkBooleanMacro(DirectionalLine, bool);
  ///@}

  /**
   * Gets the individual representation of a specific line as a vtkLineRepresentation.
   */
  vtkLineRepresentation* GetLineRepresentation(int index);

  /**
   * Overload the superclasses' GetMTime() because internal classes
   * are used to keep the state of the representation.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Overridden to set the rendererer on the internal representations.
   */
  void SetRenderer(vtkRenderer* ren) override;

  /**
   * Get the distance between the points of the line at the provided index.
   */
  double GetDistance(int index);

  /**
   * Convenience method to set the lines color.
   * Ideally one should use GetLineProperty()->SetColor().
   */
  void SetLineColor(double r, double g, double b);

  ///@{
  /**
   * Set the widget color, and the color of interactive handles.
   */
  void SetInteractionColor(double, double, double);
  void SetInteractionColor(double c[3]) { this->SetInteractionColor(c[0], c[1], c[2]); }
  void SetForegroundColor(double, double, double);
  void SetForegroundColor(double c[3]) { this->SetForegroundColor(c[0], c[1], c[2]); }
  ///@}

protected:
  vtkMultiLineRepresentation();
  ~vtkMultiLineRepresentation() override;

private:
  vtkMultiLineRepresentation(const vtkMultiLineRepresentation&) = delete;
  void operator=(const vtkMultiLineRepresentation&) = delete;

  void CreateDefaultProperties();

  // Helper methods
  void AddNewLine(int index);
  void ApplyProperties(int index);
  void UpdatePoint1Positions();
  void UpdatePoint2Positions();

  // Manage how the representation appears
  bool DirectionalLine = false;

  int LineCount = 0;
  std::vector<vtkSmartPointer<vtkLineRepresentation>> LineRepresentationVector;

  vtkNew<vtkDoubleArray> Point1WorldPositions;
  vtkNew<vtkDoubleArray> Point2WorldPositions;

  vtkNew<vtkDoubleArray> Point1DisplayPositions;
  vtkNew<vtkDoubleArray> Point2DisplayPositions;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkNew<vtkProperty> EndPointProperty;
  vtkNew<vtkProperty> SelectedEndPointProperty;
  vtkNew<vtkProperty> EndPoint2Property;
  vtkNew<vtkProperty> SelectedEndPoint2Property;
  vtkNew<vtkProperty> LineProperty;
  vtkNew<vtkProperty> SelectedLineProperty;

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;

  // Selection tolerance for the handles and the lines
  int Tolerance = 5;

  int Resolution = 5;

  int RepresentationState = vtkMultiLineRepresentation::MOUSE_OUTSIDE_LINES;
};

VTK_ABI_NAMESPACE_END
#endif
