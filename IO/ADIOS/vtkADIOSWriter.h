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
/**
 * @class   vtkADIOSWriter
 * @brief   Write ADIOS files.
 *
 * vtkADIOSWriter is the base class for all ADIOS writers
*/

#ifndef vtkADIOSWriter_h
#define vtkADIOSWriter_h

#include <map>    // For independently stepped array indexing
#include <string> // For independently stepped array indexing
#include <vector> // For independently stepped array indexing

#include "vtkDataObjectAlgorithm.h"
#include "vtkMultiProcessController.h" // For the MPI controller member
#include "vtkSetGet.h"                 // For property get/set macros

#include "ADIOSDefs.h"                 // For enum definitions

#include "vtkIOADIOSModule.h"          // For export macro

namespace ADIOS
{
  class Writer;
}

class vtkAbstractArray;
class vtkCellArray;
class vtkDataArray;
class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkImageData;
class vtkPolyData;
class vtkUnstructuredGrid;

class VTKIOADIOS_EXPORT vtkADIOSWriter : public vtkDataObjectAlgorithm
{
public:
  static vtkADIOSWriter* New();
  vtkTypeMacro(vtkADIOSWriter,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  const char* GetDefaultFileExtension();

  //@{
  /**
   * Get/Set the output filename
   */
  vtkGetStringMacro(FileName)
  vtkSetStringMacro(FileName)
  //@}

  //@{
  /**
   * Get/Set the ADIOS transport method.
   */
  vtkGetMacro(TransportMethod, int);
  vtkSetClampMacro(TransportMethod, int,
                   static_cast<int>(ADIOS::TransportMethod_NULL),
                   static_cast<int>(ADIOS::TransportMethod_NetCDF4));
  void SetTransportMethodToNULL()         { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_NULL)); }
  void SetTransportMethodToPOSIX()        { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_POSIX)); }
  void SetTransportMethodToMPI()          { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_MPI)); }
  void SetTransportMethodToMPILustre()    { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_MPI_LUSTRE)); }
  void SetTransportMethodToMPIAggregate() { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_MPI_AGGREGATE)); }
  void SetTransportMethodToVarMerge()     { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_VAR_MERGE)); }
  void SetTransportMethodToDataSpaces()   { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_DataSpaces)); }
  void SetTransportMethodToDIMES()        { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_DIMES)); }
  void SetTransportMethodToFlexPath()     { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_FlexPath)); }
  void SetTransportMethodToPHDF5()        { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_PHDF5)); }
  void SetTransportMethodToNetCDF4()      { this->SetTransportMethod(static_cast<int>(ADIOS::TransportMethod_NetCDF4)); }
  //@}

  //@{
  /**
   * Get/Set arguments to the ADIOS transport method (default is "").  If
   * called, it must be called BEFORE SetController
   */
  vtkSetStringMacro(TransportMethodArguments)
  vtkGetStringMacro(TransportMethodArguments)
  //@}

  //@{
  /**
   * Get/Set the data transformation.
   */
  vtkGetMacro(Transform, int);
  vtkSetClampMacro(Transform, int,
                   static_cast<int>(ADIOS::Transform_NONE),
                   static_cast<int>(ADIOS::Transform_SZIP));
  void SetTransformToNone()  { this->SetTransform(static_cast<int>(ADIOS::Transform_NONE)); }
  void SetTransformToZLib()  { this->SetTransform(static_cast<int>(ADIOS::Transform_ZLIB)); }
  void SetTransformToBZip2() { this->SetTransform(static_cast<int>(ADIOS::Transform_BZLIB2)); }
  void SetTransformToSZip()  { this->SetTransform(static_cast<int>(ADIOS::Transform_SZIP)); }
  //@}

  //@{
  /**
   * Controls whether writer automatically writes all input time steps, or
   * just the timestep that is currently on the input.
   * Default is OFF.
   */
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  vtkBooleanMacro(WriteAllTimeSteps, bool);
  //@}

  //@{
  /**
   * Set the MPI controller.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * The main interface which triggers the writer to start
   */
  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*);

  /**
   * Declare data if necessary and write the current step to the output stream
   */
  void Write() { return this->Update(); }

protected:

  //@{
  /**
   * Define a VTK data type
   */
  void Define(const std::string& path, const vtkAbstractArray* value);
  void Define(const std::string& path, const vtkDataArray* value);
  void Define(const std::string& path, const vtkCellArray* value);
  void Define(const std::string& path, const vtkFieldData* value);
  void Define(const std::string& path, const vtkDataSet* value);
  void Define(const std::string& path, const vtkImageData* value);
  void Define(const std::string& path, const vtkPolyData* value);
  void Define(const std::string& path, const vtkUnstructuredGrid* value);
  //@}

  //@{
  /**
   * Open a file and prepare for writing already defined variables.
   * NOTE: The data is declared only once but the file must be opened and
   * closed for every timestep.  Data is not guaranteed to be flushed until
   * application exit and final ADIOS deconstruction.
   */
  void OpenFile();
  void CloseFile();
  //@}

  //@{
  /**
   * Write a previously defined VTK data type
   */
  void Write(const std::string& path, const vtkAbstractArray* value);
  void Write(const std::string& path, const vtkDataArray* value);
  void Write(const std::string& path, const vtkCellArray* value);
  void Write(const std::string& path, const vtkFieldData* value);
  void Write(const std::string& path, const vtkDataSet* value);
  void Write(const std::string& path, const vtkImageData* value);
  void Write(const std::string& path, const vtkPolyData* value);
  void Write(const std::string& path, const vtkUnstructuredGrid* value);
  //@}

  char *FileName;
  int TransportMethod;
  char *TransportMethodArguments;
  int Transform;
  int Rank;
  int CurrentStep;
  vtkMultiProcessController *Controller;
  ADIOS::Writer *Writer;

  vtkADIOSWriter();
  ~vtkADIOSWriter();

protected:
  // Used to implement vtkAlgorithm

  int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **input,
                                 vtkInformationVector *output);
  virtual int RequestUpdateExtent(vtkInformation *request,
                                  vtkInformationVector **input,
                                  vtkInformationVector *output);
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **input,
                          vtkInformationVector *output);

  int NumberOfPieces;
  int RequestPiece;
  int NumberOfGhostLevels;
  bool WriteAllTimeSteps;
  std::vector<double> TimeSteps;
  int CurrentTimeStepIndex;
  int RequestExtent[6];

  // Used to determine whether or not the data getting written is stale
  bool UpdateMTimeTable(const std::string& path, const vtkObject* value);
  std::map<std::string, unsigned long> LastUpdated;
private:
  bool WriteInternal();

  template<typename T>
  bool DefineAndWrite(vtkDataObject *input);

  vtkADIOSWriter(const vtkADIOSWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkADIOSWriter&) VTK_DELETE_FUNCTION;
};

#endif
