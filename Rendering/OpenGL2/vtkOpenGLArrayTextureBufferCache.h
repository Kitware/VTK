// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLArrayTextureBufferCache
 * @brief   manage texture-buffer-backed data arrays shared within a context
 *
 * This cache lets multiple mappers share a single uploaded texture buffer for
 * the same vtkDataArray with different layouts. Entries are keyed by
 * (array, scalarComponents, integerTexture) and track array MTime to avoid
 * unnecessary reuploads. Unused entries are evicted in least-recently-used
 * order to stay within the cache budget (default 256 MiB) while preserving
 * externally pinned resources.
 *
 * The cache is owned by vtkOpenGLState (one per context), accessible via
 * vtkOpenGLRenderWindow::GetArrayTextureBufferCache().
 */

#ifndef vtkOpenGLArrayTextureBufferCache_h
#define vtkOpenGLArrayTextureBufferCache_h

#include "vtkObject.h"
#include "vtkOpenGLBufferObject.h"     // for entry ivar
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // for entry ivars
#include "vtkTextureObject.h"          // for entry ivar

#include <cstddef> // for std::size_t
#include <cstdint> // for std::uint64_t
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
    /// Monotonic stamp of the last GetTextureBuffer() that returned this entry;
    /// orders entries least-recently-used-first for budget eviction.
    std::uint64_t LastAccess = 0;
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
   * Return the host-side memory, in KiB, consumed by the source arrays the
   * cached entries keep alive.
   */
  unsigned long GetCPUMemorySize() const;

  ///@{
  /**
   * Byte budget for cached entries that are not currently referenced by any
   * adapter. The default budget is 256 MiB, which is a reasonable compromise between
   * memory usage and avoiding re-uploads of shared arrays. The budget can be
   * raised to accommodate larger shared arrays, or lowered to reduce memory
   * usage at the cost of more frequent re-uploads.
   */
  vtkSetMacro(MaximumCacheSize, std::size_t);
  vtkGetMacro(MaximumCacheSize, std::size_t);
  ///@}

  /**
   * Sum of GPU bytes held by all cached entries, computed on demand (the owning
   * adapters may reallocate an entry's buffer in place, so no incremental
   * counter is kept).
   */
  std::size_t GetCurrentCacheSize() const;

  /**
   * Eviction ladder is:
   *
   * 1. Pinned: entries whose Texture/Buffer are externally referenced are never evicted.
   * 2. Orphaned: entries whose Array is only held by the cache and whose Texture/Buffer are not
   * externally referenced are evicted first.
   * 3. LRU: least-recently-used unpinned entries are evicted until the cache is under budget (until
   * GetCurrentCacheSize() <= MaximumCacheSize).
   *
   * For large scenes with many live mappers, the budget is allowed to be exceeded because the
   * cache only evicts unpinned entries. The budget is only enforced at the end of
   * vtkOpenGLRenderWindow::Render() when RemoveUnusedTextureBuffers() is called.
   */
  void RemoveUnusedTextureBuffers(vtkWindow* window);

  /**
   * Erase every unpinned entry keyed on the given array (all layout variants).
   * Pinned entries are left alone. Returns the number of entries removed.
   * No-op when window is null.
   */
  std::size_t RemoveTextureBuffer(vtkDataArray* array, vtkWindow* window);

  /**
   * Release the GL resources held by every cached entry and forget them. Called
   * when the owning context is torn down.
   */
  void ReleaseGraphicsResources(vtkWindow* window);

protected:
  vtkOpenGLArrayTextureBufferCache();
  ~vtkOpenGLArrayTextureBufferCache() override;

  /// GPU bytes held by one entry: exact buffer size on desktop, computed from
  /// the emulation texture's dimensions on GLES, 0 if never uploaded.
  static std::size_t GetEntrySize(const Entry& entry);

  /// True when the entry's Texture or Buffer is referenced outside the entry
  /// itself (the entry accounts for exactly one reference to each), i.e. some
  /// adapter is still using the GL objects and eviction would be unsafe.
  static bool EntryIsPinned(const Entry& entry);

  /// Release the entry's GL objects and reset it to the never-uploaded state.
  static void ReleaseEntry(Entry& entry, vtkWindow* window);

  using Key = std::tuple<vtkDataArray*, bool, bool>;
  std::map<Key, std::shared_ptr<Entry>> Cache;

  std::size_t MaximumCacheSize = 256 * 1024 * 1024;
  /// Monotonic clock for Entry::LastAccess stamps.
  std::uint64_t AccessCounter = 0;

private:
  vtkOpenGLArrayTextureBufferCache(const vtkOpenGLArrayTextureBufferCache&) = delete;
  void operator=(const vtkOpenGLArrayTextureBufferCache&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
