/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageTracerWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkExtractVOI.h"
#include "vtkGlyphSource2D.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkImageStencil.h"
#include "vtkImageTracerWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleImage.h"
#include "vtkLinearExtrusionFilter.h"
#include "vtkMapper.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataToImageStencil.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSplineWidget.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkVolume16Reader.h"

#include "vtkTestUtilities.h"

const char ImageTracerWidgetEventLog[] =
  "# StreamVersion 1\n"
  "MouseMoveEvent 322 145 0 0 0 0  b\n"
  "LeftButtonPressEvent 322 145 0 0 0 0  b\n"
  "LeftButtonReleaseEvent 322 145 0 0 0 0  b\n"
  "MouseMoveEvent 146 166 0 0 0 0  b\n"
  "LeftButtonPressEvent 146 166 0 0 0 0  b\n"
  "MouseMoveEvent 154 161 0 0 0 0  b\n"
  "MouseMoveEvent 162 148 0 0 0 0  b\n"
  "MouseMoveEvent 169 129 0 0 0 0  b\n"
  "MouseMoveEvent 168 100 0 0 0 0  b\n"
  "MouseMoveEvent 161 95 0 0 0 0  b\n"
  "MouseMoveEvent 131 90 0 0 0 0  b\n"
  "MouseMoveEvent 113 95 0 0 0 0  b\n"
  "MouseMoveEvent 77 116 0 0 0 0  b\n"
  "MouseMoveEvent 68 132 0 0 0 0  b\n"
  "MouseMoveEvent 67 151 0 0 0 0  b\n"
  "MouseMoveEvent 73 165 0 0 0 0  b\n"
  "MouseMoveEvent 89 179 0 0 0 0  b\n"
  "MouseMoveEvent 98 182 0 0 0 0  b\n"
  "MouseMoveEvent 111 182 0 0 0 0  b\n"
  "MouseMoveEvent 118 182 0 0 0 0  b\n"
  "MouseMoveEvent 130 177 0 0 0 0  b\n"
  "MouseMoveEvent 134 175 0 0 0 0  b\n"
  "MouseMoveEvent 144 170 0 0 0 0  b\n"
  "MouseMoveEvent 146 167 0 0 0 0  b\n"
  "LeftButtonReleaseEvent 146 167 0 0 0 0  b\n"
  "MouseMoveEvent 132 164 0 0 0 0  b\n"
  "MiddleButtonPressEvent 132 164 0 0 0 0  b\n"
  "MiddleButtonReleaseEvent 132 164 0 0 0 0  b\n"
  "MouseMoveEvent 131 163 0 0 0 0  b\n"
  "MouseMoveEvent 127 161 0 0 0 0  b\n"
  "MouseMoveEvent 120 153 0 0 0 0  b\n"
  "MouseMoveEvent 110 146 0 0 0 0  b\n"
  "MouseMoveEvent 104 140 0 0 0 0  b\n"
  "MouseMoveEvent 101 132 0 0 0 0  b\n"
  "MouseMoveEvent 99 128 0 0 0 0  b\n"
  "MouseMoveEvent 95 123 0 0 0 0  b\n"
  "MouseMoveEvent 91 116 0 0 0 0  b\n"
  "MiddleButtonPressEvent 91 116 0 0 0 0  b\n"
  "MiddleButtonReleaseEvent 91 116 0 0 0 0  b\n"
  "MouseMoveEvent 95 116 0 0 0 0  b\n"
  "MouseMoveEvent 105 118 0 0 0 0  b\n"
  "MouseMoveEvent 115 121 0 0 0 0  b\n"
  "MouseMoveEvent 124 124 0 0 0 0  b\n"
  "MouseMoveEvent 136 127 0 0 0 0  b\n"
  "MouseMoveEvent 144 128 0 0 0 0  b\n"
  "MouseMoveEvent 150 130 0 0 0 0  b\n"
  "MouseMoveEvent 154 132 0 0 0 0  b\n"
  "MouseMoveEvent 157 133 0 0 0 0  b\n"
  "MouseMoveEvent 161 133 0 0 0 0  b\n"
  "MouseMoveEvent 164 134 0 0 0 0  b\n"
  "MouseMoveEvent 167 135 0 0 0 0  b\n"
  "MouseMoveEvent 169 136 0 0 0 0  b\n"
  "KeyPressEvent 169 136 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 169 136 8 0 0 0 Control_L\n"
  "MiddleButtonReleaseEvent 169 136 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 169 136 0 0 0 1 Control_L\n"
  "RightButtonPressEvent 169 136 0 0 0 0 Control_L\n"
  "MouseMoveEvent 167 142 0 0 0 0 Control_L\n"
  "MouseMoveEvent 164 146 0 0 0 0 Control_L\n"
  "MouseMoveEvent 162 149 0 0 0 0 Control_L\n"
  "MouseMoveEvent 159 152 0 0 0 0 Control_L\n"
  "MouseMoveEvent 155 155 0 0 0 0 Control_L\n"
  "MouseMoveEvent 152 157 0 0 0 0 Control_L\n"
  "MouseMoveEvent 148 159 0 0 0 0 Control_L\n"
  "MouseMoveEvent 143 163 0 0 0 0 Control_L\n"
  "MouseMoveEvent 137 165 0 0 0 0 Control_L\n"
  "MouseMoveEvent 133 166 0 0 0 0 Control_L\n"
  "MouseMoveEvent 132 164 0 0 0 0 Control_L\n"
  "RightButtonReleaseEvent 132 164 0 0 0 0 Control_L\n"
  "MouseMoveEvent 133 164 0 0 0 0 Control_L\n"
  "KeyPressEvent 133 164 -128 0 0 1 Control_L\n"
  "RightButtonPressEvent 133 164 8 0 0 0 Control_L\n"
  "RightButtonReleaseEvent 133 164 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 133 164 0 0 0 1 Control_L\n"
  "MouseMoveEvent 133 164 0 0 0 0 Control_L\n"
  "MouseMoveEvent 129 162 0 0 0 0 Control_L\n"
  "MouseMoveEvent 125 160 0 0 0 0 Control_L\n"
  "MouseMoveEvent 125 156 0 0 0 0 Control_L\n"
  "MouseMoveEvent 122 154 0 0 0 0 Control_L\n"
  "MouseMoveEvent 121 152 0 0 0 0 Control_L\n"
  "KeyPressEvent 121 152 0 -128 0 1 Shift_L\n"
  "RightButtonPressEvent 121 152 0 4 0 0 Shift_L\n"
  "RightButtonReleaseEvent 121 152 0 4 0 0 Shift_L\n"
  "KeyReleaseEvent 121 152 0 0 0 1 Shift_L\n"
  "MouseMoveEvent 108 137 0 0 0 0 Shift_L\n"
  "KeyPressEvent 108 137 0 -128 0 1 Shift_L\n"
  "RightButtonPressEvent 108 137 0 4 0 0 Shift_L\n"
  "RightButtonReleaseEvent 108 137 0 4 0 0 Shift_L\n"
  "KeyReleaseEvent 108 137 0 0 0 1 Shift_L\n"
  "RightButtonPressEvent 108 137 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 112 127 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 118 116 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 121 109 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 128 97 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 134 88 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 136 86 0 0 0 0 Shift_L\n"
  "RightButtonReleaseEvent 136 86 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 122 152 0 0 0 0 Shift_L\n"
  "RightButtonPressEvent 122 152 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 125 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 156 143 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 164 141 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 168 140 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 170 140 0 0 0 0 Shift_L\n"
  "RightButtonReleaseEvent 170 140 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 129 166 0 0 0 0 Shift_L\n"
  "RightButtonPressEvent 129 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 127 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 115 152 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 104 140 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 95 130 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 89 124 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 88 118 0 0 0 0 Shift_L\n"
  "RightButtonReleaseEvent 88 118 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 168 140 0 0 0 0 Shift_L\n"
  "RightButtonPressEvent 168 140 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 165 140 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 162 142 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 159 145 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 156 146 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 153 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 150 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 147 153 0 0 0 0 Shift_L\n"
  "RightButtonReleaseEvent 147 153 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 137 84 0 0 0 0 Shift_L\n"
  "RightButtonPressEvent 137 84 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 133 94 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 130 107 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 123 124 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 110 147 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 99 160 0 0 0 0 Shift_L\n"
  "RightButtonReleaseEvent 99 160 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 337 163 0 0 0 0 Shift_L\n"
  "RightButtonPressEvent 337 163 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 337 162 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 337 160 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 338 158 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 342 153 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 346 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 349 147 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 352 144 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 354 141 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 356 139 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 358 136 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 359 135 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 360 133 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 360 131 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 361 130 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 362 128 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 364 124 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 365 122 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 367 119 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 368 117 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 369 114 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 370 113 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 370 112 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 370 113 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 368 114 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 367 115 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 366 116 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 366 118 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 365 118 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 365 120 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 364 121 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 363 123 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 362 125 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 362 127 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 361 128 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 360 130 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 360 131 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 359 133 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 358 134 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 357 136 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 356 139 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 355 141 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 354 143 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 353 145 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 352 147 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 352 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 352 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 351 152 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 350 156 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 349 158 0 0 0 0 Shift_L\n"
  "RightButtonReleaseEvent 349 158 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 381 179 0 0 0 0 Shift_L\n"
  "LeftButtonPressEvent 381 179 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 382 179 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 379 179 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 376 177 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 371 174 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 364 167 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 353 156 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 348 146 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 345 139 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 342 129 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 340 121 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 337 111 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 336 101 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 336 98 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 335 95 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 335 93 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 333 91 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 331 87 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 329 85 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 329 84 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 328 84 0 0 0 0 Shift_L\n"
  "LeftButtonReleaseEvent 328 84 0 0 0 0 Shift_L\n"
  ;

