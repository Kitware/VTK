// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBufferedArchiver
 * @brief   Writes an archive to a buffer for vtk-js datasets
 *
 * vtkvtkJSBufferedArchiver is a specialized archiver for writing datasets into
 * a memory buffer with zip compression.
 *
 * @sa
 * vtkArchiver
 */

#ifndef vtkBufferedArchiver_h
#define vtkBufferedArchiver_h

#include "vtkCommonArchiveModule.h" // For export macro

#include "vtkArchiver.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONARCHIVE_EXPORT vtkBufferedArchiver : public vtkArchiver
{
public:
  static vtkBufferedArchiver* New();
  vtkTypeMacro(vtkBufferedArchiver, vtkArchiver);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Open the archive for writing.
   */
  void OpenArchive() override;
  ///@}

  ///@{
  /**
   * Close the archive.
   */
  void CloseArchive() override;
  ///@}

  ///@{
  /**
   * Insert \p data of size \p size into the archive at \p relativePath.
   */
  void InsertIntoArchive(
    const std::string& relativePath, const char* data, std::size_t size) override;
  ///@}

  ///@{
  /**
   * Checks if \p relativePath represents an entry in the archive.
   */
  bool Contains(const std::string& relativePath) override;
  ///@}

  ///@{
  /**
   * Access the buffer.
   */
  const char* GetBuffer();
  ///@}

  ///@{
  /**
   * Access the address of the buffer.
   */
  const void* GetBufferAddress();
  ///@}

  ///@{
  /**
   * Set/Get the allocated buffer size.
   */
  void SetAllocatedSize(std::size_t);
  std::size_t GetAllocatedSize();
  ///@}

  ///@{
  /**
   * Get the buffer used size.
   */
  std::size_t GetBufferSize();
  ///@}

protected:
  vtkBufferedArchiver();
  ~vtkBufferedArchiver() override;

  struct Internal;
  Internal* Internals;

private:
  vtkBufferedArchiver(const vtkBufferedArchiver&) = delete;
  void operator=(const vtkBufferedArchiver&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
