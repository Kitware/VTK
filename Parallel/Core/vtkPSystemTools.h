/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSystemTools.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   vtkPSystemTools
 * @brief   System tools for file system introspection
 *
 * A class with only static methods for doing parallel file system
 * introspection. It limits doing file stats on process 0 and
 * broadcasting the results to other processes. It is built on VTK's
 * SystemTools class and uses the global controller for communication.
 * It uses blocking collective communication operations.
*/

#ifndef vtkPSystemTools_h
#define vtkPSystemTools_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"
#include <string> // for string functions in SystemTools

class VTKPARALLELCORE_EXPORT vtkPSystemTools : public vtkObject
{
 public:
  static vtkPSystemTools *New();
  vtkTypeMacro(vtkPSystemTools,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Given a string on process proc, broadcast that string to
   * all of the other processes. This method does not have a
   * correspondence to anything in SystemTools.
   */

  static void BroadcastString(std::string&, int proc);

  /**
   * Given a path to a file or directory, convert it to a full path.
   * This collapses away relative paths relative to the cwd argument
   * (which defaults to the current working directory).  The full path
   * is returned.
   */

  static std::string CollapseFullPath(const std::string& in_relative);
  static std::string CollapseFullPath(const std::string& in_relative,
                                      const char* in_base);

  //@{
  /**
   * Return true if a file exists in the current directory.
   * If isFile = true, then make sure the file is a file and
   * not a directory.  If isFile = false, then return true
   * if it is a file or a directory.  Note that the file will
   * also be checked for read access.  (Currently, this check
   * for read access is only done on POSIX systems.)
   */
  static bool FileExists(const char* filename, bool isFile);
  static bool FileExists(const std::string& filename, bool isFile);
  static bool FileExists(const char* filename);
  static bool FileExists(const std::string& filename);
  //@}

  /**
   * Return true if the file is a directory
   */
  static bool FileIsDirectory(const std::string& name);

  /**
   * Given argv[0] for a unix program find the full path to a running
   * executable.  argv0 can be null for windows WinMain programs
   * in this case GetModuleFileName will be used to find the path
   * to the running executable.  If argv0 is not a full path,
   * then this will try to find the full path.  If the path is not
   * found false is returned, if found true is returned.  An error
   * message of the attempted paths is stored in errorMsg.
   * exeName is the name of the executable.
   * buildDir is a possibly null path to the build directory.
   * installPrefix is a possibly null pointer to the install directory.
   */
  static bool FindProgramPath(const char* argv0,
                              std::string& pathOut,
                              std::string& errorMsg,
                              const char* exeName = 0,
                              const char* buildDir = 0,
                              const char* installPrefix = 0);

  /**
   * Get current working directory CWD
   */
  static std::string GetCurrentWorkingDirectory(bool collapse =true);

  /**
   * Given the path to a program executable, get the directory part of
   * the path with the file stripped off.  If there is no directory
   * part, the empty string is returned.
   */
  static std::string GetProgramPath(const std::string&);

protected:
  vtkPSystemTools() {}
  ~vtkPSystemTools() {}

private:
  vtkPSystemTools(const vtkPSystemTools&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPSystemTools&) VTK_DELETE_FUNCTION;
};

#endif
