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
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"

#include <ctype.h>


static double vtkMoleculeReaderBaseCovRadius[103] = {
0.32 , 1.6 , 0.68 , 0.352 , 0.832 , 0.72 ,
0.68 , 0.68 , 0.64 , 1.12 , 0.972 , 1.1 , 1.352 , 1.2 , 1.036 ,
1.02 , 1 , 1.568 , 1.328 , 0.992 , 1.44 , 1.472 , 1.328 , 1.352 ,
1.352 , 1.34 , 1.328 , 1.62 , 1.52 , 1.448 , 1.22 , 1.168 , 1.208 ,
1.22 , 1.208 , 1.6 , 1.472 , 1.12 , 1.78 , 1.56 , 1.48 , 1.472 ,
1.352 , 1.4 , 1.448 , 1.5 , 1.592 , 1.688 , 1.632 , 1.46 , 1.46 ,
1.472 , 1.4 , 1.7 , 1.672 , 1.34 , 1.872 , 1.832 , 1.82 , 1.808 ,
1.8 , 1.8 , 1.992 , 1.792 , 1.76 , 1.752 , 1.74 , 1.728 , 1.72 ,
1.94 , 1.72 , 1.568 , 1.432 , 1.368 , 1.352 , 1.368 , 1.32 , 1.5 ,
1.5 , 1.7 , 1.552 , 1.54 , 1.54 , 1.68 , 1.208 , 1.9 , 1.8 ,
1.432 , 1.18 , 1.02 , 0.888 , 0.968 , 0.952 , 0.928 , 0.92 , 0.912 ,
0.9 , 0.888 , 0.88 , 0.872 , 0.86 , 0.848 , 0.84 };

static double vtkMoleculeReaderBaseAtomColors[][3] = {
  {255, 255, 255}, {127, 0, 127}, {255, 0, 255},
  {127, 127, 127}, {127, 0, 127}, {0, 255, 0},
  {0, 0, 255}, {255, 0, 0}, {0, 255, 255},
  {127, 127, 127}, {127, 127, 127}, {178, 153, 102},
  {127, 127, 127}, {51, 127, 229}, {0, 255, 255},
  {255, 255, 0}, {255, 127, 127}, {255, 255, 127},
  {127, 127, 127}, {51, 204, 204}, {127, 127, 127},
  {0, 178, 178}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {204, 0, 255}, {255, 0, 255},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {229, 102, 51}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 255, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {102, 51, 204}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {51, 127, 51}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}, {127, 127, 127},
  {127, 127, 127}, {127, 127, 127}
};

static double vtkMoleculeReaderBaseRadius[] = {
  1.2, 1.22, 1.75,  /* "H " "He" "Li" */
  1.50, 1.90, 1.80, /* "Be" "B " "C " */
  1.70, 1.60, 1.35, /* "N " "O " "F " */
  1.60, 2.31, 1.70, 
  2.05, 2.00, 2.70,
  1.85, 1.81, 1.91,
  2.31, 1.74, 1.80,
  1.60, 1.50, 1.40,  /* Ti-Cu and Ge are guestimates. */
  1.40, 1.40, 1.40,
  1.60, 1.40, 1.40,
  1.90, 1.80, 2.00,
  2.00, 1.95, 1.98,
  2.44, 2.40, 2.10,  /* Sr-Rh and Ba and La are guestimates. */
  2.00, 1.80, 1.80,
  1.80, 1.80, 1.80,
  1.60, 1.70, 1.60,
  1.90, 2.20, 2.20,
  2.20, 2.15, 2.20,
  2.62, 2.30, 2.30,
  2.30, 2.30, 2.30,  /* All of these are guestimates. */
  2.30, 2.30, 2.40,
  2.30, 2.30, 2.30,
  2.30, 2.30, 2.30,
  2.40, 2.50, 2.30,
  2.30, 2.30, 2.30,  /* All but Pt and Bi are guestimates. */
  2.30, 2.30, 2.40,
  2.30, 2.40, 2.50,
  2.50, 2.40, 2.40,
  2.40, 2.40, 2.90,
  2.60, 2.30, 2.30,   /* These are all guestimates. */
  2.30, 2.30, 2.30,
  2.30, 2.30, 2.30,
  2.30, 2.30, 2.30,
  2.30, 2.30, 2.30,
  2.30, 1.50 } ;

vtkMoleculeReaderBase::vtkMoleculeReaderBase()
{
  this->FileName = NULL;
  this->BScale = 1.0;
  this->HBScale = 1.0;
  this->AtomType = NULL;
  this->Points = NULL;
  this->RGB = NULL;
  this->Radii = NULL;
  this->NumberOfAtoms = 0;

  this->SetNumberOfInputPorts(0);
}

vtkMoleculeReaderBase::~vtkMoleculeReaderBase()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if(this->AtomType)
    {
    this->AtomType->Delete();
    }
  if(this->Points)
    {
    this->Points->Delete();
    }
  if(this->RGB)
    {
    this->RGB->Delete();
    }
  if(this->Radii)
    {
    this->Radii->Delete();
    }
}

