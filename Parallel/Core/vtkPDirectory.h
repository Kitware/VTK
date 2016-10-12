/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDirectory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
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

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"
#include <string> // for string functions in Directory

class vtkStringArray;

class VTKPARALLELCORE_EXPORT vtkPDirectory : public vtkObject
{
 public:
  static vtkPDirectory *New();
  vtkTypeMacro(vtkPDirectory,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Open/Load the specified directory and load the names of the files
   * in that directory. false/0 is returned if the directory can not be
   * opened, true/1 if it is opened. The reason that there are two versions
   * of this is that Directory uses Load() and vtkDirectory uses Open()
   * for this functionality.
   */
  bool Load(const std::string&);
  int Open(const char* dir);
  //@}

  /**
   * Return the number of files in the current directory.
   */
  vtkIdType GetNumberOfFiles() const;

  /**
   * Return the file at the given index, the indexing is 0 based
   */
  const char* GetFile(vtkIdType index) const;

  /**
   * Return true if the file is a directory.  If the file is not an
   * absolute path, it is assumed to be relative to the opened
   * directory. If no directory has been opened, it is assumed to
   * be relative to the current working directory.
   */
  int FileIsDirectory(const char *name);

  //@{
  /**
   * Get an array that contains all the file names.
   */
  vtkGetObjectMacro(Files, vtkStringArray);
  //@}

  /**
   * Return the path to Open'ed directory
   */
  const char* GetPath() const;

  /**
   * Clear the internal structure. Used internally at beginning of Load(...)
   * to clear the cache.
   */
  void Clear();

 protected:
  vtkPDirectory();
  ~vtkPDirectory();

 private:
  // Array of Files
  vtkStringArray *Files;    // VTK array of files

  // Path to Open'ed directory
  std::string Path;

  vtkPDirectory(const vtkPDirectory&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPDirectory&) VTK_DELETE_FUNCTION;
}; // End Class: vtkPDirectory

#endif
