#include "vtk.h"

main ()
{
  vtkSLCReader *reader = vtkSLCReader::New();
      reader->SetFileName("../../../vtkdata/poship.slc");

  // Create transfer functions for opacity and color
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
      opacityTransferFunction->AddPoint(20, 0.0);
      opacityTransferFunction->AddPoint(255,0.2);

  vtkColorTransferFunction *colorTransferFunction = 
             vtkColorTransferFunction::New();
      colorTransferFunction->AddRedPoint(0.0, 0.0);
      colorTransferFunction->AddRedPoint(64.0, 1.0);
      colorTransferFunction->AddRedPoint(128.0, 0.0);
      colorTransferFunction->AddRedPoint(255.0, 0.0);
      colorTransferFunction->AddBluePoint(0.0, 0.0);
      colorTransferFunction->AddBluePoint(64.0, 0.0);
      colorTransferFunction->AddBluePoint(128.0, 1.0);
      colorTransferFunction->AddBluePoint(192.0, 0.0);
      colorTransferFunction->AddBluePoint(255.0, 0.0);
      colorTransferFunction->AddGreenPoint(0.0, 0.0);
      colorTransferFunction->AddGreenPoint(128.0, 0.0);
      colorTransferFunction->AddGreenPoint(192.0, 1.0);
      colorTransferFunction->AddGreenPoint(255.0, 0.2);

  // Create properties, mappers, volume actors, and ray cast function
  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
      volumeProperty->SetColor(colorTransferFunction);
      volumeProperty->SetOpacity(opacityTransferFunction);

  vtkVolumeRayCastCompositeFunction  *compositeFunction = 
            vtkVolumeRayCastCompositeFunction::New();

  vtkVolumeRayCastMapper *volumeMapper = vtkVolumeRayCastMapper::New();
      volumeMapper->SetScalarInput(reader->GetOutput());
      volumeMapper->SetVolumeRayCastFunction(compositeFunction);

  vtkVolume *volume = vtkVolume::New();
      volume->SetVolumeMapper(volumeMapper);
      volume->SetVolumeProperty(volumeProperty);

  // Create outline
  vtkOutlineFilter *outline = vtkOutlineFilter::New();
      outline->SetInput(reader->GetOutput());

  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
      outlineMapper->SetInput(outline->GetOutput());

  vtkActor *outlineActor = vtkActor::New();
      outlineActor->SetMapper(outlineMapper);
      outlineActor->GetProperty()->SetColor(1, 1, 1);

  // Okay now the graphics stuff
  vtkRenderer *ren1 = vtkRenderer::New();
      ren1->SetBackground(0.1, 0.2, 0.4);
  vtkRenderWindow *renWin = vtkRenderWindow::New();
      renWin->AddRenderer(ren1);
      renWin->SetSize(256, 256);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
      iren->SetRenderWindow(renWin);

  ren1->AddActor(outlineActor);
  ren1->AddVolume(volume);
  renWin->Render();

  // interact with data
  iren->Start();

  // Clean up
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
  reader->Delete();
  opacityTransferFunction->Delete();
  colorTransferFunction->Delete();
  volumeProperty->Delete();
  compositeFunction->Delete();
  volumeMapper->Delete();
  volume->Delete();
}
