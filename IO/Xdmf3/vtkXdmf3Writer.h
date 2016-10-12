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

class VTKIOXDMF3_EXPORT vtkXdmf3Writer : public vtkDataObjectAlgorithm
{
public:
  static vtkXdmf3Writer *New();
  vtkTypeMacro(vtkXdmf3Writer,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  ~vtkXdmf3Writer();

  //Overridden to set up automatic loop over time steps.
  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);
  //Overridden to continue automatic loop over time steps.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  //Write out the input data objects as XDMF and HDF output files.
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  char *FileName;
  unsigned int LightDataLimit;
  bool WriteAllTimeSteps;

private:
  vtkXdmf3Writer(const vtkXdmf3Writer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXdmf3Writer&) VTK_DELETE_FUNCTION;

  class Internals;
  Internals *Internal;
};

#endif /* vtkXdmf3Writer_h */
