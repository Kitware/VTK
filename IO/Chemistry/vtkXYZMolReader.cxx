// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkXYZMolReader.h"

#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"
#include "vtkStringScanner.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include <vtksys/SystemTools.hxx>

#include <iostream>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXYZMolReader);

//------------------------------------------------------------------------------
vtkXYZMolReader::vtkXYZMolReader()
{
  this->TimeStep = 0;
  this->MaxTimeStep = 0;
}

//------------------------------------------------------------------------------
vtkXYZMolReader::~vtkXYZMolReader() = default;

//------------------------------------------------------------------------------
char* vtkXYZMolReader::GetNextLine(FILE* fp, char* line, int maxlen)
{
  int cc;
  int len;
  int comment;
  char* ptr;
  do
  {
    comment = 0;
    if (!fgets(line, maxlen, fp))
    {
      // std::cout << "Problem when reading. EOF?" << endl;
      return nullptr;
    }
    len = static_cast<int>(strlen(line));
    for (cc = 0; cc < len; cc++)
    {
      int ch = line[cc];
      if (ch == '#')
      {
        comment = 1;
        break;
      }
      else if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
      {
        break;
      }
    }
    if (cc == len)
    {
      comment = 1;
    }
  } while (comment);
  // std::cout << "Have line that is not a comment: [" << line << "]" << endl;
  len = static_cast<int>(strlen(line));
  int ft = 0;
  ptr = line;
  for (cc = 0; cc < len; cc++)
  {
    int ch = line[cc];
    if (!ft && (ch == ' ' || ch == '\t'))
    {
      ptr++;
    }
    else if (ch == '#' || ch == '\n' || ch == '\r')
    {
      line[cc] = 0;
      break;
    }
    else
    {
      ft = 1;
    }
  }
  if (strlen(ptr) == 0)
  {
    return nullptr;
  }
  return ptr;
}

