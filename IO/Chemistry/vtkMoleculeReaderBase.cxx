// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMoleculeReaderBase.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtksys/SystemTools.hxx"

#include <cctype>

VTK_ABI_NAMESPACE_BEGIN
vtkMoleculeReaderBase::vtkMoleculeReaderBase()
{
  this->FileName = nullptr;
  this->BScale = 1.0;
  this->HBScale = 1.0;
  this->Molecule = nullptr;
  this->AtomType = nullptr;
  this->AtomTypeStrings = nullptr;
  this->Points = nullptr;
  this->RGB = nullptr;
  this->Radii = nullptr;
  this->Chain = nullptr;
  this->Residue = nullptr;
  this->SecondaryStructures = nullptr;
  this->SecondaryStructuresBegin = nullptr;
  this->SecondaryStructuresEnd = nullptr;
  this->IsHetatm = nullptr;
  this->Model = nullptr;
  this->NumberOfAtoms = 0;
  this->NumberOfModels = 0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

vtkMoleculeReaderBase::~vtkMoleculeReaderBase()
{
  delete[] this->FileName;
}

int vtkMoleculeReaderBase::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMolecule");
    return 1;
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

int vtkMoleculeReaderBase::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outMoleculeInfo = outputVector->GetInformationObject(1);
  if (outMoleculeInfo)
  {
    this->Molecule = vtkMolecule::SafeDownCast(outMoleculeInfo->Get(vtkDataObject::DATA_OBJECT()));
  }

  if (!this->FileName)
  {
    return 0;
  }

  FILE* fp;
  if ((fp = vtksys::SystemTools::Fopen(this->FileName, "r")) == nullptr)
  {
    vtkErrorMacro(<< "Unable to open " << this->FileName);
    return 0;
  }

  vtkDebugMacro(<< "Opening molecule base file " << this->FileName);
  this->ReadMolecule(fp, output);
  fclose(fp);

  output->Squeeze();

  return 1;
}

