/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests a  vtkProp3DFollower with a volume
//
#include "vtkProp3DFollower.h"
#include "vtkPropPicker.h"
#include "vtkCellPicker.h"
#include "vtkStructuredPointsReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolumeTextureMapper2D.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// -----------------------------------------------------------------------
// This does the actual work: updates the vtkPline implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkPickFollowerCallback : public vtkCommand
{
public:
  static vtkPickFollowerCallback *New()
    { return new vtkPickFollowerCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkCellPicker *picker = reinterpret_cast<vtkCellPicker*>(caller);
      if ( picker->GetViewProp() != NULL )
        {
        cout << "Picked\n";
        }
    }

  vtkPickFollowerCallback() {}
};

// -----------------------------------------------------------------------
char PickFollowerLog[] =
  "# StreamVersion 1\n"
  "RenderEvent 0 0 0 0 0 0 0\n"
  "EnterEvent 123 298 0 0 0 0 0\n"
  "MouseMoveEvent 123 298 0 0 0 0 0\n"
  "MouseMoveEvent 123 293 0 0 0 0 0\n"
  "MouseMoveEvent 123 288 0 0 0 0 0\n"
  "MouseMoveEvent 123 281 0 0 0 0 0\n"
  "MouseMoveEvent 123 275 0 0 0 0 0\n"
  "MouseMoveEvent 123 268 0 0 0 0 0\n"
  "MouseMoveEvent 123 260 0 0 0 0 0\n"
  "MouseMoveEvent 123 256 0 0 0 0 0\n"
  "MouseMoveEvent 124 245 0 0 0 0 0\n"
  "MouseMoveEvent 124 237 0 0 0 0 0\n"
  "MouseMoveEvent 124 230 0 0 0 0 0\n"
  "MouseMoveEvent 124 226 0 0 0 0 0\n"
  "MouseMoveEvent 124 220 0 0 0 0 0\n"
  "MouseMoveEvent 125 216 0 0 0 0 0\n"
  "MouseMoveEvent 125 210 0 0 0 0 0\n"
  "MouseMoveEvent 125 207 0 0 0 0 0\n"
  "MouseMoveEvent 125 203 0 0 0 0 0\n"
  "MouseMoveEvent 126 200 0 0 0 0 0\n"
  "MouseMoveEvent 126 197 0 0 0 0 0\n"
  "MouseMoveEvent 126 194 0 0 0 0 0\n"
  "MouseMoveEvent 126 193 0 0 0 0 0\n"
  "MouseMoveEvent 126 192 0 0 0 0 0\n"
  "MouseMoveEvent 126 191 0 0 0 0 0\n"
  "KeyPressEvent 126 191 0 0 116 1 t\n"
  "CharEvent 126 191 0 0 116 1 t\n"
  "KeyReleaseEvent 126 191 0 0 116 1 t\n"
  "MouseMoveEvent 126 190 0 0 0 0 t\n"
  "MouseMoveEvent 126 189 0 0 0 0 t\n"
  "MouseMoveEvent 127 187 0 0 0 0 t\n"
  "MouseMoveEvent 127 185 0 0 0 0 t\n"
  "MouseMoveEvent 127 184 0 0 0 0 t\n"
  "MouseMoveEvent 128 183 0 0 0 0 t\n"
  "MouseMoveEvent 129 183 0 0 0 0 t\n"
  "MouseMoveEvent 130 182 0 0 0 0 t\n"
  "MouseMoveEvent 130 183 0 0 0 0 t\n"
  "MouseMoveEvent 130 184 0 0 0 0 t\n"
  "MiddleButtonPressEvent 130 184 0 0 0 0 t\n"
  "StartInteractionEvent 130 184 0 0 0 0 t\n"
  "MouseWheelForwardEvent 130 184 0 0 0 0 t\n"
  "RenderEvent 130 184 0 0 0 0 t\n"
  "MouseWheelForwardEvent 130 184 0 0 0 0 t\n"
  "RenderEvent 130 184 0 0 0 0 t\n"
  "MiddleButtonReleaseEvent 130 184 0 0 0 0 t\n"
  "EndInteractionEvent 130 184 0 0 0 0 t\n"
  "RenderEvent 130 184 0 0 0 0 t\n"
  "MouseWheelForwardEvent 130 184 0 0 0 0 t\n"
  "StartInteractionEvent 130 184 0 0 0 0 t\n"
  "RenderEvent 130 184 0 0 0 0 t\n"
  "EndInteractionEvent 130 184 0 0 0 0 t\n"
  "RenderEvent 130 184 0 0 0 0 t\n"
  "MouseMoveEvent 130 186 0 0 0 0 t\n"
  "MouseWheelBackwardEvent 130 186 0 0 0 0 t\n"
  "StartInteractionEvent 130 186 0 0 0 0 t\n"
  "RenderEvent 130 186 0 0 0 0 t\n"
  "EndInteractionEvent 130 186 0 0 0 0 t\n"
  "RenderEvent 130 186 0 0 0 0 t\n"
  "MouseWheelBackwardEvent 130 186 0 0 0 0 t\n"
  "StartInteractionEvent 130 186 0 0 0 0 t\n"
  "RenderEvent 130 186 0 0 0 0 t\n"
  "EndInteractionEvent 130 186 0 0 0 0 t\n"
  "RenderEvent 130 186 0 0 0 0 t\n"
  "MouseMoveEvent 131 186 0 0 0 0 t\n"
  "MouseWheelBackwardEvent 131 186 0 0 0 0 t\n"
  "StartInteractionEvent 131 186 0 0 0 0 t\n"
  "RenderEvent 131 186 0 0 0 0 t\n"
  "EndInteractionEvent 131 186 0 0 0 0 t\n"
  "RenderEvent 131 186 0 0 0 0 t\n"
  "MouseMoveEvent 132 186 0 0 0 0 t\n"
  "MouseMoveEvent 133 186 0 0 0 0 t\n"
  "MouseMoveEvent 132 186 0 0 0 0 t\n"
  "KeyPressEvent 132 186 0 -128 0 1 Shift_L\n"
  "LeftButtonPressEvent 132 186 0 4 0 0 Shift_L\n"
  "StartInteractionEvent 132 186 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 131 186 0 4 0 0 Shift_L\n"
  "RenderEvent 131 186 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 124 188 0 4 0 0 Shift_L\n"
  "RenderEvent 124 188 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 113 188 0 4 0 0 Shift_L\n"
  "RenderEvent 113 188 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 112 188 0 4 0 0 Shift_L\n"
  "RenderEvent 112 188 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 109 188 0 4 0 0 Shift_L\n"
  "RenderEvent 109 188 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 99 189 0 4 0 0 Shift_L\n"
  "RenderEvent 99 189 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 95 189 0 4 0 0 Shift_L\n"
  "RenderEvent 95 189 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 91 190 0 4 0 0 Shift_L\n"
  "RenderEvent 91 190 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 82 191 0 4 0 0 Shift_L\n"
  "RenderEvent 82 191 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 77 191 0 4 0 0 Shift_L\n"
  "RenderEvent 77 191 0 4 0 0 Shift_L\n"
  "KeyPressEvent 77 191 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 70 191 0 4 0 0 Shift_L\n"
  "RenderEvent 70 191 0 4 0 0 Shift_L\n"
  "KeyPressEvent 70 191 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 67 191 0 4 0 0 Shift_L\n"
  "RenderEvent 67 191 0 4 0 0 Shift_L\n"
  "KeyPressEvent 67 191 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 64 192 0 4 0 0 Shift_L\n"
  "RenderEvent 64 192 0 4 0 0 Shift_L\n"
  "KeyPressEvent 64 192 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 54 192 0 4 0 0 Shift_L\n"
  "RenderEvent 54 192 0 4 0 0 Shift_L\n"
  "KeyPressEvent 54 192 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 47 192 0 4 0 0 Shift_L\n"
  "RenderEvent 47 192 0 4 0 0 Shift_L\n"
  "KeyPressEvent 47 192 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 45 192 0 4 0 0 Shift_L\n"
  "RenderEvent 45 192 0 4 0 0 Shift_L\n"
  "KeyPressEvent 45 192 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 43 191 0 4 0 0 Shift_L\n"
  "RenderEvent 43 191 0 4 0 0 Shift_L\n"
  "KeyPressEvent 43 191 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 39 190 0 4 0 0 Shift_L\n"
  "RenderEvent 39 190 0 4 0 0 Shift_L\n"
  "KeyPressEvent 39 190 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 35 189 0 4 0 0 Shift_L\n"
  "RenderEvent 35 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 35 189 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 32 189 0 4 0 0 Shift_L\n"
  "RenderEvent 32 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 32 189 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 31 189 0 4 0 0 Shift_L\n"
  "RenderEvent 31 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 31 189 0 -128 0 2 Shift_L\n"
  "MouseMoveEvent 30 189 0 4 0 0 Shift_L\n"
  "RenderEvent 30 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 30 189 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 30 189 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 32 188 0 4 0 0 Shift_L\n"
  "RenderEvent 32 188 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 36 188 0 4 0 0 Shift_L\n"
  "RenderEvent 36 188 0 4 0 0 Shift_L\n"
  "KeyPressEvent 36 188 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 47 189 0 4 0 0 Shift_L\n"
  "RenderEvent 47 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 47 189 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 63 190 0 4 0 0 Shift_L\n"
  "RenderEvent 63 190 0 4 0 0 Shift_L\n"
  "KeyPressEvent 63 190 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 71 190 0 4 0 0 Shift_L\n"
  "RenderEvent 71 190 0 4 0 0 Shift_L\n"
  "KeyPressEvent 71 190 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 81 190 0 4 0 0 Shift_L\n"
  "RenderEvent 81 190 0 4 0 0 Shift_L\n"
  "KeyPressEvent 81 190 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 96 190 0 4 0 0 Shift_L\n"
  "RenderEvent 96 190 0 4 0 0 Shift_L\n"
  "KeyPressEvent 96 190 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 100 189 0 4 0 0 Shift_L\n"
  "RenderEvent 100 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 100 189 0 -128 0 2 Shift_L\n"
  "MouseMoveEvent 101 189 0 4 0 0 Shift_L\n"
  "RenderEvent 101 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 101 189 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 101 189 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 101 189 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 101 189 0 -128 0 1 Shift_L\n"
  "LeftButtonReleaseEvent 101 189 0 4 0 0 Shift_L\n"
  "EndInteractionEvent 101 189 0 4 0 0 Shift_L\n"
  "RenderEvent 101 189 0 4 0 0 Shift_L\n"
  "KeyPressEvent 101 189 0 -128 0 2 Shift_L\n"
  "KeyReleaseEvent 101 189 0 0 0 1 Shift_L\n"
  "MouseMoveEvent 101 189 0 0 0 0 Shift_L\n"
  "LeftButtonPressEvent 101 189 0 0 0 0 Shift_L\n"
  "StartInteractionEvent 101 189 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 101 188 0 0 0 0 Shift_L\n"
  "RenderEvent 101 188 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 87 186 0 0 0 0 Shift_L\n"
  "RenderEvent 87 186 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 82 185 0 0 0 0 Shift_L\n"
  "RenderEvent 82 185 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 75 185 0 0 0 0 Shift_L\n"
  "RenderEvent 75 185 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 72 185 0 0 0 0 Shift_L\n"
  "RenderEvent 72 185 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 71 185 0 0 0 0 Shift_L\n"
  "RenderEvent 71 185 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 69 185 0 0 0 0 Shift_L\n"
  "RenderEvent 69 185 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 67 184 0 0 0 0 Shift_L\n"
  "RenderEvent 67 184 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 59 183 0 0 0 0 Shift_L\n"
  "RenderEvent 59 183 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 55 182 0 0 0 0 Shift_L\n"
  "RenderEvent 55 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 54 182 0 0 0 0 Shift_L\n"
  "RenderEvent 54 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 53 182 0 0 0 0 Shift_L\n"
  "RenderEvent 53 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 53 182 0 0 0 0 Shift_L\n"
  "RenderEvent 53 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 54 182 0 0 0 0 Shift_L\n"
  "RenderEvent 54 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 63 182 0 0 0 0 Shift_L\n"
  "RenderEvent 63 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 69 183 0 0 0 0 Shift_L\n"
  "RenderEvent 69 183 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 76 183 0 0 0 0 Shift_L\n"
  "RenderEvent 76 183 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 86 182 0 0 0 0 Shift_L\n"
  "RenderEvent 86 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 90 182 0 0 0 0 Shift_L\n"
  "RenderEvent 90 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 96 183 0 0 0 0 Shift_L\n"
  "RenderEvent 96 183 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 99 183 0 0 0 0 Shift_L\n"
  "RenderEvent 99 183 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 100 184 0 0 0 0 Shift_L\n"
  "RenderEvent 100 184 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 104 186 0 0 0 0 Shift_L\n"
  "RenderEvent 104 186 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 106 188 0 0 0 0 Shift_L\n"
  "RenderEvent 106 188 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 108 189 0 0 0 0 Shift_L\n"
  "RenderEvent 108 189 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 109 189 0 0 0 0 Shift_L\n"
  "RenderEvent 109 189 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 109 190 0 0 0 0 Shift_L\n"
  "RenderEvent 109 190 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 110 191 0 0 0 0 Shift_L\n"
  "RenderEvent 110 191 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 114 194 0 0 0 0 Shift_L\n"
  "RenderEvent 114 194 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 116 197 0 0 0 0 Shift_L\n"
  "RenderEvent 116 197 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 116 201 0 0 0 0 Shift_L\n"
  "RenderEvent 116 201 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 116 202 0 0 0 0 Shift_L\n"
  "RenderEvent 116 202 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 116 203 0 0 0 0 Shift_L\n"
  "RenderEvent 116 203 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 116 205 0 0 0 0 Shift_L\n"
  "RenderEvent 116 205 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 117 209 0 0 0 0 Shift_L\n"
  "RenderEvent 117 209 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 117 211 0 0 0 0 Shift_L\n"
  "RenderEvent 117 211 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 117 212 0 0 0 0 Shift_L\n"
  "RenderEvent 117 212 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 117 214 0 0 0 0 Shift_L\n"
  "RenderEvent 117 214 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 118 215 0 0 0 0 Shift_L\n"
  "RenderEvent 118 215 0 0 0 0 Shift_L\n"
  "LeftButtonReleaseEvent 118 215 0 0 0 0 Shift_L\n"
  "EndInteractionEvent 118 215 0 0 0 0 Shift_L\n"
  "RenderEvent 118 215 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 118 215 0 0 0 0 Shift_L\n"
