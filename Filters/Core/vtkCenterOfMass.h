/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCenterOfMass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCenterOfMass
 * @brief   Find the center of mass of a set of points.
 *
 * vtkCenterOfMass finds the "center of mass" of a vtkPointSet (vtkPolyData
 * or vtkUnstructuredGrid). Optionally, the user can specify to use the scalars
 * as weights in the computation. If this option, UseScalarsAsWeights, is off,
 * each point contributes equally in the calculation.
 *
 * You must ensure Update() has been called before GetCenter will produce a valid
 * value.
*/

#ifndef vtkCenterOfMass_h
#define vtkCenterOfMass_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class vtkPoints;
class vtkDataArray;

class VTKFILTERSCORE_EXPORT vtkCenterOfMass : public vtkPointSetAlgorithm
{
public:
  static vtkCenterOfMass *New();
  vtkTypeMacro(vtkCenterOfMass,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output of the center of mass computation.
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);
  //@}

  //@{
  /**
   * Set a flag to determine if the points are weighted.
   */
  vtkSetMacro(UseScalarsAsWeights, bool);
  vtkGetMacro(UseScalarsAsWeights, bool);
  //@}

  /**
   * This function is called by RequestData. It exists so that
   * other classes may use this computation without constructing
   * a vtkCenterOfMass object.  The scalars can be set to NULL
   * if all points are to be weighted equally.  If scalars are
   * used, it is the caller's responsibility to ensure that the
   * number of scalars matches the number of points, and that
   * the sum of the scalars is a positive value.
   */
  static void ComputeCenterOfMass(
    vtkPoints* input, vtkDataArray *scalars, double center[3]);

protected:
  vtkCenterOfMass();

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector ) VTK_OVERRIDE;

private:

  vtkCenterOfMass(const vtkCenterOfMass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCenterOfMass&) VTK_DELETE_FUNCTION;

  bool UseScalarsAsWeights;
  double Center[3];
};

#endif
