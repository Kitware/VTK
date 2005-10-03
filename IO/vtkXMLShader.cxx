/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLShader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLShader.h"

#include "vtkObjectFactory.h"
#include "vtkShaderCodeLibrary.h"
#include "vtkToolkits.h" // for VTK_SHADERS_DIRS.
#include "vtkXMLDataElement.h"

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkXMLShader);
vtkCxxRevisionMacro(vtkXMLShader, "1.1.2.6");
vtkCxxSetObjectMacro(vtkXMLShader, SourceLibraryElement, vtkXMLDataElement);
//-----------------------------------------------------------------------------
vtkXMLShader::vtkXMLShader()
{
  this->RootElement = 0;
  this->SourceLibraryElement = 0;
  this->Code = 0;
  this->Args = 0;
}

//-----------------------------------------------------------------------------
vtkXMLShader::~vtkXMLShader()
{
  if (this->RootElement)
    {
    this->RootElement->UnRegister(this);
    this->RootElement = 0;
    }
  this->SetSourceLibraryElement(0);
  this->SetCode(0);
  this->CleanupArgs();
}

//-----------------------------------------------------------------------------
void vtkXMLShader::SetRootElement(vtkXMLDataElement* root)
{
  vtkSetObjectBodyMacro(RootElement, vtkXMLDataElement, root);
  this->SetCode(0);
  this->SetSourceLibraryElement(0); // release the SourceLibrary element.
  // Determine if this shader description uses a library.
  if (this->RootElement)
    {
    switch (this->GetLocation())
      {
    case vtkXMLShader::LOCATION_LIBRARY:
        {
        const char* name = this->RootElement->GetAttribute("name");
        this->Code = vtkShaderCodeLibrary::GetShaderCode(name);
        // TODO: the library should be XML enclosed.
        // For now, it's not.
        if (!this->Code)
          {
          vtkErrorMacro("Failed to locate library " << name);
          return;
          }
        }
      break;
    case vtkXMLShader::LOCATION_FILE:
        {
        const char* filename = this->RootElement->GetAttribute("location");
        const char* fullpath = this->LocateFile(filename);
        if (!fullpath)
          {
          vtkErrorMacro("Failed to locate file " << filename);
          return;
          }
        this->ReadCodeFromFile(fullpath);
        delete [] fullpath;
        }
      break;
      }
    }
}

//-----------------------------------------------------------------------------
// Note that this method allocates a new string which must be deleted by 
// the caller.
char* vtkXMLShader::LocateFile(const char* filename)
{
  // if filename is absolute path, return the same.
  if (vtksys::SystemTools::FileExists(filename))
    {
    return vtksys::SystemTools::DuplicateString(filename);
    }

#ifdef VTK_SHADERS_DIRS
  // search thru default paths to locate file.
  vtkstd::vector<vtkstd::string> paths;
  vtksys::SystemTools::Split(VTK_SHADERS_DIRS, paths, ';');
  for (unsigned int i =0; i < paths.size(); i++)
    {
    vtkstd::string path = paths[i];
    if (path.size() == 0)
      {
      continue;
      }
    vtksys::SystemTools::ConvertToUnixSlashes(path);
    if (path[path.size()-1] != '/')
      {
      path += "/";
      }
    path += filename;
    if (vtksys::SystemTools::FileExists(path.c_str()))
      {
      return vtksys::SystemTools::DuplicateString(path.c_str()); 
      }
    }
#endif
  return 0;
}

//-----------------------------------------------------------------------------
void vtkXMLShader::ReadCodeFromFile(const char* filepath)
{
  ifstream ifp;
  ifp.open(filepath);
  if (!ifp)
    {
    vtkErrorMacro("Failed to open file " << filepath);
    return;
    }
  if (this->Code)
    {
    delete [] this->Code;
    this->Code = 0;
    }

  // determine the length of the file.
  long length;
  ifp.seekg(0, ios::end);
  length = ifp.tellg();
  ifp.seekg(0, ios::beg);

  this->Code = new char[length+10];
  ifp.read(this->Code, length);
  ifp.close();
}

//-----------------------------------------------------------------------------
int vtkXMLShader::GetLanguage()
{
  if (this->RootElement)
    {
    const char* language = this->RootElement->GetAttribute("language");
    if (!language)
      {
      vtkErrorMacro("Shader description missing Language attribute.");
      }
    else if (strcmp(language, "Cg") == 0)
      {
      return vtkXMLShader::LANGUAGE_CG;
      }
    else if (strcmp(language, "GLSL") == 0)
      {
      return vtkXMLShader::LANGUAGE_GLSL;
      }
    }
  return vtkXMLShader::LANGUAGE_NONE;
}

