/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMaskPolyData
 * @brief   sample subset of input polygonal data cells
 *
 * vtkMaskPolyData is a filter that sub-samples the cells of input polygonal
 * data. The user specifies every nth item, with an initial offset to begin
 * sampling.
 *
 * @sa
 * vtkMaskPoints
*/

#ifndef vtkMaskPolyData_h
#define vtkMaskPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkMaskPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkMaskPolyData *New();
  vtkTypeMacro(vtkMaskPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Turn on every nth entity (cell).
   */
  vtkSetClampMacro(OnRatio,int,1,VTK_INT_MAX);
  vtkGetMacro(OnRatio,int);
  //@}

  //@{
  /**
   * Start with this entity (cell).
   */
  vtkSetClampMacro(Offset,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(Offset,vtkIdType);
  //@}

protected:
  vtkMaskPolyData();
  ~vtkMaskPolyData() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int OnRatio; // every OnRatio entity is on; all others are off.
  vtkIdType Offset;  // offset (or starting point id)

private:
  vtkMaskPolyData(const vtkMaskPolyData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMaskPolyData&) VTK_DELETE_FUNCTION;
};

#endif