// Callback for the tracer interaction
class vtkITWCallback : public vtkCommand
{
public:
  static vtkITWCallback *New()
  { return new vtkITWCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkImageTracerWidget *tracerWidget =
      reinterpret_cast<vtkImageTracerWidget*>(caller);
    if(!tracerWidget) { return; }

    int closed = tracerWidget->IsClosed();
    SplineWidget->SetClosed(closed);

    if (!closed)
      {
      Actor->GetMapper()->SetInputConnection(Extract->GetOutputPort());
      }

    int npts = tracerWidget->GetNumberOfHandles();
    if (npts < 2) { return; }

    tracerWidget->GetPath(PathPoly);
    vtkPoints* points = PathPoly->GetPoints();
    if (!points){ return; }

    SplineWidget->InitializeHandles(points);

    if (closed)
      {
      SplineWidget->GetPolyData(SplinePoly);
      Stencil->Update();
      Actor->GetMapper()->SetInputConnection(Stencil->GetOutputPort());
      }
  }

  vtkITWCallback():SplineWidget(0),Actor(0),Stencil(0),Extract(0),
                   PathPoly(0),SplinePoly(0){}

  vtkSplineWidget *SplineWidget;
  vtkImageActor   *Actor;
  vtkImageStencil *Stencil;
  vtkExtractVOI   *Extract;
  vtkPolyData     *PathPoly;
  vtkPolyData     *SplinePoly;
};

