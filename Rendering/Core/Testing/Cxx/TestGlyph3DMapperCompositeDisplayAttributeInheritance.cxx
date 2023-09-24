// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This test exercises whether display attributes can be overriden for certain
// datasets in a composite dataset.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// In interactive mode, you can press the 'n' or 'N' key to cycle visibility of
// the individual shapes.

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkColor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkConeSource.h"
#include "vtkGlyph3DMapper.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

namespace
{
struct KeyCallbackBridgeData
{
  vtkSmartPointer<vtkPartitionedDataSetCollection> PartitionedDataSetCollection;
  vtkSmartPointer<vtkCompositeDataDisplayAttributes> DisplayAttributes;
  int CurrentInvisibleId = 0;
};

void HideNextBlock(vtkObject* caller, unsigned long, void* clientdata, void*)
{
  auto iren = reinterpret_cast<vtkRenderWindowInteractor*>(caller);
  const char key = iren->GetKeySym()[0];
  if (key != 'n' && key != 'N')
  {
    return;
  }
  auto bridge = reinterpret_cast<KeyCallbackBridgeData*>(clientdata);
  int maxId = bridge->PartitionedDataSetCollection->GetNumberOfPartitionedDataSets();
  auto dobj =
    bridge->PartitionedDataSetCollection->GetPartitionAsDataObject(bridge->CurrentInvisibleId, 0);
  bridge->DisplayAttributes->RemoveBlockVisibilities();
  std::cout << "Hide partitioned dataset " << bridge->CurrentInvisibleId << ": "
            << dobj->GetObjectDescription() << std::endl;
  bridge->DisplayAttributes->SetBlockVisibility(dobj, false);
  iren->Render();
  bridge->CurrentInvisibleId++;
  bridge->CurrentInvisibleId %= maxId;
}

}

int TestGlyph3DMapperCompositeDisplayAttributeInheritance(int argc, char* argv[])
{
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPartitionedDataSetCollectionSource> pdscSource;
  pdscSource->SetNumberOfShapes(12);
  pdscSource->Update();
  auto shapes = vtkPartitionedDataSetCollection::SafeDownCast(pdscSource->GetOutput());

  vtkNew<vtkGlyph3DMapper> mapper;
  mapper->ScalarVisibilityOff();
  mapper->SetScaleFactor(0.5);
  mapper->SetOrientationArray("Normals");
  mapper->SetOrientationModeToDirection();
  mapper->SetInputDataObject(shapes);
  mapper->SetSourceConnection(cone->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkCompositeDataDisplayAttributes> attrs;
  mapper->SetBlockAttributes(attrs);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  renWin->SetInteractor(iren);

  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(0, 0), vtkColor3d(1., 1., 0.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(1, 0), vtkColor3d(1., 0., 0.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(2, 0), vtkColor3d(1., 0., 1.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(3, 0), vtkColor3d(1., 1., 0.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(4, 0), vtkColor3d(1., 0., 0.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(5, 0), vtkColor3d(1., 0., 1.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(6, 0), vtkColor3d(1., 1., 0.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(7, 0), vtkColor3d(1., 0., 0.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(8, 0), vtkColor3d(1., 0., 1.).GetData());
  attrs->SetBlockColor(shapes->GetPartitionAsDataObject(10, 0), vtkColor3d(1., 0., 0.).GetData());
  attrs->SetBlockOpacity(shapes->GetPartitionAsDataObject(3, 0), 0.5);
  attrs->SetBlockVisibility(shapes->GetPartitionAsDataObject(9, 0), false); // hides that big shape.
  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    ::KeyCallbackBridgeData bridge;
    bridge.PartitionedDataSetCollection = shapes;
    bridge.DisplayAttributes = attrs;
    vtkNew<vtkCallbackCommand> keyCommand;
    keyCommand->SetCallback(&::HideNextBlock);
    keyCommand->SetClientData(&bridge);
    iren->AddObserver(vtkCommand::KeyPressEvent, keyCommand);
    iren->Start();
  }

  return !retVal;
}