//-----------------------------------------------------------------------------
int vtkXMLShader::GetScope()
{
  if (this->RootElement)
    {
    const char* scope = this->RootElement->GetAttribute("type");
    if (!scope)
      {
      vtkErrorMacro("Shader description missing Type attribute.");
      }
    else if (strcmp(scope, "Vertex") == 0)
      {
      return vtkXMLShader::SCOPE_VERTEX;
      }
    else if (strcmp(scope, "Fragment") == 0)
      {
      return vtkXMLShader::SCOPE_FRAGMENT;
      }
    }
  return vtkXMLShader::SCOPE_NONE;
}

//-----------------------------------------------------------------------------
int vtkXMLShader::GetLocation()
{
  if (this->RootElement)
    {
    const char* loc= this->RootElement->GetAttribute("location");
    if (!loc)
      {
      vtkErrorMacro("Shader description missing 'location' attribute.");
      }
    else if (strcmp(loc, "Inline") == 0)
      {
      return vtkXMLShader::LOCATION_INLINE;
      }
    else if (strcmp(loc, "Library") == 0)
      {
      return vtkXMLShader::LOCATION_LIBRARY;
      }
    else
      {
      // assume its a filename.
      return vtkXMLShader::LOCATION_FILE;
      }
    }
  return vtkXMLShader::LOCATION_NONE;
}

//-----------------------------------------------------------------------------
const char* vtkXMLShader::GetName()
{
  return (this->RootElement)? this->RootElement->GetAttribute("name") : 0;
}

//-----------------------------------------------------------------------------
const char* vtkXMLShader::GetEntry()
{
  return (this->RootElement)? this->RootElement->GetAttribute("entry") : 0;
}

//-----------------------------------------------------------------------------
const char** vtkXMLShader::GetArgs()
{
  this->CleanupArgs();
  if (!this->RootElement || !this->RootElement->GetAttribute("args"))
    {
    return 0;
    }
  
  vtkstd::vector<vtkstd::string> args;
  vtksys::SystemTools::Split(this->RootElement->GetAttribute("args"), args, ' ');
  
  int i;
  int size = args.size();
  if (size == 0)
    {
    return 0;
    }
  this->Args = new char*[size+1];
  for (i=0; i < size; i++)
    {
    this->Args[i] = vtksys::SystemTools::DuplicateString(args[i].c_str());
    }
  this->Args[size] = 0;
  return (const char**)this->Args;
}

//-----------------------------------------------------------------------------
const char* vtkXMLShader::GetCode()
{
  switch(this->GetLocation())
    {
  case vtkXMLShader::LOCATION_INLINE:
    return this->RootElement->GetCharacterData();

  case vtkXMLShader::LOCATION_LIBRARY:
    // until the ShaderCode library starts providing XMLs, we just return the code.
    return this->Code;
  
  case vtkXMLShader::LOCATION_FILE:
    return this->Code;
    }
  return 0;
}


//-----------------------------------------------------------------------------
void vtkXMLShader::CleanupArgs()
{
  if (this->Args)
    {
    char** a = this->Args;
    while (*a)
      {
      delete [] (*a);
      a++;
      }
    delete [] this->Args;
    this->Args = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkXMLShader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << (this->GetName()? this->GetName() : "(none)")
                                                                    << endl;
  os << indent << "Type: ";
  switch(this->GetScope())
    {
  case SCOPE_NONE:
    os << "None";
    break;
  case SCOPE_MIXED:
    os << "Mixed";
    break;
  case SCOPE_VERTEX:
    os << "Vertex";
    break;
  case SCOPE_FRAGMENT:
    os << "Fragment";
    break;
    }
  os << endl;

  os << indent << "Language: ";
  switch (this->GetLanguage())
    {
  case LANGUAGE_NONE:
    os << "None";
    break;
  case LANGUAGE_MIXED:
    os << "Mixed";
    break;
  case LANGUAGE_CG:
    os << "Cg";
    break;
  case LANGUAGE_GLSL:
    os << "GLSL";
    }
  os << endl;
 
  os << indent << "Location: ";
  switch (this->GetLocation())
    {
  case LOCATION_NONE:
    os << "None";
    break;
  case LOCATION_INLINE:
    os << "Inline";
    break;
  case LOCATION_FILE:
    os << "(loaded from a source file)";
    break;
  case LOCATION_LIBRARY:
    os << "Library";
    break;
    }
  os << endl;

  os << indent << "Entry: " 
    <<  (this->GetEntry()? this->GetEntry() : "(none)") << endl;
  os << indent << "Args: ";
  const char** args = this->GetArgs();
  if (!*args)
    {
    os << "(none)" << endl;
    }
  while (*args)
    {
    os << indent << *args << " ";
    args++;
    }
  os << endl;

  os << indent << "RootElement: ";
  if (this->RootElement)
    {
    os << endl;
    this->RootElement->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
