/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExodusReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkPExodusReader - Read exodus 2 files .ex2
// .SECTION Description
// vtkPExodusReader is a unstructured grid source object that reads
// PExodusReaderII files. Most of the meta data associated with the
// file is loaded when UpdateInformation is called. This includes
// information like Title, number of blocks, number and names of
// arrays. This data can be retrieved from methods in this
// reader. Separate arrays that are meant to be a single vector, are
// combined internally for convenience. To be combined, the array
// names have to be identical except for a trailing X,Y and Z (or
// x,y,z). By default all cell and point arrays are loaded. However,
// the user can flag arrays not to load with the methods
// "SetPointDataArrayLoadFlag" and "SetCellDataArrayLoadFlag". The
// reader responds to piece requests by loading only a range of the
// possible blocks. Unused points are filtered out internally.


#ifndef __vtkPExodusReader_h
#define __vtkPExodusReader_h

#include "vtkExodusReader.h"

#include <vtkstd/vector> // Required for vector

class vtkTimerLog;

class VTK_HYBRID_EXPORT vtkPExodusReader : public vtkExodusReader 
{
public:
  static vtkPExodusReader *New();
  vtkTypeMacro(vtkPExodusReader,vtkExodusReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These methods tell the reader that the data is ditributed across
  // multiple files. This is for distributed execution. It this case,
  // pieces are mapped to files. The pattern should have one %d to
  // format the file number. FileNumberRange is used to generate file
  // numbers. I was thinking of having an arbitrary list of file
  // numbers. This may happen in the future. (That is why there is no
  // GetFileNumberRange method.
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);

  // Description:
  // Set the range of files that are being loaded. The range for single
  // file should add to 0.
  void SetFileRange(int,int);
  void SetFileRange(int* r) { this->SetFileRange(r[0], r[1]); }
  vtkGetVector2Macro(FileRange, int);

  // Description:
  //   Provide an arbitrary list of file names instead of a prefix,
  //   pattern and range.  Overrides any prefix, pattern and range
  //   that is specified.  vtkPExodusReader makes it's own copy
  //   of your file names.
  void SetFileNames(int nfiles, const char **names);

  virtual void SetFileName(const char *name);

  // Description:
  //   Return pointer to list of file names set in SetFileNames
  char **GetFileNames(){return this->FileNames;}

  // Description:
  //   Return number of file names set in SetFileNames
  int GetNumberOfFileNames(){return this->NumberOfFileNames;}

  // Description:
  //   Return the number of files to be read.
  vtkGetMacro(NumberOfFiles, int);

  // Description:
  // Extra cell data array that can be generated.  By default, this array
  // is OFF.  The value of the array is the integer id which is part of
  // the name of the file from which the cell was read.
  // The name of the array is "vtkFileId".

  void SetGenerateFileIdArray(int flag);
  vtkGetMacro(GenerateFileIdArray, int);
  vtkBooleanMacro(GenerateFileIdArray, int); 
  virtual int GetTotalNumberOfElements();
  virtual int GetTotalNumberOfNodes();


//begin USE_EXO_DSP_FILTERS
  int GetNumberOfVariableArrays();
  const char *GetVariableArrayName(int a_which);
  void EnableDSPFiltering();
  void AddFilter(vtkDSPFilterDefinition *a_filter);
  void StartAddingFilter();
  void AddFilterInputVar(char *name);
  void AddFilterOutputVar(char *name);
  void AddFilterNumeratorWeight(double weight);
  void AddFilterForwardNumeratorWeight(double weight);
  void AddFilterDenominatorWeight(double weight);
  void FinishAddingFilter();
  void RemoveFilter(char *a_outputVariableName);
//end USE_EXO_DSP_FILTERS


protected:
  vtkPExodusReader();
  ~vtkPExodusReader();

//begin USE_EXO_DSP_FILTERS
  void GetDSPOutputArrays(int exoid, vtkUnstructuredGrid* output);
//end USE_EXO_DSP_FILTERS

  // Description:
  // Try to "guess" the pattern of files.
  int DeterminePattern(const char* file);
  static int DetermineFileId(const char* file);

  // This method sets up a ugrid with
  // all meta data but zero cells
  void SetUpEmptyGrid();

  // **KEN** Previous discussions concluded with std classes in header
  // files is bad.  Perhaps we should change readerList.

  char* FilePattern;
  char* CurrentFilePattern;
  char* FilePrefix;
  char* CurrentFilePrefix;
  char* MultiFileName;
  int FileRange[2];
  int CurrentFileRange[2];
  int NumberOfFiles;
  char **FileNames;
  int NumberOfFileNames;
  int GenerateFileIdArray;
//BTX
  vtkstd::vector<vtkExodusReader*> readerList;
//ETX

  int Timing;
  vtkTimerLog *TimerLog;

  int RequestInformation(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkPExodusReader(const vtkPExodusReader&); // Not implemented
  void operator=(const vtkPExodusReader&); // Not implemented
};

#endif
