/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCastToConcrete.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCastToConcrete
 * @brief   works around type-checking limitations
 *
 * vtkCastToConcrete is a filter that works around type-checking limitations
 * in the filter classes. Some filters generate abstract types on output,
 * and cannot be connected to the input of filters requiring a concrete
 * input type. For example, vtkElevationFilter generates vtkDataSet for output,
 * and cannot be connected to vtkDecimate, because vtkDecimate requires
 * vtkPolyData as input. This is true even though (in this example) the input
 * to vtkElevationFilter is of type vtkPolyData, and you know the output of
 * vtkElevationFilter is the same type as its input.
 *
 * vtkCastToConcrete performs run-time checking to insure that output type
 * is of the right type. An error message will result if you try to cast
 * an input type improperly. Otherwise, the filter performs the appropriate
 * cast and returns the data.
 *
 * @warning
 * You must specify the input before you can get the output. Otherwise an
 * error results.
 *
 * @sa
 * vtkDataSetAlgorithm vtkPointSetToPointSetFilter
*/

#ifndef vtkCastToConcrete_h
#define vtkCastToConcrete_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkCastToConcrete : public vtkDataSetAlgorithm
{

public:
  static vtkCastToConcrete *New();
  vtkTypeMacro(vtkCastToConcrete,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkCastToConcrete() {}
  ~vtkCastToConcrete() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE; //insures compatibility; satisfies abstract api in vtkFilter
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
private:
  vtkCastToConcrete(const vtkCastToConcrete&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCastToConcrete&) VTK_DELETE_FUNCTION;
};

#endif
