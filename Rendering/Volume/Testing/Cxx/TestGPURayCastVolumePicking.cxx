/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumePicking.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers additive method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the additive method.

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkRenderedAreaPicker.h"

#include "vtkCommand.h"
#include "vtkHardwareSelector.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkInformationIntegerKey.h"


typedef vtkSmartPointer<vtkHardwareSelector> SelectorPtr;
typedef vtkSmartPointer<vtkRenderer> RendererPtr;

class VolumePickingCommand : public vtkCommand
{
public:

  VolumePickingCommand()
  : Renderer(NULL)
  {
  };

  ~VolumePickingCommand()
  {
  };

  virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId),
    void* vtkNotUsed(callData))
  {
    assert(this->Renderer != NULL);

    SelectorPtr sel = SelectorPtr::New();
    sel->SetRenderer(this->Renderer);
    sel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);

    unsigned int const x1 = static_cast<unsigned int>(this->Renderer->GetPickX1());
    unsigned int const y1 = static_cast<unsigned int>(this->Renderer->GetPickY1());
    unsigned int const x2 = static_cast<unsigned int>(this->Renderer->GetPickX2());
    unsigned int const y2 = static_cast<unsigned int>(this->Renderer->GetPickY2());
    sel->SetArea(x1, y1, x2, y2);

    vtkSelection* result = sel->Select();
    //result->Print(std::cout);

    unsigned int const numProps = result->GetNumberOfNodes();
    if (numProps > 0)
      std::cout << "->>> Hit Props: " << '\n';

    for (unsigned int n = 0; n < numProps; n++)
      {
        vtkSelectionNode* node = result->GetNode(n);
        vtkInformation* properties = node->GetProperties();
        vtkInformationIntegerKey* infoIntKey = node->PROP_ID();
        int const propId = infoIntKey->Get(properties);

        vtkAbstractArray* abs = node->GetSelectionList();
        vtkIdType size = abs->GetSize();

        std::cout << "PropId: " << propId << "/ Num. Attr.:  " << size << '\n';
      }

    if (numProps > 0)
      std::cout << '\n';

    result->Delete();
  };

  ////////// members variables ///////////////////

  RendererPtr Renderer;
};

// =============================================================================
int TestGPURayCastVolumePicking(int argc, char *argv[])
{
  // volume source and mapper
  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);

  vtkSmartPointer<vtkImageChangeInformation> changeInformation = vtkSmartPointer<vtkImageChangeInformation>::New();
  changeInformation->SetInputConnection(reader->GetOutputPort());
  changeInformation->SetOutputSpacing(1, 2, 3);
  changeInformation->SetOutputOrigin(10, 20, 30);
  changeInformation->Update();

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  double scalarRange[2];
  volumeMapper->SetInputConnection(changeInformation->GetOutputPort());
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkNew<vtkVolume> volume;
  volume.GetPointer()->PickableOn();
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // polygonal sources and mapper
  vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
  cone->SetHeight(100.0);
  cone->SetRadius(50.0);
  cone->SetResolution(200.0);
  cone->SetCenter(80, 100, 100);
  cone->Update();

  vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());

  vtkSmartPointer<vtkActor> coneActor = vtkSmartPointer<vtkActor>::New();
  coneActor->SetMapper(coneMapper);
  coneActor->PickableOn();

  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetPhiResolution(20.0);
  sphere->SetThetaResolution(20.0);
  sphere->SetCenter(90, 40, 170);
  sphere->SetRadius(40.0);
  sphere->Update();

  vtkSmartPointer<vtkPolyDataMapper> sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->AddInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
  sphereActor->SetMapper(sphereMapper);
  sphereActor->PickableOn();

  // rendering setup
  RendererPtr ren = RendererPtr::New();
  ren->SetBackground(0.2, 0.2, 0.5);
  ren->AddActor(coneActor);
  ren->AddActor(sphereActor);
  ren->AddViewProp(volume.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->Render();
  ren->ResetCamera();

  // interaction & picking
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkInteractorStyleRubberBandPick* rbp = vtkInteractorStyleRubberBandPick::New();
  rwi->SetInteractorStyle(rbp);
  vtkRenderedAreaPicker* areaPicker = vtkRenderedAreaPicker::New();
  rwi->SetPicker(areaPicker);

  // Add selection observer
  VolumePickingCommand* vpc = new VolumePickingCommand;
  vpc->Renderer = ren;
  rwi->AddObserver(vtkCommand::EndPickEvent, vpc);
  vpc->Delete();

  // initialize render loop
  iren->Initialize();
  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
