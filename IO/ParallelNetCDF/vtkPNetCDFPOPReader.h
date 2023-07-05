// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPNetCDFPOPReader
 * @brief   read NetCDF files in parallel with MPI
 * .Author Ross Miller 03.14.2011
 *
 * vtkNetCDFPOPReader is a source object that reads NetCDF files.
 * It should be able to read most any NetCDF file that wants to output a
 * rectilinear grid.  The ordering of the variables is changed such that
 * the NetCDF x, y, z directions correspond to the vtkRectilinearGrid
 * z, y, x directions, respectively.  The striding is done with
 * respect to the vtkRectilinearGrid ordering.  Additionally, the
 * z coordinates of the vtkRectilinearGrid are negated so that the
 * first slice/plane has the highest z-value and the last slice/plane
 * has the lowest z-value.
 */

#ifndef vtkPNetCDFPOPReader_h
#define vtkPNetCDFPOPReader_h

#include "vtkIOParallelNetCDFModule.h" // For export macro
#include "vtkRectilinearGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkMPIController;
class vtkPNetCDFPOPReaderInternal;

class VTKIOPARALLELNETCDF_EXPORT vtkPNetCDFPOPReader : public vtkRectilinearGridAlgorithm
{
public:
  vtkTypeMacro(vtkPNetCDFPOPReader, vtkRectilinearGridAlgorithm);
  static vtkPNetCDFPOPReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The file to open
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Enable subsampling in i,j and k dimensions in the vtkRectilinearGrid
   */
  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);
  ///@}

  ///@{
  /**
   * Variable array selection.
   */
  virtual int GetNumberOfVariableArrays();
  virtual const char* GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char* name);
  virtual void SetVariableArrayStatus(const char* name, int status);
  ///@}

  /**
   * Set ranks that will actually open and read the netCDF files.  Pass in
   * null to chose reasonable defaults)
   */
  void SetReaderRanks(vtkIdList*);

  // Set/Get the vtkMultiProcessController which will handle communications
  // for the parallel rendering.
  vtkGetObjectMacro(Controller, vtkMPIController);
  void SetController(vtkMPIController* controller);

protected:
  vtkPNetCDFPOPReader();
  ~vtkPNetCDFPOPReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Helper function for RequestData:  Reads part of the netCDF
  // file and sends sub-arrays to all ranks that need that data
  int ReadAndSend(vtkInformation* outInfo, int varID);

  // Returns the MPI rank of the process that should read the specified depth
  int ReaderForDepth(unsigned depth);

  bool IsReaderRank();
  bool IsFirstReaderRank();

  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void EventCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  vtkCallbackCommand* SelectionObserver;

  char* FileName;
  char* OpenedFileName;
  vtkSetFilePathMacro(OpenedFileName);

  int NCDFFD; // netcdf file descriptor

  int Stride[3];

  vtkMPIController* Controller;

private:
  vtkPNetCDFPOPReader(const vtkPNetCDFPOPReader&) = delete;
  void operator=(const vtkPNetCDFPOPReader&) = delete;

  vtkPNetCDFPOPReaderInternal* Internals;
};
VTK_ABI_NAMESPACE_END
#endif
