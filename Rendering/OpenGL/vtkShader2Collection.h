/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShader2Collection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShader2Collection - a list of Shader2 objects.
// .SECTION Description
// vtkShader2Collection represents and provides methods to manipulate a
// list of Shader2 objects. The list is unsorted and duplicate entries are not
// prevented.

// .SECTION see also
// vtkShader2 vtkCollection

#ifndef __vtkShader2Collection_h
#define __vtkShader2Collection_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkCollection.h"

class vtkShader2;

class VTKRENDERINGOPENGL_EXPORT vtkShader2Collection : public vtkCollection
{
 public:
  static vtkShader2Collection *New();
  vtkTypeMacro(vtkShader2Collection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Modified GetMTime because the collection time depends on the
  // content of the shaders.
  unsigned long GetMTime();

  // Description:
  // Add a shader to the list.
  void AddItem(vtkShader2 *shader);

  // Description:
  // Get the next shader in the list.
  vtkShader2 *GetNextShader();

  // Description:
  // Get the last shader in the list.
  vtkShader2 *GetLastShader();

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkShader2 *GetNextShader(vtkCollectionSimpleIterator &cookie);
  //ETX

  // Description:
  // Add the elements of `other' to the end of `this'.
  // \pre other_exists: other!=0
  // \pre not_self: other!=this
  // \post added: this->GetNumberOfItems()=old this->GetNumberOfItems()+other->GetNumberOfItems()
  void AddCollection(vtkShader2Collection *other);

  // Description:
  // Remove the elements of `other' from `this'. It assumes that `this' already
  // has all the elements of `other' added contiguously.
  // \pre other_exists: other!=0
  // \pre not_self: other!=this
  // \post removed: this->GetNumberOfItems()=old this->GetNumberOfItems()-other->GetNumberOfItems()
  void RemoveCollection(vtkShader2Collection *other);

  // Description:
  // Tells if at least one of the shaders is a vertex shader.
  // If yes, it means the vertex processing of the fixed-pipeline is bypassed.
  // If no, it means the vertex processing of the fixed-pipeline is used.
  bool HasVertexShaders();

  // Description:
  // Tells if at least one of the shaders is a tessellation control shader.
  bool HasTessellationControlShaders();

  // Description:
  // Tells if at least one of the shaders is a tessellation evaluation shader.
  bool HasTessellationEvaluationShaders();

  // Description:
  // Tells if at least one of the shaders is a geometry shader.
  bool HasGeometryShaders();

  // Description:
  // Tells if at least one of the shaders is a fragment shader.
  // If yes, it means the fragment processing of the fixed-pipeline is
  // bypassed.
  // If no, it means the fragment processing of the fixed-pipeline is used.
  bool HasFragmentShaders();

  // Description:
  // Release OpenGL resources (shader id of each item).
  void ReleaseGraphicsResources();

protected:
  vtkShader2Collection();
  ~vtkShader2Collection();

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o);

private:
  vtkShader2Collection(const vtkShader2Collection&);  // Not implemented.
  void operator=(const vtkShader2Collection&);  // Not implemented.
};

#endif
