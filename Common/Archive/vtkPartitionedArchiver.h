// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPartitionedArchiver
 * @brief   Writes an archive to several buffers
 *
 * vtkPartitionedArchiver is a specialized archiver for writing datasets into
 * several memory buffers with zip compression. Each insertion into the archiver
 * is assigned to its own buffer.
 *
 * @sa
 * vtkArchiver
 */

#ifndef vtkPartitionedArchiver_h
#define vtkPartitionedArchiver_h

#include "vtkCommonArchiveModule.h" // For export macro

#include "vtkArchiver.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONARCHIVE_EXPORT vtkPartitionedArchiver : public vtkArchiver
{
public:
  static vtkPartitionedArchiver* New();
  vtkTypeMacro(vtkPartitionedArchiver, vtkArchiver);
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
  const char* GetBuffer(const char* relativePath);
  ///@}

  ///@{
  /**
   * Access the address of the buffer.
   */
  const void* GetBufferAddress(const char* relativePath);
  ///@}

  ///@{
  /**
   * Get the buffer used size.
   */
  std::size_t GetBufferSize(const char* relativePath);
  ///@}

  ///@{
  /**
   * Get the number of buffers in the archive.
   */
  std::size_t GetNumberOfBuffers();
  ///@}

  ///@{
  /**
   * Get the name of buffer \p i.
   */
  const char* GetBufferName(std::size_t i);
  ///@}

protected:
  vtkPartitionedArchiver();
  ~vtkPartitionedArchiver() override;

  struct Internal;
  Internal* Internals;

private:
  vtkPartitionedArchiver(const vtkPartitionedArchiver&) = delete;
  void operator=(const vtkPartitionedArchiver&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
