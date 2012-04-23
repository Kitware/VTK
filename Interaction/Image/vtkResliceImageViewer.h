/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceImageViewer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkResliceImageViewer - Display an image along with a reslice cursor
// .SECTION Description
// This class is similar to vtkImageViewer2. It displays the image along with
// a two cross hairs for reslicing. The cross hairs may be interactively
// manipulated and are typically used to reslice two other views of
// vtkResliceImageViewer. See QtVTKRenderWindows for an example. The reslice
// cursor is used to perform thin or thick MPR through data. The class can
// also default to the behaviour of vtkImageViewer2 if the Reslice mode is
// set to RESLICE_AXIS_ALIGNED.
// .SECTION see also
// vtkResliceCursor vtkResliceCursorWidget vtkResliceCursorRepresentation

#ifndef __vtkResliceImageViewer_h
#define __vtkResliceImageViewer_h

#include "vtkInteractionImageModule.h" // For export macro
#include "vtkImageViewer2.h"

class vtkResliceCursorWidget;
class vtkResliceCursor;
class vtkScalarsToColors;
class vtkBoundedPlanePointPlacer;
class vtkResliceImageViewerMeasurements;
class vtkResliceImageViewerScrollCallback;
class vtkPlane;

class VTKINTERACTIONIMAGE_EXPORT vtkResliceImageViewer : public vtkImageViewer2
{
public:

  // Description:
  // Standard VTK methods.
  static vtkResliceImageViewer *New();
  vtkTypeMacro(vtkResliceImageViewer,vtkImageViewer2);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Render the resulting image.
  virtual void Render();

  // Description:
  // Set/Get the input image to the viewer.
  virtual void SetInputData(vtkImageData *in);
  virtual void SetInputConnection(vtkAlgorithmOutput* input);

  // Description:
  // Set window and level for mapping pixels to colors.
  virtual void SetColorWindow(double s);
  virtual void SetColorLevel(double s);

  // Description:
  // Get the internal render window, renderer, image actor, and
  // image map instances.
  vtkGetObjectMacro(ResliceCursorWidget,vtkResliceCursorWidget);

  // Description:
  // Set/get the slice orientation
  //BTX
  enum
  {
    RESLICE_AXIS_ALIGNED = 0,
    RESLICE_OBLIQUE = 1
  };
  //ETX

  vtkGetMacro(ResliceMode, int);
  virtual void SetResliceMode(int resliceMode);
  virtual void SetResliceModeToAxisAligned()
    { this->SetResliceMode(vtkResliceImageViewer::RESLICE_AXIS_ALIGNED); };
  virtual void SetResliceModeToOblique()
    { this->SetResliceMode(vtkResliceImageViewer::RESLICE_OBLIQUE); };

  // Description:
  // Set/Get the reslice cursor.
  vtkResliceCursor * GetResliceCursor();
  void SetResliceCursor( vtkResliceCursor * rc );

  // Description:
  // Set the lookup table
  virtual void SetLookupTable( vtkScalarsToColors * );
  vtkScalarsToColors * GetLookupTable();

  // Description:
  // Switch to / from thick mode
  virtual void SetThickMode( int );
  virtual int GetThickMode();

  // Description:
  // Reset all views back to initial state
  virtual void Reset();

  // Description:
  // Get the point placer.
  vtkGetObjectMacro( PointPlacer, vtkBoundedPlanePointPlacer );

  // Description:
  // Get the measurements manager
  vtkGetObjectMacro( Measurements, vtkResliceImageViewerMeasurements );

  // Description:
  // Get the render window interactor
  vtkGetObjectMacro( Interactor, vtkRenderWindowInteractor );

  // Description:
  // Scroll slices on the mouse wheel ? In the case of MPR
  // view, it moves one "normalized spacing" in the direction of the normal to
  // the resliced plane, provided the new center will continue to lie within
  // the volume.
  vtkSetMacro( SliceScrollOnMouseWheel, int );
  vtkGetMacro( SliceScrollOnMouseWheel, int );
  vtkBooleanMacro( SliceScrollOnMouseWheel, int );

  // Description:
  // Increment/Decrement slice by 'n' slices
  virtual void IncrementSlice( int n );

  // Description:
  enum { SliceChangedEvent = 1001 };

protected:
  vtkResliceImageViewer();
  ~vtkResliceImageViewer();

  virtual void InstallPipeline();
  virtual void UnInstallPipeline();
  virtual void UpdateOrientation();
  virtual void UpdateDisplayExtent();
  virtual void UpdatePointPlacer();

  // Description:
  // Convenience methods to get the reslice plane and the normalized
  // spacing between slices in reslice mode.
  vtkPlane * GetReslicePlane();
  double GetInterSliceSpacingInResliceMode();

  vtkResliceCursorWidget            * ResliceCursorWidget;
  vtkBoundedPlanePointPlacer        * PointPlacer;
  int                                 ResliceMode;
  vtkResliceImageViewerMeasurements * Measurements;
  int                                 SliceScrollOnMouseWheel;
  vtkResliceImageViewerScrollCallback * ScrollCallback;

private:
  vtkResliceImageViewer(const vtkResliceImageViewer&);  // Not implemented.
  void operator=(const vtkResliceImageViewer&);  // Not implemented.
};

#endif
