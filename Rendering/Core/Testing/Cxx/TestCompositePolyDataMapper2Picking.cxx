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

#include <algorithm>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCullerCollection.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTrivialProducer.h"
#include "vtkPointDataToCellData.h"

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

#include "vtkCylinderSource.h"
#include "vtkElevationFilter.h"
#include "vtkPlaneSource.h"
#include "vtkAppendPolyData.h"
#include "vtkExtractEdges.h"
#include "vtkCommand.h"
#include "vtkAreaPicker.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkProp3DCollection.h"
#include "vtkHardwareSelector.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkRenderedAreaPicker.h"

namespace {

class PointPickCommand : public vtkCommand
{
protected:
  vtkRenderer *Renderer;
  vtkAreaPicker *Picker;
  vtkPolyDataMapper *Mapper;
  std::map<int,std::vector<int> > BlockPrims;

public:
  static PointPickCommand * New() {return new PointPickCommand;}
  vtkTypeMacro(PointPickCommand, vtkCommand);

  PointPickCommand()
  {
  }

  ~PointPickCommand() VTK_OVERRIDE
  {
  }

  void SetPointIds(vtkSelection *selection)
  {
  // Find selection node that we're interested in:
  const vtkIdType numNodes = selection->GetNumberOfNodes();
  for (vtkIdType nodeId = 0; nodeId < numNodes; ++nodeId)
  {
    vtkSelectionNode *node = selection->GetNode(nodeId);

    // Check if the mapper is this instance of MoleculeMapper
    vtkActor *selActor = vtkActor::SafeDownCast(
               node->GetProperties()->Get(vtkSelectionNode::PROP()));
    if (selActor && (selActor->GetMapper() == this->Mapper))
    {
      int blockIndex =
        node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX());
        cerr << "Block ID " << blockIndex << " with prim ids of: ";

      vtkIdTypeArray *selIds = vtkArrayDownCast<vtkIdTypeArray>(
            node->GetSelectionList());
      if (selIds)
      {
        vtkIdType numIds = selIds->GetNumberOfTuples();
        for (vtkIdType i = 0; i < numIds; ++i)
        {
          vtkIdType curId = selIds->GetValue(i);
          this->BlockPrims[blockIndex].push_back(curId);
          cerr << " " << curId;
        }
      }
      cerr << "\n";
    }
  }
  }

  std::map<int, std::vector<int> > &GetBlockPrims()
  {
    return this->BlockPrims;
  }

  void SetMapper(vtkPolyDataMapper *m)
  {
    this->Mapper = m;
  }

  void SetRenderer(vtkRenderer *r)
  {
    this->Renderer = r;
  }

  void SetPicker(vtkAreaPicker *p)
  {
    this->Picker = p;
  }

  void Execute(vtkObject *, unsigned long, void *) VTK_OVERRIDE
  {
    vtkProp3DCollection *props = this->Picker->GetProp3Ds();
    if (props->GetNumberOfItems() != 0)
    {
      // If anything was picked during the fast area pick, do a more detailed
      // pick.
      vtkNew<vtkHardwareSelector> selector;
      selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
      selector->SetRenderer(this->Renderer);
      selector->SetArea(
            static_cast<unsigned int>(this->Renderer->GetPickX1()),
            static_cast<unsigned int>(this->Renderer->GetPickY1()),
            static_cast<unsigned int>(this->Renderer->GetPickX2()),
            static_cast<unsigned int>(this->Renderer->GetPickY2()));
      // Make the actual pick and pass the result to the convenience function
      // defined earlier
      vtkSelection *result = selector->Select();
      this->SetPointIds(result);
      //this->DumpPointSelection();
      result->Delete();
    }
  }

};

}

