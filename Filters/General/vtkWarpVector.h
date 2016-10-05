/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWarpVector
 * @brief   deform geometry with vector data
 *
 * vtkWarpVector is a filter that modifies point coordinates by moving
 * points along vector times the scale factor. Useful for showing flow
 * profiles or mechanical deformation.
 *
 * The filter passes both its point data and cell data to its output.
*/

#ifndef vtkWarpVector_h
#define vtkWarpVector_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkWarpVector : public vtkPointSetAlgorithm
{
public:
  static vtkWarpVector *New();
  vtkTypeMacro(vtkWarpVector,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify value to scale displacement.
   */
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);
  //@}

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

protected:
  vtkWarpVector();
  ~vtkWarpVector() VTK_OVERRIDE;

  int RequestDataObject(vtkInformation *request,
                        vtkInformationVector **inputVector,
                        vtkInformationVector *outputVector) VTK_OVERRIDE;
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;
  double ScaleFactor;

private:
  vtkWarpVector(const vtkWarpVector&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWarpVector&) VTK_DELETE_FUNCTION;
};

#endif
