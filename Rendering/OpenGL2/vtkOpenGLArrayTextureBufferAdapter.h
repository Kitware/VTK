// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLArrayTextureBufferAdapter
 * @brief   Interfaces vtkDataArray to an OpenGL texture buffer.
 *
 * The desktop OpenGL and GLES/WebGL2 upload paths are deliberately asymmetric
 * because the two APIs store buffer-backed sampler data differently:
 *
 * - On desktop, a `samplerBuffer` is a real `GL_TEXTURE_BUFFER`, which OpenGL
 *   defines as a typed *view* over a buffer object: `glTexBuffer` binds the
 *   buffer's data store to the texture; there is no `glTexImage*` entry point for
 *   a buffer texture. The `Buffer` member is therefore the texture's actual
 *   storage (not a staging copy) and must outlive every fetch, so it is held for
 *   the lifetime of the texture rather than freed after upload. This keeps the
 *   flat `texelFetch(samplerBuffer, int)` indexing and the large
 *   `GL_MAX_TEXTURE_BUFFER_SIZE` limit.
 *
 * - On GLES/WebGL2 there is no `GL_TEXTURE_BUFFER`, so `vtkTextureObject`
 *   emulates it with an ordinary 2D texture that owns its storage and is filled
 *   directly from client memory via `glTexImage2D` (see
 *   `vtkTextureObject::EmulateTextureBufferWith2DTexturesFromRaw`). No buffer
 *   object is involved, so `Buffer` stays null on that path; the shader instead
 *   reconstructs a 1D index into the tiled 2D image.
 *
 */

#ifndef vtkOpenGLArrayTextureBufferAdapter_h
#define vtkOpenGLArrayTextureBufferAdapter_h

#include "vtkDataArray.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkRenderingOpenGL2Module.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkWindow.h"

