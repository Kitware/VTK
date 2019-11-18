/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPointGaussianMapper.h"
#include "vtkPointSource.h"
#include "vtkProp3DCollection.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

int TestPointGaussianSelection(int argc, char* argv[])
{
  int desiredPoints = 1.0e3;

  vtkNew<vtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(pow(desiredPoints, 0.33) * 20.0);
  points->Update();

  vtkNew<vtkRandomAttributeGenerator> randomAttr;
  randomAttr->SetInputConnection(points->GetOutputPort());

  vtkNew<vtkPointGaussianMapper> mapper;

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

#ifdef TestPoints
  randomAttr->SetDataTypeToUnsignedChar();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->SetMinimumComponentValue(0);
  randomAttr->SetMaximumComponentValue(255);
  randomAttr->Update();
  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetScaleFactor(0.0);
  mapper->EmissiveOff();
#else
  randomAttr->SetDataTypeToFloat();
  randomAttr->GeneratePointScalarsOn();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->Update();

  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetInterpolateScalarsBeforeMapping(0);
  mapper->SetScaleArray("RandomPointVectors");
  mapper->SetScaleArrayComponent(3);

  // Note that LookupTable is 4x faster than
  // ColorTransferFunction. So if you have a choice
  // Usa a lut instead.
  //
  vtkNew<vtkLookupTable> lut;
  lut->SetHueRange(0.1, 0.2);
  lut->SetSaturationRange(1.0, 0.5);
  lut->SetValueRange(0.8, 1.0);
  mapper->SetLookupTable(lut);
#endif

  renderWindow->Render();
  renderer->GetActiveCamera()->Zoom(3.5);
  renderWindow->Render();

  vtkNew<vtkHardwareSelector> selector;
  selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
  selector->SetRenderer(renderer);
  selector->SetArea(10, 10, 50, 50);
  vtkSelection* result = selector->Select();

  bool goodPick = false;

  if (result->GetNumberOfNodes() == 1)
  {
    vtkSelectionNode* node = result->GetNode(0);
    vtkIdTypeArray* selIds = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());

    if (selIds)
    {
      vtkIdType numIds = selIds->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numIds; ++i)
      {
        vtkIdType curId = selIds->GetValue(i);
        cerr << curId << "\n";
      }
    }

    if (node->GetProperties()->Has(vtkSelectionNode::PROP_ID()) &&
      node->GetProperties()->Get(vtkSelectionNode::PROP()) == actor.Get() &&
      node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()) == 1 && selIds &&
      selIds->GetNumberOfTuples() == 14 && selIds->GetValue(4) == 227)
    {
      goodPick = true;
    }
  }
  result->Delete();

  if (!goodPick)
  {
    cerr << "Incorrect splats picked!\n";
    return EXIT_FAILURE;
  }

  // Interact if desired
  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
