/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePlaneWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImagePlaneWidget - 3D widget for reslicing image data
// .SECTION Description
// This 3D widget defines a plane that can be interactively placed in an
// image volume. A nice feature of the object is that the
// vtkImagePlaneWidget, like any 3D widget, will work with the current
// interactor style. That is, if vtkImagePlaneWidget does not handle an
// event, then all other registered observers (including the interactor
// style) have an opportunity to process the event. Otherwise, the
// vtkImagePlaneWidget will terminate the processing of the event that it
// handles.
//
// The core functionality of the widget is provided by a vtkImageReslice
// object which passes its output onto a texture mapping pipeline for fast
// slicing through volumetric data. See the key methods: GenerateTexturePlane(),
// UpdateOrigin() and UpdateNormal() for implementation details.
//
// To use this object, just invoke SetInteractor() with the argument of the
// method a vtkRenderWindowInteractor.  You may also wish to invoke
// "PlaceWidget()" to initially position the widget. If the "i" key (for
// "interactor") is pressed, the vtkImagePlaneWidget will appear. (See
// superclass documentation for information about changing this behavior.)
//
// Selecting the widget with the middle mouse button with and without holding
// the shift or control keys enables complex reslicing capablilites.
// To facilitate use, a set of 'margins' (left, right, top, bottom) are shown as
// a set of plane-axes aligned lines, the properties of which can be changed
// as a group.
// Without keyboard modifiers: selecting in the middle of the margins
// enables translation of the plane along its normal. Selecting one of the
// corners within the margins enables spinning around the plane's normal at its
// center.  Selecting within a margin allows rotating about the center of the
// plane around an axis aligned with the margin (i.e., selecting left margin
// enables rotating around the plane's local y-prime axis).
// With control key modifier: margin selection enables edge translation (i.e., a
// constrained form of scaling). Selecting within the margins enables
// translation of the entire plane.
// With shift key modifier: uniform plane scaling is enabled.  Moving the mouse
// up enlarges the plane while downward movement shrinks it.
//
// Window-level is achieved by using the right mouse button.
// The left mouse button can be used to query the underlying image data
// with a cross-hair cursor. Text display of window-level and image
// coordinates/data values are provided by a text actor/mapper pair.
// Events that occur outside of the widget (i.e., no part of the widget is
// picked) are propagated to any other registered obsevers (such as the
// interaction style). Turn off the widget by pressing the "i" key again
// (or invoke the Off() method).
//
// The vtkImagePlaneWidget has several methods that can be used in
// conjunction with other VTK objects. The GetPolyData() method can be used
// to get the polygonal representation of the plane and can be used as input
// for other VTK objects. Typical usage of the widget is to make use of the
// StartInteractionEvent, InteractionEvent, and EndInteractionEvent
// events. The InteractionEvent is called on mouse motion; the other two
// events are called on button down and button up (either left or right
// button).
//
// Some additional features of this class include the ability to control the
// properties of the widget. You can set the properties of: the selected and
// unselected representations of the plane's outline; the text actor via its
// vtkTextProperty; the cross-hair cursor. In addition there are methods to
// constrain the plane so that it is aligned along the x-y-z axes.  Finally,
// one can specify the degree of interpolation (vtkImageReslice): nearest
// neighbour, linear, and cubic.

// .SECTION Thanks
// Thanks to Dean Inglis for developing and contributing this class.
// Based on the Python SlicePlaneFactory from Atamai, Inc.

// .SECTION Caveats
// Note that handles and plane can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkLineWidget  vtkPlaneWidget vtkPointWidget
// vtkPolyDataSourceWidget vtkSphereWidget vtkImplicitPlaneWidget


#ifndef __vtkImagePlaneWidget_h
#define __vtkImagePlaneWidget_h

#include "vtkPolyDataSourceWidget.h"

class vtkActor;
class vtkCellPicker;
class vtkDataSetMapper;
class vtkImageData;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkLookupTable;
class vtkMatrix4x4;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;
class vtkTextActor;
class vtkTextProperty;
class vtkTexture;
class vtkTextureMapToPlane;
class vtkTransform;

