/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeReaderBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMoleculeReaderBase.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cctype>

// TODO reorder arrays for atom type decrement
static double vtkMoleculeReaderBaseCovRadius[103] = { 0.32, 1.6, 0.68, 0.352, 0.832, 0.72, 0.68,
  0.68, 0.64, 1.12, 0.972, 1.1, 1.352, 1.2, 1.036, 1.02, 1, 1.568, 1.328, 0.992, 1.44, 1.472, 1.328,
  1.352, 1.352, 1.34, 1.328, 1.62, 1.52, 1.448, 1.22, 1.168, 1.208, 1.22, 1.208, 1.6, 1.472, 1.12,
  1.78, 1.56, 1.48, 1.472, 1.352, 1.4, 1.448, 1.5, 1.592, 1.688, 1.632, 1.46, 1.46, 1.472, 1.4, 1.7,
  1.672, 1.34, 1.872, 1.832, 1.82, 1.808, 1.8, 1.8, 1.992, 1.792, 1.76, 1.752, 1.74, 1.728, 1.72,
  1.94, 1.72, 1.568, 1.432, 1.368, 1.352, 1.368, 1.32, 1.5, 1.5, 1.7, 1.552, 1.54, 1.54, 1.68,
  1.208, 1.9, 1.8, 1.432, 1.18, 1.02, 0.888, 0.968, 0.952, 0.928, 0.92, 0.912, 0.9, 0.888, 0.88,
  0.872, 0.86, 0.848, 0.84 };

// TODO reorder arrays for atom type decrement
static double vtkMoleculeReaderBaseAtomColors[][3] = { { 255, 255, 255 }, { 127, 0, 127 },
  { 255, 0, 255 }, { 127, 127, 127 }, { 127, 0, 127 }, { 0, 255, 0 }, { 0, 0, 255 }, { 255, 0, 0 },
  { 0, 255, 255 }, { 127, 127, 127 }, { 127, 127, 127 }, { 178, 153, 102 }, { 127, 127, 127 },
  { 51, 127, 229 }, { 0, 255, 255 }, { 255, 255, 0 }, { 255, 127, 127 }, { 255, 255, 127 },
  { 127, 127, 127 }, { 51, 204, 204 }, { 127, 127, 127 }, { 0, 178, 178 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 204, 0, 255 }, { 255, 0, 255 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 229, 102, 51 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 255, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 102, 51, 204 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 51, 127, 51 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 }, { 127, 127, 127 },
  { 127, 127, 127 } };

// TODO update table
static double vtkMoleculeReaderBaseRadius[] = {
  1.2, 1.22, 1.75,  /* "H " "He" "Li" */
  1.50, 1.90, 1.80, /* "Be" "B " "C " */
  1.70, 1.60, 1.35, /* "N " "O " "F " */
  1.60, 2.31, 1.70, //
  2.05, 2.00, 2.70, //
  1.85, 1.81, 1.91, //
  2.31, 1.74, 1.80, //
  1.60, 1.50, 1.40, /* Ti-Cu and Ge are guestimates. */
  1.40, 1.40, 1.40, //
  1.60, 1.40, 1.40, //
  1.90, 1.80, 2.00, //
  2.00, 1.95, 1.98, //
  2.44, 2.40, 2.10, /* Sr-Rh and Ba and La are guestimates. */
  2.00, 1.80, 1.80, //
  1.80, 1.80, 1.80, //
  1.60, 1.70, 1.60, //
  1.90, 2.20, 2.20, //
  2.20, 2.15, 2.20, //
  2.62, 2.30, 2.30, //
  2.30, 2.30, 2.30, /* All of these are guestimates. */
  2.30, 2.30, 2.40, //
  2.30, 2.30, 2.30, //
  2.30, 2.30, 2.30, //
  2.40, 2.50, 2.30, //
  2.30, 2.30, 2.30, /* All but Pt and Bi are guestimates. */
  2.30, 2.30, 2.40, //
  2.30, 2.40, 2.50, //
  2.50, 2.40, 2.40, //
  2.40, 2.40, 2.90, //
  2.60, 2.30, 2.30, /* These are all guestimates. */
  2.30, 2.30, 2.30, //
  2.30, 2.30, 2.30, //
  2.30, 2.30, 2.30, //
  2.30, 2.30, 2.30, //
  2.30, 1.50        //
};

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
  vtkSmartPointer<vtkInformation> outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkSmartPointer<vtkPolyData> output =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSmartPointer<vtkInformation> outMoleculeInfo = outputVector->GetInformationObject(1);
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
  vtkDebugMacro(<< "Scanning molecule file");

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

  vtkDebugMacro(<< "End of molecule scanning");
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

  for (unsigned int i = 0; i < this->NumberOfAtoms; ++i)
  {
    this->RGB->InsertNextTuple(
      &vtkMoleculeReaderBaseAtomColors[AtomType->GetValue(i)][0]); // TODO fix atom types
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

  // Assign atom types
  // We're obliged here to insert the scalars "radius" 3 times to make it a
  // vector in order to use Glyph3D to color AND scale at the same time.
  for (unsigned int i = 0; i < this->NumberOfAtoms; ++i)
  {
    this->Radii->InsertNextTuple3(
      vtkMoleculeReaderBaseRadius[AtomType->GetValue(i)],  // TODO fix atom types
      vtkMoleculeReaderBaseRadius[AtomType->GetValue(i)],  // TODO fix atom types
      vtkMoleculeReaderBaseRadius[AtomType->GetValue(i)]); // TODO fix atom types
  }
  output->GetPointData()->SetVectors(this->Radii);

  return 0;
}

