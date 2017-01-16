/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKINTERACTIONWIDGETS_EXPORT vtkResliceCursorRepresentation : public vtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkResliceCursorRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * The tolerance representing the distance to the representation (in
   * pixels) in which the cursor is considered near enough to the
   * representation to be active.
   */
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);
  //@}

  //@{
  /**
   * Show the resliced image ?
   */
  vtkSetMacro( ShowReslicedImage, int );
  vtkGetMacro( ShowReslicedImage, int );
  vtkBooleanMacro( ShowReslicedImage, int );
  //@}

  //@{
  /**
   * Make sure that the resliced image remains within the volume.
   * Default is On.
   */
  vtkSetMacro(RestrictPlaneToVolume,int);
  vtkGetMacro(RestrictPlaneToVolume,int);
  vtkBooleanMacro(RestrictPlaneToVolume,int);
  //@}

  //@{
  /**
   * Specify the format to use for labelling the distance. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the thickness value.
   */
  vtkSetStringMacro(ThicknessLabelFormat);
  vtkGetStringMacro(ThicknessLabelFormat);
  //@}

  // Used to communicate about the state of the representation
  enum { Outside=0, NearCenter, NearAxis1, NearAxis2,
         OnCenter, OnAxis1, OnAxis2};
  enum { None=0, PanAndRotate, RotateBothAxes,
         ResizeThickness, WindowLevelling };

  /**
   * Get the text shown in the widget's label.
   */
  virtual char* GetThicknessLabelText();

  //@{
  /**
   * Get the position of the widget's label in display coordinates.
   */
  virtual double* GetThicknessLabelPosition();
  virtual void GetThicknessLabelPosition(double pos[3]);
  virtual void GetWorldThicknessLabelPosition(double pos[3]);
  //@}

  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void BuildRepresentation() VTK_OVERRIDE;

  //@{
  /**
   * Get the current reslice class and reslice axes
   */
  vtkGetObjectMacro( ResliceAxes, vtkMatrix4x4 );
  vtkGetObjectMacro( Reslice, vtkImageAlgorithm );
  //@}

  //@{
  /**
   * Get the displayed image actor
   */
  vtkGetObjectMacro( ImageActor, vtkImageActor );
  //@}

  //@{
  /**
   * Set/Get the internal lookuptable (lut) to one defined by the user, or,
   * alternatively, to the lut of another Reslice cusror widget.  In this way,
   * a set of three orthogonal planes can share the same lut so that
   * window-levelling is performed uniformly among planes.  The default
   * internal lut can be re- set/allocated by setting to 0 (NULL).
   */
  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);
  //@}

  //@{
  /**
   * Convenience method to get the vtkImageMapToColors filter used by this
   * widget.  The user can properly render other transparent actors in a
   * scene by calling the filter's SetOutputFormatToRGB and
   * PassAlphaToOutputOff.
   */
  vtkGetObjectMacro(ColorMap, vtkImageMapToColors);
  virtual void SetColorMap(vtkImageMapToColors *);
  //@}

  //@{
  /**
   * Set/Get the current window and level values.  SetWindowLevel should
   * only be called after SetInput.  If a shared lookup table is being used,
   * a callback is required to update the window level values without having
   * to update the lookup table again.
   */
  void SetWindowLevel(double window, double level, int copy = 0);
  void GetWindowLevel(double wl[2]);
  double GetWindow(){return this->CurrentWindow;}
  double GetLevel(){return this->CurrentLevel;}
  //@}

  virtual vtkResliceCursor * GetResliceCursor() = 0;

  //@{
  /**
   * Enable/disable text display of window-level, image coordinates and
   * scalar values in a render window.
   */
  vtkSetMacro(DisplayText,int);
  vtkGetMacro(DisplayText,int);
  vtkBooleanMacro(DisplayText,int);
  //@}

  //@{
  /**
   * Set/Get the text property for the image data and window-level annotation.
   */
  void SetTextProperty(vtkTextProperty* tprop);
  vtkTextProperty* GetTextProperty();
  //@}

  //@{
  /**
   * Render as a 2D image, or render as a plane with a texture in physical
   * space.
   */
  vtkSetMacro( UseImageActor, int );
  vtkGetMacro( UseImageActor, int );
  vtkBooleanMacro( UseImageActor, int );
  //@}

  //@{
  /**
   * INTERNAL - Do not use
   * Set the manipulation mode. This is done by the widget
   */
  void SetManipulationMode( int m );
  vtkGetMacro(ManipulationMode, int);
  //@}

  //@{
  /**
   * INTERNAL - Do not use.
   * Internal methods used by the widget to manage text displays
   * for annotations.
   */
  void ActivateText(int);
  void ManageTextDisplay();
  //@}

  //@{
  /**
   * Initialize the reslice planes and the camera center. This is done
   * automatically, the first time we render.
   */
  virtual void InitializeReslicePlane();
  virtual void ResetCamera();
  //@}

  /**
   * Get the underlying cursor source.
   */
  virtual vtkResliceCursorPolyDataAlgorithm * GetCursorAlgorithm() = 0;

  //@{
  /**
   * Get the plane source on which the texture (the thin/thick resliced
   * image is displayed)
   */
  vtkGetObjectMacro( PlaneSource, vtkPlaneSource );
  //@}

protected:
  vtkResliceCursorRepresentation();
  ~vtkResliceCursorRepresentation() VTK_OVERRIDE;

  //@{
  /**
   * Create New Reslice plane. Allows subclasses to override and crate
   * their own reslice filters to respond to the widget.
   */
  virtual void CreateDefaultResliceAlgorithm();
  virtual void SetResliceParameters(
      double outputSpacingX, double outputSpacingY,
      int extentX, int extentY );
  //@}

  /**
   * Process window level
   */
  virtual void WindowLevel( double x, double y );

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
  virtual void ComputeOrigin( vtkMatrix4x4 * );

  //@{
  void GetVector1( double d[3] );
  void GetVector2( double d[3] );
  //@}

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
  char *ThicknessLabelFormat;

  vtkImageAlgorithm       * Reslice;
  vtkPlaneSource          * PlaneSource;
  int                       RestrictPlaneToVolume;
  int                       ShowReslicedImage;
  vtkTextProperty         * ThicknessTextProperty;
  vtkTextMapper           * ThicknessTextMapper;
  vtkActor2D              * ThicknessTextActor;
  vtkMatrix4x4            * ResliceAxes;
  vtkMatrix4x4            * NewResliceAxes;
  vtkImageMapToColors     * ColorMap;
  vtkActor                * TexturePlaneActor;
  vtkTexture              * Texture;
  vtkScalarsToColors      * LookupTable;
  vtkImageActor           * ImageActor;
  vtkTextActor            * TextActor;
  double                    OriginalWindow;
  double                    OriginalLevel;
  double                    CurrentWindow;
  double                    CurrentLevel;
  double                    InitialWindow;
  double                    InitialLevel;
  double                    LastEventPosition[2];
  int                       UseImageActor;
  char                      TextBuff[128];
  int                       DisplayText;

  vtkScalarsToColors      * CreateDefaultLookupTable();
  void                      GenerateText();

private:
  vtkResliceCursorRepresentation(const vtkResliceCursorRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResliceCursorRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
