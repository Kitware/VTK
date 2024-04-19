// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPDirectory
 *
 * vtkPDirectory provides a portable way of finding the names of the files
 * in a system directory where process 0 finds the information and
 * broadcasts it to other processes. It tries to replicate the API for both
 * Directory and vtkDirectory though there are slight mismatches between the
 * two. This is a blocking collective operation.
 */

#ifndef vtkPDirectory_h
#define vtkPDirectory_h

#include "vtkObject.h"
#include "vtkParallelCoreModule.h" // For export macro
#include <string>                  // for string functions in Directory

VTK_ABI_NAMESPACE_BEGIN
class vtkStringArray;

class VTKPARALLELCORE_EXPORT vtkPDirectory : public vtkObject
{
public:
  static vtkPDirectory* New();
  vtkTypeMacro(vtkPDirectory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Open/Load the specified directory and load the names of the files
   * in that directory. false/0 is returned if the directory can not be
   * opened, true/1 if it is opened. The reason that there are two versions
   * of this is that Directory uses Load() and vtkDirectory uses Open()
   * for this functionality.
   */
  bool Load(const std::string&);
  int Open(const char* dir);
  ///@}

  /**
   * Return the number of files in the current directory.
   */
  vtkIdType GetNumberOfFiles() const;

  /**
   * Return the file at the given index, the indexing is 0 based
   */
  VTK_FILEPATH const char* GetFile(vtkIdType index) const;

  /**
   * Return true if the file is a directory.  If the file is not an
   * absolute path, it is assumed to be relative to the opened
   * directory. If no directory has been opened, it is assumed to
   * be relative to the current working directory.
   */
  int FileIsDirectory(const char* name);

  ///@{
  /**
   * Get an array that contains all the file names.
   */
  vtkGetObjectMacro(Files, vtkStringArray);
  ///@}

  /**
   * Return the path to Open'ed directory
   */
  VTK_FILEPATH const char* GetPath() const;

  /**
   * Clear the internal structure. Used internally at beginning of Load(...)
   * to clear the cache.
   */
  void Clear();

  /**
   * Get the current working directory.
   */
  static VTK_FILEPATH const char* GetCurrentWorkingDirectory(char* buf, unsigned int len);

  /**
   * Create directory.
   */
  static int MakeDirectory(VTK_FILEPATH const char* dir);

  /**
   * Remove a directory.
   */
  static int DeleteDirectory(VTK_FILEPATH const char* dir);

  /**
   * Rename a file or directory.
   */
  static int Rename(VTK_FILEPATH const char* oldname, VTK_FILEPATH const char* newname);

protected:
  vtkPDirectory();
  ~vtkPDirectory() override;

private:
  // Array of Files
  vtkStringArray* Files; // VTK array of files

  // Path to Open'ed directory
  std::string Path;

  vtkPDirectory(const vtkPDirectory&) = delete;
  void operator=(const vtkPDirectory&) = delete;
}; // End Class: vtkPDirectory

VTK_ABI_NAMESPACE_END
#endif
