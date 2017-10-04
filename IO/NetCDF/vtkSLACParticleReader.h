// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLACParticleReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkSLACParticleReader
 *
 *
 *
 * A reader for a data format used by Omega3p, Tau3p, and several other tools
 * used at the Standford Linear Accelerator Center (SLAC).  The underlying
 * format uses netCDF to store arrays, but also imposes some conventions
 * to store a list of particles in 3D space.
 *
 * This reader supports pieces, but in actuality only loads anything in
 * piece 0.  All other pieces are empty.
 *
*/

#ifndef vtkSLACParticleReader_h
#define vtkSLACParticleReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkDataArraySelection;
class vtkIdTypeArray;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;

class VTKIONETCDF_EXPORT vtkSLACParticleReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSLACParticleReader, vtkPolyDataAlgorithm);
  static vtkSLACParticleReader *New();
  void PrintSelf(ostream &os, vtkIndent indent) override;

  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  /**
   * Returns true if the given file can be read by this reader.
   */
  static int CanReadFile(const char *filename);

protected:
  vtkSLACParticleReader();
  ~vtkSLACParticleReader() override;

  char *FileName;

  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector) override;

  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) override;

  /**
   * Convenience function that checks the dimensions of a 2D netCDF array that
   * is supposed to be a set of tuples.  It makes sure that the number of
   * dimensions is expected and that the number of components in each tuple
   * agree with what is expected.  It then returns the number of tuples.  An
   * error is emitted and 0 is returned if the checks fail.
   */
  virtual vtkIdType GetNumTuplesInVariable(int ncFD, int varId,
                                           int expectedNumComponents);

private:
  vtkSLACParticleReader(const vtkSLACParticleReader &) = delete;
  void operator=(const vtkSLACParticleReader &) = delete;
};

#endif //vtkSLACParticleReader_h
