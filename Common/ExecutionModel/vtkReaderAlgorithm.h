/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReaderAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkReaderAlgorithm
 * @brief   Superclass for readers that implement a simplified API.
 *
 * This class and associated subclasses were created to make it easier to
 * develop readers. When directly subclassing from other algorithm classes
 * one has to learn a general purpose API that somewhat obfuscates pipeline
 * functionality behind information keys. One has to know how to find
 * time and pieces requests using keys for example. Furthermore, these
 * classes together with specialized executives can implement common
 * reader functionality for things such as file series (for time and/or
 * partitions), caching, mapping time requests to indices etc.
 * This class implements the most basic API which is specialized as
 * needed by subclasses (for file series for example).
*/

#ifndef vtkReaderAlgorithm_h
#define vtkReaderAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkReaderAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkReaderAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provide meta-data for the pipeline. These include things like
   * time steps and whole extent. Subclasses may have specialized
   * interfaces making this simpler.
   */
  virtual int ReadMetaData(vtkInformation* metadata) = 0;

  /**
   * Read the mesh (connectivity) for a given set of data partitioning,
   * number of ghost levels and time step (index). The reader populates
   * the data object passed in as the last argument. It is OK to read
   * more than the mesh (points, arrays etc.). However, this may interfere
   * with any caching implemented by the executive (i.e. cause more reads).
   */
  virtual int ReadMesh(
    int piece, int npieces, int nghosts, int timestep,
    vtkDataObject* output) = 0;

  /**
   * Read the points. The reader populates the input data object. This is
   * called after ReadMesh() so the data object should already contain the
   * mesh.
   */
  virtual int ReadPoints(
    int piece, int npieces, int nghosts, int timestep,
    vtkDataObject* output) = 0;

  /**
   * Read all the arrays (point, cell, field etc.). This is called after
   * ReadPoints() so the data object should already contain the mesh and
   * points.
   */
  virtual int ReadArrays(
    int piece, int npieces, int nghosts, int timestep,
    vtkDataObject* output) = 0;

protected:
  vtkReaderAlgorithm();
  ~vtkReaderAlgorithm() override;

private:
  vtkReaderAlgorithm(const vtkReaderAlgorithm&) = delete;
  void operator=(const vtkReaderAlgorithm&) = delete;
};

#endif
