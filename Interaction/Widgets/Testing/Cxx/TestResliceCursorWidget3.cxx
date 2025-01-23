// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkBiDimensionalWidget.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkDICOMImageReader.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkImagePlaneWidget.h"
#include "vtkImageReader.h"
#include "vtkImageReslice.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleImage.h"
#include "vtkLookupTable.h"
#include "vtkMathUtilities.h"
#include "vtkOutlineFilter.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkResliceCursorThickLineRepresentation.h"
#include "vtkResliceCursorWidget.h"
#include "vtkVolume16Reader.h"

#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
class vtkResliceCursorCallback3 : public vtkCommand
{
public:
  static vtkResliceCursorCallback3* New() { return new vtkResliceCursorCallback3; }

  void Execute(vtkObject* caller, unsigned long /*ev*/, void* callData) override
  {
    vtkImagePlaneWidget* ipw = dynamic_cast<vtkImagePlaneWidget*>(caller);
    if (ipw)
    {
      double* wl = static_cast<double*>(callData);

      if (ipw == this->IPW[0])
      {
        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
      }
      else if (ipw == this->IPW[1])
      {
        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
      }
      else if (ipw == this->IPW[2])
      {
        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
      }
    }

    vtkResliceCursorWidget* rcw = dynamic_cast<vtkResliceCursorWidget*>(caller);
    if (rcw)
    {
      vtkResliceCursorLineRepresentation* rep =
        dynamic_cast<vtkResliceCursorLineRepresentation*>(rcw->GetRepresentation());
      vtkResliceCursor* rc = rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();
      for (int i = 0; i < 3; i++)
      {
        vtkPlaneSource* ps = static_cast<vtkPlaneSource*>(this->IPW[i]->GetPolyDataAlgorithm());
        ps->SetNormal(rc->GetPlane(i)->GetNormal());
        ps->SetCenter(rc->GetPlane(i)->GetOrigin());

        // If the reslice plane has modified, update it on the 3D widget
        this->IPW[i]->UpdatePlacement();

        // std::cout << "Updating placement of plane: " << i << " " <<
        //  rc->GetPlane(i)->GetNormal()[0] << " " <<
        //  rc->GetPlane(i)->GetNormal()[1] << " " <<
        //  rc->GetPlane(i)->GetNormal()[2] << std::endl;
        // this->IPW[i]->GetReslice()->Print(cout);
        // rep->GetReslice()->Print(cout);
        // std::cout << "---------------------" << std::endl;
      }
    }

    // Render everything
    this->RCW[0]->Render();
  }

  vtkResliceCursorCallback3() = default;
  vtkImagePlaneWidget* IPW[3];
  vtkResliceCursorWidget* RCW[3];
};

static const char* TestIndependentThicknessEvents =
  "# StreamVersion 1.1\n"
  // Reslice cursor thickness (Right click on axis)
  "RightButtonPressEvent 201 152 0 0 0 0\n"
  "MouseMoveEvent 201 152 0 0 0 0\n"
  "MouseMoveEvent 201 168 0 0 0 0\n"
  "MouseMoveEvent 205 187 0 0 0 0\n"
  "MouseMoveEvent 219 210 0 0 0 0\n"
  "MouseMoveEvent 232 233 0 0 0 0\n"
  "RightButtonReleaseEvent 232 233 0 0 0 0\n";

static const char* TestResliceCursorWidget3Events =
  "# StreamVersion 1.1\n"
  // Reslice cursor thickness (Right click on axis)
  "RightButtonPressEvent 201 152 0 0 0 0\n"
  "MouseMoveEvent 201 152 0 0 0 0\n"
  "MouseMoveEvent 201 168 0 0 0 0\n"
  "MouseMoveEvent 205 187 0 0 0 0\n"
  "MouseMoveEvent 219 210 0 0 0 0\n"
  "MouseMoveEvent 232 233 0 0 0 0\n"
  "RightButtonReleaseEvent 232 233 0 0 0 0\n"
  // Camera spin (Ctrl + Left button click outside image)
  "LeftButtonPressEvent 273 86 2 0 0 Control_L\n"
  "MouseMoveEvent 273 86 2 0 0 Control_L\n"
  "MouseMoveEvent 271 81 2 0 0 Control_L\n"
  "MouseMoveEvent 268 68 2 0 0 Control_L\n"
  "MouseMoveEvent 264 55 2 0 0 Control_L\n"
  "MouseMoveEvent 260 48 2 0 0 Control_L\n"
  "MouseMoveEvent 254 39 2 0 0 Control_L\n"
  "MouseMoveEvent 248 33 2 0 0 Control_L\n"
  "LeftButtonReleaseEvent 248 33 2 0 0 Control_L\n"
  // Widget translation (Left button click on center)
  "LeftButtonPressEvent 454 148 0 0 0 0\n"
  "MouseMoveEvent 454 148 0 0 0 Control_L\n"
  "MouseMoveEvent 445 148 0 0 0 Control_L\n"
  "MouseMoveEvent 424 146 0 0 0 Control_L\n"
  "MouseMoveEvent 416 146 0 0 0 Control_L\n"
  "LeftButtonReleaseEvent 416 146 0 0 0 0\n"
  // Widget rotation (Ctrl + Left button click on axis)
  "LeftButtonPressEvent 368 147 2 0 0 Control_L\n"
  "MouseMoveEvent 367 147 2 0 0 Control_L\n"
  "MouseMoveEvent 367 137 2 0 0 Control_L\n"
  "MouseMoveEvent 395 89 2 0 0 Control_L\n"
  "MouseMoveEvent 492 100 2 0 0 Control_L\n"
  "MouseMoveEvent 511 175 2 0 0 Control_L\n"
  "MouseMoveEvent 492 206 2 0 0 Control_L\n"
  "MouseMoveEvent 491 219 2 0 0 Control_L\n"
  "LeftButtonReleaseEvent 491 219 2 0 0 Control_L\n";

