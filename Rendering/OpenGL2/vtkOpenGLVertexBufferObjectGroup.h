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
 * @class   vtkOpenGLVertexBufferObjectGroup
 * @brief   manage vertex buffer objects shared within a mapper
 *
 *
 * Use case:
 *   make this an ivar of your mapper
 *   vbg->SetDataArray("vertexMC", vtkDataArray);
 *   vbg->BuildAllVBOs();
 *   vbg->AddAllAttributesToVAO(...);
 *
 * use VAO
 *
*/

#ifndef vtkOpenGLVertexBufferObjectGroup_h
#define vtkOpenGLVertexBufferObjectGroup_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"
#include <map> // for methods
#include <vector> // for ivars

class vtkDataArray;
class vtkOpenGLVertexArrayObject;
class vtkOpenGLVertexBufferObject;
class vtkOpenGLVertexBufferObjectCache;
class vtkShaderProgram;
class vtkViewport;
class vtkWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLVertexBufferObjectGroup : public vtkObject
{
public:
  static vtkOpenGLVertexBufferObjectGroup *New();
  vtkTypeMacro(vtkOpenGLVertexBufferObjectGroup, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the number of components for this attribute
   * zero if the attribute does not exist
   */
  int GetNumberOfComponents(const char *attribute);

  /**
   * Returns the number of tuples for this attribute
   * zero if the attribute does not exist
   */
  int GetNumberOfTuples(const char *attribute);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *);

  /**
   * Returns the VBO for an attribute, NULL if
   * it is not present.
   */
  vtkOpenGLVertexBufferObject *GetVBO(const char *attribute);

  /**
   * Attach all VBOs to their attributes
   */
  void AddAllAttributesToVAO(
    vtkShaderProgram *program,
    vtkOpenGLVertexArrayObject *vao);

  /**
   * Set the data array for an attribute in the VBO Group
   * registers the data array until build is called
   * once this is called a valid VBO will exist
   */
  void CacheDataArray(const char *attribute, vtkDataArray *da,
    vtkOpenGLVertexBufferObjectCache *cache,
    int destType);
  void CacheDataArray(const char *attribute, vtkDataArray *da,
    vtkViewport *vp,
    int destType);

  /**
   * Append a data array for an attribute in the VBO Group
   * registers the data array until build is called
   */
  void AppendDataArray(const char *attribute, vtkDataArray *da,
    int destType);

  /**
   * using the data arays in this group
   * build all the VBOs, once this has been called the
   * reference to the data arrays will be freed.
   */
  void BuildAllVBOs(vtkOpenGLVertexBufferObjectCache *);
  void BuildAllVBOs(vtkViewport *);

  void ClearAllDataArrays();
  void ClearAllVBOs();

protected:
  vtkOpenGLVertexBufferObjectGroup();
  ~vtkOpenGLVertexBufferObjectGroup();

  std::map<std::string, vtkOpenGLVertexBufferObject*> UsedVBOs;
  std::map<std::string, std::vector<vtkDataArray*> > UsedDataArrays;

private:
  vtkOpenGLVertexBufferObjectGroup(const vtkOpenGLVertexBufferObjectGroup&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLVertexBufferObjectGroup&) VTK_DELETE_FUNCTION;

};

#endif
