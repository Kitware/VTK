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
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at
// Sandia National Labs.

#ifndef __vtkXMLMaterial_h
#define __vtkXMLMaterial_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkXMLDataElement;
class vtkXMLMaterialInternals;
class vtkXMLShader;

class VTKRENDERINGCORE_EXPORT vtkXMLMaterial : public vtkObject
{
public:
  static vtkXMLMaterial* New();
  vtkTypeMacro(vtkXMLMaterial, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new instance. It searches for the material
  // using the following order: first, check the MaterialLibrary; second,
  // treat the name as an absolute path and try to locate it; third,
  // search the Material repository. Returns null is it fails to
  // locate the material.
  static vtkXMLMaterial* CreateInstance(const char* name);

  // Description:
  // Get number of elements of type Property.
  int GetNumberOfProperties();

  // Description:
  // Get number of elements of type Texture.
  int GetNumberOfTextures();

  // Description:
  // Get number of Vertex shaders.
  int GetNumberOfVertexShaders();

  // Description:
  // Get number of fragment shaders.
  int GetNumberOfFragmentShaders();

  // Description:
  // Get the ith vtkXMLDataElement of type <Property />.
  vtkXMLDataElement* GetProperty(int id=0);

  // Description:
  // Get the ith vtkXMLDataElement of type <Texture />.
  vtkXMLDataElement* GetTexture(int id=0);

  // Description:
  // Get the ith vtkXMLDataElement of type <VertexShader />.
  vtkXMLShader* GetVertexShader(int id=0);

  // Description:
  // Get the ith vtkXMLDataElement of type <FragmentShader />.
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

  // Description:
  // Get the style the shaders.
  // \post valid_result: result==1 || result==2
  int GetShaderStyle();

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

