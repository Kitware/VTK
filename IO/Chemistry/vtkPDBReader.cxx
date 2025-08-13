// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPDBReader.h"

#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStringScanner.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
inline void StdStringToUpper(std::string& s)
{
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}

vtkStandardNewMacro(vtkPDBReader);

vtkPDBReader::vtkPDBReader() = default;

vtkPDBReader::~vtkPDBReader() = default;

void vtkPDBReader::ReadSpecificMolecule(FILE* fp)
{
  this->NumberOfAtoms = 0;
  this->Points->Allocate(500);
  this->AtomType->Allocate(500);
  this->AtomTypeStrings->Allocate(500);
  this->Model->Allocate(500);

  vtkIntArray* Sheets = vtkIntArray::New();
  Sheets->SetNumberOfComponents(4);
  Sheets->Allocate(500);

  vtkIntArray* Helix = vtkIntArray::New();
  Helix->SetNumberOfComponents(4);
  Helix->Allocate(50);

  vtkDebugMacro(<< "PDB File (" << this->HBScale << ", " << this->BScale << ")");

  // Loop variables
  char linebuf[82];
  std::array<float, 3> x;

  unsigned int currentModelNumber = 1;
  bool modelCommandFound = false;

  // Read PDB file until we encounter a command starting with "END" which is not "ENDMDL"
  while (fgets(linebuf, sizeof linebuf, fp) != nullptr &&
    !(strncmp("END", linebuf, 3) == 0 && strncmp("ENDMDL", linebuf, 6) != 0))
  {
    const std::string_view lineBuffer(linebuf);
    char elem[3] = { 0 };
    auto command = vtk::scan<std::string>(lineBuffer, "{:s}")->value();
    StdStringToUpper(command);
    if (command == "ATOM" || command == "HETATM")
    {
      auto atomName = vtk::scan<std::string_view>(lineBuffer.substr(12), "{:s}")->value();
      // auto dum2 = vtk::scan<std::string_view>(lineBuffer.substr(17), "{:3s}")->value();
      const char chain = lineBuffer[21];
      const int resi = vtk::scan_int<int>(lineBuffer.substr(22))->value();
      std::tie(x[0], x[1], x[2]) =
        vtk::scan<float, float, float>(lineBuffer.substr(30), "{:8f}{:8f}{:8f}")->values();
      if (lineBuffer.size() >= 78)
      {
        if (auto result = vtk::scan<std::string_view>(lineBuffer.substr(76), "{:2s}"))
        {
          auto elemSymbol = result->value();
          elem[0] = elemSymbol[0];
          elem[1] = elemSymbol[1];
          elem[2] = '\0';
        }
      }
      if (elem[0] == '\0')
      {
        // If element symbol was not specified, just use the "Atom name".
        elem[0] = atomName[0];
        elem[1] = atomName[1];
        elem[2] = '\0';
      }

      // Only insert non-hydrogen atoms
      if (!((elem[0] == 'H' || elem[0] == 'h') && elem[1] == '\0'))
      {
        this->Points->InsertNextPoint(x.data());
        this->Residue->InsertNextValue(resi);
        this->Chain->InsertNextValue(chain);
        this->AtomType->InsertNextValue(this->MakeAtomType(elem));
        this->AtomTypeStrings->InsertNextValue(vtkStdString(atomName.data(), atomName.size()));
        this->IsHetatm->InsertNextValue(command[0] == 'H');
        this->Model->InsertNextValue(currentModelNumber);
        this->NumberOfAtoms++;
      }
    }
    else if (command == "SHEET")
    {
      const auto startChain = lineBuffer[21];
      const auto startResi = vtk::scan_int<int>(lineBuffer.substr(22))->value();
      const auto endChain = lineBuffer[32];
      const auto endResi = vtk::scan_int<int>(lineBuffer.substr(33))->value();
      const int tuple[4] = { startChain, startResi, endChain, endResi };
      Sheets->InsertNextTypedTuple(tuple);
    }
    else if (command == "HELIX")
    {
      const auto startChain = lineBuffer[19];
      const auto startResi = vtk::scan_int<int>(lineBuffer.substr(21))->value();
      const auto endChain = lineBuffer[31];
      const auto endResi = vtk::scan_int<int>(lineBuffer.substr(33))->value();
      const int tuple[4] = { startChain, startResi, endChain, endResi };
      Helix->InsertNextTypedTuple(tuple);
    }
    else if (command == "MODEL")
    {
      // Only increment current model number if we have found at least two models
      if (modelCommandFound)
      {
        ++currentModelNumber;
      }
      else
      {
        modelCommandFound = true;
      }
    }
  }

  this->Points->Squeeze();
  this->AtomType->Squeeze();
  this->AtomTypeStrings->Squeeze();
  this->Residue->Squeeze();
  this->IsHetatm->Squeeze();
  this->Model->Squeeze();

  this->NumberOfModels = currentModelNumber;

  int len = this->Points->GetNumberOfPoints();
  this->SecondaryStructures->SetNumberOfValues(len);
  this->SecondaryStructuresBegin->SetNumberOfValues(len);
  this->SecondaryStructuresEnd->SetNumberOfValues(len);

  // Assign secondary structures
  for (vtkIdType i = 0; i < this->Points->GetNumberOfPoints(); i++)
  {
    this->SecondaryStructures->SetValue(i, 'c');
    const vtkIdType resi = this->Residue->GetValue(i);

    for (vtkIdType j = 0; j < Sheets->GetNumberOfTuples(); j++)
    {
      int sheet[4];
      Sheets->GetTypedTuple(j, sheet);
      if (this->Chain->GetValue(i) != sheet[0])
        continue;
      if (resi < sheet[1])
        continue;
      if (resi > sheet[3])
        continue;
      this->SecondaryStructures->SetValue(i, 's');
      if (resi == sheet[1])
        this->SecondaryStructuresBegin->SetValue(i, true);
      if (resi == sheet[3])
        this->SecondaryStructuresEnd->SetValue(i, true);
    }

    for (vtkIdType j = 0; j < Helix->GetNumberOfTuples(); j++)
    {
      int helix[4];
      Helix->GetTypedTuple(j, helix);
      if (this->Chain->GetValue(i) != helix[0])
        continue;
      if (resi < helix[1])
        continue;
      if (resi > helix[3])
        continue;
      this->SecondaryStructures->SetValue(i, 'h');
      if (resi == helix[1])
        this->SecondaryStructuresBegin->SetValue(i, true);
      else if (resi == helix[3])
        this->SecondaryStructuresEnd->SetValue(i, true);
    }
  }
  Sheets->Delete();
  Helix->Delete();
}

void vtkPDBReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