int TestCompositePolyDataMapper2Picking(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindow> win =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderer> ren =
    vtkSmartPointer<vtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  win->SetMultiSamples(0);

  vtkSmartPointer<vtkCompositePolyDataMapper2> mapper =
    vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  vtkNew<vtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa.GetPointer());

  int resolution = 18;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(resolution, resolution);
  plane->SetOrigin(-0.2, -0.2, 0.0);
  plane->SetPoint1(0.2, -0.2, 0.0);
  plane->SetPoint2(-0.2, 0.2, 0.0);

  vtkNew<vtkExtractEdges> extract;
  extract->SetInputConnection(plane->GetOutputPort());

  vtkNew<vtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(resolution);

  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(cyl->GetOutputPort());

  vtkNew<vtkPointDataToCellData> p2c;
  p2c->SetInputConnection(elev->GetOutputPort());
  p2c->PassPointDataOff();

  // build a composite dataset
  vtkNew<vtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = {1, 8, 16};
  std::vector<vtkSmartPointer<vtkMultiBlockDataSet> > blocks;
  blocks.push_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numLeaves = 0;
  int numNodes = 0;
  vtkStdString blockName("Rolf");
  for (int level = 1; level < numLevels; ++level)
  {
    int nblocks=blocksPerLevel[level];
    for (unsigned parent = levelStart; parent < levelEnd; ++parent)
    {
      blocks[parent]->SetNumberOfBlocks(nblocks);
      for (int block=0; block < nblocks; ++block, ++numNodes)
      {
        if (level == numLevels - 1)
        {
          vtkNew<vtkPolyData> child;
          if ((block/6)%2)
          {
            cyl->SetCenter(block*0.25, 0.0, parent*0.5);
            plane->SetCenter(block*0.25, 0.5, parent*0.5);
            elev->SetLowPoint(block*0.25 - 0.2 + 0.2*block/nblocks, -0.02, 0.0);
            elev->SetHighPoint(block*0.25 + 0.1 + 0.2*block/nblocks, 0.02, 0.0);
            p2c->Update();
            child->DeepCopy(p2c->GetOutput(0));
          }
          else
          {
            plane->SetCenter(block*0.25, 0.5, parent*0.5);
            extract->Update();
            child->DeepCopy(extract->GetOutput(0));
          }
          blocks[parent]->SetBlock(
            block, (block % 2) ? NULL : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(
            vtkCompositeDataSet::NAME(), blockName.c_str());
          // test not seting it on some
          if (block % 11)
          {
            mapper->SetBlockVisibility(parent+numLeaves, (block % 7) != 0);
          }
          ++numLeaves;
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

  mapper->SetInputData((vtkPolyData *)(data.GetPointer()));
  mapper->SetScalarModeToUseCellData();

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeColor(1,0,0);
  //actor->GetProperty()->EdgeVisibilityOn();
  ren->AddActor(actor);
  win->SetSize(400,400);

  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  ren->ResetCamera();
  win->Render();  // get the window up

  // modify the data to force a rebuild of OpenGL structs
  // after rendering set one cylinder to white
  mapper->SetBlockColor(80,1.0,1.0,1.0);
  mapper->SetBlockOpacity(80,1.0);
  mapper->SetBlockVisibility(80,1.0);

  // Setup picker
  vtkNew<vtkInteractorStyleRubberBandPick> pickerInt;
  iren->SetInteractorStyle(pickerInt.GetPointer());
  vtkNew<vtkRenderedAreaPicker> picker;
  iren->SetPicker(picker.GetPointer());

  ren->GetActiveCamera()->Elevation(30.0);
  ren->GetActiveCamera()->Azimuth(-40.0);
  ren->GetActiveCamera()->Zoom(3.0);
  ren->GetActiveCamera()->Roll(10.0);
  win->Render();

  // We'll follow up the cheap RenderedAreaPick with a detailed selection
  vtkNew<PointPickCommand> com;
  com->SetRenderer(ren.GetPointer());
  com->SetPicker(picker.GetPointer());
  com->SetMapper(mapper.GetPointer());
  picker->AddObserver(vtkCommand::EndPickEvent, com.GetPointer());

  // Make pick
  win->Render();
  picker->AreaPick(250, 300, 380, 380, ren.GetPointer());
  win->Render();

  // Interact if desired
  int retVal = vtkRegressionTestImage(win.GetPointer());
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Verify pick
  std::map<int,std::vector<int> > &bPrims =
    com->GetBlockPrims();
  if (
      bPrims.find(48) == bPrims.end() ||
      std::find(bPrims[48].begin(), bPrims[48].end(), 14) == bPrims[48].end() ||
      bPrims.find(97) == bPrims.end() ||
      std::find(bPrims[82].begin(), bPrims[82].end(), 114) == bPrims[82].end()
      )
  {
    cerr << "Incorrect pick results (if any picks were performed inter"
            "actively this could be ignored).\n";
    return EXIT_FAILURE;
  }

  return !retVal;
}
