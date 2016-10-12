/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCMLMoleculeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkCMLMoleculeReader.h"

#include "vtkDataObject.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkXMLParser.h"

#include <string>
#include <vector>

// Subclass of vtkXMLParser -- definitions at end of file
class vtkCMLParser : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkCMLParser, vtkXMLParser);
  static vtkCMLParser * New();

  vtkSetObjectMacro(Target, vtkMolecule);
  vtkGetObjectMacro(Target, vtkMolecule);

protected:
  vtkCMLParser();
  ~vtkCMLParser() VTK_OVERRIDE;
  void StartElement(const char *name, const char **attr) VTK_OVERRIDE;
  void EndElement(const char *name) VTK_OVERRIDE;

  std::vector<std::string> AtomNames;

  vtkMolecule *Target;

  void NewMolecule(const char **attr);
  void NewAtom(const char **attr);
  void NewBond(const char **attr);

  vtkNew<vtkPeriodicTable> pTab;

private:
  vtkCMLParser(const vtkCMLParser&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCMLParser&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkCMLMoleculeReader);

//----------------------------------------------------------------------------
vtkCMLMoleculeReader::vtkCMLMoleculeReader()
  : FileName(NULL)
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkCMLMoleculeReader::~vtkCMLMoleculeReader()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
vtkMolecule *vtkCMLMoleculeReader::GetOutput()
{
  return vtkMolecule::SafeDownCast(this->GetOutputDataObject(0));;
}

//----------------------------------------------------------------------------
void vtkCMLMoleculeReader::SetOutput(vtkMolecule *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

int vtkCMLMoleculeReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{

  vtkMolecule *output = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(outputVector));

  if (!output)
  {
    vtkErrorMacro(<<"vtkCMLMoleculeReader does not have a vtkMolecule "
                  "as output.");
    return 1;
  }

  vtkCMLParser *parser = vtkCMLParser::New();
  parser->SetDebug(this->GetDebug());
  parser->SetFileName(this->FileName);
  parser->SetTarget(output);

  if (!parser->Parse())
  {
    vtkWarningMacro(<<"Cannot parse file " << this->FileName << " as CML.");
    parser->Delete();
    return 1;
  }

  parser->Delete();

  return 1;
}

int vtkCMLMoleculeReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMolecule");
  return 1;
}

void vtkCMLMoleculeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//
// vtkCMLParser Methods
//

vtkStandardNewMacro(vtkCMLParser);

vtkCMLParser::vtkCMLParser()
  : vtkXMLParser(),
    Target(0)
{
}

vtkCMLParser::~vtkCMLParser()
{
  this->SetTarget(NULL);
}

void vtkCMLParser::StartElement(const char *name, const char **attr)
{
  if (strcmp(name, "atom") == 0)
  {
    this->NewAtom(attr);
  }
  else if (strcmp(name, "bond") == 0)
  {
    this->NewBond(attr);
  }
  else if (strcmp(name, "molecule") == 0)
  {
    this->NewMolecule(attr);
  }
  else if (this->GetDebug())
  {
    std::string desc;
    desc += "Unhandled CML Element. Name: ";
    desc += name;
    desc += "\n\tAttributes:";
    int attrIndex = 0;
    while (const char * cur = attr[attrIndex])
    {
      if (attrIndex > 0)
      {
        desc.push_back(' ');
      }
      desc += cur;
      ++attrIndex;
    }
    vtkDebugMacro(<<desc);
  }

  return;
}

void vtkCMLParser::EndElement(const char *)
{
}

void vtkCMLParser::NewMolecule(const char **)
{
  this->Target->Initialize();
}

void vtkCMLParser::NewAtom(const char **attr)
{
  vtkAtom atom = this->Target->AppendAtom();
  int attrInd = 0;
  unsigned short atomicNum = 0;
  float pos[3];
  const char * id = NULL;
  while (const char * cur = attr[attrInd])
  {
    // Get atomic number
    if (strcmp(cur, "elementType") == 0)
    {
      const char *symbol = attr[++attrInd];
      atomicNum = pTab->GetAtomicNumber(symbol);
    }

    // Get position
    else if (strcmp(cur, "x3") == 0)
      pos[0] = atof(attr[++attrInd]);
    else if (strcmp(cur, "y3") == 0)
      pos[1] = atof(attr[++attrInd]);
    else if (strcmp(cur, "z3") == 0)
      pos[2] = atof(attr[++attrInd]);

    // string id / names
    else if (strcmp(cur, "id") == 0)
      id = attr[++attrInd];

    else
    {
      vtkDebugMacro(<< "Unhandled atom attribute: " << cur);
    }

    ++attrInd;
  }

  atom.SetAtomicNumber(atomicNum);
  atom.SetPosition(pos);

  // Store name for lookups
  size_t atomId = static_cast<size_t>(atom.GetId());
  if (atomId >= this->AtomNames.size())
  {
    this->AtomNames.resize(atomId + 1);
  }

  this->AtomNames[atomId] = std::string(id);

  vtkDebugMacro(<< "Added atom #" << atomId << " ('" << id << "') ");

}

void vtkCMLParser::NewBond(const char **attr)
{
  int attrInd = 0;
  vtkIdType atomId1 = -1;
  vtkIdType atomId2 = -1;
  unsigned short order = 0;

  while (const char * cur = attr[attrInd])
  {
    // Get names of bonded atoms
    if (strcmp(cur, "atomRefs2") == 0)
    {
      char atomRefs[128];
      strncpy(atomRefs, attr[++attrInd], 128);
      // Parse out atom names:
      const char *nameChar = strtok(atomRefs, " ");
      while (nameChar != NULL)
      {
        vtkIdType currentAtomId;
        bool found = false;
        for (currentAtomId = 0;
             currentAtomId < static_cast<vtkIdType>(this->AtomNames.size());
             ++currentAtomId)
        {
          if (this->AtomNames[currentAtomId].compare(nameChar) == 0)
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          // Create list of known atom names:
          std::string allAtomNames ("");
          for (size_t i = 0; i < this->AtomNames.size(); ++i)
          {
            allAtomNames += this->AtomNames[i];
            allAtomNames.push_back(' ');
          }
          vtkWarningMacro(<< "NewBond(): unknown atom name '"
                          << nameChar << "'. Known atoms:\n"
                          << allAtomNames.c_str());

          nameChar = strtok(NULL, " ");
          continue;
        }
        else if (atomId1 == -1)
        {
          atomId1 = currentAtomId;
        }
        else if (atomId2 == -1)
        {
          atomId2 = currentAtomId;
        }
        else
        {
          vtkWarningMacro(<< "NewBond(): atomRef2 string has >2 atom names: "
                          << atomRefs);
        }

        nameChar = strtok(NULL, " ");
      }
    }

    // Get bond order
    else if (strcmp(cur, "order") == 0)
    {
      order = static_cast<unsigned short>(atoi(attr[++attrInd]));
    }

    else
    {
      vtkDebugMacro(<< "Unhandled bond attribute: " << cur);
    }

    ++attrInd;
  }

  if (atomId1 < 0 || atomId2 < 0)
  {
    vtkWarningMacro(<< "NewBond(): Invalid atom ids: " << atomId1
                    << " " << atomId2);
    return;
  }

  vtkDebugMacro(<< "Adding bond between atomids " << atomId1 << " "
                << atomId2);

  this->Target->AppendBond(atomId1, atomId2, order);
}
