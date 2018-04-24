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
 * This class holds onto the VBOs that a mapper is using.
 * The basic operation is that during the render process
 * the mapper may cache a number of dataArrays as VBOs
 * associated with attributes. This class keep track of
 * freeing VBOs no longer used by the mapper and uploading
 * new data as needed.
 *
 * When using CacheCataArray the same array can be set
 * each time and this class will not rebuild or upload
 * unless needed.
 *
 * When using the AppendDataArray API no caching is done
 * and the VBOs will be rebuilt and uploaded each time.
 * So when appending th emapper need to handle checking
 * if the VBO should be updated.
 *
 * Use case:
 *   make this an ivar of your mapper
 *   vbg->CacheDataArray("vertexMC", vtkDataArray);
 *   vbg->BuildAllVBOs();
 *   if (vbg->GetMTime() > your VAO update time)
 *   {
 *     vbg->AddAllAttributesToVAO(...);
 *   }
 *
 * Appended Use case:
 *   make this an ivar of your mapper
 *   if (you need to update your VBOs)
 *   {
 *     vbg->ClearAllVBOs();
 *     vbg->AppendDataArray("vertexMC", vtkDataArray1);
 *     vbg->AppendDataArray("vertexMC", vtkDataArray2);
 *     vbg->AppendDataArray("vertexMC", vtkDataArray3);
 *     vbg->BuildAllVBOs();
 *     vbg->AddAllAttributesToVAO(...);
 *   }
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * used to remove a no longer needed attribute
   * Calling CacheDataArray with a nullptr
   * attribute will also work.
   */
  void RemoveAttribute(const char *attribute);

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
   * Check if the array already exists.
   * offset is the index of the first vertex of the array if it exists.
   * totalOffset is the total number of vertices in the appended arrays.
   * Note that if the array does not exist, offset is equal to totalOffset.
   */
  bool ArrayExists(const char *attribute, vtkDataArray *da,
    vtkIdType& offset,
    vtkIdType& totalOffset);

  /**
   * Append a data array for an attribute in the VBO Group
   * registers the data array until build is called
   */
  void AppendDataArray(const char *attribute, vtkDataArray *da, int destType);

  /**
   * using the data arrays in this group
   * build all the VBOs, once this has been called the
   * reference to the data arrays will be freed.
   */
  void BuildAllVBOs(vtkOpenGLVertexBufferObjectCache *);
  void BuildAllVBOs(vtkViewport *);

  /**
   * Force all the VBOs to be freed from this group.
   * Call this prior to starting appending operations.
   * Not needed for single array caching.
   */
  void ClearAllVBOs();

  /**
   * Clear all the data arrays. Typically an internal method.
   * Automatically called at the end of BuildAllVBOs to prepare
   * for the next set of attributes.
   */
  void ClearAllDataArrays();

  /**
   * Get the mtime of this groups VBOs
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkOpenGLVertexBufferObjectGroup();
  ~vtkOpenGLVertexBufferObjectGroup() override;

  std::map<std::string, vtkOpenGLVertexBufferObject*> UsedVBOs;
  std::map<std::string, std::vector<vtkDataArray*> > UsedDataArrays;
  std::map<std::string, std::map<vtkDataArray*, vtkIdType> > UsedDataArrayMaps;
  std::map<std::string, vtkIdType> UsedDataArraySizes;

private:
  vtkOpenGLVertexBufferObjectGroup(const vtkOpenGLVertexBufferObjectGroup&) = delete;
  void operator=(const vtkOpenGLVertexBufferObjectGroup&) = delete;

};

#endif
