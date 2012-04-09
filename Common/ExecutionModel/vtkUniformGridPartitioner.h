/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridPartitioner.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkUniformGridPartitioner.h -- Partitions a uniform grid by RCB
//
// .SECTION Description
//  A concrete implementation of vtkMultiBlockDataSetAlgorithm that provides
//  functionality for partitioning a uniform grid. The partitioning method
//  that is used is Recursive Coordinate Bisection (RCB) where each time
//  the longest dimension is split.
//
// .SECTION See Also
// vtkStructuredGridPartitioner vtkRectilinearGridPartitioner

#ifndef VTKUNIFORMGRIDPARTITIONER_H_
#define VTKUNIFORMGRIDPARTITIONER_H_

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkIndent;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUniformGridPartitioner :
  public vtkMultiBlockDataSetAlgorithm
{
  public:
      static vtkUniformGridPartitioner *New();
      vtkTypeMacro(vtkUniformGridPartitioner, vtkMultiBlockDataSetAlgorithm);
      void PrintSelf(ostream &oss, vtkIndent indent );

      // Description:
      // Set/Get macro for the number of subdivisions.
      vtkGetMacro(NumberOfPartitions,int);
      vtkSetMacro(NumberOfPartitions,int);

      // Description:
      // Set/Get macro for the number of ghost layers.
      vtkGetMacro(NumberOfGhostLayers,int);
      vtkSetMacro(NumberOfGhostLayers,int);

  protected:
    vtkUniformGridPartitioner();
    virtual ~vtkUniformGridPartitioner();

    // Standard Pipeline methods
    virtual int RequestData(
       vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

    int NumberOfPartitions;
    int NumberOfGhostLayers;
  private:
    vtkUniformGridPartitioner(const vtkUniformGridPartitioner &); // Not implemented
    void operator=(const vtkUniformGridPartitioner &); // Not implemented

};

#endif /* VTKUNIFORMGRIDPARTITIONER_H_ */
