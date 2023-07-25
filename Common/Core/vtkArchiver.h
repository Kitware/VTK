// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArchiver
 * @brief   Writes an archive
 *
 * vtkArchiver is a base class for constructing an archive. The default
 * implementation constructs a directory at the location of the ArchiveName
 * and populates it with files and directories as requested by Insert().
 * Classes that derive from vtkArchiver can customize the output using such
 * features as compression, in-memory serialization and third-party archival
 * tools.
 */

#ifndef vtkArchiver_h
#define vtkArchiver_h

#include "vtkCommonCoreModule.h" // For export macro

#include "vtkObject.h"

#include <ios> // For std::streamsize

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkArchiver : public vtkObject
{
public:
  static vtkArchiver* New();
  vtkTypeMacro(vtkArchiver, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the archive to generate.
   */
  vtkGetStringMacro(ArchiveName);
  vtkSetStringMacro(ArchiveName);
  ///@}

  ///@{
  /**
   * Open the archive for writing.
   */
  virtual void OpenArchive();
  ///@}

  ///@{
  /**
   * Close the archive.
   */
  virtual void CloseArchive();
  ///@}

  ///@{
  /**
   * Insert \p data of size \p size into the archive at \p relativePath.
   */
  virtual void InsertIntoArchive(
    const std::string& relativePath, const char* data, std::size_t size);
  ///@}

  ///@{
  /**
   * Checks if \p relativePath represents an entry in the archive.
   */
  virtual bool Contains(const std::string& relativePath);
  ///@}

protected:
  vtkArchiver();
  ~vtkArchiver() override;

  char* ArchiveName;

private:
  vtkArchiver(const vtkArchiver&) = delete;
  void operator=(const vtkArchiver&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
