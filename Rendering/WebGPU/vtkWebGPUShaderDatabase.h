// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUShaderDatabase
 * @brief   Class to add and retrieve source code for shader files for a specified key.
 *
 * vtkWebGPUShaderDatabase has three methods that allow you to insert, retrieve and remove
 * shader source code for a specified key string. The key string is very significant here,
 * as it can be used in another shader to share a single shader source file.
 * This is facilitated by using the `#include "key/string/here"` statement in your shader code.
 *
 * The preprocessing logic is in `vtkWebGPURenderWindow::PreprocessShaderSource(const std::string&
 * source)`.
 *
 * The constructor preloads the contents of basic helper shader source files from the
 * "Rendering/WebGPU/wgsl" directory under the key strings "VTK/wgsl/NameOfShaderSource.wgsl".
 *
 * @sa
 * vtkWebGPURenderer, vtkWebGPURenderWindow
 */

#ifndef vtkWebGPUShaderDatabase_h
#define vtkWebGPUShaderDatabase_h

#include "vtkObject.h"

#include "vtkRenderingWebGPUModule.h" // for export macro

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUShaderDatabase : public vtkObject
{
public:
  static vtkWebGPUShaderDatabase* New();
  vtkTypeMacro(vtkWebGPUShaderDatabase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add shader source code into the database.
   * `key` can be used to retrieve the source code later.
   */
  void AddShaderSource(const std::string& key, const std::string& source);

  /**
   * Retrieve the shader source code for the given `key`.
   */
  std::string GetShaderSource(const std::string& key) const;

  /**
   * Remove the shader source code for the given `key`.
   */
  void RemoveShaderSource(const std::string& key);

protected:
  vtkWebGPUShaderDatabase();
  ~vtkWebGPUShaderDatabase() override;

private:
  vtkWebGPUShaderDatabase(const vtkWebGPUShaderDatabase&) = delete;
  void operator=(const vtkWebGPUShaderDatabase&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END

#endif