;

// -----------------------------------------------------------------------
int TestProp3DFollower( int argc, char* argv[] )
{
  // A volume rendered button!
  // Create the reader for the data
  char* fname3 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
  VTK_CREATE(vtkStructuredPointsReader, reader);
  reader->SetFileName(fname3);
  delete [] fname3;

  // Create transfer mapping scalar value to opacity
  VTK_CREATE(vtkPiecewiseFunction, opacityTransferFunction);
  opacityTransferFunction->AddPoint(20,0);
  opacityTransferFunction->AddPoint(255,1);

  // Create transfer mapping scalar value to color
  VTK_CREATE(vtkColorTransferFunction, colorTransferFunction);
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  // The property describes how the data will look
  VTK_CREATE(vtkVolumeProperty, volumeProperty);
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper / ray cast function know how to render the data
  VTK_CREATE(vtkVolumeTextureMapper2D, volumeMapper);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  // The volume holds the mapper and the property and
  // can be used to position/orient the volume
  VTK_CREATE(vtkVolume, volume);
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  volume->SetOrigin(-32,-32,-32);

  VTK_CREATE(vtkProp3DFollower,vFollower);
  vFollower->SetProp3D(volume);

  // Picking callback
  VTK_CREATE(vtkPickFollowerCallback, myCallback);

  VTK_CREATE(vtkCellPicker,picker);
  picker->AddObserver(vtkCommand::EndPickEvent,myCallback);

  // Create the rendering machinary
  //
  VTK_CREATE(vtkRenderer, ren1);
  vFollower->SetCamera(ren1->GetActiveCamera());

  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);
  iren->SetPicker(picker);

  ren1->AddActor(vFollower);

  // record events
  VTK_CREATE(vtkInteractorEventRecorder, recorder);
  recorder->SetInteractor(iren);
//  recorder->SetFileName("record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(PickFollowerLog);
  recorder->EnabledOn();

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);
  ren1->ResetCamera();
  iren->Initialize();
  renWin->Render();
  recorder->Play();
//  recorder->Off();

  int retVal = vtkRegressionTestImageThreshold( renWin, 10 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
