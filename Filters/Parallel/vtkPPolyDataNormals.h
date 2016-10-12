/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPolyDataNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPPolyDataNormals
 * @brief   compute normals for polygonal mesh
 *
*/

#ifndef vtkPPolyDataNormals_h
#define vtkPPolyDataNormals_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataNormals.h"

class VTKFILTERSPARALLEL_EXPORT vtkPPolyDataNormals : public vtkPolyDataNormals
{
public:
  vtkTypeMacro(vtkPPolyDataNormals,vtkPolyDataNormals);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPPolyDataNormals *New();

  //@{
  /**
   * To get piece invariance, this filter has to request an
   * extra ghost level.  By default piece invariance is on.
   */
  vtkSetMacro(PieceInvariant, int);
  vtkGetMacro(PieceInvariant, int);
  vtkBooleanMacro(PieceInvariant, int);
  //@}

protected:
  vtkPPolyDataNormals();
  ~vtkPPolyDataNormals() {}

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int PieceInvariant;
private:
  vtkPPolyDataNormals(const vtkPPolyDataNormals&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPPolyDataNormals&) VTK_DELETE_FUNCTION;
};

#endif
