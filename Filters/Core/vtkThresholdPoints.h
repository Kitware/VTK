/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkThresholdPoints
 * @brief   extracts points whose scalar value satisfies threshold criterion
 *
 * vtkThresholdPoints is a filter that extracts points from a dataset that
 * satisfy a threshold criterion. The criterion can take three forms:
 * 1) greater than a particular value; 2) less than a particular value; or
 * 3) between a particular value. The output of the filter is polygonal data.
 * @sa
 * vtkThreshold
*/

#ifndef vtkThresholdPoints_h
#define vtkThresholdPoints_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkThresholdPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkThresholdPoints *New();
  vtkTypeMacro(vtkThresholdPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Criterion is cells whose scalars are less or equal to lower threshold.
   */
  void ThresholdByLower(double lower);

  /**
   * Criterion is cells whose scalars are greater or equal to upper threshold.
   */
  void ThresholdByUpper(double upper);

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Set/Get the upper threshold.
   */
  vtkSetMacro(UpperThreshold,double);
  vtkGetMacro(UpperThreshold,double);
  //@}

  //@{
  /**
   * Set/Get the lower threshold.
   */
  vtkSetMacro(LowerThreshold,double);
  vtkGetMacro(LowerThreshold,double);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkThresholdPoints();
  ~vtkThresholdPoints() VTK_OVERRIDE {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  double LowerThreshold;
  double UpperThreshold;
  int OutputPointsPrecision;

  int (vtkThresholdPoints::*ThresholdFunction)(double s);

  int Lower(double s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(double s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(double s) {return ( s >= this->LowerThreshold ?
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
private:
  vtkThresholdPoints(const vtkThresholdPoints&) VTK_DELETE_FUNCTION;
  void operator=(const vtkThresholdPoints&) VTK_DELETE_FUNCTION;
};

#endif