// Callback for the spline interaction.
// Note: this callback has to have a name different from that already
// used in another test: see TestSplineWidget.cxx!
class vtkSW2Callback : public vtkCommand
{
public:
  static vtkSW2Callback *New()
  { return new vtkSW2Callback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkSplineWidget *splineWidget =
      reinterpret_cast<vtkSplineWidget*>(caller);
    if(!splineWidget) { return; }

    int npts = splineWidget->GetNumberOfHandles();
    int closed = splineWidget->IsClosed();

    Points->Reset();
    for (int i = 0; i < npts; ++i)
      {
      Points->InsertNextPoint(splineWidget->GetHandlePosition(i));
      }

    if (closed)
      {
      if (TracerWidget->GetAutoClose())
        {
        Points->InsertNextPoint(splineWidget->GetHandlePosition(0));
        }
      splineWidget->GetPolyData(SplinePoly);
      Stencil->Update();
      Actor->GetMapper()->SetInputConnection(Stencil->GetOutputPort());
      }

    TracerWidget->InitializeHandles(Points);
  }

  vtkSW2Callback():Points(0),TracerWidget(0),Actor(0),Stencil(0),SplinePoly(0){}

  vtkPoints            *Points;
  vtkImageTracerWidget *TracerWidget;
  vtkImageActor        *Actor;
  vtkImageStencil      *Stencil;
  vtkPolyData          *SplinePoly;
};

