// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkResliceCursorRepresentation
 * @brief   represent the vtkResliceCursorWidget
 *
 * This class is the base class for the reslice cursor representation
 * subclasses. It represents a cursor that may be interactively translated,
 * rotated through an image and perform thick / thick reformats.
 * @sa
 * vtkResliceCursorLineRepresentation vtkResliceCursorThickLineRepresentation
 * vtkResliceCursorWidget vtkResliceCursor
 */

#ifndef vtkResliceCursorRepresentation_h
#define vtkResliceCursorRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkTextProperty;
class vtkActor2D;
class vtkTextMapper;
class vtkImageData;
class vtkImageReslice;
class vtkPlane;
class vtkPlaneSource;
class vtkResliceCursorPolyDataAlgorithm;
class vtkResliceCursor;
class vtkMatrix4x4;
class vtkScalarsToColors;
class vtkImageMapToColors;
class vtkActor;
class vtkImageActor;
class vtkTexture;
class vtkTextActor;
class vtkImageAlgorithm;

// Private.
#define VTK_RESLICE_CURSOR_REPRESENTATION_MAX_TEXTBUFF 128

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkResliceCursorRepresentation
  : public vtkWidgetRepresentation
{
public:
  ///@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkResliceCursorRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * The tolerance representing the distance to the representation (in
   * pixels) in which the cursor is considered near enough to the
   * representation to be active.
   */
  vtkSetClampMacro(Tolerance, int, 1, 100);
  vtkGetMacro(Tolerance, int);
  ///@}

  ///@{
  /**
   * Show the resliced image ?
   */
  vtkSetMacro(ShowReslicedImage, vtkTypeBool);
  vtkGetMacro(ShowReslicedImage, vtkTypeBool);
  vtkBooleanMacro(ShowReslicedImage, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Make sure that the resliced image remains within the volume.
   * Default is On.
   */
  vtkSetMacro(RestrictPlaneToVolume, vtkTypeBool);
  vtkGetMacro(RestrictPlaneToVolume, vtkTypeBool);
  vtkBooleanMacro(RestrictPlaneToVolume, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the format to use for labelling the distance. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the thickness value.
   */
  vtkSetStringMacro(ThicknessLabelFormat);
  vtkGetStringMacro(ThicknessLabelFormat);
  ///@}

  // Used to communicate about the state of the representation
  enum
  {
    Outside = 0,
    NearCenter,
    NearAxis1,
    NearAxis2,
    OnCenter,
    OnAxis1,
    OnAxis2
  };
  enum
  {
    None = 0,
    PanAndRotate,
    RotateBothAxes,
    ResizeThickness,
    WindowLevelling,
    TranslateSingleAxis
  };

  /**
   * Get the text shown in the widget's label.
   */
  virtual char* GetThicknessLabelText();

  ///@{
  /**
   * Get the position of the widget's label in display coordinates.
   */
  virtual double* GetThicknessLabelPosition();
  virtual void GetThicknessLabelPosition(double pos[3]);
  virtual void GetWorldThicknessLabelPosition(double pos[3]);
  ///@}

  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void BuildRepresentation() override;

  ///@{
  /**
   * Get the current reslice class and reslice axes
   */
  vtkGetObjectMacro(ResliceAxes, vtkMatrix4x4);
  vtkGetObjectMacro(Reslice, vtkImageAlgorithm);
  ///@}

  ///@{
  /**
   * Get the displayed image actor
   */
  vtkGetObjectMacro(ImageActor, vtkImageActor);
  ///@}

  ///@{
  /**
   * Set/Get the internal lookuptable (lut) to one defined by the user, or,
   * alternatively, to the lut of another Reslice cusror widget.  In this way,
   * a set of three orthogonal planes can share the same lut so that
   * window-levelling is performed uniformly among planes.  The default
   * internal lut can be re- set/allocated by setting to 0 (nullptr).
   */
  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);
  ///@}

  ///@{
  /**
   * Convenience method to get the vtkImageMapToColors filter used by this
   * widget.  The user can properly render other transparent actors in a
   * scene by calling the filter's SetOutputFormatToRGB and
   * PassAlphaToOutputOff.
   */
  vtkGetObjectMacro(ColorMap, vtkImageMapToColors);
  virtual void SetColorMap(vtkImageMapToColors*);
  ///@}

  ///@{
  /**
   * Set/Get the current window and level values.  SetWindowLevel should
   * only be called after SetInput.  If a shared lookup table is being used,
   * a callback is required to update the window level values without having
   * to update the lookup table again.
   */
  void SetWindowLevel(double window, double level, int copy = 0);
  void GetWindowLevel(double wl[2]);
  double GetWindow() { return this->CurrentWindow; }
  double GetLevel() { return this->CurrentLevel; }
  ///@}

  virtual vtkResliceCursor* GetResliceCursor() = 0;

  ///@{
  /**
   * Enable/disable text display of window-level, image coordinates and
   * scalar values in a render window.
   */
  vtkSetMacro(DisplayText, vtkTypeBool);
  vtkGetMacro(DisplayText, vtkTypeBool);
  vtkBooleanMacro(DisplayText, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the text property for the image data and window-level annotation.
   */
  void SetTextProperty(vtkTextProperty* tprop);
  vtkTextProperty* GetTextProperty();
  ///@}

  ///@{
  /**
   * Render as a 2D image, or render as a plane with a texture in physical
   * space.
   */
  vtkSetMacro(UseImageActor, vtkTypeBool);
  vtkGetMacro(UseImageActor, vtkTypeBool);
  vtkBooleanMacro(UseImageActor, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable independent modification of the thickness based on the selected axis.
   * Disabled by default, which applies the modified thickness to every axis of the reslice cursor.
   */
  vtkSetMacro(IndependentThickness, bool);
  vtkGetMacro(IndependentThickness, bool);
  vtkBooleanMacro(IndependentThickness, bool);
  ///@}

  ///@{
  /**
   * INTERNAL - Do not use
   * Set the manipulation mode. This is done by the widget
   */
  void SetManipulationMode(int m);
  vtkGetMacro(ManipulationMode, int);
  ///@}

  ///@{
  /**
   * INTERNAL - Do not use.
   * Internal methods used by the widget to manage text displays
   * for annotations.
   */
  void ActivateText(int);
  void ManageTextDisplay();
  ///@}

  ///@{
  /**
   * Initialize the reslice planes and the camera center. This is done
   * automatically, the first time we render.
   */
  virtual void InitializeReslicePlane();
  virtual void ResetCamera();
  ///@}

  /**
   * Get the underlying cursor source.
   */
  virtual vtkResliceCursorPolyDataAlgorithm* GetCursorAlgorithm() = 0;

  ///@{
  /**
   * Get the plane source on which the texture (the thin/thick resliced
   * image is displayed)
   */
  vtkGetObjectMacro(PlaneSource, vtkPlaneSource);
  ///@}

  /**
   * Fit the plane defined by origin, p1, p2 onto the bounds.
   * Plane is untouched if does not intersect bounds.
   * return 1 if the bounds is intersected, else 0
   */
  static int BoundPlane(double bounds[6], double origin[3], double p1[3], double p2[3]);
  /**
   * First rotate planeToTransform to match targetPlane normal.
   * Then rotate around targetNormal to enforce targetViewUp "up" vector (i.e. Origin->p2 ).
   * There is an infinite number of options to rotate a plane normal to another. Here we attempt to
   * preserve Origin, P1 and P2 when rotating around targetPlane.
   */
  static void TransformPlane(vtkPlaneSource* planeToTransform, double targetCenter[3],
    double targetNormal[3], double targetViewUp[3]);

protected:
  vtkResliceCursorRepresentation();
  ~vtkResliceCursorRepresentation() override;

  ///@{
  /**
   * Create New Reslice plane. Allows subclasses to override and create
   * their own reslice filters to respond to the widget.
   */
  virtual void CreateDefaultResliceAlgorithm();
  virtual void SetResliceParameters(
    double outputSpacingX, double outputSpacingY, int extentX, int extentY);
  ///@}

  /**
   * Process window level
   */
  virtual void WindowLevel(double x, double y);

  /**
   * Update the reslice plane
   */
  virtual void UpdateReslicePlane();

  /**
   * Compute the origin of the planes so as to capture the entire image.
   */
  virtual void ComputeReslicePlaneOrigin();

  // for negative window values.
  void InvertTable();

  // recompute origin to make the location of the reslice cursor consistent
  // with its physical location
  virtual void ComputeOrigin(vtkMatrix4x4*);

  ///@{
  void GetVector1(double d[3]);
  void GetVector2(double d[3]);
  ///@}

  /**
   * The widget sets the manipulation mode. This can be one of :
   * None, PanAndRotate, RotateBothAxes, ResizeThickness
   */
  int ManipulationMode;

  // Keep track if modifier is set
  int Modifier;

  // Selection tolerance for the handles
  int Tolerance;

  // Format for printing the distance
  char* ThicknessLabelFormat;

  vtkImageAlgorithm* Reslice;
  vtkPlaneSource* PlaneSource;
  vtkTypeBool RestrictPlaneToVolume;
  vtkTypeBool ShowReslicedImage;
  vtkTextProperty* ThicknessTextProperty;
  vtkTextMapper* ThicknessTextMapper;
  vtkActor2D* ThicknessTextActor;
  vtkMatrix4x4* ResliceAxes;
  vtkMatrix4x4* NewResliceAxes;
  vtkImageMapToColors* ColorMap;
  vtkActor* TexturePlaneActor;
  vtkTexture* Texture;
  vtkScalarsToColors* LookupTable;
  vtkImageActor* ImageActor;
  vtkTextActor* TextActor;
  double OriginalWindow;
  double OriginalLevel;
  double CurrentWindow;
  double CurrentLevel;
  double InitialWindow;
  double InitialLevel;
  double LastEventPosition[2];
  vtkTypeBool UseImageActor;
  char TextBuff[VTK_RESLICE_CURSOR_REPRESENTATION_MAX_TEXTBUFF];
  vtkTypeBool DisplayText;
  bool IndependentThickness = false;

  vtkScalarsToColors* CreateDefaultLookupTable();
  void GenerateText();

private:
  vtkResliceCursorRepresentation(const vtkResliceCursorRepresentation&) = delete;
  void operator=(const vtkResliceCursorRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
