/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlobFileNames.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGlobFileNames
 * @brief   find files that match a wildcard pattern
 *
 * vtkGlobFileNames is a utility for finding files and directories
 * that match a given wildcard pattern.  Allowed wildcards are
 * *, ?, [...], [!...]. The "*" wildcard matches any substring,
 * the "?" matches any single character, the [...] matches any one of
 * the enclosed characters, e.g. [abc] will match one of a, b, or c,
 * while [0-9] will match any digit, and [!...] will match any single
 * character except for the ones within the brackets.  Special
 * treatment is given to "/" (or "\" on Windows) because these are
 * path separators.  These are never matched by a wildcard, they are
 * only matched with another file separator.
 * @warning
 * This function performs case-sensitive matches on UNIX and
 * case-insensitive matches on Windows.
 * @sa
 * vtkDirectory
*/

#ifndef vtkGlobFileNames_h
#define vtkGlobFileNames_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkStringArray;

class VTKIOCORE_EXPORT vtkGlobFileNames : public vtkObject
{
public:
  //@{
  /**
   * Return the class name as a string.
   */
  vtkTypeMacro(vtkGlobFileNames,vtkObject);
  //@}

  /**
   * Create a new vtkGlobFileNames object.
   */
  static vtkGlobFileNames *New();

  /**
   * Print directory to stream.
   */
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Reset the glob by clearing the list of output filenames.
   */
  void Reset();

  //@{
  /**
   * Set the directory in which to perform the glob.  If this is
   * not set, then the current directory will be used.  Also, if
   * you use a glob pattern that contains absolute path (one that
   * starts with "/" or a drive letter) then that absolute path
   * will be used and Directory will be ignored.
   */
  vtkSetStringMacro(Directory);
  vtkGetStringMacro(Directory);
  //@}

  /**
   * Search for all files that match the given expression,
   * sort them, and add them to the output.  This method can
   * be called repeatedly to add files matching additional patterns.
   * Returns 1 if successful, otherwise returns zero.
   */
  int AddFileNames(const char* pattern);

  //@{
  /**
   * Recurse into subdirectories.
   */
  vtkSetMacro(Recurse, int);
  vtkBooleanMacro(Recurse, int);
  vtkGetMacro(Recurse, int);
  //@}

  /**
   * Return the number of files found.
   */
  int GetNumberOfFileNames();

  /**
   * Return the file at the given index, the indexing is 0 based.
   */
  const char* GetNthFileName(int index);

  //@{
  /**
   * Get an array that contains all the file names.
   */
  vtkGetObjectMacro(FileNames, vtkStringArray);
  //@}

protected:
  //@{
  /**
   * Set the wildcard pattern.
   */
  vtkSetStringMacro(Pattern);
  vtkGetStringMacro(Pattern);
  //@}

  vtkGlobFileNames();
  ~vtkGlobFileNames();

private:
  char* Directory;          // Directory for search.
  char* Pattern;            // Wildcard pattern
  int Recurse;              // Recurse into subdirectories
  vtkStringArray *FileNames;    // VTK array of files

private:
  vtkGlobFileNames(const vtkGlobFileNames&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGlobFileNames&) VTK_DELETE_FUNCTION;
};

#endif