int TestImageTracerWidget( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  // Increase polygon offsets to support some OpenGL drivers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(10,10);

// Start by loading some data.
//
  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->SetFilePrefix(fname);
  v16->ReleaseDataFlagOn();
  v16->SetDataMask(0x7fff);
  v16->Update();

  delete[] fname;

  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderer> ren2 =
    vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);

  vtkSmartPointer<vtkInteractorStyleImage> interactorStyle =
    vtkSmartPointer<vtkInteractorStyleImage>::New();

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetInteractorStyle(interactorStyle);
  iren->SetRenderWindow(renWin);

  double range[2];
  v16->GetOutput()->GetScalarRange(range);

  vtkSmartPointer<vtkImageShiftScale> shifter =
    vtkSmartPointer<vtkImageShiftScale>::New();
  shifter->SetShift(-1.0*range[0]);
  shifter->SetScale(255.0/(range[1]-range[0]));
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->SetInputConnection(v16->GetOutputPort());
  shifter->ReleaseDataFlagOff();
  shifter->Update();

// Display a y-z plane.
//
  vtkSmartPointer<vtkImageActor> imageActor1 =
    vtkSmartPointer<vtkImageActor>::New();
  imageActor1->GetMapper()->SetInputConnection(shifter->GetOutputPort());
  imageActor1->VisibilityOn();
  imageActor1->SetDisplayExtent(31, 31, 0, 63, 0, 92);
  imageActor1->InterpolateOff();

  vtkSmartPointer<vtkExtractVOI> extract =
    vtkSmartPointer<vtkExtractVOI>::New();
  extract->SetVOI(imageActor1->GetDisplayExtent());
  extract->SetSampleRate(1, 1, 1);
  extract->SetInputConnection(shifter->GetOutputPort());
  extract->ReleaseDataFlagOff();
  extract->Update();

  vtkSmartPointer<vtkImageActor> imageActor2 =
    vtkSmartPointer<vtkImageActor>::New();
  imageActor2->GetMapper()->SetInputConnection(extract->GetOutputPort());
  imageActor2->VisibilityOn();
  imageActor2->SetDisplayExtent(extract->GetVOI());
  imageActor2->InterpolateOff();

// Set up the image tracer widget
//
  vtkSmartPointer<vtkImageTracerWidget> imageTracerWidget =
    vtkSmartPointer<vtkImageTracerWidget>::New();
  imageTracerWidget->SetDefaultRenderer(ren1);
  imageTracerWidget->SetCaptureRadius(1.5);
  imageTracerWidget->GetGlyphSource()->SetColor(1, 0, 0);
  imageTracerWidget->GetGlyphSource()->SetScale(3.0);
  imageTracerWidget->GetGlyphSource()->SetRotationAngle(45.0);
  imageTracerWidget->GetGlyphSource()->Modified();
  imageTracerWidget->ProjectToPlaneOn();
  imageTracerWidget->SetProjectionNormalToXAxes();
  imageTracerWidget->SetProjectionPosition(imageActor1->GetBounds()[0]);
  imageTracerWidget->SetViewProp(imageActor1);
  imageTracerWidget->SetInputConnection(shifter->GetOutputPort());
  imageTracerWidget->SetInteractor(iren);
  imageTracerWidget->PlaceWidget();
  imageTracerWidget->SnapToImageOff();
  imageTracerWidget->AutoCloseOn();

// Set up a vtkSplineWidget in the second renderer and have
// its handles set by the tracer widget.
//
  vtkSmartPointer<vtkSplineWidget> splineWidget =
    vtkSmartPointer<vtkSplineWidget>::New();
  splineWidget->SetCurrentRenderer(ren2);
  splineWidget->SetDefaultRenderer(ren2);
  splineWidget->SetInputConnection(extract->GetOutputPort());
  splineWidget->SetInteractor(iren);
  splineWidget->PlaceWidget(imageActor2->GetBounds());
  splineWidget->ProjectToPlaneOn();
  splineWidget->SetProjectionNormalToXAxes();
  splineWidget->SetProjectionPosition(imageActor2->GetBounds()[0]);

  vtkSmartPointer<vtkPolyData> pathPoly =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPolyData> splinePoly =
    vtkSmartPointer<vtkPolyData>::New();

