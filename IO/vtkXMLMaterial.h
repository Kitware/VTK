/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMaterial.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLMaterial - encapsulates a VTK Material description.
// .SECTION Description
// vtkXMLMaterial encapsulates VTK Material description. It keeps a pointer
// to vtkXMLDataElement that defines the material and provides 
// access to Shaders/Properties defined in it.

#ifndef __vtkXMLMaterial_h
#define __vtkXMLMaterial_h

#include "vtkObject.h"

class vtkXMLDataElement;
class vtkXMLMaterialInternals;
class vtkXMLShader;

class VTK_IO_EXPORT vtkXMLMaterial : public vtkObject
{
public:
  static vtkXMLMaterial* New();
  vtkTypeRevisionMacro(vtkXMLMaterial, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new instance using the vtkMaterialLibrary.
  // If a material by the given name is found, a new instance
  // is returned, else it returns NULL.
  static vtkXMLMaterial* CreateInstance(const char* name);
  
  // Description:
  // Get number of elements of type vtkProperty.
  int GetNumberOfProperties();

  // Description:
  // Get number of Vertex shaders.
  int GetNumberOfVertexShaders();

  // Description:
  // Get number of fragment shaders.
  int GetNumberOfFragmentShaders();

  // Description:
  // Get the ith vtkXMLDataElement of type vtkProperty
  vtkXMLDataElement* GetProperty(int id=0);

  // Description:
  // Get the ith vtkXMLDataElement of type vtkVertexShader
  vtkXMLShader* GetVertexShader(int id=0);

  // Description:
  // Get the ith vtkXMLDataElement of type vtkFragmentShader
  vtkXMLShader* GetFragmentShader(int id=0);

  // Description:
  // Get/Set the XML root element that describes this material.
  vtkGetObjectMacro(RootElement, vtkXMLDataElement);
  void SetRootElement(vtkXMLDataElement*);

  // Description:
  // Get the Language used by the shaders in this Material.
  // The Language of a vtkXMLMaterial is based on the Language of it's
  // shaders. 
  int GetShaderLanguage();
protected:
  vtkXMLMaterial();
  ~vtkXMLMaterial();

  vtkXMLDataElement* RootElement;
  vtkXMLMaterialInternals* Internals;
private:
  vtkXMLMaterial(const vtkXMLMaterial&); // Not implemented.
  void operator=(const vtkXMLMaterial&); // Not implemented.
};

#endif

