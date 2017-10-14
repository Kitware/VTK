/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoundedPointSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBoundedPointSource
 * @brief   create a random cloud of points within a
 * specified bounding box
 *
 *
 * vtkBoundedPointSource is a source object that creates a user-specified
 * number of points within a specified bounding box. The points are scattered
 * randomly throughout the box. Optionally, the user can produce a
 * vtkPolyVertex cell as well as random scalar values within a specified
 * range. The class is typically used for debugging and testing, as well as
 * seeding streamlines.
*/

#ifndef vtkBoundedPointSource_h
#define vtkBoundedPointSource_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSPOINTS_EXPORT vtkBoundedPointSource : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information and printing.
   */
  static vtkBoundedPointSource *New();
  vtkTypeMacro(vtkBoundedPointSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Set the number of points to generate.
   */
  vtkSetClampMacro(NumberOfPoints,vtkIdType,1,VTK_ID_MAX);
  vtkGetMacro(NumberOfPoints,vtkIdType);
  //@}

  //@{
  /**
   * Set the bounding box for the point distribution. By default the bounds is
   * (-1,1,-1,1,-1,1).
   */
  vtkSetVector6Macro(Bounds,double);
  vtkGetVectorMacro(Bounds,double,6);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

  //@{
  /**
   * Indicate whether to produce a vtkPolyVertex cell to go along with the
   * output vtkPoints generated. By default a cell is NOT produced. Some filters
   * do not need the vtkPolyVertex which just consumes a lot of memory.
   */
  vtkSetMacro(ProduceCellOutput, bool);
  vtkGetMacro(ProduceCellOutput, bool);
  vtkBooleanMacro(ProduceCellOutput, bool);
  //@}

  //@{
  /**
   * Indicate whether to produce random point scalars in the output. By default
   * this is off.
   */
  vtkSetMacro(ProduceRandomScalars, bool);
  vtkGetMacro(ProduceRandomScalars, bool);
  vtkBooleanMacro(ProduceRandomScalars, bool);
  //@}

  //@{
  /**
   * Set the range in which the random scalars should be produced. By default the
   * scalar range is (0,1).
   */
  vtkSetVector2Macro(ScalarRange,double);
  vtkGetVectorMacro(ScalarRange,double,2);
  //@}

protected:
  vtkBoundedPointSource();
  ~vtkBoundedPointSource() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  vtkIdType NumberOfPoints;
  double Bounds[6];
  int OutputPointsPrecision;
  bool ProduceCellOutput;
  bool ProduceRandomScalars;
  double ScalarRange[2];

private:
  vtkBoundedPointSource(const vtkBoundedPointSource&) = delete;
  void operator=(const vtkBoundedPointSource&) = delete;
};

#endif
