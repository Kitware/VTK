/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureUnitManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTextureUnitManager
 * @brief   allocate/free texture units.
 *
 *
 * vtkTextureUnitManager is a central place used by shaders to reserve a
 * texture unit ( Allocate() ) or release it ( Free() ).
 *
 * Don't create a vtkTextureUnitManager, query it from the
 * vtkOpenGLRenderWindow
 *
 * @sa
 * vtkOpenGLRenderWindow
*/

#ifndef vtkTextureUnitManager_h
#define vtkTextureUnitManager_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"

class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL_EXPORT vtkTextureUnitManager : public vtkObject
{
public:
  vtkTypeMacro(vtkTextureUnitManager,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkTextureUnitManager *New();

  //@{
  /**
   * Get/Set the context. This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(vtkOpenGLRenderWindow *context);
  vtkGetObjectMacro(Context,vtkOpenGLRenderWindow);
  //@}

  /**
   * Number of texture units supported by the OpenGL context.
   */
  int GetNumberOfTextureUnits();

  /**
   * Reserve a texture unit. It returns its number.
   * It returns -1 if the allocation failed (because there is no more
   * texture unit left).
   * \post valid_result: result==-1 || result>=0 && result<this->GetNumberOfTextureUnits())
   * \post allocated: result==-1 || this->IsAllocated(result)
   */
  virtual int Allocate();

  /**
   * Tell if texture unit `textureUnitId' is already allocated.
   * \pre valid_textureUnitId_range : textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
   */
  bool IsAllocated(int textureUnitId);

  /**
   * Release a texture unit.
   * \pre valid_textureUnitId: textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
   * \pre allocated_textureUnitId: this->IsAllocated(textureUnitId)
   */
  virtual void Free(int textureUnitId);

protected:
  /**
   * Default constructor.
   */
  vtkTextureUnitManager();

  /**
   * Destructor.
   */
  ~vtkTextureUnitManager();

  /**
   * Delete the allocation table and check if it is not called before
   * all the texture units have been released.
   */
  void DeleteTable();

  vtkOpenGLRenderWindow *Context;

  int NumberOfTextureUnits;
  bool *TextureUnits;

private:
  vtkTextureUnitManager(const vtkTextureUnitManager&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTextureUnitManager&) VTK_DELETE_FUNCTION;
};

#endif
