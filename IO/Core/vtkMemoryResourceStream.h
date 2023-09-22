// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMemoryResourceStream_h
#define vtkMemoryResourceStream_h

#include "vtkBuffer.h"       // vtkBuffer
#include "vtkIOCoreModule.h" // For export macro
#include "vtkResourceStream.h"

#include <memory>  // for std::unique_ptr
#include <string>  // for std::basic_string
#include <utility> // for std::forward
#include <vector>  // for std::vector

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief `vtkResourceStream` implementation for memory input.
 *
 * `vtkMemoryResourceStream` can be a view on existing data.
 * Or it can copy specified data into an internal buffer.
 * Or it can take ownership of a `vtkBuffer`, a `std::vector` or a `std::string`.
 */
class VTKIOCORE_EXPORT vtkMemoryResourceStream : public vtkResourceStream
{
  /**
   * @brief Base class of DataHolder, used to manage type erased data destruction.
   */
  struct BasicHolder
  {
    BasicHolder() = default;
    virtual ~BasicHolder() = default;
    BasicHolder(const BasicHolder&) = delete;
    BasicHolder& operator=(const BasicHolder&) = delete;
    BasicHolder(BasicHolder&&) noexcept = delete;
    BasicHolder& operator=(BasicHolder&&) noexcept = delete;
  };

  /**
   * @brief Simple single value container
   *
   * This struct is used to manage stream owned buffer lifetime.
   * Its only purpose is to be destroyer through its base class destructor.
   */
  template <typename T>
  struct DataHolder : public BasicHolder
  {
    template <typename... Args>
    DataHolder(Args&&... args)
      : Data{ std::forward<Args>(args)... }
    {
    }

    ~DataHolder() override = default;
    DataHolder(const DataHolder&) = delete;
    DataHolder& operator=(const DataHolder&) = delete;
    DataHolder(DataHolder&&) noexcept = delete;
    DataHolder& operator=(DataHolder&&) noexcept = delete;

    T Data;
  };

  // Small utility function to easily create a DataHolder with type deduction
  template <typename T>
  static std::unique_ptr<DataHolder<T>> MakeHolder(T&& value)
  {
    return std::unique_ptr<DataHolder<T>>{ new DataHolder<T>{ std::forward<T>(value) } };
  }

public:
  vtkTypeMacro(vtkMemoryResourceStream, vtkResourceStream);
  static vtkMemoryResourceStream* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * @brief Override vtkResourceStream functions
   */
  std::size_t Read(void* buffer, std::size_t bytes) override;
  bool EndOfStream() override;
  vtkTypeInt64 Seek(vtkTypeInt64 pos, SeekDirection dir) override;
  vtkTypeInt64 Tell() override;
  ///@}

  /**
   * @brief Set buffer to stream
   *
   * If `copy` is `false`, the source buffer must stay valid as it may be used.
   *
   * Otherwise, if `copy` is `true`, given buffer will be copied into an internally managed buffer.
   * If `size` is 0, this call won't allocate anything. If `size > 0`,
   * `buffer` must not be `nullptr` and must point to a contiguous buffer of at least `size` bytes.
   *
   * Regardless of `copy` value, this function also has the following effects:
   * - Reset stream position to `0`.
   * - EndOfStream will return `true` if `size` is `0`, `false` otherwise.
   * - Release currently owned buffer, if any.
   * - Increase modified time.
   *
   * @param buffer the buffer address, may be nullptr if `size` is 0.
   * @param size the buffer size in bytes, may be 0.
   * @param copy if `true` this function copies given buffer to an internally managed buffer.
   */
  void SetBuffer(const void* buffer, std::size_t size, bool copy = false);

  /**
   * @brief Set buffer to stream
   *
   * `this` will keep an owning reference to `buffer` (`buffer` reference count will be increased).
   * While `buffer` is streamed, it **must not** be invalidated (e.g. calling `vtkBuffer::Allocate`)
   * nor freed.
   * Also note that `buffer` content is still managed by the `vtkBuffer` instance, so it can be
   * modified externally.
   *
   * Streamed buffer is set as if by calling:
   * `SetBuffer(buffer->GetBuffer(), buffer->GetSize() * sizeof(T), false)`.
   * If `buffer` is nullptr, this function has the same effect as `SetBuffer(nullptr, 0)`.
   * When `buffer` is released, it only decrements the reference count.
   *
   * @param buffer `vtkBuffer` to stream, may be nullptr. `buffer` may also empty.
   */
  template <typename T>
  void SetBuffer(vtkSmartPointer<vtkBuffer<T>> buffer)
  {
    if (buffer)
    {
      this->SetBuffer(buffer->GetBuffer(), buffer->GetSize() * sizeof(T));
      this->Holder = MakeHolder(std::move(buffer));
    }
    else
    {
      this->SetBuffer(nullptr, 0);
    }
  }

  /**
   * @brief Set buffer to stream
   *
   * `this` will manage `vec` lifetime internally.
   *
   * Streamed buffer is set as if by calling:
   * `SetBuffer(vec.data(), vec.size() * sizeof(T), false)`.
   *
   * Note that this function takes `vec` by value:
   * - Call `SetBuffer(std::move(vec))` if you no longer need `vec` after the call.
   * This is the most efficient way, because it will not copy `vec` data at all,
   * it will only transfer ownership of `vec` to the stream.
   * - Call `SetBuffer(vec)` to copy vec. Useful if you need to keep `vec` on caller side.
   *
   * @param vec std::vector to stream, may be empty.
   */
  template <typename T, typename Allocator>
  void SetBuffer(std::vector<T, Allocator> vec)
  {
    this->SetBuffer(vec.data(), vec.size() * sizeof(T));
    this->Holder = MakeHolder(std::move(vec));
  }

  /**
   * @brief Set buffer to stream
   *
   * Same as `SetBuffer(std::vector<T, Allocator> vec)` but for `std::basic_string`.
   *
   * @param str std::string to stream, may be empty.
   */
  template <typename CharT, typename Traits, typename Allocator>
  void SetBuffer(std::basic_string<CharT, Traits, Allocator> str)
  {
    // unlike std::vector (and other containers), std::basic_string does not guarantee
    // iterators validity after a move (mainly for SSO).
    // So we have to move it prior to getting the pointer.
    auto holder = MakeHolder(std::move(str));
    this->SetBuffer(holder->Data.data(), holder->Data.size() * sizeof(CharT));
    this->Holder = std::move(holder);
  }

  /**
   * @brief Check if `this` has a internally managed buffer
   *
   * This is `true` after a call to `SetBuffer(vtkSmartPointer<vtkBuffer<T>>)`
   * even if only the reference count is managed by the stream.
   *
   * @return `true` if `this` manage streamed buffer, `false` otherwise.
   */
  bool OwnsBuffer() const { return this->Holder != nullptr; }

protected:
  vtkMemoryResourceStream();
  ~vtkMemoryResourceStream() override = default;
  vtkMemoryResourceStream(const vtkMemoryResourceStream&) = delete;
  vtkMemoryResourceStream& operator=(const vtkMemoryResourceStream&) = delete;

private:
  const unsigned char* Buffer = nullptr; // for pointer arithmetic
  std::size_t Size = 0;
  vtkTypeInt64 Pos = 0;
  bool Eos = false;
  std::unique_ptr<BasicHolder> Holder;
};

VTK_ABI_NAMESPACE_END

#endif
