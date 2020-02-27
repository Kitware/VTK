/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataSet.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointGaussianMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTrivialProducer.h"

#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

#include "vtkCylinderSource.h"

int TestCompositeDataPointGaussianSelection(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderer> ren;
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  vtkNew<vtkPointGaussianMapper> mapper;
  mapper->SetScaleFactor(0.01);

  int resolution = 18;
  vtkNew<vtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(resolution);

  // build a composite dataset
  vtkNew<vtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = { 1, 16, 32 };
  std::vector<vtkSmartPointer<vtkMultiBlockDataSet> > blocks;
  blocks.push_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numNodes = 0;
  vtkStdString blockName("Rolf");
  mapper->SetInputDataObject(data.GetPointer());
  for (int level = 1; level < numLevels; ++level)
  {
    int nblocks = blocksPerLevel[level];
    for (unsigned parent = levelStart; parent < levelEnd; ++parent)
    {
      blocks[parent]->SetNumberOfBlocks(nblocks);
      for (int block = 0; block < nblocks; ++block, ++numNodes)
      {
        if (level == numLevels - 1)
        {
          vtkNew<vtkPolyData> child;
          cyl->SetCenter(block * 0.25, 0.0, parent * 0.5);
          cyl->Update();
          child->DeepCopy(cyl->GetOutput(0));
          blocks[parent]->SetBlock(block, (block % 2) ? nullptr : child.GetPointer());
        }
        else
        {
          vtkNew<vtkMultiBlockDataSet> child;
          blocks[parent]->SetBlock(block, child.GetPointer());
          blocks.push_back(child.GetPointer());
        }
      }
    }
    levelStart = levelEnd;
    levelEnd = static_cast<unsigned>(blocks.size());
  }

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->ResetCamera();

  ren->GetActiveCamera()->Elevation(40.0);
  ren->GetActiveCamera()->Zoom(3.2);
  ren->GetActiveCamera()->Roll(20.0);
  win->Render();

  vtkNew<vtkHardwareSelector> selector;
  selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
  selector->SetRenderer(ren);
  selector->SetArea(10, 10, 50, 50);
  vtkSelection* result = selector->Select();

  bool goodPick = false;

  cerr << "numnodes: " << result->GetNumberOfNodes() << "\n";
  if (result->GetNumberOfNodes() == 5)
  {
    for (unsigned int nodenum = 0; nodenum < result->GetNumberOfNodes(); ++nodenum)
    {
      vtkSelectionNode* node = result->GetNode(nodenum);

      cerr << "Node: " << nodenum
           << " comp: " << node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()) << "\n";

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
    }

    vtkIdTypeArray* selIds =
      vtkArrayDownCast<vtkIdTypeArray>(result->GetNode(0)->GetSelectionList());
    if (result->GetNode(0)->GetProperties()->Has(vtkSelectionNode::PROP_ID()) &&
      result->GetNode(0)->GetProperties()->Get(vtkSelectionNode::PROP()) == actor.Get() &&
      result->GetNode(0)->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()) == 305 &&
      result->GetNode(2)->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()) == 340 &&
      selIds && selIds->GetNumberOfTuples() == 5 && selIds->GetValue(2) == 56)
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

  int retVal = vtkRegressionTestImageThreshold(win.GetPointer(), 15);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
