/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumeUpdate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test volume tests whether updating the volume MTime updates the ,
// geometry in the volume mapper.

#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOSPRayPass.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

static const char* TestGPURayCastVolumeUpdateLog = "# StreamVersion 1\n"
                                                   "EnterEvent 396 237 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 223 255 0 0 0 0 0\n"
                                                   "StartInteractionEvent 223 255 0 0 0 0 0\n"
                                                   "TimerEvent 229 240 0 0 0 0 0\n"
                                                   "RenderEvent 229 240 0 0 0 0 0\n"
                                                   "MouseMoveEvent 232 233 0 0 0 0 0\n"
                                                   "MouseMoveEvent 249 147 0 0 0 0 0\n"
                                                   "TimerEvent 249 144 0 0 0 0 0\n"
                                                   "RenderEvent 249 144 0 0 0 0 0\n"
                                                   "MouseMoveEvent 249 143 0 0 0 0 0\n"
                                                   "TimerEvent 249 142 0 0 0 0 0\n"
                                                   "RenderEvent 249 142 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 249 142 0 0 0 0 0\n"
                                                   "EndInteractionEvent 249 142 0 0 0 0 0\n"

                                                   "MouseMoveEvent 245 138 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 245 138 0 0 0 0 0\n"
                                                   "StartInteractionEvent 245 138 0 0 0 0 0\n"
                                                   "MouseMoveEvent 243 138 0 0 0 0 0\n"
                                                   "MouseMoveEvent 210 124 0 0 0 0 0\n"
                                                   "MouseMoveEvent 197 99 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 197 100 0 0 0 0 0\n"
                                                   "EndInteractionEvent 197 100 0 0 0 0 0\n"

                                                   "RenderEvent 197 100 0 0 0 0 0\n"
                                                   "MouseMoveEvent 197 96 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 198 88 0 0 0 0 0\n"
                                                   "StartInteractionEvent 198 88 0 0 0 0 0\n"
                                                   "MouseMoveEvent 199 87 0 0 0 0 0\n"
                                                   "TimerEvent 199 87 0 0 0 0 0\n"
                                                   "MouseMoveEvent 199 86 0 0 0 0 0\n"
                                                   "MouseMoveEvent 200 74 0 0 0 0 0\n"
                                                   "TimerEvent 200 67 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 200 67 0 0 0 0 0\n"
                                                   "EndInteractionEvent 200 67 0 0 0 0 0\n"

                                                   "MouseMoveEvent 201 66 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 222 131 0 0 0 0 0\n"
                                                   "StartInteractionEvent 222 131 0 0 0 0 0\n"
                                                   "TimerEvent 222 131 0 0 0 0 0\n"
                                                   "RenderEvent 222 131 0 0 0 0 0\n"
                                                   "MouseMoveEvent 224 132 0 0 0 0 0\n"
                                                   "TimerEvent 230 145 0 0 0 0 0\n"
                                                   "MouseMoveEvent 233 151 0 0 0 0 0\n"
                                                   "MouseMoveEvent 247 185 0 0 0 0 0\n"
                                                   "TimerEvent 216 247 0 0 0 0 0\n"
                                                   "MouseMoveEvent 212 251 0 0 0 0 0\n"
                                                   "TimerEvent 208 234 0 0 0 0 0\n"
                                                   "RenderEvent 208 234 0 0 0 0 0\n"
                                                   "MouseMoveEvent 221 196 0 0 0 0 0\n"
                                                   "TimerEvent 226 183 0 0 0 0 0\n"
                                                   "MouseMoveEvent 228 179 0 0 0 0 0\n"
                                                   "MouseMoveEvent 230 174 0 0 0 0 0\n"
                                                   "TimerEvent 226 152 0 0 0 0 0\n"
                                                   "RenderEvent 226 152 0 0 0 0 0\n"
                                                   "MouseMoveEvent 225 151 0 0 0 0 0\n"
                                                   "TimerEvent 225 151 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 225 151 0 0 0 0 0\n"
                                                   "EndInteractionEvent 225 151 0 0 0 0 0\n"

                                                   "MouseMoveEvent 214 151 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 214 151 0 0 0 0 0\n"
                                                   "StartInteractionEvent 214 151 0 0 0 0 0\n"
                                                   "TimerEvent 214 151 0 0 0 0 0\n"
                                                   "MouseMoveEvent 206 146 0 0 0 0 0\n"
                                                   "TimerEvent 185 135 0 0 0 0 0\n"
                                                   "RenderEvent 185 135 0 0 0 0 0\n"
                                                   "MouseMoveEvent 184 135 0 0 0 0 0\n"
                                                   "TimerEvent 180 136 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 180 136 0 0 0 0 0\n"
                                                   "EndInteractionEvent 180 136 0 0 0 0 0\n"

                                                   "MouseMoveEvent 159 184 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 159 184 0 0 0 0 0\n"
                                                   "StartInteractionEvent 159 184 0 0 0 0 0\n"
                                                   "MouseMoveEvent 158 185 0 0 0 0 0\n"
                                                   "TimerEvent 158 185 0 0 0 0 0\n"
                                                   "MouseMoveEvent 150 192 0 0 0 0 0\n"
                                                   "MouseMoveEvent 131 215 0 0 0 0 0\n"
                                                   "TimerEvent 130 220 0 0 0 0 0\n"
                                                   "MouseMoveEvent 130 221 0 0 0 0 0\n"
                                                   "MouseMoveEvent 132 226 0 0 0 0 0\n"
                                                   "TimerEvent 144 233 0 0 0 0 0\n"
                                                   "MouseMoveEvent 148 235 0 0 0 0 0\n"
                                                   "TimerEvent 176 253 0 0 0 0 0\n"
                                                   "MouseMoveEvent 176 257 0 0 0 0 0\n"
                                                   "MouseMoveEvent 174 300 0 0 0 0 0\n"
                                                   "TimerEvent 156 323 0 0 0 0 0\n"
                                                   "RenderEvent 156 323 0 0 0 0 0\n"
                                                   "MouseMoveEvent 150 323 0 0 0 0 0\n"
                                                   "MouseMoveEvent 148 323 0 0 0 0 0\n"
                                                   "TimerEvent 127 327 0 0 0 0 0\n"
                                                   "MouseMoveEvent 126 329 0 0 0 0 0\n"
                                                   "TimerEvent 126 330 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 126 330 0 0 0 0 0\n"
                                                   "EndInteractionEvent 126 330 0 0 0 0 0\n"

                                                   "MouseMoveEvent 173 242 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 173 242 0 0 0 0 0\n"
                                                   "StartInteractionEvent 173 242 0 0 0 0 0\n"
                                                   "TimerEvent 173 242 0 0 0 0 0\n"
                                                   "MouseMoveEvent 174 242 0 0 0 0 0\n"
                                                   "MouseMoveEvent 182 270 0 0 0 0 0\n"
                                                   "TimerEvent 182 270 0 0 0 0 0\n"
                                                   "MouseMoveEvent 182 274 0 0 0 0 0\n"
                                                   "TimerEvent 187 281 0 0 0 0 0\n"
                                                   "MouseMoveEvent 192 284 0 0 0 0 0\n"
                                                   "MouseMoveEvent 208 288 0 0 0 0 0\n"
                                                   "TimerEvent 226 288 0 0 0 0 0\n"
                                                   "MouseMoveEvent 228 288 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 228 288 0 0 0 0 0\n"
                                                   "EndInteractionEvent 228 288 0 0 0 0 0\n"

                                                   "RenderEvent 228 288 0 0 0 0 0\n"
                                                   "MouseMoveEvent 219 278 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 219 278 0 0 0 0 0\n"
                                                   "StartInteractionEvent 219 278 0 0 0 0 0\n"
                                                   "MouseMoveEvent 218 278 0 0 0 0 0\n"
                                                   "TimerEvent 218 278 0 0 0 0 0\n"
                                                   "MouseMoveEvent 213 278 0 0 0 0 0\n"
                                                   "MouseMoveEvent 159 294 0 0 0 0 0\n"
                                                   "TimerEvent 151 297 0 0 0 0 0\n"
                                                   "MouseMoveEvent 147 298 0 0 0 0 0\n"
                                                   "TimerEvent 142 298 0 0 0 0 0\n"
                                                   "RenderEvent 142 298 0 0 0 0 0\n"
                                                   "MouseMoveEvent 141 299 0 0 0 0 0\n"
                                                   "TimerEvent 141 299 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 141 299 0 0 0 0 0\n"
                                                   "EndInteractionEvent 141 299 0 0 0 0 0\n"

                                                   "MouseMoveEvent 191 245 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 191 245 0 0 0 0 0\n"
                                                   "StartInteractionEvent 191 245 0 0 0 0 0\n"
                                                   "TimerEvent 191 245 0 0 0 0 0\n"
                                                   "MouseMoveEvent 191 247 0 0 0 0 0\n"
                                                   "TimerEvent 197 252 0 0 0 0 0\n"
                                                   "RenderEvent 197 252 0 0 0 0 0\n"
                                                   "TimerEvent 197 252 0 0 0 0 0\n"
                                                   "MouseMoveEvent 201 252 0 0 0 0 0\n"
                                                   "MouseMoveEvent 216 251 0 0 0 0 0\n"
                                                   "TimerEvent 216 251 0 0 0 0 0\n"
                                                   "RenderEvent 216 251 0 0 0 0 0\n"
                                                   "MouseMoveEvent 218 250 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 218 250 0 0 0 0 0\n"
                                                   "EndInteractionEvent 218 250 0 0 0 0 0\n"

                                                   "MouseMoveEvent 218 220 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 218 220 0 0 0 0 0\n"
                                                   "StartInteractionEvent 218 220 0 0 0 0 0\n"
                                                   "MouseMoveEvent 218 219 0 0 0 0 0\n"
                                                   "TimerEvent 218 219 0 0 0 0 0\n"
                                                   "MouseMoveEvent 218 209 0 0 0 0 0\n"
                                                   "TimerEvent 218 202 0 0 0 0 0\n"
                                                   "MouseMoveEvent 219 199 0 0 0 0 0\n"
                                                   "RenderEvent 220 196 0 0 0 0 0\n"
                                                   "MouseMoveEvent 220 194 0 0 0 0 0\n"
                                                   "MouseMoveEvent 220 193 0 0 0 0 0\n"
                                                   "MouseMoveEvent 220 192 0 0 0 0 0\n"
                                                   "TimerEvent 221 190 0 0 0 0 0\n"
                                                   "RenderEvent 221 190 0 0 0 0 0\n"
                                                   "MouseMoveEvent 221 188 0 0 0 0 0\n"
                                                   "TimerEvent 221 187 0 0 0 0 0\n"
                                                   "RenderEvent 221 187 0 0 0 0 0\n"
                                                   "MouseMoveEvent 221 185 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 221 185 0 0 0 0 0\n"
                                                   "EndInteractionEvent 221 185 0 0 0 0 0\n"

                                                   "RenderEvent 221 185 0 0 0 0 0\n"
                                                   "MouseMoveEvent 305 399 0 0 0 0 0\n";

