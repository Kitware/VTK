/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLFileReadTester.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLFileReadTester.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkXMLFileReadTester);

//----------------------------------------------------------------------------
vtkXMLFileReadTester::vtkXMLFileReadTester()
{
  this->FileName = 0;
  this->FileDataType = 0;
  this->FileVersion = 0;
}

//----------------------------------------------------------------------------
vtkXMLFileReadTester::~vtkXMLFileReadTester()
{
  this->SetFileName(0);
  this->SetFileDataType(0);
  this->SetFileVersion(0);
}

//----------------------------------------------------------------------------
void vtkXMLFileReadTester::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName? this->FileName:"") << "\n";
  os << indent << "FileDataType: "
     << (this->FileDataType? this->FileDataType:"") << "\n";
  os << indent << "FileVersion: "
     << (this->FileVersion? this->FileVersion:"") << "\n";
}

//----------------------------------------------------------------------------
int vtkXMLFileReadTester::TestReadFile()
{
  if (!this->FileName)
  {
    return 0;
  }

  ifstream inFile(this->FileName);
  if (!inFile)
  {
    return 0;
  }

  this->SetStream(&inFile);
  this->Done = 0;

  this->Parse();

  return this->Done ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkXMLFileReadTester::StartElement(const char* name, const char** atts)
{
  this->Done = 1;
  if (strcmp(name, "VTKFile") == 0)
  {
    for(unsigned int i = 0; atts[i] && atts[i+1]; i += 2)
    {
      if (strcmp(atts[i], "type") == 0)
      {
        this->SetFileDataType(atts[i+1]);
      }
      else if (strcmp(atts[i], "version") == 0)
      {
        this->SetFileVersion(atts[i+1]);
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkXMLFileReadTester::ParsingComplete()
{
  return this->Done ? 1 : 0;
}