#include <cstddef> // for std::size_t
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLArrayTextureBufferAdapter
{
public:
  std::vector<vtkSmartPointer<vtkDataArray>> Arrays;
  vtkSmartPointer<vtkTextureObject> Texture;
  vtkSmartPointer<vtkOpenGLBufferObject> Buffer;
  vtkOpenGLBufferObject::ObjectType BufferType;
  vtkOpenGLBufferObject::ObjectUsage BufferUsage;
  bool IntegerTexture;
  bool ScalarComponents;

  vtkOpenGLArrayTextureBufferAdapter();
  vtkOpenGLArrayTextureBufferAdapter(
    vtkDataArray* array, bool asScalars, bool* integerTexture = nullptr);
  vtkOpenGLArrayTextureBufferAdapter(const vtkOpenGLArrayTextureBufferAdapter&) = default;
  vtkOpenGLArrayTextureBufferAdapter& operator=(
    const vtkOpenGLArrayTextureBufferAdapter&) = default;

  void Upload(vtkOpenGLRenderWindow* renderWindow, bool force = false);

  void ReleaseGraphicsResources(vtkWindow* window);

  ///@{
  /**
   * Get/set the "already uploaded" guard. When true, Upload() skips the GPU transfer unless
   * forced; callers clear it (SetUploaded(false)) whenever the bound array list changes to request
   * a re-upload. It is the GLES-safe replacement for the old Buffer->IsReady() check, since on
   * GLES the data goes straight into the texture and no buffer object is kept around to query.
   * The setter does not call Modified().
   */
  void SetUploaded(bool uploaded) { this->Uploaded = uploaded; }
  bool GetUploaded() const { return this->Uploaded; }
  ///@}

  ///@{
  /**
   * Get/set who owns this adapter's Texture/Buffer. True (the default) means the adapter owns
   * them: it allocates them, may partially re-upload in place, and releases them in
   * ReleaseGraphicsResources(). False means they are borrowed from a shared
   * vtkOpenGLArrayTextureBufferCache entry that owns their lifetime; the adapter only references
   * them and never frees or partially rewrites them. Upload() sets this automatically per call
   * (false when it routes a single array through the cache, true otherwise). The setter does not
   * call Modified().
   */
  void SetOwnsTexture(bool ownsTexture) { this->OwnsTexture = ownsTexture; }
  bool GetOwnsTexture() const { return this->OwnsTexture; }
  ///@}

  ///@{
  /**
   * Get/set the pending-reset flag used to recycle an adapter across rebuild cycles.
   * vtkDrawTexturedElements::BeginArrayRebuild() sets it on every adapter; the first Bind/Append
   * of the new cycle then drops the stale source-array list (keeping the GL texture/buffer and
   * layout records so the next Upload() can diff against them) and clears the flag. An adapter
   * still flagged after the cycle was not re-listed this frame, so draw-time loops skip it rather
   * than upload/activate a now-stale array. The setter does not call Modified().
   */
  void SetPendingReset(bool pendingReset) { this->PendingReset = pendingReset; }
  bool GetPendingReset() const { return this->PendingReset; }
  ///@}

private:
  /// Narrow to 32-bit type, concatenate and upload the bound arrays into this->Texture (creating
  /// it and, on desktop, this->Buffer as needed). Shared by the cached and uncached Upload() paths.
  ///
  /// When the per-sub-array layout (count of arrays and each one's byte length/offset) matches
  /// the previous upload, only the sub-arrays whose source pointer or MTime changed are pushed
  /// (a partial upload): glBufferSubData at the slice offset on desktop, or a tiled
  /// glTexSubImage2D region on GLES. Otherwise the whole buffer is reallocated and re-uploaded.
  void UploadImpl(vtkOpenGLRenderWindow* renderWindow);

  /// Shape of one narrowed source array as it lands in the concatenated upload buffer.
  struct NarrowedArrayInfo
  {
    std::size_t ByteLength = 0; //!< bytes appended to the destination
    vtkIdType TexelCount = 0;   //!< texels (tuples, or scalar values when ScalarComponents)
    int NumComps = 0;           //!< components per texel
    int Vtktype = 0;            //!< VTK data type after narrowing
  };

  /// Narrow one source array to a precision base OpenGL accepts (double->float, 64-bit
  /// int->32-bit) and append its bytes to \a dest, returning the slice's shape. Values are
  /// copied through a typed vtkArrayDispatch worker, so no vtkDataArray::GetVoidPointer() is
  /// needed (the upload then sources from the byte vector instead of the array's raw pointer).
  NarrowedArrayInfo AppendNarrowed(vtkDataArray* array, std::vector<unsigned char>& dest) const;

  /// Records, per concatenated sub-array, what was last uploaded so the next upload can detect
  /// an unchanged layout and skip re-uploading slices whose data did not change.
  struct SubArrayUpload
  {
    const void* SourceId = nullptr; //!< source vtkDataArray address (identity only, never deref)
    vtkMTimeType MTime = 0;         //!< source array MTime captured at upload
    std::size_t ByteOffset = 0;     //!< slice offset within the concatenated buffer (desktop)
    std::size_t ByteLength = 0;     //!< slice length in bytes
    vtkIdType TexelOffset = 0;      //!< slice offset in texels/tuples (GLES 2D emulation)
    vtkIdType TexelCount = 0;       //!< slice length in texels/tuples
  };
  struct UploadedLayout
  {
    std::vector<SubArrayUpload> Subs;
    std::size_t TotalBytes = 0;
    vtkIdType TotalTuples = 0;
    int NumComps = 0;
    int Vtktype = 0;
    bool Scalars = false;
    bool Valid = false;
  };
  UploadedLayout LastUpload;
  /// Whether the current array data has been uploaded to the texture. See GetUploaded().
  bool Uploaded = false;
  /// Whether Texture/Buffer are owned by this adapter. See GetOwnsTexture().
  bool OwnsTexture = true;
  /// Whether the next Bind/Append should discard the previous source arrays. See GetPendingReset().
  bool PendingReset = false;
};

VTK_ABI_NAMESPACE_END
#endif
// Need to skip header testing since we do not inherit vtkObject:
// VTK-HeaderTest-Exclude: vtkOpenGLArrayTextureBufferAdapter.h