//------------------------------------------------------------------------------
int TestResliceCursorWidget3(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkVolume16Reader> reader = vtkSmartPointer<vtkVolume16Reader>::New();
  reader->SetDataDimensions(64, 64);
  reader->SetDataByteOrderToLittleEndian();
  reader->SetImageRange(1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);
  reader->SetDataMask(0x7fff);
  reader->Update();
  delete[] fname;

  vtkSmartPointer<vtkOutlineFilter> outline = vtkSmartPointer<vtkOutlineFilter>::New();
  outline->SetInputConnection(reader->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor = vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

  vtkSmartPointer<vtkRenderer> ren[4];

  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);

  for (int i = 0; i < 4; i++)
  {
    ren[i] = vtkSmartPointer<vtkRenderer>::New();
    renWin->AddRenderer(ren[i]);
  }

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
  picker->SetTolerance(0.005);

  vtkSmartPointer<vtkProperty> ipwProp = vtkSmartPointer<vtkProperty>::New();

  // assign default props to the ipw's texture plane actor
  vtkSmartPointer<vtkImagePlaneWidget> planeWidget[3];
  int imageDims[3];
  reader->GetOutput()->GetDimensions(imageDims);

  for (int i = 0; i < 3; i++)
  {
    planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
    planeWidget[i]->SetInteractor(iren);
    planeWidget[i]->SetPicker(picker);
    planeWidget[i]->RestrictPlaneToVolumeOn();
    double color[3] = { 0, 0, 0 };
    color[i] = 1;
    planeWidget[i]->GetPlaneProperty()->SetColor(color);
    planeWidget[i]->SetTexturePlaneProperty(ipwProp);
    planeWidget[i]->TextureInterpolateOff();
    planeWidget[i]->SetResliceInterpolateToLinear();
    planeWidget[i]->SetInputConnection(reader->GetOutputPort());
    planeWidget[i]->SetPlaneOrientation(i);
    planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
    planeWidget[i]->DisplayTextOn();
    planeWidget[i]->SetDefaultRenderer(ren[3]);
    planeWidget[i]->SetWindowLevel(1358, -27);
    planeWidget[i]->On();
    planeWidget[i]->InteractionOn();
  }

  planeWidget[1]->SetLookupTable(planeWidget[0]->GetLookupTable());
  planeWidget[2]->SetLookupTable(planeWidget[0]->GetLookupTable());

  vtkSmartPointer<vtkResliceCursorCallback3> cbk =
    vtkSmartPointer<vtkResliceCursorCallback3>::New();

  // Create the reslice cursor, widget and rep

  vtkSmartPointer<vtkResliceCursor> resliceCursor = vtkSmartPointer<vtkResliceCursor>::New();
  resliceCursor->SetCenter(reader->GetOutput()->GetCenter());
  resliceCursor->SetThickMode(1);
  resliceCursor->SetThickness(10, 10, 10);
  resliceCursor->SetImage(reader->GetOutput());

  vtkSmartPointer<vtkResliceCursorWidget> resliceCursorWidget[3];
  vtkSmartPointer<vtkResliceCursorThickLineRepresentation> resliceCursorRep[3];

  double viewUp[3][3] = { { 0, 0, -1 }, { 0, 0, 1 }, { 0, 1, 0 } };
  for (int i = 0; i < 3; i++)
  {
    resliceCursorWidget[i] = vtkSmartPointer<vtkResliceCursorWidget>::New();
    resliceCursorWidget[i]->SetInteractor(iren);

    resliceCursorRep[i] = vtkSmartPointer<vtkResliceCursorThickLineRepresentation>::New();
    resliceCursorWidget[i]->SetRepresentation(resliceCursorRep[i]);
    resliceCursorRep[i]->GetResliceCursorActor()->GetCursorAlgorithm()->SetResliceCursor(
      resliceCursor);
    resliceCursorRep[i]->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(i);

    const double minVal = reader->GetOutput()->GetScalarRange()[0];
    if (vtkImageReslice* reslice = vtkImageReslice::SafeDownCast(resliceCursorRep[i]->GetReslice()))
    {
      reslice->SetBackgroundColor(minVal, minVal, minVal, minVal);
    }

    resliceCursorWidget[i]->SetDefaultRenderer(ren[i]);
    resliceCursorWidget[i]->SetEnabled(1);

    ren[i]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    double camPos[3] = { 0, 0, 0 };
    camPos[i] = 1;
    ren[i]->GetActiveCamera()->SetPosition(camPos);

    ren[i]->GetActiveCamera()->ParallelProjectionOn();
    ren[i]->GetActiveCamera()->SetViewUp(viewUp[i]);
    ren[i]->ResetCamera();
    // ren[i]->ResetCameraClippingRange();

    // Tie the Image plane widget and the reslice cursor widget together
    cbk->IPW[i] = planeWidget[i];
    cbk->RCW[i] = resliceCursorWidget[i];
    resliceCursorWidget[i]->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);

    // Initialize the window level to a sensible value
    double range[2];
    reader->GetOutput()->GetScalarRange(range);
    resliceCursorRep[i]->SetWindowLevel(range[1] - range[0], (range[0] + range[1]) / 2.0);
    planeWidget[i]->SetWindowLevel(range[1] - range[0], (range[0] + range[1]) / 2.0);

    // Make them all share the same color map.
    resliceCursorRep[i]->SetLookupTable(resliceCursorRep[0]->GetLookupTable());
    planeWidget[i]->GetColorMap()->SetLookupTable(resliceCursorRep[0]->GetLookupTable());

    // clang-format off
    // Workaround VTK issue #18441
    // Make sure vtkResliceCursorActor is visible by forcing its representation to wireframe.
    // vtkResliceCursorActor is a quad with a normal parallel to the camera view up vector.
    // When represented as a surface, it has a thickness of 0 pixels. The class internally turns
    // edge visibility on to workaround the problem, which does not seem to be enough.
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(0)->SetRepresentationToWireframe();
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(1)->SetRepresentationToWireframe();
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(2)->SetRepresentationToWireframe();
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(0)->SetRepresentationToWireframe();
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(1)->SetRepresentationToWireframe();
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(2)->SetRepresentationToWireframe();
    // Workaround rendering artefacts with Intel chipsets and osmesa, where lines are rendered
    // black if perfectly aligned with the camera viewup (see #18453)
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(0)->RenderLinesAsTubesOn();
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(1)->RenderLinesAsTubesOn();
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(2)->RenderLinesAsTubesOn();
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(0)->SetLineWidth(2);
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(1)->SetLineWidth(2);
    resliceCursorRep[i]->GetResliceCursorActor()->GetCenterlineProperty(2)->SetLineWidth(2);
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(0)->RenderLinesAsTubesOn();
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(1)->RenderLinesAsTubesOn();
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(2)->RenderLinesAsTubesOn();
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(0)->SetLineWidth(2);
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(1)->SetLineWidth(2);
    resliceCursorRep[i]->GetResliceCursorActor()->GetThickSlabProperty(2)->SetLineWidth(2);
    // clang-format on
  }

  // Add the actors
  //
  ren[0]->SetBackground(0.3, 0.1, 0.1);
  ren[1]->SetBackground(0.1, 0.3, 0.1);
  ren[2]->SetBackground(0.1, 0.1, 0.3);
  ren[3]->AddActor(outlineActor);
  ren[3]->SetBackground(0.1, 0.1, 0.1);
  renWin->SetSize(600, 600);
  // renWin->SetFullScreen(1);

  ren[0]->SetViewport(0, 0, 0.5, 0.5);
  ren[1]->SetViewport(0.5, 0, 1, 0.5);
  ren[2]->SetViewport(0, 0.5, 0.5, 1);
  ren[3]->SetViewport(0.5, 0.5, 1, 1);

  // Set the actors' positions
  //
  renWin->Render();

  ren[3]->GetActiveCamera()->Elevation(110);
  ren[3]->GetActiveCamera()->SetViewUp(0, 0, -1);
  ren[3]->GetActiveCamera()->Azimuth(45);
  ren[3]->GetActiveCamera()->Dolly(1.15);
  ren[3]->ResetCameraClippingRange();

  vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
  iren->SetInteractorStyle(style);
  iren->Initialize();

  // Test independent thickness
  for (int i = 0; i < 3; i++)
  {
    resliceCursorRep[i]->IndependentThicknessOn();
  }

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestIndependentThicknessEvents);
  recorder->SetInteractor(iren);
  recorder->Play();
  recorder->Off();

  double expected_thickness[3] = { 10.0, 10.0, 16.585247 };
  double thickness[3] = { 0.0, 0.0, 0.0 };
  resliceCursorRep[0]->GetResliceCursor()->GetThickness(thickness);

  for (int i = 0; i < 3; i++)
  {
    if (!vtkMathUtilities::NearlyEqual(thickness[i], expected_thickness[i], 1e-6))
    {
      std::cerr << "Error: Independent thickness is invalid " << thickness[i]
                << " != " << expected_thickness[i] << std::endl;
      return EXIT_FAILURE;
    }

    // Disable independent thickness
    resliceCursorRep[i]->IndependentThicknessOff();
  }
  // Restore thickness
  resliceCursor->SetThickness(10, 10, 10);

  // Test interactions
  recorder->SetInputString(TestResliceCursorWidget3Events);
  recorder->Play();
  recorder->Off();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
