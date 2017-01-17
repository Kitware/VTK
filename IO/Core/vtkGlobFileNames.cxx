/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlobFileNames.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGlobFileNames.h"
#include "vtkStringArray.h"

#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"

#include <vtksys/Glob.hxx>
#include <vtksys/SystemTools.hxx>
#include <string>
#include <vector>
#include <algorithm>


//----------------------------------------------------------------------------
vtkGlobFileNames* vtkGlobFileNames::New()
{
  VTK_STANDARD_NEW_BODY(vtkGlobFileNames)
}

//----------------------------------------------------------------------------
vtkGlobFileNames::vtkGlobFileNames()
{
  this->Directory = 0;
  this->Pattern = 0;
  this->Recurse = 0;
  this->FileNames = vtkStringArray::New();
}

//----------------------------------------------------------------------------
vtkGlobFileNames::~vtkGlobFileNames()
{
  delete [] this->Directory;
  delete [] this->Pattern;
  this->FileNames->Delete();
  this->FileNames = 0;
}

//----------------------------------------------------------------------------
void vtkGlobFileNames::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Directory: " <<
    (this->GetDirectory() ? this->GetDirectory() : " none") << "\n";
  os << indent << "Pattern: " <<
    (this->GetPattern() ? this->GetPattern() : " none") << "\n";
  os << indent << "Recurse: " << (this->GetRecurse() ? "On\n" : "Off\n");
  os << indent << "FileNames:  (" << this->GetFileNames() << ")\n";
  indent = indent.GetNextIndent();
  for(int i = 0; i < this->FileNames->GetNumberOfValues(); i++)
  {
    os << indent << this->FileNames->GetValue(i) << "\n";
  }
}

//----------------------------------------------------------------------------
void vtkGlobFileNames::Reset()
{
  this->FileNames->Reset();
}

//----------------------------------------------------------------------------
int vtkGlobFileNames::AddFileNames(const char* pattern)
{
  this->SetPattern(pattern);

  vtksys::Glob glob;

  if (this->Recurse)
  {
    glob.RecurseOn();
  }
  else
  {
    glob.RecurseOff();
  }

  if (!this->Pattern)
  {
    vtkErrorMacro(<< "FindFileNames: pattern string is null.");

    return 0;
  }

  std::string fullPattern = this->Pattern;

  if (this->Directory && this->Directory[0] != '\0')
  {
    std::vector<std::string> components;
    vtksys::SystemTools::SplitPath(fullPattern.c_str(), components);
    // If Pattern is a relative path, prepend with Directory
    if (components[0] == "")
    {
      components.insert(components.begin(), this->Directory);
      fullPattern = vtksys::SystemTools::JoinPath(components);
    }
  }

  if (!glob.FindFiles(fullPattern))
  {
    vtkErrorMacro(<< "FindFileNames: Glob action failed for \"" <<
                  fullPattern << "\"");

    return 0;
  }

  // copy the filenames from glob
  std::vector<std::string> files = glob.GetFiles();

  // sort them lexicographically
  std::sort(files.begin(), files.end());

  // add them onto the list of filenames
  for ( std::vector<std::string>::const_iterator iter = files.begin();
        iter != files.end();
        ++iter)
  {
    this->FileNames->InsertNextValue(iter->c_str());
  }

  return 1;
}


//----------------------------------------------------------------------------
const char* vtkGlobFileNames::GetNthFileName(int index)
{
  if(index >= this->FileNames->GetNumberOfValues() || index < 0)
  {
    vtkErrorMacro( << "Bad index for GetFileName on vtkGlobFileNames\n");
    return 0;
  }

  return this->FileNames->GetValue(index).c_str();
}


//----------------------------------------------------------------------------
int vtkGlobFileNames::GetNumberOfFileNames()
{
  return this->FileNames->GetNumberOfValues();
}
