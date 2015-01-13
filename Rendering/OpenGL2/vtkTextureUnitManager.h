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
// .NAME vtkTextureUnitManager - allocate/free texture units.
// .SECTION Description
//
// vtkTextureUnitManager is a central place used by shaders to reserve a
// texture unit ( Allocate() ) or release it ( Free() ).
//
// Don't create a vtkTextureUnitManager, query it from the
// vtkOpenGLRenderWindow
//
// .SECTION See Also
// vtkOpenGLRenderWindow

#ifndef vtkTextureUnitManager_h
#define vtkTextureUnitManager_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkTextureUnitManager : public vtkObject
{
public:
  vtkTypeMacro(vtkTextureUnitManager,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkTextureUnitManager *New();

  // Description:
  // Get/Set the context. This does not increase the reference count of the
  // context to avoid reference loops.
  // SetContext() may raise an error is the OpenGL context does not support the
  // required OpenGL extensions.
  void SetContext(vtkOpenGLRenderWindow *context);
  vtkGetObjectMacro(Context,vtkOpenGLRenderWindow);

  // Description:
  // Number of texture units supported by the OpenGL context.
  int GetNumberOfTextureUnits();

  // Description:
  // Reserve a texture unit. It returns its number.
  // It returns -1 if the allocation failed (because there are no more
  // texture units left).
  // \post valid_result: result==-1 || result>=0 && result<this->GetNumberOfTextureUnits())
  // \post allocated: result==-1 || this->IsAllocated(result)
  virtual int Allocate();

  // Description:
  // Tell if texture unit `textureUnitId' is already allocated.
  // \pre valid_textureUnitId_range : textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
  bool IsAllocated(int textureUnitId);

  // Description:
  // Release a texture unit.
  // \pre valid_textureUnitId: textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
  // \pre allocated_textureUnitId: this->IsAllocated(textureUnitId)
  virtual void Free(int textureUnitId);

protected:
  // Description:
  // Default constructor.
  vtkTextureUnitManager();

  // Description:
  // Destructor.
  ~vtkTextureUnitManager();

  // Description:
  // Delete the allocation table and check if it is not called before
  // all the texture units have been released.
  void DeleteTable();

  vtkOpenGLRenderWindow *Context;

  int NumberOfTextureUnits;
  unsigned int *TextureUnits;

private:
  vtkTextureUnitManager(const vtkTextureUnitManager&);  // Not implemented.
  void operator=(const vtkTextureUnitManager&);  // Not implemented.
};

#endif
