/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeOfRevolutionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeOfRevolutionFilter
 * @brief   sweep data about a line to create a volume
 *
 * vtkVolumeOfRevolutionFilter is a modeling filter. It takes a 2-dimensional
 * dataset as input and generates an unstructured grid on output. The input
 * dataset is swept around the axis of rotation to create dimension-elevated
 * primitives. For example, sweeping a vertex creates a series of lines;
 * sweeping a line creates a series of quads, etc.
 *
 * @warning
 * The user must take care to ensure that the axis of revolution does not cross
 * through the geometry, otherwise there will be intersecting cells in the
 * output.
 *
 * @sa
 * vtkRotationalExtrusionFilter
*/

#ifndef vtkVolumeOfRevolutionFilter_h
#define vtkVolumeOfRevolutionFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKFILTERSMODELING_EXPORT vtkVolumeOfRevolutionFilter :
  public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkVolumeOfRevolutionFilter,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Create object with sweep angle of 360 degrees, resolution = 12,
   * axis position (0,0,0) and axis direction (0,0,1).
   */
  static vtkVolumeOfRevolutionFilter *New();

  //@{
  /**
   * Set/Get resolution of sweep operation. Resolution controls the number
   * of intermediate node points.
   */
  vtkSetClampMacro(Resolution,int,1,VTK_INT_MAX);
  vtkGetMacro(Resolution,int);
  //@}

  //@{
  /**
   * Set/Get angle of rotation in degrees.
   */
  vtkSetClampMacro(SweepAngle,double,-360.,360.);
  vtkGetMacro(SweepAngle,double);
  //@}

  //@{
  /**
   * Set/Get the position of the axis of revolution.
   */
  vtkSetVector3Macro(AxisPosition,double);
  vtkGetVector3Macro(AxisPosition,double);
  //@}

  //@{
  /**
   * Set/Get the direction of the axis of revolution.
   */
  vtkSetVector3Macro(AxisDirection,double);
  vtkGetVector3Macro(AxisDirection,double);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION,
                   DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  vtkVolumeOfRevolutionFilter();
  ~vtkVolumeOfRevolutionFilter();

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);

  int FillInputPortInformation(int, vtkInformation *);

  int Resolution;
  double SweepAngle;
  double AxisPosition[3];
  double AxisDirection[3];
  int OutputPointsPrecision;

private:
  vtkVolumeOfRevolutionFilter(const vtkVolumeOfRevolutionFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeOfRevolutionFilter&) VTK_DELETE_FUNCTION;
};

#endif
