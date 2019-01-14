/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRotationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRotationFilter
 * @brief   Duplicates a data set by rotation about an axis
 *
 * The vtkRotationFilter duplicates a data set by rotation about one of the
 * 3 axis of the dataset's reference.
 * Since it converts data sets into unstructured grids, it is not efficient
 * for structured data sets.
 *
 * @par Thanks:
 * Theophane Foggia of The Swiss National Supercomputing Centre (CSCS)
 * for creating and contributing this filter
*/

#ifndef vtkRotationFilter_h
#define vtkRotationFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkRotationFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkRotationFilter *New();
  vtkTypeMacro(vtkRotationFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  enum RotationAxis
  {
    USE_X = 0,
    USE_Y = 1,
    USE_Z = 2
  };

  //@{
  /**
   * Set the axis of rotation to use. It is set by default to Z.
   */
  vtkSetClampMacro(Axis, int, 0, 2);
  vtkGetMacro(Axis, int);
  void SetAxisToX() { this->SetAxis(USE_X); };
  void SetAxisToY() { this->SetAxis(USE_Y); };
  void SetAxisToZ() { this->SetAxis(USE_Z); };
  //@}

  //@{
  /**
   * Set the rotation angle to use.
   */
  vtkSetMacro(Angle, double);
  vtkGetMacro(Angle, double);
  //@}

  //@{
  /**
   * Set the rotation center coordinates.
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);
  //@}

  //@{
  /**
   * Set the number of copies to create. The source will be rotated N times
   * and a new polydata copy of the original created at each angular position
   * All copies will be appended to form a single output
   */
  vtkSetMacro(NumberOfCopies, int);
  vtkGetMacro(NumberOfCopies, int);
  //@}

  //@{
  /**
   * If on (the default), copy the input geometry to the output. If off,
   * the output will only contain the rotation.
   */
  vtkSetMacro(CopyInput, vtkTypeBool);
  vtkGetMacro(CopyInput, vtkTypeBool);
  vtkBooleanMacro(CopyInput, vtkTypeBool);
  //@}


protected:
  vtkRotationFilter();
  ~vtkRotationFilter() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  int Axis;
  double Angle;
  double Center[3];
  int NumberOfCopies;
  vtkTypeBool CopyInput;

private:
  vtkRotationFilter(const vtkRotationFilter&) = delete;
  void operator=(const vtkRotationFilter&) = delete;
};

#endif


