/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDBReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDBReader.h"

#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkIntArray.h"
#include <algorithm>

inline void StdStringToUpper(std::string& s)
{
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}

vtkStandardNewMacro(vtkPDBReader);

vtkPDBReader::vtkPDBReader()
{
}

vtkPDBReader::~vtkPDBReader()
{
}

void vtkPDBReader::ReadSpecificMolecule(FILE* fp)
{
  char linebuf[82], dum1[8], dum2[8];
  char chain, startChain, endChain;
  int startResi, endResi;
  int resi;
  int i, j;
  float x[3];

  this->NumberOfAtoms = 0;
  this->Points->Allocate(500);
  this->AtomType->Allocate(500);
  this->AtomTypeStrings->Allocate(500);

  vtkIntArray* Sheets = vtkIntArray::New();
  Sheets->SetNumberOfComponents(4);
  Sheets->Allocate(500);

  vtkIntArray* Helix = vtkIntArray::New();
  Helix->SetNumberOfComponents(4);
  Helix->Allocate(50);

  vtkDebugMacro( << "PDB File (" << this->HBScale
    << ", " << this->BScale << ")");
  while(fgets(linebuf, sizeof linebuf, fp) != NULL &&
    strncmp("END", linebuf, 3))
    {
    char elem[3] = { 0 };
    char c[7] = { 0 };
    sscanf(&linebuf[0],"%6s", c);
    std::string command = c;
    StdStringToUpper(command);

    if (command == "ATOM" || command == "HETATM")
      {
      sscanf(&linebuf[12], "%4s", dum1);
      sscanf(&linebuf[17], "%3s", dum2);
      chain = linebuf[21];
      sscanf(&linebuf[22], "%d", &resi);
      sscanf(&linebuf[30],"%8f%8f%8f", x, x+1, x+2);
      sscanf(&linebuf[76], "%2s", elem);

      if (elem[0] == '\0')
        {
        strncpy(elem, dum1, 2);
        }

      if (!((elem[0]=='H' || elem[0]=='h') && elem[1]=='\0'))
        { /* skip hydrogen */
        this->Points->InsertNextPoint(x);
        this->Residue->InsertNextValue(resi);
        this->Chain->InsertNextValue(chain);
        this->AtomType->InsertNextValue(this->MakeAtomType(elem));
        this->AtomTypeStrings->InsertNextValue(dum1);
        this->IsHetatm->InsertNextValue(command[0] == 'H');
        this->NumberOfAtoms++;
        }
      }
    else if (command == "SHEET")
      {
      sscanf(&linebuf[21], "%c", &startChain);
      sscanf(&linebuf[22], "%d", &startResi);
      sscanf(&linebuf[32], "%c", &endChain);
      sscanf(&linebuf[33], "%d", &endResi);
      int tuple[4] = { startChain, startResi, endChain, endResi };
      Sheets->InsertNextTupleValue(tuple);
      }
    else if (command == "HELIX")
      {
      sscanf(&linebuf[19], "%c", &startChain);
      sscanf(&linebuf[21], "%d", &startResi);
      sscanf(&linebuf[31], "%c", &endChain);
      sscanf(&linebuf[33], "%d", &endResi);
      int tuple[4] = { startChain, startResi, endChain, endResi };
      Helix->InsertNextTupleValue(tuple);
      }
    }

  this->Points->Squeeze();
  this->AtomType->Squeeze();
  this->AtomTypeStrings->Squeeze();
  this->Residue->Squeeze();
  this->IsHetatm->Squeeze();

  int len = this->Points->GetNumberOfPoints();
  this->SecondaryStructures->SetNumberOfValues(len);
  this->SecondaryStructuresBegin->SetNumberOfValues(len);
  this->SecondaryStructuresEnd->SetNumberOfValues(len);

  // Assign secondary structures
  for (i = 0; i < this->Points->GetNumberOfPoints(); i++)
    {
    this->SecondaryStructures->SetValue(i, 'c');
    resi = this->Residue->GetValue(i);

    for (j = 0; j < Sheets->GetNumberOfTuples(); j++)
      {
      int sheet[4];
      Sheets->GetTupleValue(j, sheet);
      if (this->Chain->GetValue(i) != sheet[0]) continue;
      if (resi < sheet[1]) continue;
      if (resi > sheet[3]) continue;
      this->SecondaryStructures->SetValue(i, 's');
      if (resi == sheet[1]) this->SecondaryStructuresBegin->SetValue(i, true);
      if (resi == sheet[3]) this->SecondaryStructuresEnd->SetValue(i, true);
      }

    for (j = 0; j < Helix->GetNumberOfTuples(); j++)
      {
      int helix[4];
      Helix->GetTupleValue(j, helix);
      if (this->Chain->GetValue(i) != helix[0]) continue;
      if (resi < helix[1]) continue;
      if (resi > helix[3]) continue;
      this->SecondaryStructures->SetValue(i, 'h');
      if (resi == helix[1]) this->SecondaryStructuresBegin->SetValue(i, true);
      else if (resi == helix[3]) this->SecondaryStructuresEnd->SetValue(i, true);
      }
   }
  Sheets->Delete();
  Helix->Delete();
}

void vtkPDBReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
