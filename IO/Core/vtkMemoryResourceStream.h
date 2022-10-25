/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMemoryResourceStream.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkMemoryResourceStream_h
#define vtkMemoryResourceStream_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkResourceStream.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief vtkResourceStream implementation for memory input
 * This class is a view on existing data, it does not copy or take
 * ownership of it.
 */
class VTKIOCORE_EXPORT vtkMemoryResourceStream : public vtkResourceStream
{
public:
  vtkTypeMacro(vtkMemoryResourceStream, vtkResourceStream);
  static vtkMemoryResourceStream* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * @brief Set buffer view to stream
   *
   * This function does not do any copy, the source buffer must stay valid as
   * as it may be used.
   * Calling this function reset stream position to 0.
   * After this call EndOfStream will return false if size is 0.
   * This function will increase modified time.
   *
   * @param buffer the buffer address, may be nullptr if size is 0.
   * @param size the buffer size in bytes, may be 0.
   */
  void SetBuffer(const void* buffer, std::size_t size);

  ///@{
  /**
   * @brief Override vtkResourceStream functions
   */
  std::size_t Read(void* buffer, std::size_t bytes) override;
  bool EndOfStream() override;
  vtkTypeInt64 Seek(vtkTypeInt64 pos, SeekDirection dir) override;
  vtkTypeInt64 Tell() override;
  ///@}

protected:
  vtkMemoryResourceStream();
  ~vtkMemoryResourceStream() override = default;
  vtkMemoryResourceStream(const vtkMemoryResourceStream&) = delete;
  vtkMemoryResourceStream& operator=(const vtkMemoryResourceStream&) = delete;

private:
  const unsigned char* Buffer = nullptr; // for pointer arithmetics
  std::size_t Size = 0;
  vtkTypeInt64 Pos = 0;
  bool Eos = false;
};

VTK_ABI_NAMESPACE_END

#endif
