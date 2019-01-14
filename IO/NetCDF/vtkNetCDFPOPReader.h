/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFPOPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkNetCDFPOPReader
 * @brief   read NetCDF files
 * .Author Joshua Wu 09.15.2009
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

#ifndef vtkNetCDFPOPReader_h
#define vtkNetCDFPOPReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkRectilinearGridAlgorithm.h"

class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkNetCDFPOPReaderInternal;

class VTKIONETCDF_EXPORT vtkNetCDFPOPReader : public vtkRectilinearGridAlgorithm
{
public:
  vtkTypeMacro(vtkNetCDFPOPReader,vtkRectilinearGridAlgorithm);
  static vtkNetCDFPOPReader *New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The file to open
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable subsampling in i,j and k dimensions in the vtkRectilinearGrid
   */
  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);
  //@}

  //@{
  /**
   * Variable array selection.
   */
  virtual int GetNumberOfVariableArrays();
  virtual const char *GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char *name);
  virtual void SetVariableArrayStatus(const char *name, int status);
  //@}

protected:
  vtkNetCDFPOPReader();
  ~vtkNetCDFPOPReader() override;

  int RequestData(vtkInformation*,vtkInformationVector**,
                  vtkInformationVector*) override;
  int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector) override;

  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  static void EventCallback(vtkObject* caller, unsigned long eid,
                            void* clientdata, void* calldata);

  vtkCallbackCommand* SelectionObserver;

  char *FileName;

  /**
   * The NetCDF file descriptor.
   */
  int NCDFFD;

  /**
   * The file name of the opened file.
   */
  char* OpenedFileName;

  vtkSetStringMacro(OpenedFileName);

  int Stride[3];

private:
  vtkNetCDFPOPReader(const vtkNetCDFPOPReader&) = delete;
  void operator=(const vtkNetCDFPOPReader&) = delete;

  vtkNetCDFPOPReaderInternal* Internals;
};
#endif
