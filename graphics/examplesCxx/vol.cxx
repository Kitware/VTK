#include "vtk.h"

main ()
{
  // Create the renderer, render window, and interactor
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // Read the data from a vtk file
  vtkStructuredPointsReader *reader = vtkStructuredPointsReader::New();
    reader->SetFileName("../../../vtkdata/ironProt.vtk");
    reader->Update();

  // Create a transfer function mapping scalar value to opacity
  vtkPiecewiseFunction *oTFun = vtkPiecewiseFunction::New();
    oTFun->AddSegment(80, 0.0, 255, 1.0);

  // Create a transfer function mapping scalar value to color (grey)
  vtkPiecewiseFunction *cTFun = vtkPiecewiseFunction::New();
    cTFun->AddSegment(0, 1.0, 255, 1.0);

  // Create a property for the volume and set the transfer functions.
  // Turn shading on and use trilinear interpolation
  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetColor(cTFun);
    volumeProperty->SetOpacity(oTFun);
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->ShadeOn();

  // Create a ray function - this is a compositing ray function
  vtkVolumeRayCastCompositeFunction *compositeFunction = 
    vtkVolumeRayCastCompositeFunction::New();

  // Create the volume mapper and set the ray function and scalar input
  vtkVolumeRayCastMapper *volumeMapper = vtkVolumeRayCastMapper::New();
    volumeMapper->SetScalarInput(reader->GetOutput());
    volumeMapper->SetVolumeRayCastFunction(compositeFunction);

  // Create the volume and set the mapper and property
  vtkVolume *volume = vtkVolume::New();
    volume->SetVolumeMapper(volumeMapper);
    volume->SetVolumeProperty(volumeProperty);

  // Add this volume to the renderer and get a closer look
  ren1->AddVolume(volume);
  ren1->GetActiveCamera()->Azimuth(20.0);
  ren1->GetActiveCamera()->Dolly(1.60);

  renWin->SetSize(200,200);

  renWin->Render();

  // Interact with the data at 3 frames per second
  iren->SetDesiredUpdateRate(3.0);
  iren->SetStillUpdateRate(0.001);
  iren->Start();

  // Clean up
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  oTFun->Delete();
  cTFun->Delete();
  volumeProperty->Delete();
  compositeFunction->Delete();
  volumeMapper->Delete();
  volume->Delete();
}



