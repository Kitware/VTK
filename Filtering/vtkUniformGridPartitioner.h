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
// .NAME vtkUniformGridPartitioner.h -- {Enter documentation here!}
//
// .SECTION Description
//  TODO: Enter documentation here!

#ifndef VTKUNIFORMGRIDPARTITIONER_H_
#define VTKUNIFORMGRIDPARTITIONER_H_

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkIndent;

class VTK_FILTERING_EXPORT vtkUniformGridPartitioner :
  public vtkMultiBlockDataSetAlgorithm
{
  public:
      static vtkUniformGridPartioner *New();
      vtkTypeMacro(vtkUniformGridPartitioner, vtkMultiBlockDataSetAglorithm);
      void PrintSelf( std::ostream &oss, vtkIndent indent );

      // Description:
      // Set/Get macro for the number of subdivisions.
      vtkGetMacro(NumberOfSubdivisions,int);
      vtkSetMacro(NumberOfSubdivisions,int);

  protected:
    vtkUniformGridPartitioner();
    virtual ~vtkUniformGridPartitioner();

    // Standard Pipeline methods
    virtual int RequestData(
       vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

    int NumberOfSubdivisions;
  private:
    vtkUniformGridPartitioner(const vtkUniformGridPartitioner &); // Not implemented
    void operator=(const vtkUniformGridPartitioner &); // Not implemented

};

#endif /* VTKUNIFORMGRIDPARTITIONER_H_ */
