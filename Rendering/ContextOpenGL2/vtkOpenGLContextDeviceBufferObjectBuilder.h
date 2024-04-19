// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkOpenGLContextDeviceBufferObjectBuilder
 * @brief   Internal buffer object builder that maintains a cache of VBO groups.
 *
 */

#ifndef vtkOpenGLContextDeviceBufferObjectBuilder_h
#define vtkOpenGLContextDeviceBufferObjectBuilder_h

#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkRenderWindow.h"
#include "vtkRenderingContextOpenGL2Module.h" // for export macro
#include "vtkUnsignedCharArray.h"

#include <cstdint>       // for std::uintptr_t
#include <unordered_map> // for std::unordered_map

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCONTEXTOPENGL2_NO_EXPORT vtkOpenGLContextDeviceBufferObjectBuilder
{
public:
  /**
   * Build vertex buffer objects for these data arrays and add vertex attribute specifications
   * to the vertex array object. Existing VBOs may be reused when a VBOGroup already exists for the
   * `cacheIdentifier` and the data arrays have not been modified since last upload.
   */
  void BuildVBO(vtkOpenGLHelper* cbo, vtkDataArray* positions, vtkUnsignedCharArray* colors,
    vtkFloatArray* tcoords, std::uintptr_t cacheIdentifier, vtkRenderWindow* renderWindow);

  /**
   * Erase cache entry for given identifier.
   */
  void Erase(std::uintptr_t cacheIdentifier, vtkRenderWindow* renderWindow);

private:
  std::unordered_map<std::size_t, vtkSmartPointer<vtkOpenGLVertexBufferObjectGroup>> VBOGroups;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLContextDeviceBufferObjectBuilder_h
