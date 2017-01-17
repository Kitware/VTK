/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <algorithm>

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3DCollection.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"

class PointPickCommand : public vtkCommand
{
protected:
  std::vector<int> PointIds;
  vtkRenderer *Renderer;
  vtkAreaPicker *Picker;
  vtkPolyDataMapper *Mapper;

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
      // Separate the selection ids into atoms and bonds
      vtkIdTypeArray *selIds = vtkArrayDownCast<vtkIdTypeArray>(
            node->GetSelectionList());
      if (selIds)
      {
        vtkIdType numIds = selIds->GetNumberOfTuples();
        for (vtkIdType i = 0; i < numIds; ++i)
        {
          vtkIdType curId = selIds->GetValue(i);
          this->PointIds.push_back(curId);
        }
      }
    }
  }
  }

  std::vector<int> &GetPointIds()
  {
    return this->PointIds;
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
      this->DumpPointSelection();
      result->Delete();
    }
  }

  // Convenience function to print out the atom and bond ids that belong to
  // molMap and are contained in sel
  void DumpPointSelection()
  {
    // Print selection
    cerr << "\n### Selection ###\n";
    cerr << "Points: ";
    for (std::vector<int>::iterator i = this->PointIds.begin();
         i != this->PointIds.end(); ++i)
    {
      cerr << *i << " ";
    }
    cerr << endl;
  }
};

int TestPointSelection(int argc, char *argv[])
{
  // create a line and a mesh
  vtkNew<vtkSphereSource> sphere;

  // Set up render engine
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(sphereMapper.GetPointer());

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor.GetPointer());
  vtkNew<vtkRenderWindow> win;
  win->SetMultiSamples(0);
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->SetBackground(0.0,0.0,0.0);
  win->SetSize(450,450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.2);

  // Setup picker
  vtkNew<vtkInteractorStyleRubberBandPick> pickerInt;
  iren->SetInteractorStyle(pickerInt.GetPointer());
  vtkNew<vtkRenderedAreaPicker> picker;
  iren->SetPicker(picker.GetPointer());

  // We'll follow up the cheap RenderedAreaPick with a detailed selection
  // to obtain the atoms and bonds.
  vtkNew<PointPickCommand> com;
  com->SetRenderer(ren.GetPointer());
  com->SetPicker(picker.GetPointer());
  com->SetMapper(sphereMapper.GetPointer());
  picker->AddObserver(vtkCommand::EndPickEvent, com.GetPointer());

  // Make pick -- lower left quarter of renderer
  win->Render();
  picker->AreaPick(0, 0, 225, 225, ren.GetPointer());
  win->Render();

  // Interact if desired
  int retVal = vtkRegressionTestImage(win.GetPointer());
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Verify pick
  std::vector<int> &pIds = com->GetPointIds();
  if (pIds.size() < 7 ||
      std::find(pIds.begin(), pIds.end(), 0) == pIds.end() ||
      std::find(pIds.begin(), pIds.end(), 26) == pIds.end() ||
      std::find(pIds.begin(), pIds.end(), 27) == pIds.end() ||
      std::find(pIds.begin(), pIds.end(), 32) == pIds.end() ||
      std::find(pIds.begin(), pIds.end(), 33) == pIds.end() ||
      std::find(pIds.begin(), pIds.end(), 38) == pIds.end() ||
      std::find(pIds.begin(), pIds.end(), 39) == pIds.end()
      )
  {
    cerr << "Incorrect atoms/bonds picked! (if any picks were performed inter"
            "actively this could be ignored).\n";
    return EXIT_FAILURE;
  }

  return !retVal;
}
