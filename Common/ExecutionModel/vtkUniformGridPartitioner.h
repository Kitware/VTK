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

#ifndef vtkUniformGridPartitioner_h
#define vtkUniformGridPartitioner_h

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
      void PrintSelf(ostream &oss, vtkIndent indent ) VTK_OVERRIDE;

      // Description:
      // Set/Get macro for the number of subdivisions.
      vtkGetMacro(NumberOfPartitions,int);
      vtkSetMacro(NumberOfPartitions,int);

      // Description:
      // Set/Get macro for the number of ghost layers.
      vtkGetMacro(NumberOfGhostLayers,int);
      vtkSetMacro(NumberOfGhostLayers,int);

      // Description:
      vtkGetMacro(DuplicateNodes,int);
      vtkSetMacro(DuplicateNodes,int);
      vtkBooleanMacro(DuplicateNodes,int);

  protected:
    vtkUniformGridPartitioner();
    ~vtkUniformGridPartitioner() VTK_OVERRIDE;

    // Standard Pipeline methods
    int RequestData(
       vtkInformation*,vtkInformationVector**,vtkInformationVector*) VTK_OVERRIDE;
    int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
    int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

    int NumberOfPartitions;
    int NumberOfGhostLayers;
    int DuplicateNodes;
  private:
    vtkUniformGridPartitioner(const vtkUniformGridPartitioner &) VTK_DELETE_FUNCTION;
    void operator=(const vtkUniformGridPartitioner &) VTK_DELETE_FUNCTION;

};

#endif /* VTKUNIFORMGRIDPARTITIONER_H_ */