int vtkMoleculeReaderBase::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  FILE *fp;

  if (!this->FileName) 
    {
    return 0;
    }

  if ((fp = fopen(this->FileName, "r")) == NULL) 
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
    }
  vtkDebugMacro(<< "opening base file " << this->FileName);
  this->ReadMolecule(fp, output);
  fclose(fp);

  output->Squeeze();

  return 1;
}

int vtkMoleculeReaderBase::ReadMolecule(FILE *fp, vtkPolyData *output)
{
  int i;
  vtkCellArray *newBonds;

  vtkDebugMacro(<< "Scanning the Molecule file");

  if ( !this->AtomType )
    {
    this->AtomType = vtkIdTypeArray::New();
    }
  else
    {
    this->AtomType->Reset();
    }

  if ( !this->Points )
    {
    this->Points = vtkPoints::New();
    }
  else
    {
    this->Points->Reset();
    }

  this->ReadSpecificMolecule(fp);

  vtkDebugMacro(<< "End of scanning");
  output->SetPoints(this->Points);

  newBonds = vtkCellArray::New();
  newBonds->Allocate(500);

  this->MakeBonds(this->Points, this->AtomType, newBonds);

  output->SetLines(newBonds);
  newBonds->Delete();

  vtkDebugMacro(<< "read " << this->NumberOfAtoms << " atoms and found " 
                << newBonds->GetNumberOfCells() << " bonds" << endl);

  if ( this->RGB )
    {
    this->RGB->Reset();
    }
  else
    {
    this->RGB = vtkUnsignedCharArray::New();
    }
  this->RGB->SetNumberOfComponents(3);
//  this->RGB->SetNumberOfTuples(this->NumberOfAtoms);
  this->RGB->Allocate(3*this->NumberOfAtoms);
  this->RGB->SetName("rgb_colors");

  for(i=0; i < this->NumberOfAtoms; i++)
    {
    this->RGB->InsertNextTuple(
      &vtkMoleculeReaderBaseAtomColors[AtomType->GetValue(i)][0]);
    }

  output->GetPointData()->SetScalars(this->RGB);

  if ( this->Radii )
    {
    this->Radii->Reset();
    }
  else
    {
    this->Radii = vtkFloatArray::New();
    }
  this->Radii->SetNumberOfComponents(3);
  //  this->Radii->SetNumberOfTuples(this->NumberOfAtoms);
  this->Radii->Allocate(3 * this->NumberOfAtoms);
  this->Radii->SetName("radius");

  // We're obliged here to insert the scalars "radius" 3 times to make it a
  // vector in order to use Glyph3D to color AND scale at the same time.

  for(i=0; i < this->NumberOfAtoms; i++)
    {
    this->Radii->InsertNextTuple3(
      vtkMoleculeReaderBaseRadius[AtomType->GetValue(i)],
      vtkMoleculeReaderBaseRadius[AtomType->GetValue(i)],
      vtkMoleculeReaderBaseRadius[AtomType->GetValue(i)]);
    }

  output->GetPointData()->SetVectors(this->Radii);

  return 0;
}

int vtkMoleculeReaderBase::MakeBonds(vtkPoints *newPts,
                                     vtkIdTypeArray *atype,
                                     vtkCellArray *newBonds)
{
  register int i, j;
  register int nbonds;
  register double dx, dy, dz;
  double max, dist;
  double X[3], Y[3];
  vtkIdType bond[2];

  nbonds = 0;
  for(i = this->NumberOfAtoms - 1; i > 0; i--) 
    {
    bond[0] = i;
    newPts->GetPoint(i, X);
    for(j = i - 1; j >= 0 ; j--) 
      {
      /*
       * The outer loop index 'i' is AFTER the inner loop 'j': 'i'
       * leads 'j' in the list: since hydrogens traditionally follow
       * the heavy atom they're bonded to, this makes it easy to quit
       * bonding to hydrogens after one bond is made by breaking out of
       * the 'j' loop when 'i' is a hydrogen and we make a bond to it.
       * Working backwards like this makes it easy to find the heavy
       * atom that came 'just before' the Hydrogen. mp
       * Base distance criteria on vdw...lb                                 
       */

      /* never bond hydrogens to each other... */
      if (atype->GetValue(i) == 0 && atype->GetValue(j) == 0) 
        {
        continue;
        }

      dist = vtkMoleculeReaderBaseCovRadius[atype->GetValue(i)] +
        vtkMoleculeReaderBaseCovRadius[atype->GetValue(j)] + 0.56;
      max = dist*dist;

      if(atype->GetValue(i) == 0 || atype->GetValue(j) == 0)
        {
        max *= HBScale;
        }
      else
        {
        max *= BScale;
        }

      newPts->GetPoint(j, Y);
      dx = X[0] - Y[0];
      dist = dx * dx;

      if(dist > max ) 
        {
        continue;
        }

      dy = X[1] - Y[1];
      dist += dy * dy;
      if(dist > max ) 
        {
        continue;
        }

      dz = X[2] - Y[2];
      dist += dz * dz;
      if(dist > max ) 
        {
        continue;
        }

      bond[1] = j;
      newBonds->InsertNextCell(2, bond);

      nbonds++;
      }
    }
  newBonds-> Squeeze();
  return nbonds;
}