#define VTK_NEAREST_RESLICE 0
#define VTK_LINEAR_RESLICE  1
#define VTK_CUBIC_RESLICE   2

class VTK_EXPORT vtkImagePlaneWidget : public vtkPolyDataSourceWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkImagePlaneWidget *New();

  vtkTypeRevisionMacro(vtkImagePlaneWidget,vtkPolyDataSourceWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods that satisfy the superclass' API.
  virtual void SetEnabled(int);
  virtual void PlaceWidget(float bounds[6]);
  void PlaceWidget()
    {this->Superclass::PlaceWidget();}
  void PlaceWidget(float xmin, float xmax, float ymin, float ymax,
                   float zmin, float zmax)
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}

  // Description:
  // Set the vtkImageData* input for the vtkImageReslice.
  void SetInput(vtkDataSet* input);

  // Description:
  // Set/Get the origin of the plane.
  void SetOrigin(float x, float y, float z);
  void SetOrigin(float x[3]);
  float* GetOrigin();
  void GetOrigin(float xyz[3]);

  // Description:
  // Set/Get the position of the point defining the first axis of the plane.
  void SetPoint1(float x, float y, float z);
  void SetPoint1(float x[3]);
  float* GetPoint1();
  void GetPoint1(float xyz[3]);

  // Description:
  // Set/Get the position of the point defining the second axis of the plane.
  void SetPoint2(float x, float y, float z);
  void SetPoint2(float x[3]);
  float* GetPoint2();
  void GetPoint2(float xyz[3]);

  // Description:
  // Get the center of the plane.
  float* GetCenter();
  void GetCenter(float xyz[3]);

  // Description:
  // Get the normal to the plane.
  float* GetNormal();
  void GetNormal(float xyz[3]);

  // Description:
  // Get the vector from the plane origin to point1.
  void GetVector1(float v1[3]);

  // Description:
  // Get the vector from the plane origin to point2.
  void GetVector2(float v2[3]);  

  // Description:
  // Get the slice position in terms of the data extent.
  int GetSliceIndex();

  // Description:
  // Set the slice position in terms of the data extent.
  void SetSliceIndex(int index);

  // Description:
  // Get the position of the slice along its normal.
  float GetSlicePosition();

  // Description:
  // Set the position of the slice along its normal.
  void SetSlicePosition(float position);

  // Description:
  // Set the interpolation to use when texturing the plane.  
  void SetResliceInterpolate(int);
  vtkGetMacro(ResliceInterpolate,int);
  void SetResliceInterpolateToNearestNeighbour()
    { this->SetResliceInterpolate(VTK_NEAREST_RESLICE); }
  void SetResliceInterpolateToLinear()
    { this->SetResliceInterpolate(VTK_LINEAR_RESLICE); }
  void SetResliceInterpolateToCubic()
    { this->SetResliceInterpolate(VTK_CUBIC_RESLICE); }

  // Description:
  // Convenience method to get the vtkImageReslice output.
  vtkImageData* GetResliceOutput();

  // Description:
  // Make sure that the plane remains within the volume.
  // Default is On.
  vtkSetMacro(RestrictPlaneToVolume,int);
  vtkGetMacro(RestrictPlaneToVolume,int);
  vtkBooleanMacro(RestrictPlaneToVolume,int);

  // Description:
  // Specify whether to interpolate the texture or not. When off, the
  // reslice interpolation is nearest neighbour regardless of how the
  // interpolation is set through the API. Set before setting the
  // vtkImageData imput. Default is On.
  vtkSetMacro(TextureInterpolate,int);
  vtkGetMacro(TextureInterpolate,int);
  vtkBooleanMacro(TextureInterpolate,int);

  // Description:
  // Grab the polydata (including points) that defines the plane.  The
  // polydata consists of (res+1)*(res+1) points, and res*res quadrilateral
  // polygons, where res is the resolution of the plane. These point values
  // are guaranteed to be up-to-date when either the InteractionEvent or
  // EndInteraction events are invoked. The user provides the vtkPolyData and
  // the points and polyplane are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Satisfies superclass API.  This returns a pointer to the underlying
  // PolyData.  Make changes to this before calling the initial PlaceWidget()
  // to have the initial placement follow suit.  Or, make changes after the
  // widget has been initialised and call UpdatePlacement() to realise.
  vtkPolyDataSource* GetPolyDataSource();
   
  // Description:
  // Satisfies superclass API.  This will change the state of the widget to
  // match changes that have been made to the underlying PolyDataSource
  void UpdatePlacement(void);

  // Description:
  // Convenience method to get the texture used by this widget.  This can be
  // used in external slice viewers.
  vtkTexture *GetTexture();

  // Description:
  // Get the plane's outline properties. The properties of the plane's outline
  // when selected and unselected can be manipulated.
  virtual void SetPlaneProperty(vtkProperty*);
  vtkGetObjectMacro(PlaneProperty,vtkProperty);
  virtual void SetSelectedPlaneProperty(vtkProperty*);
  vtkGetObjectMacro(SelectedPlaneProperty,vtkProperty);

  // Description:
  // Convenience method sets the plane orientation normal to the
  // x, y, or z axes.  Default is XAxes (0).
  void SetPlaneOrientation(int);
  vtkGetMacro(PlaneOrientation,int);
  void SetPlaneOrientationToXAxes()
    { this->SetPlaneOrientation(0); }
  void SetPlaneOrientationToYAxes()
    { this->SetPlaneOrientation(1); }
  void SetPlaneOrientationToZAxes()
    { this->SetPlaneOrientation(2); }

  // Description:
  // Set the internal picker to one defined by the user.  In this way,
  // a set of three orthogonal planes can share the same picker so that
  // picking is performed correctly.  The default internal picker can be
  // re-set/allocated by setting to 0 (NULL).
  void SetPicker(vtkCellPicker*);

  // Description:
  // Set the internal lookuptable (lut) to one defined by the user, or,
  // alternatively, to the lut of another vtkImgePlaneWidget.  In this way,
  // a set of three orthogonal planes can share the same lut so that
  // window-levelling is performed uniformly among planes.  The default
  // internal lut can be re-set/allocated by setting to 0 (NULL).
  virtual void SetLookupTable(vtkLookupTable*);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);

  // Description:
  // Enable/disable text display of window-level, image coords and values in a
  // render window.
  vtkSetMacro(DisplayText,int);
  vtkGetMacro(DisplayText,int);
  vtkBooleanMacro(DisplayText,int);

  // Description:
  // Set the properties of the cross-hair cursor.
  virtual void SetCursorProperty(vtkProperty*);
  vtkGetObjectMacro(CursorProperty,vtkProperty);

  // Description:
  // Set the properties of the margins.
  virtual void SetMarginProperty(vtkProperty*);
  vtkGetObjectMacro(MarginProperty,vtkProperty);

  // Description:
  // Set/Get the text property for the image data and window-level annotation.
  void SetTextProperty(vtkTextProperty* tprop);
  vtkTextProperty* GetTextProperty();

  // Description:
  // Set/Get the property for the resliced image.
  virtual void SetTexturePlaneProperty(vtkProperty*);
  vtkGetObjectMacro(TexturePlaneProperty,vtkProperty);

  // Description:
  // Get the current window and level values.
  void GetWindowLevel(float wl[2]);

  // Description:
  // Get the image coordinate position and voxel value.  Currently only
  // supports single component image data.
  int GetCursorData(float xyzv[4]);

  // Description:
  // Enable/disable mouse interaction so the widget remains on display.
  void SetInteraction(int interact);
  vtkGetMacro(Interaction,int);
  void InteractionOn()
    { this->SetInteraction(1); }
  void InteractionOff()
    { this->SetInteraction(0); }

