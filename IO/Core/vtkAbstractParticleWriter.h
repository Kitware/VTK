/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractParticleWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractParticleWriter
 * @brief   abstract class to write particle data to file
 *
 * vtkAbstractParticleWriter is an abstract class which is used by
 * vtkTemporalStreamTracer to write particles out during simulations.
 * This class is abstract and provides a TimeStep and FileName.
 * Subclasses of this should provide the necessary IO.
 *
 * @warning
 * See vtkWriter
 *
 * @sa
 * vtkTemporalStreamTracer
*/

#ifndef vtkAbstractParticleWriter_h
#define vtkAbstractParticleWriter_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkWriter.h"

class VTKIOCORE_EXPORT vtkAbstractParticleWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkAbstractParticleWriter,vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/get the TimeStep that is being written
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  //@}

  //@{
  /**
   * Before writing the current data out, set the TimeValue (optional)
   * The TimeValue is a float/double value that corresponds to the real
   * time of the data, it may not be regular, whereas the TimeSteps
   * are simple increments.
   */
  vtkSetMacro(TimeValue,double);
  vtkGetMacro(TimeValue,double);
  //@}

  //@{
  /**
   * Set/get the FileName that is being written to
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * When running in parallel, this writer may be capable of
   * Collective IO operations (HDF5). By default, this is off.
   */
  vtkSetMacro(CollectiveIO,int);
  vtkGetMacro(CollectiveIO,int);
  void SetWriteModeToCollective();
  void SetWriteModeToIndependent();
  //@}

  /**
   * Close the file after a write. This is optional but
   * may protect against data loss in between steps
   */
  virtual void CloseFile() = 0;

protected:
   vtkAbstractParticleWriter();
  ~vtkAbstractParticleWriter() override;

  void WriteData() override = 0; //internal method subclasses must respond to
  int          CollectiveIO;
  int          TimeStep;
  double       TimeValue;
  char        *FileName;

private:
  vtkAbstractParticleWriter(const vtkAbstractParticleWriter&) = delete;
  void operator=(const vtkAbstractParticleWriter&) = delete;
};

#endif