int vtkMoleculeReaderBase::MakeAtomType(const char *atype)
{
  char      a, b;
  int       anum=0;

  a = atype[0];
  if (islower(a)) 
    {
    a = toupper(a);
    }
  b = atype[1];
  if (islower(b)) 
    {
    b = toupper(b);
    }
  switch (a) 
    {
  case 'A':
    if(b == 'C') anum = 89;
    else if(b == 'G') anum = 47;
    else if(b == 'L') anum = 13;
    else if(b == 'M') anum = 95;
    else if(b == 'R') anum = 18;
    else if(b == 'S') anum = 33;
    else if(b == 'T') anum = 85;
    else if(b == 'U') anum = 79;
    break;
  case 'B':
    if(b == 'A') anum = 56;
    else if(b == 'E') anum = 4;
    else if(b == 'I') anum = 83;
    else if(b == 'K') anum = 97;
    else if(b == 'R') anum = 35;
    else anum = 5;
    break;
  case 'C':
    if(b == 'L') anum = 17;
    else if(b == 'O') anum = 27;
    else if(b == 'R') anum = 24;
    else if(b == 'S') anum = 55;
    else if(b == 'U') anum = 29;
    else if(b == '0') anum = 6;
    else anum = 6;
    break;
  case 'D':
    anum = 66;
    break;
  case 'E':
    if(b == 'R') anum = 68;
    else if(b == 'S') anum = 99;
    else if(b == 'U') anum = 63;
    break;
  case 'F':
    if(b == 'E') anum = 26;
    else if(b == 'M') anum = 100;
    else if(b == 'R') anum = 87;
    else anum = 9;
    break;
  case 'G':
    if(b == 'A') anum = 31;
    else if(b == 'D') anum = 64;
    else if(b == 'E') anum = 32;
    break;
  case 'H':
    anum = 1;
    break;
  case 'I':
    if(b == 'N') anum = 49;
    else if(b == 'R') anum = 77;
    else anum = 53;
    break;
  case 'K':
    if(b == 'R') anum = 36;
    else anum = 19;
    break;
  case 'L':
    if(b == 'A') anum = 57;
    else if(b == 'I') anum = 3;
    else if(b == 'R') anum = 103;
    else if(b == 'U') anum = 71;
    break;
  case 'M':
    if(b == 'D') anum = 101;
    else if(b == 'G') anum = 12;
    else if(b == 'N') anum = 25;
    else if(b == 'O') anum = 42;
    break;
  case 'N':
    if(b == 'I') anum = 28;
    else anum = 7;
    break;
  case 'O':
    anum = 8;
    break;
  case 'P':
    if(b == 'A') anum = 91;
    else if(b == 'B') anum = 82;
    else if(b == 'D') anum = 46;
    else if(b == 'M') anum = 61;
    else if(b == 'O') anum = 84;
    else if(b == 'R') anum = 59;
    else if(b == 'T') anum = 78;
    else if(b == 'U') anum = 94;
    else anum = 15;
    break;
  case 'R':
    if(b == 'A') anum = 88;
    else if(b == 'B') anum = 37;
    else if(b == 'E') anum = 75;
    else if(b == 'H') anum = 45;
    else if(b == 'N') anum = 86;
    else if(b == 'U') anum = 44;
    break;
  case 'S':
    if(b == 'I') anum = 14;
    else if(b == 'R') anum = 38;
    else anum = 16;
    break;
  case 'T':
    if(b == 'A') anum = 73;
    else if(b == 'B') anum = 65;
    else if(b == 'C') anum = 43;
    else if(b == 'E') anum = 52;
    else if(b == 'H') anum = 90;
    else if(b == 'I') anum = 22;
    else if(b == 'L') anum = 81;
    else if(b == 'M') anum = 69;
    break;
  case 'U':
    anum = 92;
    break;
  case 'V':
    anum = 23;
    break;
  case 'W':
    anum = 74;
    break;
  case 'X':
    anum = 54;
    break;
  case 'Y':
    if(b == 'B') anum = 70;
    else anum = 39;
    break;
  case 'Z':
    if(b == 'N') anum = 30;
    else anum = 40;
    break;
  case ' ':
    anum = 104;
    break;
  default:
    anum = 6;
    break;
    }
  return (anum-1);
}

void vtkMoleculeReaderBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " 
    << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "NumberOfAtoms: " << this->NumberOfAtoms  << endl;
  os << indent << "HBScale: " << this->HBScale << endl;
  os << indent << "BScale: " << this->BScale << endl;
}