unsigned int vtkMoleculeReaderBase::MakeBonds(
  vtkPoints* newPoints, vtkIdTypeArray* atomTypes, vtkCellArray* newBonds)
{
  double X[3], Y[3];

  vtkSmartPointer<vtkPolyData> dataset = vtkSmartPointer<vtkPolyData>::New();
  dataset->SetPoints(newPoints);

  vtkSmartPointer<vtkIdList> neighborAtoms = vtkSmartPointer<vtkIdList>::New();

  // Add atoms to the molecule first because an atom must
  // must be declared before bonds involving it.
  if (this->Molecule)
  {
    for (unsigned int i = 0; i < this->NumberOfAtoms; ++i)
    {
      newPoints->GetPoint(i, X);
      this->Molecule->AppendAtom(
        atomTypes->GetValue(i) + 1, X[0], X[1], X[2]); // TODO fix atom type increment (bug)
    }
  }

  double max, dist, radius;
  vtkIdType bond[2];
  unsigned int numberOfBonds = 0;

  vtkSmartPointer<vtkPointLocator> locator = vtkSmartPointer<vtkPointLocator>::New();
  locator->SetDataSet(dataset);

  for (unsigned int atomId = this->NumberOfAtoms - 1; atomId > 0; --atomId)
  {
    bond[0] = atomId;
    newPoints->GetPoint(atomId, X);

    vtkIdType atom1Type = atomTypes->GetValue(atomId); // TODO fix atom type increment (bug)

    /* Find all the atoms in the neighborhood at the max acceptable
     * bond distance
     */
    radius = (vtkMoleculeReaderBaseCovRadius[atom1Type] + 2.0 + 0.56) * std::max(BScale, HBScale);
    locator->FindPointsWithinRadius(radius, X, neighborAtoms);

    for (vtkIdType k = neighborAtoms->GetNumberOfIds() - 1; k >= 0; --k)
    {
      vtkIdType neighborAtomId = neighborAtoms->GetId(k);
      vtkIdType atom2Type =
        atomTypes->GetValue(neighborAtomId); // TODO fix atom type increment (bug)

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
      if (atom1Type == 0 && atom2Type == 0)
      {
        continue;
      }

      dist = vtkMoleculeReaderBaseCovRadius[atom1Type] + vtkMoleculeReaderBaseCovRadius[atom2Type] +
        0.56;
      max = dist * dist;

      if (atom1Type == 0 || atom2Type == 0)
      {
        max *= HBScale;
      }
      else
      {
        max *= BScale;
      }

      newPoints->GetPoint(neighborAtomId, Y);
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

int vtkMoleculeReaderBase::MakeAtomType(const char* atomType)
{
  int atomNumber = 0;
  char a = toupper(atomType[0]);
  char b = toupper(atomType[1]);

  switch (a)
  {
    case 'A':
      if (b == 'C')
        atomNumber = 89;
      else if (b == 'G')
        atomNumber = 47;
      else if (b == 'L')
        atomNumber = 13;
      else if (b == 'M')
        atomNumber = 95;
      else if (b == 'R')
        atomNumber = 18;
      else if (b == 'S')
        atomNumber = 33;
      else if (b == 'T')
        atomNumber = 85;
      else if (b == 'U')
        atomNumber = 79;
      break;
    case 'B':
      if (b == 'A')
        atomNumber = 56;
      else if (b == 'E')
        atomNumber = 4;
      else if (b == 'I')
        atomNumber = 83;
      else if (b == 'K')
        atomNumber = 97;
      else if (b == 'R')
        atomNumber = 35;
      else
        atomNumber = 5;
      break;
    case 'C':
      if (b == 'L')
        atomNumber = 17;
      else if (b == 'O')
        atomNumber = 27;
      else if (b == 'R')
        atomNumber = 24;
      else if (b == 'S')
        atomNumber = 55;
      else if (b == 'U')
        atomNumber = 29;
      else if (b == '0')
        atomNumber = 6;
      else
        atomNumber = 6;
      break;
    case 'D':
      atomNumber = 66;
      break;
    case 'E':
      if (b == 'R')
        atomNumber = 68;
      else if (b == 'S')
        atomNumber = 99;
      else if (b == 'U')
        atomNumber = 63;
      break;
    case 'F':
      if (b == 'E')
        atomNumber = 26;
      else if (b == 'M')
        atomNumber = 100;
      else if (b == 'R')
        atomNumber = 87;
      else
        atomNumber = 9;
      break;
    case 'G':
      if (b == 'A')
        atomNumber = 31;
      else if (b == 'D')
        atomNumber = 64;
      else if (b == 'E')
        atomNumber = 32;
      break;
    case 'H':
      atomNumber = 1;
      break;
    case 'I':
      if (b == 'N')
        atomNumber = 49;
      else if (b == 'R')
        atomNumber = 77;
      else
        atomNumber = 53;
      break;
    case 'K':
      if (b == 'R')
        atomNumber = 36;
      else
        atomNumber = 19;
      break;
    case 'L':
      if (b == 'A')
        atomNumber = 57;
      else if (b == 'I')
        atomNumber = 3;
      else if (b == 'R')
        atomNumber = 103;
      else if (b == 'U')
        atomNumber = 71;
      break;
    case 'M':
      if (b == 'D')
        atomNumber = 101;
      else if (b == 'G')
        atomNumber = 12;
      else if (b == 'N')
        atomNumber = 25;
      else if (b == 'O')
        atomNumber = 42;
      break;
    case 'N':
      if (b == 'I')
        atomNumber = 28;
      else
        atomNumber = 7;
      break;
    case 'O':
      atomNumber = 8;
      break;
    case 'P':
      if (b == 'A')
        atomNumber = 91;
      else if (b == 'B')
        atomNumber = 82;
      else if (b == 'D')
        atomNumber = 46;
      else if (b == 'M')
        atomNumber = 61;
      else if (b == 'O')
        atomNumber = 84;
      else if (b == 'R')
        atomNumber = 59;
      else if (b == 'T')
        atomNumber = 78;
      else if (b == 'U')
        atomNumber = 94;
      else
        atomNumber = 15;
      break;
    case 'R':
      if (b == 'A')
        atomNumber = 88;
      else if (b == 'B')
        atomNumber = 37;
      else if (b == 'E')
        atomNumber = 75;
      else if (b == 'H')
        atomNumber = 45;
      else if (b == 'N')
        atomNumber = 86;
      else if (b == 'U')
        atomNumber = 44;
      break;
    case 'S':
      if (b == 'I')
        atomNumber = 14;
      else if (b == 'R')
        atomNumber = 38;
      else
        atomNumber = 16;
      break;
    case 'T':
      if (b == 'A')
        atomNumber = 73;
      else if (b == 'B')
        atomNumber = 65;
      else if (b == 'C')
        atomNumber = 43;
      else if (b == 'E')
        atomNumber = 52;
      else if (b == 'H')
        atomNumber = 90;
      else if (b == 'I')
        atomNumber = 22;
      else if (b == 'L')
        atomNumber = 81;
      else if (b == 'M')
        atomNumber = 69;
      break;
    case 'U':
      atomNumber = 92;
      break;
    case 'V':
      atomNumber = 23;
      break;
    case 'W':
      atomNumber = 74;
      break;
    case 'X':
      atomNumber = 54;
      break;
    case 'Y':
      if (b == 'B')
        atomNumber = 70;
      else
        atomNumber = 39;
      break;
    case 'Z':
      if (b == 'N')
        atomNumber = 30;
      else
        atomNumber = 40;
      break;
    case ' ':
      atomNumber = 104;
      break;
    default:
      atomNumber = 6;
      break;
  }

  return (atomNumber - 1); // TODO fix atom type decrement (bug)
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
