/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortFileNames.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSortFileNames
 * @brief   Group and sort a set of filenames
 *
 * vtkSortFileNames will take a list of filenames (e.g. from
 * a file load dialog) and sort them into one or more series.  If
 * the input list of filenames contains any directories, these can
 * be removed before sorting using the SkipDirectories flag.  This
 * class should be used where information about the series groupings
 * can be determined by the filenames, but it might not be successful
 * in cases where the information about the series groupings is
 * stored in the files themselves (e.g DICOM).
 * @sa
 * vtkImageReader2
*/

#ifndef vtkSortFileNames_h
#define vtkSortFileNames_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkStringArray;

// this is a helper class defined in the .cxx file
class vtkStringArrayVector;

class VTKIOCORE_EXPORT vtkSortFileNames : public vtkObject
{
public:

  vtkTypeMacro(vtkSortFileNames,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSortFileNames *New();

  //@{
  /**
   * Sort the file names into groups, according to similarity in
   * filename name and path.  Files in different directories,
   * or with different extensions, or which do not fit into the same
   * numbered series will be placed into different groups.  This is
   * off by default.
   */
  vtkSetMacro(Grouping, vtkTypeBool);
  vtkGetMacro(Grouping, vtkTypeBool);
  vtkBooleanMacro(Grouping, vtkTypeBool);
  //@}

  //@{
  /**
   * Sort the files numerically, rather than lexicographically.
   * For filenames that contain numbers, this means the order will be
   * ["file8.dat", "file9.dat", "file10.dat"]
   * instead of the usual alphabetic sorting order
   * ["file10.dat" "file8.dat", "file9.dat"].
   * NumericSort is off by default.
   */
  vtkSetMacro(NumericSort, vtkTypeBool);
  vtkGetMacro(NumericSort, vtkTypeBool);
  vtkBooleanMacro(NumericSort, vtkTypeBool);
  //@}

  //@{
  /**
   * Ignore case when sorting.  This flag is honored by both
   * the sorting and the grouping. This is off by default.
   */
  vtkSetMacro(IgnoreCase, vtkTypeBool);
  vtkGetMacro(IgnoreCase, vtkTypeBool);
  vtkBooleanMacro(IgnoreCase, vtkTypeBool);
  //@}

  //@{
  /**
   * Skip directories. If this flag is set, any input item that
   * is a directory rather than a file will not be included in
   * the output.  This is off by default.
   */
  vtkSetMacro(SkipDirectories, vtkTypeBool);
  vtkGetMacro(SkipDirectories, vtkTypeBool);
  vtkBooleanMacro(SkipDirectories, vtkTypeBool);
  //@}

  //@{
  /**
   * Set a list of file names to group and sort.
   */
  void SetInputFileNames(vtkStringArray *input);
  vtkGetObjectMacro(InputFileNames, vtkStringArray);
  //@}

  /**
   * Get the full list of sorted filenames.
   */
  virtual vtkStringArray *GetFileNames();

  /**
   * Get the number of groups that the names were split into, if
   * grouping is on.  The filenames are automatically split into
   * groups, where the filenames in each group will be identical
   * except for their series numbers.  If grouping is not on, this
   * method will return zero.
   */
  virtual int GetNumberOfGroups();

  /**
   * Get the Nth group of file names.  This method should only
   * be used if grouping is on.  If grouping is off, it will always
   * return null.
   */
  virtual vtkStringArray *GetNthGroup(int i);

  /**
   * Update the output filenames from the input filenames.
   * This method is called automatically by GetFileNames()
   * and GetNumberOfGroups() if the input names have changed.
   */
  virtual void Update();

protected:
  vtkSortFileNames();
  ~vtkSortFileNames() override;

  vtkTypeBool NumericSort;
  vtkTypeBool IgnoreCase;
  vtkTypeBool Grouping;
  vtkTypeBool SkipDirectories;

  vtkTimeStamp UpdateTime;

  vtkStringArray *InputFileNames;
  vtkStringArray *FileNames;
  vtkStringArrayVector *Groups;

  /**
   * Fill the output.
   */
  virtual void Execute();

  /**
   * Sort the input string array, and append the results to the output.
   */
  virtual void SortFileNames(vtkStringArray *input, vtkStringArray *output);

  /**
   * Separate a string array into groups and append them to the output.
   */
  virtual void GroupFileNames(vtkStringArray *input,
                              vtkStringArrayVector *output);


private:
  vtkSortFileNames(const vtkSortFileNames&) = delete;
  void operator=(const vtkSortFileNames&) = delete;
};

#endif