// Set up a pipleline to demonstrate extraction of a 2D
// region of interest.
//
  vtkSmartPointer<vtkLinearExtrusionFilter> extrude =
    vtkSmartPointer<vtkLinearExtrusionFilter>::New();
  extrude->SetInputData(splinePoly);
  extrude->SetScaleFactor(1);
  extrude->SetExtrusionTypeToNormalExtrusion();
  extrude->SetVector(1, 0, 0);

  vtkSmartPointer<vtkTransformPolyDataFilter> filter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  filter->SetInputConnection( extrude->GetOutputPort() );
  vtkSmartPointer<vtkTransform> transform =
    vtkSmartPointer<vtkTransform>::New();
  transform->Translate(-0.5, 0, 0);
  filter->SetTransform(transform);

  vtkSmartPointer<vtkPolyDataToImageStencil> dataToStencil =
    vtkSmartPointer<vtkPolyDataToImageStencil>::New();
  dataToStencil->SetInputConnection( filter->GetOutputPort() );

  dataToStencil->SetInformationInput( extract->GetOutput() );

  // Alternative to SetInformationInput:
  //dataToStencil->SetOutputSpacing( extract->GetOutput()->GetSpacing() );
  //dataToStencil->SetOutputOrigin( extract->GetOutput()->GetOrigin() );
  //dataToStencil->SetOutputWholeExtent( extract->GetOutput()->GetWholeExtent() );

  vtkSmartPointer<vtkImageStencil> stencil =
    vtkSmartPointer<vtkImageStencil>::New();
  stencil->SetInputConnection(extract->GetOutputPort());
  stencil->SetStencilConnection(dataToStencil->GetOutputPort());
  stencil->ReverseStencilOff();
  stencil->SetBackgroundValue(128);

// Set up callbacks for widget interactions.
//
  vtkSmartPointer<vtkITWCallback> itwCallback =
    vtkSmartPointer<vtkITWCallback>::New();
  itwCallback->SplineWidget = splineWidget;
  itwCallback->Actor = imageActor2;
  itwCallback->Stencil = stencil;
  itwCallback->Extract = extract;
  itwCallback->PathPoly = pathPoly;
  itwCallback->SplinePoly = splinePoly;

  imageTracerWidget->AddObserver(vtkCommand::EndInteractionEvent,itwCallback);

  vtkSmartPointer<vtkSW2Callback> swCallback =
    vtkSmartPointer<vtkSW2Callback>::New();
  swCallback->Points = points;
  swCallback->TracerWidget = imageTracerWidget;
  swCallback->Actor = imageActor2;
  swCallback->Stencil = stencil;
  swCallback->SplinePoly = splinePoly;

  splineWidget->AddObserver(vtkCommand::EndInteractionEvent,swCallback);

  ren1->SetBackground(0.4, 0.4, 0.5);
  ren1->SetViewport(0, 0, 0.5, 1);
  ren1->AddViewProp(imageActor1);
  ren2->SetBackground(0.5, 0.4, 0.4);
  ren2->SetViewport(0.5, 0, 1, 1);
  ren2->AddViewProp(imageActor2);

  ren1->ResetCamera();
  ren2->ResetCamera();
  renWin->SetSize(480, 240);

  imageTracerWidget->On();
  splineWidget->On();

  vtkCamera* cam = ren1->GetActiveCamera();
  cam->SetViewUp(0, 1, 0);
  cam->Azimuth(270);
  cam->Roll(270);
  cam->Dolly(1.7);
  ren1->ResetCameraClippingRange();

  cam = ren2->GetActiveCamera();
  cam->SetViewUp(0, 1, 0);
  cam->Azimuth(270);
  cam->Roll(270);
  cam->Dolly(1.7);
  ren2->ResetCameraClippingRange();

  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(ImageTracerWidgetEventLog);

  iren->Initialize();

  renWin->Render();

  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
