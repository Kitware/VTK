// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLFileReadTester.h"
#include "vtkObjectFactory.h"
#include "vtksys/FStream.hxx"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLFileReadTester);

//------------------------------------------------------------------------------
vtkXMLFileReadTester::vtkXMLFileReadTester()
{
  this->FileDataType = nullptr;
  this->FileVersion = nullptr;
}

//------------------------------------------------------------------------------
vtkXMLFileReadTester::~vtkXMLFileReadTester()
{
  this->SetFileDataType(nullptr);
  this->SetFileVersion(nullptr);
}

//------------------------------------------------------------------------------
void vtkXMLFileReadTester::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileDataType: " << (this->FileDataType ? this->FileDataType : "") << "\n";
  os << indent << "FileVersion: " << (this->FileVersion ? this->FileVersion : "") << "\n";
}

//------------------------------------------------------------------------------
int vtkXMLFileReadTester::TestReadFile()
{
  if (!this->FileName)
  {
    return 0;
  }

  vtksys::ifstream inFile(this->FileName);
  if (!inFile)
  {
    return 0;
  }

  this->SetStream(&inFile);
  this->Done = 0;

  this->Parse();

  return this->Done ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkXMLFileReadTester::StartElement(const char* name, const char** atts)
{
  this->Done = 1;
  if (strcmp(name, "VTKFile") == 0)
  {
    for (unsigned int i = 0; atts[i] && atts[i + 1]; i += 2)
    {
      if (strcmp(atts[i], "type") == 0)
      {
        this->SetFileDataType(atts[i + 1]);
      }
      else if (strcmp(atts[i], "version") == 0)
      {
        this->SetFileVersion(atts[i + 1]);
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkXMLFileReadTester::ParsingComplete()
{
  return this->Done ? 1 : 0;
}
VTK_ABI_NAMESPACE_END
