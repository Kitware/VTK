/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCountVertices.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCountVertices - Add a cell data array containing the number of
// vertices per cell.
//
// .SECTION Description
// This filter adds a cell data array containing the number of vertices per
// cell.

#ifndef vtkCountVertices_h
#define vtkCountVertices_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkCountVertices: public vtkPassInputTypeAlgorithm
{
public:
  static vtkCountVertices* New();
  vtkTypeMacro(vtkCountVertices, vtkPassInputTypeAlgorithm)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // The name of the new output array containing the vertex counts.
  vtkSetStringMacro(OutputArrayName)
  vtkGetStringMacro(OutputArrayName)

protected:
  vtkCountVertices();
  ~vtkCountVertices();

  int RequestData(vtkInformation* request, vtkInformationVector **inInfoVec,
                  vtkInformationVector *outInfoVec);

  int FillOutputPortInformation(int port, vtkInformation* info);
  int FillInputPortInformation(int port, vtkInformation* info);

  char *OutputArrayName;

private:
  vtkCountVertices(const vtkCountVertices&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCountVertices&) VTK_DELETE_FUNCTION;
};

#endif // vtkCountVertices_h
