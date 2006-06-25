/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortSeriesFileNames.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSortSeriesFileNames - Group and sort a set of filenames
// .SECTION Description
// vtkSortSeriesFileNames will take a list of filenames (e.g. from
// a file load dialog) and sort them into one or more series.  If
// the input list of filenames contains any directories, these are
// removed before sorting. This class should be specilized for file
// types where information about the groupings is stored in the files
// themselves, e.g for DICOM.
// .SECTION See Also
// vtkImageReader2

#ifndef __vtkSortSeriesFileNames_h
#define __vtkSortSeriesFileNames_h

#include "vtkObject.h"
#include "vtkTimeStamp.h"
#include "vtkStringArray.h"

//BTX
// this is a helper class defined in the .cxx file
class vtkStringArrayVector;
//ETX

class VTK_IO_EXPORT vtkSortSeriesFileNames : public vtkObject
{
public:
  
  vtkTypeRevisionMacro(vtkSortSeriesFileNames,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);   
  static vtkSortSeriesFileNames *New();

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
  vtkSetObjectMacro(InputFileNames, vtkStringArray);
  vtkGetObjectMacro(InputFileNames, vtkStringArray);
  
  // Description:
  // Get the sorted names.  If Grouping is on, you must supply
  // an integer that specifies which group of names to get.
  virtual vtkStringArray *GetFileNames();
  virtual vtkStringArray *GetFileNames(int i);

  // Description:
  // Get the number of groups that the names were split into.
  // The filenames are automatically split into groups according
  // to file type, or according to series numbering.
  virtual int GetNumberOfGroups();

  // Description:
  // Update the output filenames from the input filenames.
  // This method is called automatically by GetFileNames()
  // and GetNumberOfGroups() if the input names have changed.
  virtual void Update();

protected:
  vtkSortSeriesFileNames();
  ~vtkSortSeriesFileNames();

  int NumericSort;
  int IgnoreCase;
  int Grouping;
  int SkipDirectories;

  vtkTimeStamp UpdateTime;

  vtkStringArray *InputFileNames;
  vtkStringArrayVector *FileNames;

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
  vtkSortSeriesFileNames(const vtkSortSeriesFileNames&);  // Not implemented.
  void operator=(const vtkSortSeriesFileNames&);  // Not implemented.
};

#endif
