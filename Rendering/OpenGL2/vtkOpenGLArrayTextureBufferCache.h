// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLArrayTextureBufferCache
 * @brief   manage texture-buffer-backed data arrays shared within a context
 *
 * This is the texture-buffer analog of vtkOpenGLVertexBufferObjectCache. The
 * vertex-pulling mappers (vtkOpenGLLowMemoryPolyDataMapper via
 * vtkDrawTexturedElements / vtkOpenGLArrayTextureBufferAdapter) reconstruct
 * every vertex in-shader from data bound as samplerBuffer texture objects.
 * Historically each adapter owned its own texture, so the same vtkDataArray
 * (positions, normals, colors, connectivity, ...) rendered through several
 * mappers — composite, glyph, LIC, or simply many actors sharing one polydata —
 * was uploaded once per mapper. This cache lets those mappers share a single
 * uploaded texture buffer.
 *
 * Entries are keyed on the composite (array, scalarComponents, integerTexture):
 * not the array pointer alone, because the same array may legitimately be
 * uploaded with different layouts (e.g. a 3-component position array bound as
 * RGB tuples versus as scalars). Each entry stamps the array MTime at which it
 * was uploaded so a mutated array is lazily re-uploaded.
 *
 * The cache is owned by vtkOpenGLState (one per context), reachable via
 * vtkOpenGLRenderWindow::GetArrayTextureBufferCache(), mirroring GetVBOCache().
 */

#ifndef vtkOpenGLArrayTextureBufferCache_h
#define vtkOpenGLArrayTextureBufferCache_h

#include "vtkObject.h"
#include "vtkOpenGLBufferObject.h"     // for entry ivar
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // for entry ivars
#include "vtkTextureObject.h"          // for entry ivar

#include <cstddef> // for std::size_t
#include <map>     // for cache map
#include <memory>  // for shared_ptr
#include <tuple>   // for composite key

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLArrayTextureBufferCache : public vtkObject
{
public:
  static vtkOpenGLArrayTextureBufferCache* New();
  vtkTypeMacro(vtkOpenGLArrayTextureBufferCache, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * A cached texture buffer for one (array, role). Texture holds the
   * samplerBuffer (a 2D emulation texture on GLES, a real GL_TEXTURE_BUFFER on
   * desktop); Buffer is the desktop TBO backing store and stays null on GLES,
   * where data is uploaded straight from client memory. UploadTime records the
   * source array's MTime captured at the last upload, so the owning adapter can
   * skip re-uploading until the array changes.
   */
  struct Entry
  {
    vtkSmartPointer<vtkDataArray> Array;
    vtkSmartPointer<vtkTextureObject> Texture;
    vtkSmartPointer<vtkOpenGLBufferObject> Buffer;
    vtkMTimeType UploadTime = 0;
  };

  /**
   * Return the shared entry for the given (array, scalarComponents,
   * integerTexture), creating an empty entry if none exists yet. The returned
   * entry is owned by the cache; callers hold it via shared_ptr and fill in /
   * read back its Texture, Buffer and UploadTime. The array is held alive by the
   * entry so its pointer cannot be reused by a different array while cached.
   */
  std::shared_ptr<Entry> GetTextureBuffer(
    vtkDataArray* array, bool scalarComponents, bool integerTexture);

  /**
   * Return the number of distinct (array, role) texture buffers currently cached.
   * Mainly useful for tests and diagnostics: rendering the same shared array
   * through several mappers yields a single entry, so this count stays well below
   * the per-mapper total when geometry is shared.
   */
  std::size_t GetNumberOfCachedTextureBuffers() const { return this->Cache.size(); }

  /**
   * Release the GL resources held by every cached entry and forget them. Called
   * when the owning context is torn down.
   */
  void ReleaseGraphicsResources(vtkWindow* window);

protected:
  vtkOpenGLArrayTextureBufferCache();
  ~vtkOpenGLArrayTextureBufferCache() override;

  using Key = std::tuple<vtkDataArray*, bool, bool>;
  std::map<Key, std::shared_ptr<Entry>> Cache;

private:
  vtkOpenGLArrayTextureBufferCache(const vtkOpenGLArrayTextureBufferCache&) = delete;
  void operator=(const vtkOpenGLArrayTextureBufferCache&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
