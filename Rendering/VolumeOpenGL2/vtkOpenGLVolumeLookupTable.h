// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOpenGLVolumeLookupTable
 * @brief Base class for OpenGL texture management of scalar color,
 * opacity and gradient opacity lookup tables.
 */

#ifndef vtkOpenGLVolumeLookupTable_h
#define vtkOpenGLVolumeLookupTable_h

#include "vtkObject.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkTextureObject;
class vtkWindow;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLVolumeLookupTable : public vtkObject
{
public:
  vtkTypeMacro(vtkOpenGLVolumeLookupTable, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // static vtkOpenGLVolumeLookupTable* New();

  /**
   * Get the texture unit associated with the managed texture object
   */
  int GetTextureUnit();

  /**
   * Activate / deactivate the internal texture object
   */
  ///@{
  void Activate();
  void Deactivate();
  ///@}

  /**
   * Get the maximum supported texture width for the target OpenGL environment.
   */
  int GetMaximumSupportedTextureWidth(vtkOpenGLRenderWindow* renWin, int idealWidth);

  /**
   * Release graphics resources
   */
  void ReleaseGraphicsResources(vtkWindow* window);

  /**
   * Update the internal texture object using the transfer function provided.
   */
  virtual void Update(vtkObject* func, double scalarRange[2], int blendMode, double sampleDistance,
    double unitDistance, int filterValue, vtkOpenGLRenderWindow* renWin);

  /**
   * Get access to the texture height used by this object
   */
  vtkGetMacro(TextureHeight, int);

  /**
   * Get access to the texture width used by this object
   */
  vtkGetMacro(TextureWidth, int);

protected:
  vtkOpenGLVolumeLookupTable() = default;
  ~vtkOpenGLVolumeLookupTable() override;

  double LastRange[2] = { 0.0, 0.0 };
  float* Table = nullptr;
  int LastInterpolation = -1;
  int NumberOfColorComponents = 1;
  int TextureWidth = 1024;
  int TextureHeight = 1;
  vtkTextureObject* TextureObject = nullptr;
  vtkTimeStamp BuildTime;

  /**
   * Test whether the internal function needs to be updated.
   */
  virtual bool NeedsUpdate(
    vtkObject* func, double scalarRange[2], int blendMode, double sampleDistance);

  /**
   * Internal method to actually update the texture object
   */
  virtual void InternalUpdate(
    vtkObject* func, int blendMode, double sampleDistance, double unitDistance, int filterValue);

  /**
   * Compute ideal width and height for the texture based on function provided
   */
  virtual void ComputeIdealTextureSize(
    vtkObject* func, int& width, int& height, vtkOpenGLRenderWindow* renWin);

  /**
   * Allocate internal data table
   */
  virtual void AllocateTable();

private:
  vtkOpenGLVolumeLookupTable(const vtkOpenGLVolumeLookupTable&) = delete;
  void operator=(const vtkOpenGLVolumeLookupTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLVolumeLookupTable_h
