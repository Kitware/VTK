/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeSourceExample.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTimeSource
 * @brief   creates a simple time varying data set.
 *
 * Creates a small easily understood time varying data set for testing.
 * The output is a vtkUntructuredGrid in which the point and cell values vary
 * over time in a sin wave. The analytic ivar controls whether the output
 * corresponds to a step function over time or is continuous.
 * The X and Y Amplitude ivars make the output move in the X and Y directions
 * over time. The Growing ivar makes the number of cells in the output grow
 * and then shrink over time.
*/

#ifndef vtkTimeSourceExample_h
#define vtkTimeSourceExample_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkTimeSourceExample : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkTimeSourceExample *New();
  vtkTypeMacro(vtkTimeSourceExample,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * When off (the default) this source produces a discrete set of values.
   * When on, this source produces a value analytically for any queried time.
   */
  vtkSetClampMacro(Analytic, int, 0, 1);
  vtkGetMacro(Analytic, int);
  vtkBooleanMacro(Analytic, int);
  //@}

  //@{
  /**
   * When 0.0 (the default) this produces a data set that is stationary.
   * When on the data set moves in the X/Y plane over a sin wave over time,
   * amplified by the value.
   */
  vtkSetMacro(XAmplitude, double);
  vtkGetMacro(XAmplitude, double);
  vtkSetMacro(YAmplitude, double);
  vtkGetMacro(YAmplitude, double);
  //@}

  //@{
  /**
   * When off (the default) this produces a single cell data set.
   * When on the the number of cells (in the Y direction) grows
   * and shrinks over time along a hat function.
   */
  vtkSetClampMacro(Growing, int, 0, 1);
  vtkGetMacro(Growing, int);
  vtkBooleanMacro(Growing, int);
  //@}

protected:
  vtkTimeSourceExample();
  ~vtkTimeSourceExample() VTK_OVERRIDE;

  int RequestInformation(vtkInformation*,
                         vtkInformationVector**,
                         vtkInformationVector*) VTK_OVERRIDE;

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;


  void LookupTimeAndValue(double &time, double &value);
  double ValueFunction(double time);
  double XFunction(double time);
  double YFunction(double time);
  int NumCellsFunction(double time);

  int Analytic;
  double XAmplitude;
  double YAmplitude;
  int Growing;

  int NumSteps;
  double *Steps;
  double *Values;
private:
  vtkTimeSourceExample(const vtkTimeSourceExample&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTimeSourceExample&) VTK_DELETE_FUNCTION;
};

#endif

