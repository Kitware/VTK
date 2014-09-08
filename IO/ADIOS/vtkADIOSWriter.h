/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkADIOSWriter - Write ADIOS files.
// .SECTION Description
// vtkADIOSWriter is the base class for all ADIOS writers

#ifndef __vtkADIOSWriter_h
#define __vtkADIOSWriter_h

#include <string> // For independently stepped array indexing
#include <vector> // For independently stepped array indexing
#include <map>    // For independently stepped array indexing

#include <vtkAlgorithm.h>
#include <vtkMultiProcessController.h> // For the MPI controller member
#include <vtkSetGet.h>                 // For property get/set macros

#include <vtkIOADIOSModule.h>          // For export macro

class ADIOSWriter;

class vtkAbstractArray;
class vtkCellArray;
class vtkDataArray;
class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkImageData;
class vtkPolyData;
class vtkUnstructuredGrid;

class VTKIOADIOS_EXPORT vtkADIOSWriter : public vtkAlgorithm
{
public:
  static vtkADIOSWriter* New();
  vtkTypeMacro(vtkADIOSWriter,vtkAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the output filename
  vtkGetStringMacro(FileName)
  vtkSetStringMacro(FileName)

  // Keep in sync with ADIOSDefs.h
  enum
  {
    NULLTransport = 0,
    POSIX         = 1,
    MPI           = 2,
    MPILustre     = 3,
    MPIAggregate  = 4,
    VarMerge      = 5,
    DataSpaces    = 6,
    DIMES         = 7,
    FlexPath      = 8,
    PHDF5         = 9,
    NetCDF4       = 10
  };

  // Description:
  // Get/Set the ADIOS transport method.
  vtkGetMacro(TransportMethod, int);
  vtkSetClampMacro(TransportMethod, int, NULLTransport, NetCDF4);
  void SetTransportMethodToNULL()         { this->SetTransportMethod(NULLTransport); }
  void SetTransportMethodToPOSIX()        { this->SetTransportMethod(POSIX); }
  void SetTransportMethodToMPI()          { this->SetTransportMethod(MPI); }
  void SetTransportMethodToMPILustre()    { this->SetTransportMethod(MPILustre); }
  void SetTransportMethodToMPIAggregate() { this->SetTransportMethod(MPIAggregate); }
  void SetTransportMethodToVarMerge()     { this->SetTransportMethod(VarMerge); }
  void SetTransportMethodToDataSpaces()   { this->SetTransportMethod(DataSpaces); }
  void SetTransportMethodToDIMES()        { this->SetTransportMethod(DIMES); }
  void SetTransportMethodToFlexPath()     { this->SetTransportMethod(FlexPath); }
  void SetTransportMethodToPHDF5()        { this->SetTransportMethod(PHDF5); }
  void SetTransportMethodToNetCDF4()      { this->SetTransportMethod(NetCDF4); }

  // Description:
  // Get/Set arguments to the ADIOS transport method (default is "").  If
  // called, it must be called BEFORE SetController
  vtkSetStringMacro(TransportMethodArguments)
  vtkGetStringMacro(TransportMethodArguments)

  enum
  {
    None  = 0,
    ZLib  = 1,
    BZip2 = 2,
    SZip  = 3
  };

  // Description:
  // Get/Set the data transformation.
  vtkGetMacro(Transform, int);
  vtkSetClampMacro(Transform, int, None, SZip)
  void SetTransformToNone()  { this->SetTransform(None); }
  void SetTransformToZLib()  { this->SetTransform(ZLib); }
  void SetTransformToBZip2() { this->SetTransform(BZip2); }
  void SetTransformToSZip()  { this->SetTransform(SZip); }

  enum
  {
    Always = 0,
    OnChange = 1
  };

  // Description:
  // Get/Set the write mode for array data
  vtkGetMacro(WriteMode, int);
  vtkSetClampMacro(WriteMode, int, Always, OnChange);
  void SetWriteModeToAlways()   { this->SetWriteMode(Always); }
  void SetWriteModeToOnChange() { this->SetWriteMode(OnChange); }

  //Description:
  //Controls whether writer automatically writes all input time steps, or
  //just the timestep that is currently on the input.
  //Default is OFF.
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  vtkBooleanMacro(WriteAllTimeSteps, bool);

  // Description:
  // Set the MPI controller.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // The main interface which triggers the writer to start
  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*);

  // Description:
  // Declare data if necessary and write the current step to the output stream
  void Write() { return this->Update(); }

protected:

  // Description:
  // Define a VTK data type
  void Define(const std::string& path, const vtkAbstractArray* value);
  void Define(const std::string& path, const vtkDataArray* value);
  void Define(const std::string& path, const vtkCellArray* value);
  void Define(const std::string& path, const vtkFieldData* value);
  void Define(const std::string& path, const vtkDataSet* value);
  void Define(const std::string& path, const vtkImageData* value);
  void Define(const std::string& path, const vtkPolyData* value);
  void Define(const std::string& path, const vtkUnstructuredGrid* value);

  // Description:
  // Open a file and prepare for writing already defined variables.
  // NOTE: The data is declared only once but the file must be opened and
  // closed for every timestep.  Data is not guaranteed to be flushed until
  // application exit and final ADIOS deconstruction.
  void OpenFile();
  void CloseFile();

  // Description:
  // Write a previously defined VTK data type
  void Write(const std::string& path, const vtkAbstractArray* value);
  void Write(const std::string& path, const vtkDataArray* value);
  void Write(const std::string& path, const vtkCellArray* value);
  void Write(const std::string& path, const vtkFieldData* value);
  void Write(const std::string& path, const vtkDataSet* value);
  void Write(const std::string& path, const vtkImageData* value);
  void Write(const std::string& path, const vtkPolyData* value);
  void Write(const std::string& path, const vtkUnstructuredGrid* value);

  char *FileName;
  int TransportMethod;
  char *TransportMethodArguments;
  int Transform;
  int WriteMode;
  int Rank;
  int CurrentStep;
  typedef std::map<std::string, size_t> NameIdMap;
  NameIdMap BlockStepIndexIdMap;
  std::vector<vtkTypeInt64> BlockStepIndex;
  vtkMultiProcessController *Controller;
  ADIOSWriter *Writer;
  int BLOCKDEBUG;

  vtkADIOSWriter();
  ~vtkADIOSWriter();

protected:
  // Used to implement vtkAlgorithm

  int FillInputPortInformation(int port, vtkInformation* info);

  bool RequestInformation(vtkInformation *request,
    vtkInformationVector **input, vtkInformationVector *output);
  bool RequestUpdateExtent(vtkInformation *request,
    vtkInformationVector **input, vtkInformationVector *output);
  bool RequestData(vtkInformation *request,
    vtkInformationVector **input, vtkInformationVector *output);

  int NumberOfPieces;
  int RequestPiece;
  int NumberOfGhostLevels;
  bool WriteAllTimeSteps;
  std::vector<double> TimeSteps;
  int CurrentTimeStepIndex;
  int RequestExtent[6];

  // Used to determine whether or not the data getting written is stale
  bool UpdateMTimeTable(const std::string path, const vtkObject* value);
  std::map<std::string, unsigned long> LastUpdated;
private:
  // Synchronize the block step index map across all processes
  std::string GatherBlockStepIdMap();

  bool WriteInternal();

  template<typename T>
  bool DefineAndWrite(vtkDataObject *input);

  vtkADIOSWriter(const vtkADIOSWriter&);  // Not implemented.
  void operator=(const vtkADIOSWriter&);  // Not implemented.
};

#endif
