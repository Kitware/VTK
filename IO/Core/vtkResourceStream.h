// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkResourceStream_h
#define vtkResourceStream_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

#include <cstdlib> // for std::size_t
#include <memory>  // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief Abstract class used for custom streams
 *
 * vtkResourceStream provides a high-level interface for manipulating
 * custom streams. This class is designed to be used instead of "SetFileName"
 * and "SetInputString" or similar that exists in most of reader or reader-like
 * classes in VTK.
 *
 * vtkResourceStream only support unformatted input, user must use external
 * solution to format the input, such as sscanf or scnlib, fast_float or strtoX
 *
 * vtkResourceStream **may** be support seeking, code that uses
 * vtkResourceStream should take in account this, and support for both seekable
 * stream and not seekable streams, if possible. `stream.SupportSeek()` can be
 * used for support checking.
 */
class VTKIOCORE_EXPORT vtkResourceStream : public vtkObject
{
  struct vtkInternals;

public:
  enum class SeekDirection
  {
    Begin = 0,
    Current = 1,
    End = 2
  };

  vtkTypeMacro(vtkResourceStream, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * @brief Read data form the stream
   *
   * Read at most `bytes` bytes from input stream.
   * If this function reads less that `bytes` bytes, then EndOfStream must
   * return true.
   *
   * @param buffer User provided storage buffer, may be nullptr if bytes == 0
   * @param bytes Maximum amount of bytes to read.
   * @return The number of bytes read, may be 0.
   */
  virtual std::size_t Read(void* buffer, std::size_t bytes) = 0;

  /**
   * @brief Check if the stream reached an end.
   *
   * The stream may be out of range (EOF) if a Seek call move before stream
   * begin (0), or after stream end (max) Seek on an EndOfStream stream may
   * restore it back to a valid state. Read on an invalid stream must do
   * nothing and return 0.
   *
   * @return Return true if the stream reached the end of input, false
   * otherwise.
   */
  virtual bool EndOfStream() = 0;

  /**
   * @brief Move the stream cursor position
   *
   * Base version does nothing and returns 0.
   * This function does not check if the position is out of range.
   * EndOfFile() result must only change after a call of Read with
   * non-zero size.
   *
   * @return Current position in stream.
   */
  virtual vtkTypeInt64 Seek(vtkTypeInt64 pos, SeekDirection dir);

  /**
   * @brief Get the stream cursor position
   *
   * Base version calls `Seek(0, std::ios_base::cur)`.
   * If seeking is unsupported, return 0.
   * Subclasses may reimplement it to provide a more efficient version.
   *
   * @return Current position in stream.
   */
  virtual vtkTypeInt64 Tell();

  /**
   * @brief Check if stream supports Seek and Tell functions
   *
   * @return true if Seek and Tell functions are supported.
   */
  bool SupportSeek() const;

protected:
  /**
   * @brief Constructor
   *
   * Only constructor, subclasses must fill the vtkResourceStreamInfo and pass it
   * to this constructor.
   *
   * @param supportSeek true is Seek and Tell are supported
   */
  vtkResourceStream(bool supportSeek);
  ~vtkResourceStream() override;
  vtkResourceStream(const vtkResourceStream&) = delete;
  vtkResourceStream& operator=(const vtkResourceStream&) = delete;

private:
  std::unique_ptr<vtkInternals> Impl;
};

VTK_ABI_NAMESPACE_END

#endif
