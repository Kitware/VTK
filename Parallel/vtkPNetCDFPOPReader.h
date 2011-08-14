/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtPkNetCDFPOPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPNetCDFPOPReader - read NetCDF files in parallel with MPI
// .Author Ross Miller 03.14.2011
// .SECTION Description
// vtkNetCDFPOPReader is a source object that reads NetCDF files.
// It should be able to read most any NetCDF file that wants to output a
// rectilinear grid.  The ordering of the variables is changed such that
// the NetCDF x, y, z directions correspond to the vtkRectilinearGrid
// z, y, x directions, respectively.  The striding is done with
// respect to the vtkRectilinearGrid ordering.  Additionally, the
// z coordinates of the vtkRectilinearGrid are negated so that the
// first slice/plane has the highest z-value and the last slice/plane
// has the lowest z-value.

#ifndef __vtkPNetCDFPOPReader_h
#define __vtkPNetCDFPOPReader_h

#include "vtkRectilinearGridAlgorithm.h"

class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkMPIController;
class vtkPNetCDFPOPReaderInternal;

class VTK_PARALLEL_EXPORT vtkPNetCDFPOPReader : public vtkRectilinearGridAlgorithm
{
public:
  vtkTypeMacro(vtkPNetCDFPOPReader,vtkRectilinearGridAlgorithm);
  static vtkPNetCDFPOPReader *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //The file to open
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  //Description:
  //Enable subsampling in i,j and k dimensions in the vtkRectilinearGrid
  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);

  // Description:
  // Variable array selection.
  virtual int GetNumberOfVariableArrays();
  virtual const char *GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char *name);
  virtual void SetVariableArrayStatus(const char *name, int status);

  // Description:
  // Set/Get the vtkMultiProcessController which will handle communications
  // for the parallel rendering.  Added by RGM.
  vtkGetObjectMacro(Controller, vtkMPIController);
  void SetController(vtkMPIController *controller);

  // Description:
  // Set the list of processes that will actually open the file.
  void SetReaderRanks(vtkIdList*);

  // Description:
  // Set the number of processes that will actually read the file.
  // If this is less than 1 it will be set to 1 and if it is greater
  // than the number of processes in the communicator than it will
  // be set to the number of processes in the communicator.
  // The reader processes will be set in a round robin fashion (e.g.
  // if number = 5 and number of processes = 14 then processes
  // 0, 3, 6, 9, and 12 will be the reader processes.
  void SetNumberOfReaderProcesses(int number);

protected:
  vtkPNetCDFPOPReader();
  ~vtkPNetCDFPOPReader();

  int RequestData(vtkInformation*,vtkInformationVector**,
                  vtkInformationVector*);
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  // Description:
  // Given the number of processes that should be assigned as
  // reader processes, generate a list of ranks of those
  // processes in a round robin fashion.
  void AssignRoundRobin(int numReaders, vtkIdList* readerRanks);

  // Functions added by RGM....

  // Description:
  // Helper function for RequestData:  Reads part of the netCDF
  // file and sends sub-arrays to all ranks that need that data
  int ReadAndSend( vtkInformation* outInfo, int varID);

  // Description:
  // Returns the MPI rank of the process that should read the specified depth
  int ReaderForDepth( unsigned depth);

  bool IsReaderRank();
  bool IsFirstReaderRank();

  // end of functions added by RGM...

  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  static void EventCallback(vtkObject* caller, unsigned long eid,
                            void* clientdata, void* calldata);

  vtkCallbackCommand* SelectionObserver;

  char *FileName;
  char *OpenedFileName;
  vtkSetStringMacro(OpenedFileName);

  int NCDFFD; //netcdf file descriptor

  int Stride[3];

  vtkMPIController *Controller;

private:
  vtkPNetCDFPOPReader(const vtkPNetCDFPOPReader&);  // Not implemented.
  void operator=(const vtkPNetCDFPOPReader&);  // Not implemented.

  vtkPNetCDFPOPReaderInternal* Internals;
};
#endif