int TestGPURayCastVolumeUpdate(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      useOSP = false;
    }
  }

  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  volumeMapper->SetInputConnection(reader->GetOutputPort());

  // Add outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  renWin->Render(); // make sure we have an OpenGL context.

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->AddRenderer(ren);

  // Attach OSPRay render pass
  vtkNew<vtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    ren->SetPass(osprayPass);
  }

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);
  volumeProperty->SetColor(colorTransferFunction);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  /// Add sphere in the center of volume
  int dims[3];
  double spacing[3], center[3], origin[3];
  reader->Update();
  vtkSmartPointer<vtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  center[0] = origin[0] + spacing[0] * dims[0] / 2.0;
  center[1] = origin[1] + spacing[1] * dims[1] / 2.0;
  center[2] = origin[2] + spacing[2] * dims[2] / 2.0;

  ren->AddVolume(volume);
  ren->AddActor(outlineActor);
  ren->ResetCamera();

  int valid = volumeMapper->IsRenderSupported(renWin, volumeProperty);

  int retVal;
  if (valid)
  {
    renWin->Render();

    vtkNew<vtkRTAnalyticSource> wavelet;
    wavelet->SetWholeExtent(-127, 128, -127, 128, -127, 128);
    wavelet->SetCenter(center);
    outlineFilter->SetInputConnection(wavelet->GetOutputPort());
    volumeMapper->SetInputConnection(wavelet->GetOutputPort());
    outlineFilter->UpdateWholeExtent();
    ren->ResetCamera();

    iren->Initialize();
    retVal = !(vtkTesting::InteractorEventLoop(argc, argv, iren, TestGPURayCastVolumeUpdateLog));
  }
  else
  {
    retVal = vtkTesting::PASSED;
    cout << "Required extensions not supported" << endl;
  }

  return !retVal;
}