//------------------------------------------------------------------------------
int vtkXYZMolReader::GetLine1(const char* line, int* cnt)
{
  auto resultCount = vtk::scan_int<int>(std::string_view(line));
  if (!resultCount)
  {
    return 0;
  }
  *cnt = resultCount->value();
  auto resultDummy = vtk::scan_value<std::string_view>(resultCount->range());
  if (resultDummy)
  {
    const auto dummy = resultDummy->value();
    for (const char& ch : dummy)
    {
      if (!std::isspace(ch))
      {
        return 0;
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkXYZMolReader::GetLine2(const char* line, char* name)
{
  auto resultName = vtk::scan_value<std::string_view>(std::string_view(line));
  if (!resultName)
  {
    return 0;
  }
  auto nameStr = resultName->value();
  std::copy_n(nameStr.data(), nameStr.size(), name);
  name[nameStr.size()] = '\0'; // Ensure null-termination
  return 1;
}

//------------------------------------------------------------------------------
int vtkXYZMolReader::GetAtom(const char* line, char* atom, float* x)
{
  auto resultAtom =
    vtk::scan<std::string_view, float, float, float>(std::string_view(line), "{:s} {:f} {:f} {:f}");
  if (!resultAtom)
  {
    return 0;
  }
  std::string_view atomStr;
  std::tie(atomStr, x[0], x[1], x[2]) = resultAtom->values();
  std::copy_n(atomStr.data(), atomStr.size(), atom);
  atom[atomStr.size()] = '\0'; // Ensure null-termination
  auto resultDummy = vtk::scan_value<std::string_view>(resultAtom->range());
  if (resultDummy)
  {
    const auto dummy = resultDummy->value();
    for (const char& ch : dummy)
    {
      if (!std::isspace(ch))
      {
        return 0;
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkXYZMolReader::InsertAtom(const char* atom, float* pos)
{
  this->Points->InsertNextPoint(pos);
  this->AtomType->InsertNextValue(this->MakeAtomType(atom));
  this->AtomTypeStrings->InsertNextValue(atom);
  this->Residue->InsertNextValue(-1);
  this->Chain->InsertNextValue(0);
  this->SecondaryStructures->InsertNextValue(0);
  this->SecondaryStructuresBegin->InsertNextValue(0);
  this->SecondaryStructuresEnd->InsertNextValue(0);
  this->IsHetatm->InsertNextValue(0);
}

//------------------------------------------------------------------------------
int vtkXYZMolReader::CanReadFile(const char* name)
{
  if (!name)
  {
    return 0;
  }

  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(name, &fs) != 0)
  {
    return 0;
  }

  FILE* fp = vtksys::SystemTools::Fopen(name, "r");
  if (!fp)
  {
    return 0;
  }

  int valid = 0;

  constexpr int maxlen = 1024;
  char buffer[maxlen];
  char comment[maxlen];
  char atom[maxlen];
  char* lptr;
  int num = 0;
  float pos[3];

  lptr = this->GetNextLine(fp, buffer, maxlen);
  if (this->GetLine1(lptr, &num))
  {
    // Have header
    lptr = this->GetNextLine(fp, buffer, maxlen);
    if (this->GetLine2(lptr, comment))
    {
      lptr = this->GetNextLine(fp, buffer, maxlen);
      if (this->GetAtom(lptr, atom, pos))
      {
        valid = 3;
      }
    }
    else if (this->GetAtom(lptr, atom, pos))
    {
      valid = 3;
    }
  }
  else
  {
    // No header
    lptr = this->GetNextLine(fp, buffer, maxlen);
    if (this->GetAtom(lptr, atom, pos))
    {
      valid = 3;
    }
  }

  fclose(fp);
  return valid;
}

//------------------------------------------------------------------------------
void vtkXYZMolReader::ReadSpecificMolecule(FILE* fp)
{
  constexpr int maxlen = 1024;
  char buffer[maxlen];
  char comment[maxlen];
  char* lptr;

  int have_header = 0;
  int num = 0;
  int cnt = 0;
  int ccnt = 0;
  int rcnt = 0;
  int timestep = 1;

  int selectstep = this->TimeStep;

  float pos[3];
  char atom[maxlen];

  this->AtomType->Allocate(1024);
  this->Points->Allocate(1024);

  while ((lptr = this->GetNextLine(fp, buffer, maxlen)))
  {
    if ((cnt == 0 || ccnt == num) && this->GetLine1(lptr, &num))
    {
      have_header = 1;
      vtkDebugMacro("Have header. Number of atoms is: " << num);
      ccnt = 0;
      if (cnt > 0)
      {
        timestep++;
      }
    }
    else if (have_header)
    {
      if (this->GetAtom(lptr, atom, pos))
      {
        // std::cout << "Found atom: " << atom << endl;
        if (ccnt >= num)
        {
          vtkErrorMacro("Expecting " << num << " atoms, found: " << ccnt);
          return;
        }
        else
        {
          if (selectstep == timestep - 1)
          {
            // Got atom with full signature
            // std::cout << "Insert atom: " << atom << endl;
            this->InsertAtom(atom, pos);
            rcnt++;
          }
          ccnt++;
        }
      }
      else if (ccnt == 0 && this->GetLine2(lptr, comment))
      {
        vtkDebugMacro("Have comment");
      }
      else
      {
        vtkErrorMacro("Expecting atom, got: " << lptr);
        return;
      }
    }
    else
    {
      if (this->GetAtom(lptr, atom, pos))
      {
        // Got atom with simple signature
        this->InsertAtom(atom, pos);
        rcnt++;
      }
      else
      {
        vtkErrorMacro("Expecting atom, got: " << lptr);
        return;
      }
    }
    ++cnt;
  }

  // Just some more checking and cleanups
  if (num == 0)
  {
    num = rcnt;
  }

  this->AtomType->Squeeze();
  this->Points->Squeeze();

  if (selectstep >= timestep)
  {
    this->NumberOfAtoms = 0;
    vtkErrorMacro("Only have " << timestep << " time step(s)");
    return;
  }

  vtkDebugMacro("Number of atoms: " << num << " (" << rcnt << ")");
  if (num != rcnt)
  {
    this->NumberOfAtoms = 0;
    vtkErrorMacro("Expecting " << num << " atoms, got " << rcnt);
    return;
  }

  this->SetMaxTimeStep(timestep);
  this->NumberOfAtoms = num;

  // We only have one submodel for XYZ files
  this->Model->SetNumberOfValues(this->NumberOfAtoms);
  for (vtkIdType i = 0; i < this->NumberOfAtoms; ++i)
  {
    this->Model->SetValue(i, 1);
  }
}

//------------------------------------------------------------------------------
void vtkXYZMolReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "MaxTimeStep: " << this->MaxTimeStep;
}
VTK_ABI_NAMESPACE_END
