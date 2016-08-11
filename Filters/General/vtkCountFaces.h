/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCountFaces.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCountFaces - Add a cell data array containing the number of faces
// per cell.
//
// .SECTION Description
// This filter adds a cell data array containing the number of faces per cell.

#ifndef vtkCountFaces_h
#define vtkCountFaces_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkCountFaces: public vtkPassInputTypeAlgorithm
{
public:
  static vtkCountFaces* New();
  vtkTypeMacro(vtkCountFaces, vtkPassInputTypeAlgorithm)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // The name of the new output array containing the face counts.
  vtkSetStringMacro(OutputArrayName)
  vtkGetStringMacro(OutputArrayName)

protected:
  vtkCountFaces();
  ~vtkCountFaces();

  int RequestData(vtkInformation* request, vtkInformationVector **inInfoVec,
                  vtkInformationVector *outInfoVec);

  int FillOutputPortInformation(int port, vtkInformation* info);
  int FillInputPortInformation(int port, vtkInformation* info);

  char *OutputArrayName;

private:
  vtkCountFaces(const vtkCountFaces&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCountFaces&) VTK_DELETE_FUNCTION;
};

#endif // vtkCountFaces_h