int vtkMoleculeReaderBase::ReadMolecule(FILE* fp, vtkPolyData* output)
{
  vtkDebugMacro(<< "Reading molecule file");

  if (!this->AtomType)
  {
    this->AtomType = vtkSmartPointer<vtkIdTypeArray>::New();
  }
  else
  {
    this->AtomType->Reset();
  }
  this->AtomType->SetName("atom_type");
  output->GetPointData()->AddArray(this->AtomType);

  if (!this->AtomTypeStrings)
  {
    this->AtomTypeStrings = vtkSmartPointer<vtkStringArray>::New();
  }
  else
  {
    this->AtomTypeStrings->Reset();
  }
  this->AtomTypeStrings->SetName("atom_types");
  output->GetPointData()->AddArray(this->AtomTypeStrings);

  if (!this->Residue)
  {
    this->Residue = vtkSmartPointer<vtkIdTypeArray>::New();
  }
  else
  {
    this->Residue->Reset();
  }
  this->Residue->SetName("residue");
  output->GetPointData()->AddArray(this->Residue);

  if (!this->Chain)
  {
    this->Chain = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  else
  {
    this->Chain->Reset();
  }
  this->Chain->SetName("chain");
  output->GetPointData()->AddArray(this->Chain);

  if (!this->SecondaryStructures)
  {
    this->SecondaryStructures = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  else
  {
    this->SecondaryStructures->Reset();
  }
  this->SecondaryStructures->SetName("secondary_structures");
  output->GetPointData()->AddArray(this->SecondaryStructures);

  if (!this->SecondaryStructuresBegin)
  {
    this->SecondaryStructuresBegin = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  else
  {
    this->SecondaryStructuresBegin->Reset();
  }
  this->SecondaryStructuresBegin->SetName("secondary_structures_begin");
  output->GetPointData()->AddArray(this->SecondaryStructuresBegin);

  if (!this->SecondaryStructuresEnd)
  {
    this->SecondaryStructuresEnd = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  else
  {
    this->SecondaryStructuresEnd->Reset();
  }
  this->SecondaryStructuresEnd->SetName("secondary_structures_end");
  output->GetPointData()->AddArray(this->SecondaryStructuresEnd);

  if (!this->IsHetatm)
  {
    this->IsHetatm = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  else
  {
    this->IsHetatm->Reset();
  }
  this->IsHetatm->SetName("ishetatm");
  output->GetPointData()->AddArray(this->IsHetatm);

  if (!this->Model)
  {
    this->Model = vtkSmartPointer<vtkUnsignedIntArray>::New();
  }
  else
  {
    this->Model->Reset();
  }
  this->Model->SetName("model");
  output->GetPointData()->AddArray(this->Model);

  if (!this->Points)
  {
    this->Points = vtkSmartPointer<vtkPoints>::New();
  }
  else
  {
    this->Points->Reset();
  }

  this->ReadSpecificMolecule(fp);

  vtkDebugMacro(<< "End of molecule reading");
  output->SetPoints(this->Points);

  // Assign bonds
  vtkSmartPointer<vtkCellArray> newBonds = vtkSmartPointer<vtkCellArray>::New();
  newBonds->AllocateEstimate(512, 1);
  this->MakeBonds(this->Points, this->AtomType, newBonds);
  output->SetLines(newBonds);

  vtkDebugMacro(<< "Read " << this->NumberOfAtoms << " atoms and found "
                << newBonds->GetNumberOfCells() << " bonds" << endl);

  // Assign RGB colors
  if (this->RGB)
  {
    this->RGB->Reset();
  }
  else
  {
    this->RGB = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  this->RGB->SetNumberOfComponents(3);
  this->RGB->Allocate(3 * this->NumberOfAtoms);
  this->RGB->SetName("rgb_colors");

  float rgb[3];
  for (vtkIdType i = 0; i < this->NumberOfAtoms; ++i)
  {
    this->PeriodicTable->GetDefaultRGBTuple(AtomType->GetValue(i), rgb);
    rgb[0] *= 255;
    rgb[1] *= 255;
    rgb[2] *= 255;
    this->RGB->InsertNextTuple(rgb);
  }
  output->GetPointData()->SetScalars(this->RGB);

  // Assign Van der Waals radii
  if (this->Radii)
  {
    this->Radii->Reset();
  }
  else
  {
    this->Radii = vtkSmartPointer<vtkFloatArray>::New();
  }
  this->Radii->SetNumberOfComponents(3);
  this->Radii->Allocate(3 * this->NumberOfAtoms);
  this->Radii->SetName("radius");

  // Assign atom radii
  // We're obliged here to insert the scalars "radius" 3 times to make it a
  // vector in order to use Glyph3D to color AND scale at the same time.
  for (vtkIdType i = 0; i < this->NumberOfAtoms; ++i)
  {
    double radius = this->PeriodicTable->GetVDWRadius(AtomType->GetValue(i));
    this->Radii->InsertNextTuple3(radius, radius, radius);
  }
  output->GetPointData()->SetVectors(this->Radii);

  return 0;
}

unsigned int vtkMoleculeReaderBase::MakeBonds(
  vtkPoints* points, vtkIdTypeArray* atomTypes, vtkCellArray* newBonds)
{
  double X[3], Y[3];

  vtkSmartPointer<vtkPolyData> dataset = vtkSmartPointer<vtkPolyData>::New();
  dataset->SetPoints(points);

  vtkSmartPointer<vtkIdList> neighborAtoms = vtkSmartPointer<vtkIdList>::New();

  // Add atoms to the molecule first because an atom must
  // must be declared before bonds involving it.
  if (this->Molecule)
  {
    for (vtkIdType i = 0; i < this->NumberOfAtoms; ++i)
    {
      points->GetPoint(i, X);
      this->Molecule->AppendAtom(atomTypes->GetValue(i), X[0], X[1], X[2]);
    }
  }

  double max, dist, radius;
  vtkIdType bond[2];
  unsigned int numberOfBonds = 0;

  vtkSmartPointer<vtkPointLocator> locator = vtkSmartPointer<vtkPointLocator>::New();
  locator->SetDataSet(dataset);

  for (vtkIdType atomId = this->NumberOfAtoms - 1; atomId > 0; --atomId)
  {
    bond[0] = atomId;
    points->GetPoint(atomId, X);

    vtkIdType atom1Type = atomTypes->GetValue(atomId);

    /* Find all the atoms in the neighborhood at the max acceptable
     * bond distance
     */
    radius =
      (this->PeriodicTable->GetCovalentRadius(atom1Type) + 2.0 + 0.56) * std::max(BScale, HBScale);
    locator->FindPointsWithinRadius(radius, X, neighborAtoms);

    for (vtkIdType k = neighborAtoms->GetNumberOfIds() - 1; k >= 0; --k)
    {
      vtkIdType neighborAtomId = neighborAtoms->GetId(k);
      vtkIdType atom2Type = atomTypes->GetValue(neighborAtomId);

      // Skip points with which a bond may have already been created
      if (neighborAtomId >= atomId)
      {
        continue;
      }

      /*
       * The outer loop index 'atomId' is AFTER the inner loop 'neighborAtomId': 'atomId'
       * leads 'neighborAtomId' in the list: since hydrogens traditionally follow
       * the heavy atom they're bonded to, this makes it easy to quit
       * bonding to hydrogens after one bond is made by breaking out of
       * the 'neighborAtomId' loop when 'atomId' is a hydrogen and we make a bond to it.
       * Working backwards like this makes it easy to find the heavy
       * atom that came 'just before' the Hydrogen. mp
       * Base distance criteria on vdw...lb
       */

      // Never bond hydrogens to each other
      if (atom1Type == 1 && atom2Type == 1)
      {
        continue;
      }

      dist = this->PeriodicTable->GetCovalentRadius(atom1Type) +
        this->PeriodicTable->GetCovalentRadius(atom2Type) + 0.56;
      max = dist * dist;

      if (atom1Type == 1 || atom2Type == 1)
      {
        max *= HBScale;
      }
      else
      {
        max *= BScale;
      }

      points->GetPoint(neighborAtomId, Y);
      double dx = X[0] - Y[0];
      double dy = X[1] - Y[1];
      double dz = X[2] - Y[2];
      dist = dx * dx + dy * dy + dz * dz;

      if (dist <= max)
      {
        bond[1] = neighborAtomId;
        newBonds->InsertNextCell(2, bond);

        // Add bond to the molecule
        if (this->Molecule)
        {
          this->Molecule->AppendBond(bond[0], bond[1]);
        }

        ++numberOfBonds;
      }
    }
    neighborAtoms->Reset();
  }
  newBonds->Squeeze();

  return numberOfBonds;
}

unsigned int vtkMoleculeReaderBase::MakeAtomType(const char* atomType)
{
  std::string atomTypeName(atomType);
  atomTypeName[0] = toupper(atomTypeName[0]);
  if (atomTypeName.size() == 2)
  {
    atomTypeName[1] = tolower(atomTypeName[1]);
  }

  unsigned int atomNumber = this->PeriodicTable->GetAtomicNumber(atomTypeName);

  // This check is required for atom type symbols that do not exist in the dataset, such as O1, N1.
  if (atomNumber != 0)
  {
    return atomNumber;
  }
  else
  {
    return this->PeriodicTable->GetAtomicNumber(std::string(1, atomTypeName[0]));
  }
}

void vtkMoleculeReaderBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "NumberOfAtoms: " << this->NumberOfAtoms << endl;
  os << indent << "NumberOfModels: " << this->NumberOfModels << endl;
  os << indent << "HBScale: " << this->HBScale << endl;
  os << indent << "BScale: " << this->BScale << endl;
}
VTK_ABI_NAMESPACE_END
