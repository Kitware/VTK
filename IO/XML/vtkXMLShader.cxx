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
#include "vtkToolkits.h" // for VTK_MATERIALS_DIRS.
#include "vtkXMLDataElement.h"

#include <vtksys/SystemTools.hxx>
#include <assert.h>

vtkStandardNewMacro(vtkXMLShader);
vtkCxxSetObjectMacro(vtkXMLShader, SourceLibraryElement, vtkXMLDataElement);
//-----------------------------------------------------------------------------
vtkXMLShader::vtkXMLShader()
  : Code(NULL),
    RootElement(NULL),
    SourceLibraryElement(NULL),
    Args(NULL)
{
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
        char* fullpath = this->LocateFile(filename);
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
  if(!filename)
    {
    return NULL;
    }

  // if filename is absolute path, return the same.
  if (vtksys::SystemTools::FileExists(filename))
    {
    return vtksys::SystemTools::DuplicateString(filename);
    }

  // Fetch any runtime defined user paths for materials
  std::vector<std::string> paths;
  std::string userpaths;
  vtksys::SystemTools::GetEnv("USER_MATERIALS_DIRS", userpaths);
  if (userpaths.size()>0)
    {
    vtksys::SystemTools::Split(userpaths.c_str(), paths, ';');
    }

#ifdef VTK_MATERIALS_DIRS
  // search thru default paths to locate file.
  vtksys::SystemTools::Split(VTK_MATERIALS_DIRS, paths, ';');
#endif
  for (unsigned int i =0; i < paths.size(); i++)
    {
    std::string path = paths[i];
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
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkXMLShader::ReadCodeFromFile(const char* filepath)
{
  // Delete the existing code first. If 'filepath' doesn't exist,
  // default to standard rendering.
  if (this->Code)
    {
    delete [] this->Code;
    this->Code = 0;
    }

  ifstream ifp;
  ifp.open(filepath, ios::binary);
  if (!ifp)
    {
    vtkErrorMacro("Failed to open file " << filepath);
    return;
    }

  // determine the length of the file.
  long length;
  ifp.seekg(0, ios::end);
  length = ifp.tellg();
  ifp.seekg(0, ios::beg);

  // Allocate for the file and the null terminator.
  this->Code = new char[length+1];
  ifp.read(this->Code, length);
  ifp.close();
  // Null terminate the string so GL doesn't get confused.
  this->Code[length] = '\0';
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
    const char* scope = this->RootElement->GetAttribute("scope");
    if (!scope)
      {
      vtkErrorMacro("Shader description missing \"scope\" attribute.");
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

// ----------------------------------------------------------------------------
// \post valid_result: result==1 || result==2
int vtkXMLShader::GetStyle()
{
  int result=1;
  if(this->RootElement)
    {
    const char *loc=this->RootElement->GetAttribute("style");
    if(loc==0)
      {
      // fine. this attribute is optional.
      }
    else
      {
      if(strcmp(loc,"1")==0)
        {
        // fine. default value.
        }
      else
        {
        if(strcmp(loc,"2")==0)
          {
          result=2; // new style
          }
        else
          {
          vtkErrorMacro(<<"style number not supported. Expect 1 or 2. We force it to be 1.");
          }
        }
      }
    }
  
  assert("post valid_result" && (result==1 || result==2) );
  return result;
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
  
  std::vector<std::string> args;
  vtksys::SystemTools::Split(this->RootElement->GetAttribute("args"), args, ' ');
  
  int i;
  int size = static_cast<int>(args.size());
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
  return const_cast<const char**>(this->Args);
}

//-----------------------------------------------------------------------------
const char* vtkXMLShader::GetCode()
{
  switch(this->GetLocation())
    {
  case vtkXMLShader::LOCATION_INLINE:
    return this->RootElement->GetCharacterData();
    break;
  case vtkXMLShader::LOCATION_LIBRARY:
    // until the ShaderCode library starts providing XMLs, we just return the code.
    return this->Code;
    break;
  case vtkXMLShader::LOCATION_FILE:
    return this->Code;
    break;
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
  os << indent << "Scope: ";
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
  if (!args)
    {
    os << "(none)" << endl;
    }
  else
    {
    while (*args)
      {
      os << indent << *args << " ";
      args++;
      }
    os << endl;
    }

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
