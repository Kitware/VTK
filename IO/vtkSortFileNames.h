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
// .NAME vtkSortFileNames - Group and sort a set of filenames
// .SECTION Description
// vtkSortFileNames will take a list of filenames (e.g. from
// a file load dialog) and sort them into one or more series.  If
// the input list of filenames contains any directories, these are
// removed before sorting. This class should be specilized for file
// types where information about the groupings is stored in the files
// themselves, e.g for DICOM.
// .SECTION See Also
// vtkImageReader2

#ifndef __vtkSortFileNames_h
#define __vtkSortFileNames_h

#include "vtkObject.h"

class vtkStringArray;

//BTX
// this is a helper class defined in the .cxx file
class vtkStringArrayVector;
//ETX

class VTK_IO_EXPORT vtkSortFileNames : public vtkObject
{
public:
  
  vtkTypeRevisionMacro(vtkSortFileNames,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);   
  static vtkSortFileNames *New();

  // Description:
  // Sort the file names into groups, according to similarity in
  // filename name and path.  Files in different directories,
  // or with different extensions, or which do not fit into the same
  // numbered series will be placed into different groups.  This is
  // off by default.
  vtkSetMacro(Grouping, int);
  vtkGetMacro(Grouping, int);
  vtkBooleanMacro(Grouping, int);

  // Description:
  // Sort the files numerically, rather than lexicographically.
  // For filenames that contain numbers, this means the order will be
  // ["file8.dat", "file9.dat", "file10.dat"]
  // instead of the usual alphabetic sorting order
  // ["file10.dat" "file8.dat", "file9.dat"].
  // NumericSort is off by default.
  vtkSetMacro(NumericSort, int);
  vtkGetMacro(NumericSort, int);
  vtkBooleanMacro(NumericSort, int);

  // Description:
  // Ignore case when sorting.  This flag is honored by both
  // the sorting and the grouping. This is off by default.
  vtkSetMacro(IgnoreCase, int);
  vtkGetMacro(IgnoreCase, int);
  vtkBooleanMacro(IgnoreCase, int);

  // Description:
  // Skip directories. If this flag is set, any path that is
  // a directory rather than a file will not be included in
  // the output.  This is off by default.
  vtkSetMacro(SkipDirectories, int);
  vtkGetMacro(SkipDirectories, int);
  vtkBooleanMacro(SkipDirectories, int);

  // Description:
  // Set a list of file names to group and sort.
  void SetInputFileNames(vtkStringArray *input);
  vtkGetObjectMacro(InputFileNames, vtkStringArray);
  
  // Description:
  // Get the sorted names.  
  virtual vtkStringArray *GetFileNames();

  // Description:
  // Get the number of groups that the names were split into.
  // The filenames are automatically split into groups according
  // to file type, or according to series numbering.  If grouping
  // is not on, this method will return zero.
  virtual int GetNumberOfGroups();

  // Description:
  // Get the Nth group of file names.  This method is only
  // to be used if grouping is on.
  virtual vtkStringArray *GetNthGroup(int i);

  // Description:
  // Update the output filenames from the input filenames.
  // This method is called automatically by GetFileNames()
  // and GetNumberOfGroups() if the input names have changed.
  virtual void Update();

protected:
  vtkSortFileNames();
  ~vtkSortFileNames();

  int NumericSort;
  int IgnoreCase;
  int Grouping;
  int SkipDirectories;

  vtkTimeStamp UpdateTime;

  vtkStringArray *InputFileNames;
  vtkStringArray *FileNames;
  vtkStringArrayVector *Groups;

  // Description:
  // Fill the output.
  virtual void Execute();

  // Description:
  // Sort the input string array, put the results in the output, where
  // the input and output can be the same.
  virtual void SortFileNames(vtkStringArray *input, vtkStringArray *output);

  // Description:
  // Separate a string array into several groups of string arrays.
  virtual void GroupFileNames(vtkStringArray *arglist,
                              vtkStringArrayVector *groupedFiles);
  
  
private:
  vtkSortFileNames(const vtkSortFileNames&);  // Not implemented.
  void operator=(const vtkSortFileNames&);  // Not implemented.
};

#endif
