/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExodusIIReader.h

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

// .NAME vtkPExodusIIReader - Read Exodus II files (.exii)
// .SECTION Description
// vtkPExodusIIReader is a unstructured grid source object that reads
// ExodusII files. Most of the meta data associated with the
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


#ifndef vtkPExodusIIReader_h
#define vtkPExodusIIReader_h

#include "vtkIOParallelExodusModule.h" // For export macro
#include "vtkExodusIIReader.h"

#include <vector> // Required for vector

class vtkTimerLog;
class vtkMultiProcessController;

class VTKIOPARALLELEXODUS_EXPORT vtkPExodusIIReader : public vtkExodusIIReader
{
public:
  static vtkPExodusIIReader* New();
  vtkTypeMacro(vtkPExodusIIReader,vtkExodusIIReader);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/get the communication object used to relay a list of files
  // from the rank 0 process to all others. This is the only interprocess
  // communication required by vtkPExodusIIReader.
  void SetController(vtkMultiProcessController* c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // These methods tell the reader that the data is distributed across
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
  void SetFileRange( int, int );
  void SetFileRange( int* r ) { this->SetFileRange( r[0], r[1] ); }
  vtkGetVector2Macro(FileRange,int);

  // Description:
  //   Provide an arbitrary list of file names instead of a prefix,
  //   pattern and range.  Overrides any prefix, pattern and range
  //   that is specified.  vtkPExodusIIReader makes it's own copy
  //   of your file names.
  void SetFileNames( int nfiles, const char** names );

  virtual void SetFileName( const char* name );

  // Description:
  //   Return pointer to list of file names set in SetFileNames
  char** GetFileNames() { return this->FileNames; }

  // Description:
  //   Return number of file names set in SetFileNames
  int GetNumberOfFileNames() { return this->NumberOfFileNames; }

  // Description:
  //   Return the number of files to be read.
  vtkGetMacro(NumberOfFiles,int);

  virtual vtkIdType GetTotalNumberOfElements();
  virtual vtkIdType GetTotalNumberOfNodes();

  // Description:
  // Calls UpdateTimeInformation() on all serial readers so they'll re-read
  // their time info from the file.
  // The last time step that they all have in common
  // is stored in LastCommonTimeStep, which is used in RequestInformation()
  // to override the output time-specific information keys with the range
  // of times that ALL readers can actually read.
  virtual void UpdateTimeInformation();

  // Description:
  // Sends metadata (that read from the input file, not settings modified
  // through this API) from the rank 0 node to all other processes in a job.
  virtual void Broadcast( vtkMultiProcessController* ctrl );

  //Description:
  //The size of the variable cache in MegaByes. This represents the maximum
  //size of cache that a single partition reader can have while reading. When
  //a reader is finished its cache size will be set to a fraction of this based
  //on the number of partitions.
  //The Default for this is 100MiB.
  //Note that because each reader still holds
  //a fraction of the cache size after reading the total amount of data cached
  //can be at most twice this size.
  vtkGetMacro(VariableCacheSize,double);
  vtkSetMacro(VariableCacheSize,double);

protected:
  vtkPExodusIIReader();
  ~vtkPExodusIIReader();

  // Description:
  // Try to "guess" the pattern of files.
  int DeterminePattern( const char* file );
  static int DetermineFileId( const char* file );

  //holds the size of the variable cache in GigaBytes
  double VariableCacheSize;

  // **KEN** Previous discussions concluded with std classes in header
  // files is bad.  Perhaps we should change ReaderList.

  vtkMultiProcessController* Controller;
  vtkIdType ProcRank;
  vtkIdType ProcSize;
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
//BTX
  std::vector<vtkExodusIIReader*> ReaderList;
  std::vector<int> NumberOfPointsPerFile;
  std::vector<int> NumberOfCellsPerFile;
//ETX

  int LastCommonTimeStep;

  int Timing;
  vtkTimerLog *TimerLog;

  int RequestInformation( vtkInformation*, vtkInformationVector**, vtkInformationVector* );
  int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

private:
  vtkPExodusIIReader( const vtkPExodusIIReader& ); // Not implemented
  void operator = ( const vtkPExodusIIReader& ); // Not implemented
};

#endif
