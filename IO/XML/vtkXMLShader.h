/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLShader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLShader - encapsulates a Shader XML description.
// .SECTION Description
// vtkXMLShader encapsulates the XML description for a Shader.
// It provides convenient access to various attributes/properties
// of a shader.
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at 
// Sandia National Labs.

#ifndef __vtkXMLShader_h
#define __vtkXMLShader_h

#include "vtkObject.h"

class vtkXMLDataElement;

class VTK_IO_EXPORT vtkXMLShader : public vtkObject
{
public:
  static vtkXMLShader* New();
  vtkTypeMacro(vtkXMLShader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the XML root element that describes this shader.
  vtkGetObjectMacro(RootElement, vtkXMLDataElement);
  void SetRootElement(vtkXMLDataElement*);

  // Description:
  // Returns the shader's language as defined in the XML description.
  int GetLanguage();

  // Description:
  // Returns the type of the shader as defined in the XML description.
  int GetScope();

  // Description:
  // Returns the location of the shader as defined in the XML description.
  int GetLocation();

  // Description:
  // Returns the style of the shader as optionaly defined in the XML
  // description. If not present, default style is 1. "style=2" means it is
  // a shader without a main(). In style 2, the "main" function for the vertex
  // shader part is void propFuncVS(void), the main function for the fragment
  // shader part is void propFuncFS(). This is useful when combining a shader
  // at the actor level and a shader defines at the renderer level, like
  // the depth peeling pass.
  // \post valid_result: result==1 || result==2
  int GetStyle();
  
  // Description:
  // Get the name of the Shader.
  const char* GetName();

  // Description:
  // Get the entry point to the shader code as defined in the XML.
  const char* GetEntry();

  // Description:
  // Get the shader code.
  const char* GetCode();

  // Description:
  // Returns an null terminate array of the pointers to space sepatared Args
  // defined in the XML description.
  const char** GetArgs();

  // Description:
  // Searches the file in the VTK_MATERIALS_DIRS.
  // Note that this allocates new memory for the string.
  // The caller must delete it.
  static char* LocateFile(const char* filename);

//BTX
  enum LanguageCodes
    {
    LANGUAGE_NONE=0,
    LANGUAGE_MIXED,
    LANGUAGE_CG,
    LANGUAGE_GLSL
    };
  
  enum ScopeCodes
    {
    SCOPE_NONE=0,
    SCOPE_MIXED,
    SCOPE_VERTEX,
    SCOPE_FRAGMENT
    };

  enum LocationCodes 
    {
    LOCATION_NONE=0,
    LOCATION_INLINE,
    LOCATION_FILE,
    LOCATION_LIBRARY
    };
//ETX
protected:
  vtkXMLShader();
  ~vtkXMLShader();

  // Reads the file and fills it in this->Code.
  void ReadCodeFromFile(const char* fullpath);
  
  char* Code; // cache for the code.
  vtkSetStringMacro(Code);
  
  vtkXMLDataElement* RootElement;
  vtkXMLDataElement* SourceLibraryElement;
  void SetSourceLibraryElement(vtkXMLDataElement*);

  char** Args;
  void CleanupArgs();
private:
  vtkXMLShader(const vtkXMLShader&); // Not implemented.
  void operator=(const vtkXMLShader&); // Not implemented.
};

#endif

