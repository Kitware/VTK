/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLVertexBufferObjectCache
 * @brief   manage vertex buffer objects shared within a context
 *
 * This class allows mappers to share VBOs. Specifically it
 * is used by the V..B..O..Group to see if a VBO already exists
 * for a given vtkDataArray.
 *
 *
 *
*/

#ifndef vtkOpenGLVertexBufferObjectCache_h
#define vtkOpenGLVertexBufferObjectCache_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"
#include <map> // for methods

class vtkOpenGLVertexBufferObject;
class vtkDataArray;
class vtkTimeStamp;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLVertexBufferObjectCache : public vtkObject
{
public:
  static vtkOpenGLVertexBufferObjectCache *New();
  vtkTypeMacro(vtkOpenGLVertexBufferObjectCache, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the vertex buffer object which holds the
   * data array's data. If such a VBO does not exist
   * a new empty VBO will be created you need to append to.
   * The return value has been registered, you are responsible
   * for deleting it. The data array pointers are also registered.
   */
  vtkOpenGLVertexBufferObject* GetVBO(
    vtkDataArray *array,
    int destType);

  /**
   * Removes all references to a given vertex buffer
   * object.
   */
  void RemoveVBO(vtkOpenGLVertexBufferObject *vbo);

  typedef std::map<vtkDataArray*, vtkOpenGLVertexBufferObject *> VBOMap;

protected:
  vtkOpenGLVertexBufferObjectCache();
  ~vtkOpenGLVertexBufferObjectCache() override;

  VBOMap MappedVBOs;

private:
  vtkOpenGLVertexBufferObjectCache(const vtkOpenGLVertexBufferObjectCache&) = delete;
  void operator=(const vtkOpenGLVertexBufferObjectCache&) = delete;

};

#endif
