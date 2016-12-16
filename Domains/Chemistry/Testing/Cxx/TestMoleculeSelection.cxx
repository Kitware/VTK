/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkNew.h"
#include "vtkProp3DCollection.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkTrivialProducer.h"

class MoleculePickCommand : public vtkCommand
{
protected:
  vtkNew<vtkIdTypeArray> AtomIds;
  vtkNew<vtkIdTypeArray> BondIds;
  vtkRenderer *Renderer;
  vtkAreaPicker *Picker;
  vtkAlgorithm *MoleculeSource;
  vtkMoleculeMapper *MoleculeMapper;

public:
  static MoleculePickCommand * New() {return new MoleculePickCommand;}
  vtkTypeMacro(MoleculePickCommand, vtkCommand);

  MoleculePickCommand()
  {
  }

  ~MoleculePickCommand() VTK_OVERRIDE
  {
  }

  vtkIdTypeArray *GetAtomIds()
  {
    return this->AtomIds.GetPointer();
  }

  vtkIdTypeArray *GetBondIds()
  {
    return this->BondIds.GetPointer();
  }

  void SetRenderer(vtkRenderer *r)
  {
    this->Renderer = r;
  }

  void SetPicker(vtkAreaPicker *p)
  {
    this->Picker = p;
  }

  void SetMoleculeSource(vtkAlgorithm *m)
  {
    this->MoleculeSource = m;
  }

  void SetMoleculeMapper(vtkMoleculeMapper *m)
  {
    this->MoleculeMapper = m;
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
      this->SetIdArrays(result);
      this->DumpMolSelection();
      result->Delete();
    }
  }

  // Set the ids for the atom/bond selection
  void SetIdArrays(vtkSelection *sel)
  {
    this->MoleculeMapper->GetSelectedAtomsAndBonds(
          sel, this->AtomIds.GetPointer(), this->BondIds.GetPointer());
  }

  // Convenience function to print out the atom and bond ids that belong to
  // molMap and are contained in sel
  void DumpMolSelection()
  {
    vtkMolecule *mol = this->MoleculeMapper->GetInput();

    // Print selection
    cerr << "\n### Selection ###\n";
    cerr << "Atoms: ";
    for (vtkIdType i = 0; i < this->AtomIds->GetNumberOfTuples(); i++)
    {
      cerr << this->AtomIds->GetValue(i) << " ";
    }
    cerr << "\nBonds: ";
    for (vtkIdType i = 0; i < this->BondIds->GetNumberOfTuples(); i++)
    {
      vtkBond bond = mol->GetBond(this->BondIds->GetValue(i));
      cerr << bond.GetId() << " (" << bond.GetBeginAtomId() << "-"
           << bond.GetEndAtomId() << ") ";
    }
    cerr << endl;
  }
};

