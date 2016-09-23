/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPWindBladeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPWindBladeReader
 * @brief   class for reading WindBlade data files
 *
 * vtkPWindBladeReader is a source object that reads WindBlade files
 * which are block binary files with tags before and after each block
 * giving the number of bytes within the block.  The number of data
 * variables dumped varies.  There are 3 output ports with the first
 * being a structured grid with irregular spacing in the Z dimension.
 * The second is an unstructured grid only read on on process 0 and
 * used to represent the blade.  The third is also a structured grid
 * with irregular spacing on the Z dimension.  Only the first and
 * second output ports have time dependent data.
 * Parallel version of vtkWindBladeReader.h
*/

#ifndef vtkPWindBladeReader_h
#define vtkPWindBladeReader_h

#include "vtkIOMPIParallelModule.h" // For export macro
#include "vtkWindBladeReader.h"

class PWindBladeReaderInternal;

class VTKIOMPIPARALLEL_EXPORT vtkPWindBladeReader : public vtkWindBladeReader
{
public:
  static vtkPWindBladeReader *New();
  vtkTypeMacro(vtkPWindBladeReader, vtkWindBladeReader);

  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkPWindBladeReader();
  ~vtkPWindBladeReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  virtual void CalculatePressure(int pressure, int prespre,
                                 int tempg, int density);
  virtual void CalculateVorticity(int vort, int uvw, int density);
  virtual void LoadVariableData(int var);
  virtual bool ReadGlobalData();
  virtual bool FindVariableOffsets();
  virtual void CreateZTopography(float* zValues);
  virtual void SetupBladeData();
  virtual void LoadBladeData(int timeStep);

private:
  PWindBladeReaderInternal * PInternal;

  vtkPWindBladeReader(const vtkPWindBladeReader &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPWindBladeReader &) VTK_DELETE_FUNCTION;
};

#endif
