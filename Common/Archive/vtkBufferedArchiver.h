/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBufferedArchiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCOMMONARCHIVE_EXPORT vtkBufferedArchiver : public vtkArchiver
{
public:
  static vtkBufferedArchiver* New();
  vtkTypeMacro(vtkBufferedArchiver, vtkArchiver);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Open the arhive for writing.
   */
  void OpenArchive() override;
  //@}

  //@{
  /**
   * Close the arhive.
   */
  void CloseArchive() override;
  //@}

  //@{
  /**
   * Insert \p data of size \p size into the archive at \p relativePath.
   */
  void InsertIntoArchive(
    const std::string& relativePath, const char* data, std::streamsize size) override;
  //@}

  //@{
  /**
   * Checks if \p relativePath represents an entry in the archive.
   */
  bool Contains(const std::string& relativePath) override;
  //@}

  //@{
  /**
   * Access the buffer.
   */
  const char* GetBuffer();
  //@}

  //@{
  /**
   * Access the address of the buffer.
   */
  const void* GetBufferAddress();
  //@}

  //@{
  /**
   * Set/Get the allocated buffer size.
   */
  void SetAllocatedSize(std::size_t);
  std::size_t GetAllocatedSize();
  //@}

  //@{
  /**
   * Get the buffer used size.
   */
  std::size_t GetBufferSize();
  //@}

protected:
  vtkBufferedArchiver();
  ~vtkBufferedArchiver() override;

  struct Internal;
  Internal* Internals;

private:
  vtkBufferedArchiver(const vtkBufferedArchiver&) = delete;
  void operator=(const vtkBufferedArchiver&) = delete;
};

#endif
