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

/**
 * @class   vtkCountFaces
 * @brief   Add a cell data array containing the number of faces
 * per cell.
 *
 *
 * This filter adds a cell data array containing the number of faces per cell.
*/

#ifndef vtkCountFaces_h
#define vtkCountFaces_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkCountFaces: public vtkPassInputTypeAlgorithm
{
public:
  static vtkCountFaces* New();
  vtkTypeMacro(vtkCountFaces, vtkPassInputTypeAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The name of the new output array containing the face counts.
   */
  vtkSetStringMacro(OutputArrayName)
  vtkGetStringMacro(OutputArrayName)
  //@}

protected:
  vtkCountFaces();
  ~vtkCountFaces() VTK_OVERRIDE;

  int RequestData(vtkInformation* request, vtkInformationVector **inInfoVec,
                  vtkInformationVector *outInfoVec) VTK_OVERRIDE;

  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  char *OutputArrayName;

private:
  vtkCountFaces(const vtkCountFaces&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCountFaces&) VTK_DELETE_FUNCTION;
};

#endif // vtkCountFaces_h
