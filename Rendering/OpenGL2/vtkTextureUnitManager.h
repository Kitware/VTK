// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkTextureUnitManager : public vtkObject
{
public:
  vtkTypeMacro(vtkTextureUnitManager, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkTextureUnitManager* New();

  /**
   * Update the number of hardware texture units for the current context
   */
  void Initialize();

  /**
   * Number of texture units supported by the OpenGL context.
   */
  int GetNumberOfTextureUnits();

  /**
   * Reserve a texture unit. It returns its number.
   * It returns -1 if the allocation failed (because there are no more
   * texture units left).
   * \post valid_result: result==-1 || result>=0 && result<this->GetNumberOfTextureUnits())
   * \post allocated: result==-1 || this->IsAllocated(result)
   */
  virtual int Allocate();

  /**
   * Reserve a specific texture unit if not already in use.
   * This method should only be used when interacting with 3rd
   * party code that is allocating and using textures. It allows
   * someone to reserve a texture unit for that code and later release
   * it. VTK will not use that texture unit until it is released.
   * It returns -1 if the allocation failed (because there are no more
   * texture units left).
   * \post valid_result: result==-1 || result>=0 && result<this->GetNumberOfTextureUnits())
   * \post allocated: result==-1 || this->IsAllocated(result)
   */
  virtual int Allocate(int unit);

  /**
   * Tell if texture unit `textureUnitId' is already allocated.
   * \pre valid_textureUnitId_range : textureUnitId>=0 &&
   * textureUnitId<this->GetNumberOfTextureUnits()
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
  ~vtkTextureUnitManager() override;

  /**
   * Delete the allocation table and check if it is not called before
   * all the texture units have been released.
   */
  void DeleteTable();

  int NumberOfTextureUnits;
  bool* TextureUnits;

private:
  vtkTextureUnitManager(const vtkTextureUnitManager&) = delete;
  void operator=(const vtkTextureUnitManager&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
