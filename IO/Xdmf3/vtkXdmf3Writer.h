/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3Writer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkXdmf3Writer
 * @brief   write eXtensible Data Model and Format files
 *
 * vtkXdmf3Writer converts vtkDataObjects to XDMF format. This is intended to
 * replace vtkXdmfWriter, which is not up to date with the capabilities of the
 * newer XDMF3 library. This writer understands VTK's composite data types and
 * produces full trees in the output XDMF files.
 */

#ifndef vtkXdmf3Writer_h
#define vtkXdmf3Writer_h

#include "vtkIOXdmf3Module.h" // For export macro

#include "vtkDataObjectAlgorithm.h"

class vtkDoubleArray;

class VTKIOXDMF3_EXPORT vtkXdmf3Writer : public vtkDataObjectAlgorithm
{
public:
  static vtkXdmf3Writer* New();
  vtkTypeMacro(vtkXdmf3Writer, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the input data set.
   */
  virtual void SetInputData(vtkDataObject* dobj);

  //@{
  /**
   * Set or get the file name of the xdmf file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * We never write out ghost cells.  This variable is here to satisfy
   * the behavior of ParaView on invoking a parallel writer.
   */
  void SetGhostLevel(int) {}
  int GetGhostLevel() { return 0; }
  //@}

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartMethod() and EndMethod() methods.
   * Returns 1 on success and 0 on failure.
   */
  virtual int Write();

  //@{
  /**
   * Topology Geometry and Attribute arrays smaller than this are written in line into the XML.
   * Default is 100.
   */
  vtkSetMacro(LightDataLimit, unsigned int);
  vtkGetMacro(LightDataLimit, unsigned int);
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

protected:
  vtkXdmf3Writer();
  ~vtkXdmf3Writer() override;

  // Overridden to set up automatic loop over time steps.
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  // Overridden to continue automatic loop over time steps.
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  // Write out the input data objects as XDMF and HDF output files.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* FileName;
  unsigned int LightDataLimit;
  bool WriteAllTimeSteps;
  int NumberOfProcesses;
  int MyRank;

  vtkDoubleArray* TimeValues;
  vtkDataObject* OriginalInput;
  void WriteDataInternal(vtkInformation* request);
  int CheckParametersInternal(int NumberOfProcesses, int MyRank);
  virtual int CheckParameters();
  // If writing in parallel multiple time steps exchange after each time step
  // if we should continue the execution. Pass local continueExecution as a
  // parameter and return the global continueExecution.
  virtual int GlobalContinueExecuting(int localContinueExecution);

  bool InitWriters;

private:
  vtkXdmf3Writer(const vtkXdmf3Writer&) = delete;
  void operator=(const vtkXdmf3Writer&) = delete;

  class Internals;
  Internals* Internal;
};

#endif /* vtkXdmf3Writer_h */