int TestMoleculeSelection(int argc, char *argv[])
{
  vtkNew<vtkMolecule> mol;

  // Use a trivial producer, since the molecule was created by hand
  vtkNew<vtkTrivialProducer> molSource;
  molSource->SetOutput(mol.GetPointer());

  // Create a 4x4 grid of atoms one angstrom apart
  vtkAtom a1  = mol->AppendAtom( 1, 0.0, 0.0, 0.0);
  vtkAtom a2  = mol->AppendAtom( 2, 0.0, 1.0, 0.0);
  vtkAtom a3  = mol->AppendAtom( 3, 0.0, 2.0, 0.0);
  vtkAtom a4  = mol->AppendAtom( 4, 0.0, 3.0, 0.0);
  vtkAtom a5  = mol->AppendAtom( 5, 1.0, 0.0, 0.0);
  vtkAtom a6  = mol->AppendAtom( 6, 1.0, 1.0, 0.0);
  vtkAtom a7  = mol->AppendAtom( 7, 1.0, 2.0, 0.0);
  vtkAtom a8  = mol->AppendAtom( 8, 1.0, 3.0, 0.0);
  vtkAtom a9  = mol->AppendAtom( 9, 2.0, 0.0, 0.0);
  vtkAtom a10 = mol->AppendAtom(10, 2.0, 1.0, 0.0);
  vtkAtom a11 = mol->AppendAtom(11, 2.0, 2.0, 0.0);
  vtkAtom a12 = mol->AppendAtom(12, 2.0, 3.0, 0.0);
  vtkAtom a13 = mol->AppendAtom(13, 3.0, 0.0, 0.0);
  vtkAtom a14 = mol->AppendAtom(14, 3.0, 1.0, 0.0);
  vtkAtom a15 = mol->AppendAtom(15, 3.0, 2.0, 0.0);
  vtkAtom a16 = mol->AppendAtom(16, 3.0, 3.0, 0.0);

  // Add bonds along the grid
  mol->AppendBond( a1,  a2, 1);
  mol->AppendBond( a2,  a3, 1);
  mol->AppendBond( a3,  a4, 1);
  mol->AppendBond( a5,  a6, 1);
  mol->AppendBond( a6,  a7, 1);
  mol->AppendBond( a7,  a8, 1);
  mol->AppendBond( a9, a10, 1);
  mol->AppendBond(a10, a11, 1);
  mol->AppendBond(a11, a12, 1);
  mol->AppendBond(a13, a14, 1);
  mol->AppendBond(a14, a15, 1);
  mol->AppendBond(a15, a16, 1);
  mol->AppendBond( a1,  a5, 1);
  mol->AppendBond( a2,  a6, 1);
  mol->AppendBond( a3,  a7, 1);
  mol->AppendBond( a4,  a8, 1);
  mol->AppendBond( a5,  a9, 1);
  mol->AppendBond( a6, a10, 1);
  mol->AppendBond( a7, a11, 1);
  mol->AppendBond( a8, a12, 1);
  mol->AppendBond( a9, a13, 1);
  mol->AppendBond(a10, a14, 1);
  mol->AppendBond(a11, a15, 1);
  mol->AppendBond(a12, a16, 1);

  // Set up render engine
  vtkNew<vtkMoleculeMapper> molmapper;
  molmapper->SetInputData(mol.GetPointer());
  molmapper->UseBallAndStickSettings();
  molmapper->SetAtomicRadiusTypeToUnitRadius();

  vtkNew<vtkActor> actor;
  actor->SetMapper(molmapper.GetPointer());

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
  // For easier debugging of clipping planes:
  ren->GetActiveCamera()->ParallelProjectionOn();
  ren->GetActiveCamera()->Zoom(2.2);

  // Setup picker
  vtkNew<vtkInteractorStyleRubberBandPick> pickerInt;
  iren->SetInteractorStyle(pickerInt.GetPointer());
  vtkNew<vtkRenderedAreaPicker> picker;
  iren->SetPicker(picker.GetPointer());

  // We'll follow up the cheap RenderedAreaPick with a detailed selection
  // to obtain the atoms and bonds.
  vtkNew<MoleculePickCommand> com;
  com->SetRenderer(ren.GetPointer());
  com->SetPicker(picker.GetPointer());
  com->SetMoleculeSource(molSource.GetPointer());
  com->SetMoleculeMapper(molmapper.GetPointer());
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
  if (com->GetAtomIds()->GetNumberOfTuples() < 4 ||
      com->GetAtomIds()->GetValue(0) != 0  ||
      com->GetAtomIds()->GetValue(1) != 1  ||
      com->GetAtomIds()->GetValue(2) != 4  ||
      com->GetAtomIds()->GetValue(3) != 5  ||
      com->GetBondIds()->GetNumberOfTuples() < 8 ||
      com->GetBondIds()->GetValue(0) != 0  ||
      com->GetBondIds()->GetValue(1) != 1  ||
      com->GetBondIds()->GetValue(2) != 3  ||
      com->GetBondIds()->GetValue(3) != 4  ||
      com->GetBondIds()->GetValue(4) != 12 ||
      com->GetBondIds()->GetValue(5) != 13 ||
      com->GetBondIds()->GetValue(6) != 16 ||
      com->GetBondIds()->GetValue(7) != 17 )
  {
    cerr << "Incorrect atoms/bonds picked! (if any picks were performed inter"
            "actively this could be ignored).\n";
    return EXIT_FAILURE;
  }

  return !retVal;
}
