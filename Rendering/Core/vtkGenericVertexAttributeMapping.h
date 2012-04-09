/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericVertexAttributeMapping.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericVertexAttributeMapping - stores mapping for data arrays to
// generic vertex attributes.
// .SECTION Description
// vtkGenericVertexAttributeMapping stores mapping between data arrays and
// generic vertex attributes. It is used by vtkPainterPolyDataMapper to pass the
// mappings to the painter which rendering the attributes.
// .SECTION Thanks
// Support for generic vertex attributes in VTK was contributed in
// collaboration with Stephane Ploix at EDF.

#ifndef __vtkGenericVertexAttributeMapping_h
#define __vtkGenericVertexAttributeMapping_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKRENDERINGCORE_EXPORT vtkGenericVertexAttributeMapping : public vtkObject
{
public:
  static vtkGenericVertexAttributeMapping* New();
  vtkTypeMacro(vtkGenericVertexAttributeMapping, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Select a data array from the point/cell data
  // and map it to a generic vertex attribute.
  // Note that indices change when a mapping is added/removed.
  void AddMapping(const char* attributeName, const char* arrayName,
    int fieldAssociation, int component);

  // Description:
  // Select a data array and use it as multitexture texture
  // coordinates.
  // Note the texture unit parameter should correspond to the texture
  // unit set on the texture.
  void AddMapping(
    int unit, const char* arrayName, int fieldAssociation,
    int component);

  // Description:
  // Remove a vertex attribute mapping.
  bool RemoveMapping(const char* attributeName);

  // Description:
  // Remove all mappings.
  void RemoveAllMappings();

  // Description:
  // Get number of mapppings.
  unsigned int GetNumberOfMappings();

  // Description:
  // Get the attribute name at the given index.
  const char* GetAttributeName(unsigned int index);

  // Description:
  // Get the array name at the given index.
  const char* GetArrayName(unsigned int index);

  // Description:
  // Get the field association at the given index.
  int GetFieldAssociation(unsigned int index);

  // Description:
  // Get the component no. at the given index.
  int GetComponent(unsigned int index);

  // Description:
  // Get the component no. at the given index.
  int GetTextureUnit(unsigned int index);

//BTX
protected:
  vtkGenericVertexAttributeMapping();
  ~vtkGenericVertexAttributeMapping();

private:
  vtkGenericVertexAttributeMapping(const vtkGenericVertexAttributeMapping&); // Not implemented.
  void operator=(const vtkGenericVertexAttributeMapping&); // Not implemented.

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif


