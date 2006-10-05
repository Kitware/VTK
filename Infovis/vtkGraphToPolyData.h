/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGraphToPolyData - convert a vtkGraph to vtkPolyData
//
// .SECTION Description
//

#ifndef __vtkGraphToPolyData_h
#define __vtkGraphToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkGraphToPolyData : public vtkPolyDataAlgorithm 
{
public:
  static vtkGraphToPolyData *New();

  vtkTypeRevisionMacro(vtkGraphToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkGraphToPolyData();
  ~vtkGraphToPolyData() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkGraphToPolyData(const vtkGraphToPolyData&);  // Not implemented.
  void operator=(const vtkGraphToPolyData&);  // Not implemented.
};

#endif
