/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDirectory
 * @brief   OS independent class for access and manipulation of system directories
 *
 * vtkDirectory provides a portable way of finding the names of the files
 * in a system directory.  It also provides methods of manipulating directories.
 *
 * @warning
 * vtkDirectory works with windows and unix only.
*/

#ifndef vtkDirectory_h
#define vtkDirectory_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkObject.h"

class vtkStringArray;

class VTKCOMMONSYSTEM_EXPORT vtkDirectory : public vtkObject
{
public:
  //@{
  /**
   * Return the class name as a string.
   */
  vtkTypeMacro(vtkDirectory,vtkObject);
  //@}

  /**
   * Create a new vtkDirectory object.
   */
  static vtkDirectory *New();

  /**
   * Print directory to stream.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Open the specified directory and load the names of the files
   * in that directory. 0 is returned if the directory can not be
   * opened, 1 if it is opened.
   */
  int Open(const char* dir);

  /**
   * Return the number of files in the current directory.
   */
  vtkIdType GetNumberOfFiles();

  /**
   * Return the file at the given index, the indexing is 0 based
   */
  const char* GetFile(vtkIdType index);

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
   * Get the current working directory.
   */
  static const char* GetCurrentWorkingDirectory(char* buf, unsigned int len);

  /**
   * Create directory.
   */
  static int MakeDirectory(const char* dir);

  /**
   * Remove a directory.
   */
  static int DeleteDirectory(const char* dir);

  /**
   * Rename a file or directory.
   */
  static int Rename(const char* oldname, const char* newname);

protected:
  // delete the Files and Path ivars and set
  // NumberOfFiles to 0
  void CleanUpFilesAndPath();
  vtkDirectory();
  ~vtkDirectory() override;

private:
  char* Path;           // Path to Open'ed directory
  vtkStringArray *Files;    // VTK array of files

  static int CreateDirectoryInternal(const char* dir);

private:
  vtkDirectory(const vtkDirectory&) = delete;
  void operator=(const vtkDirectory&) = delete;
};

#endif