protected:
  vtkImagePlaneWidget();
  ~vtkImagePlaneWidget();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    Cursoring,
    WindowLevelling,
    Pushing,
    Spinning,
    Rotating,
    Moving,
    Scaling,
    Outside
  };
//ETX

  // Handles the events
  static void ProcessEvents(vtkObject* object,
                            unsigned long event,
                            void* clientdata,
                            void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnMouseMove();
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnMiddleButtonDown();
  void OnMiddleButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();

  // controlling ivars
  int   Interaction; // Is the widget responsive to mouse events  
  int   PlaneOrientation;
  int   RestrictPlaneToVolume;
  float OriginalWindow;
  float OriginalLevel;
  float CurrentWindow;
  float CurrentLevel;
  int   ResliceInterpolate;
  int   TextureInterpolate;
  int   UserPickerEnabled;
  int   UserLookupTableEnabled;
  int   DisplayText;

  // The geometric represenation of the plane and it's outline
  vtkPlaneSource    *PlaneSource;
  float              Normal[3]; // plane normal normalized
  vtkPolyData       *PlaneOutlinePolyData;
  vtkActor          *PlaneOutlineActor;
  vtkPolyDataMapper *PlaneOutlineMapper;
  void               HighlightPlane(int highlight);
  void               GeneratePlaneOutline();

  // Re-builds the plane outline based on the plane source
  void BuildRepresentation();

  // Do the picking
  vtkCellPicker *PlanePicker;

  // Methods to manipulate the plane
  void WindowLevel(int X, int Y);
  void Push(double *p1, double *p2);
  void Spin(double *p1, double *p2);
  void Rotate(double *p1, double *p2, double *vpn);
  void Scale(double *p1, double *p2, int X, int Y);
  void Translate(double *p1, double *p2);

  vtkImageData         *ImageData;
  vtkImageReslice      *Reslice;
  vtkMatrix4x4         *ResliceAxes;
  vtkTransform         *Transform;
  vtkTextureMapToPlane *TexturePlaneCoords;
  vtkDataSetMapper     *TexturePlaneMapper;
  vtkActor             *TexturePlaneActor;
  vtkLookupTable       *LookupTable;
  vtkImageMapToColors  *ColorMap;
  vtkTexture           *Texture;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.  The plane property is actually that for
  // the outline.  The TexturePlaneProperty can be used to control the
  // lighting etc. of the resliced image data.
  vtkProperty   *PlaneProperty;
  vtkProperty   *SelectedPlaneProperty;
  vtkProperty   *CursorProperty;
  vtkProperty   *MarginProperty;
  vtkProperty   *TexturePlaneProperty;
  void           CreateDefaultProperties();

  // Reslice and texture management
  void UpdateNormal();
  void UpdateOrigin();
  void GenerateTexturePlane();

  // The cross-hair cursor
  vtkPoints         *CursorPoints;
  vtkPolyData       *CursorPolyData;
  vtkPolyDataMapper *CursorMapper;
  vtkActor          *CursorActor;
  int                CurrentCursorPosition[3];
  float              CurrentImageValue; // Set to VTK_FLOAT_MAX when invalid
  void               GenerateCursor();
  void               UpdateCursor(int,int);
  void               ActivateCursor(int);
  void               ComputeImageToWorldCoords(float* in, float* out);
  void               ComputeWorldToImageCoords(float* in, float* out);

  // The text to display W/L, image data
  vtkTextActor *TextActor;
  char          TextBuff[128];
  void          GenerateText();
  void          ManageTextDisplay();
  void          ActivateText(int);

  // Oblique reslice control
  float RotateAxis[3];
  float RadiusVector[3];
  void  AdjustState();

  // Visible margins to assist user interaction
  vtkPoints         *MarginPoints;
  vtkPolyData       *MarginPolyData;
  vtkPolyDataMapper *MarginMapper;
  vtkActor          *MarginActor;
  int                MarginSelectMode;
  void               GenerateMargins();
  void               UpdateMargins();
  void               ActivateMargins(int);

private:
  vtkImagePlaneWidget(const vtkImagePlaneWidget&);  //Not implemented
  void operator=(const vtkImagePlaneWidget&);  //Not implemented
};

#endif
