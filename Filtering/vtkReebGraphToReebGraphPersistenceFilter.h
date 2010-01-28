/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReebGraphToReebGraphPersistenceFilter - filter an input Reeb graph
// by Persistence.
// .SECTION Description
// The filter takes an input vtkReebGraph object and outputs a
// vtkReebGraph object.

#ifndef __vtkReebGraphToReebGraphPersistenceFilter_h
#define __vtkReebGraphToReebGraphPersistenceFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkReebGraph.h"

class VTK_FILTERING_EXPORT vtkReebGraphToReebGraphPersistenceFilter :
  public vtkDataSetAlgorithm
{
public:
  static vtkReebGraphToReebGraphPersistenceFilter* New();
  vtkTypeRevisionMacro(vtkReebGraphToReebGraphPersistenceFilter,
    vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the persistence threshold for simplification (from 0 to 1).
  // Default value: 0 (no simplification).
  vtkSetMacro(PersistenceThreshold, double);
  vtkGetMacro(PersistenceThreshold, double);

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int, vtkInformation *);

  vtkReebGraph* GetOutput();

protected:
  vtkReebGraphToReebGraphPersistenceFilter();
  ~vtkReebGraphToReebGraphPersistenceFilter();

  double PersistenceThreshold;

  int RequestDataObject(vtkInformation *request,
                        vtkInformationVector **inputVector,
                        vtkInformationVector *outputVector);

private:
  vtkReebGraphToReebGraphPersistenceFilter(
    const vtkReebGraphToReebGraphPersistenceFilter&);
  // Not implemented.
  void operator=(const vtkReebGraphToReebGraphPersistenceFilter&);
  // Not implemented.
};

#endif
