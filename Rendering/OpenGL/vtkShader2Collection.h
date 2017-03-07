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
/**
 * @class   vtkShader2Collection
 * @brief   an ordered list of Shader2 objects.
 *
 * vtkShader2Collection represents and provides methods to manipulate a
 * list of Shader2 objects. The list is ordered and duplicate entries are not
 * prevented.
 *
 * @sa
 * vtkShader2 vtkCollection
*/

#ifndef vtkShader2Collection_h
#define vtkShader2Collection_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkCollection.h"

class vtkShader2;

class VTKRENDERINGOPENGL_EXPORT vtkShader2Collection : public vtkCollection
{
 public:
  static vtkShader2Collection *New();
  vtkTypeMacro(vtkShader2Collection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Modified GetMTime because the collection time depends on the
   * content of the shaders.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Add a shader to the bottom of the list.
   */
  void AddItem(vtkShader2 *shader);

  /**
   * Get the next shader in the list.
   */
  vtkShader2 *GetNextShader();

  /**
   * Get the last shader in the list.
   */
  vtkShader2 *GetLastShader();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkShader2 *GetNextShader(vtkCollectionSimpleIterator &cookie);

  /**
   * Add the elements of `other' to the end of `this'.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   * \post added: this->GetNumberOfItems()=old this->GetNumberOfItems()+other->GetNumberOfItems()
   */
  void AddCollection(vtkShader2Collection *other);

  /**
   * Remove the elements of `other' from `this'. It assumes that `this' already
   * has all the elements of `other' added contiguously.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   * \post removed: this->GetNumberOfItems()=old this->GetNumberOfItems()-other->GetNumberOfItems()
   */
  void RemoveCollection(vtkShader2Collection *other);

  /**
   * Tells if at least one of the shaders is a vertex shader.
   * If yes, it means the vertex processing of the fixed-pipeline is bypassed.
   * If no, it means the vertex processing of the fixed-pipeline is used.
   */
  bool HasVertexShaders();

  /**
   * Tells if at least one of the shaders is a tessellation control shader.
   */
  bool HasTessellationControlShaders();

  /**
   * Tells if at least one of the shaders is a tessellation evaluation shader.
   */
  bool HasTessellationEvaluationShaders();

  /**
   * Tells if at least one of the shaders is a geometry shader.
   */
  bool HasGeometryShaders();

  /**
   * Tells if at least one of the shaders is a fragment shader.
   * If yes, it means the fragment processing of the fixed-pipeline is
   * bypassed.
   * If no, it means the fragment processing of the fixed-pipeline is used.
   */
  bool HasFragmentShaders();

  /**
   * Release OpenGL resources (shader id of each item).
   */
  void ReleaseGraphicsResources();

protected:
  vtkShader2Collection();
  ~vtkShader2Collection() VTK_OVERRIDE;

  bool HasShadersOfType(int);

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o);

  vtkShader2Collection(const vtkShader2Collection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkShader2Collection&) VTK_DELETE_FUNCTION;
};

#endif
