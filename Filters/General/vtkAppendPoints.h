/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAppendPoints - appends points of one or more vtkPolyData data sets
//
// .SECTION Description
// vtkAppendPoints is a filter that appends the points and associated data
// of one or more polygonal (vtkPolyData) datasets. This filter can optionally
// add a new array marking the input index that the point came from.

// .SECTION See Also
// vtkAppendFilter vtkAppendPolyData

#ifndef vtkAppendPoints_h
#define vtkAppendPoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkAppendPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkAppendPoints *New();
  vtkTypeMacro(vtkAppendPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Sets the output array name to fill with the input connection index
  // for each point. This provides a way to trace a point back to a
  // particular input. If this is NULL (the default), the array is not generated.
  vtkSetStringMacro(InputIdArrayName);
  vtkGetStringMacro(InputIdArrayName);

  // Description:
  // Set/get the desired precision for the output type. See the documentation
  // for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
  // the available precision settings. If the desired precision is
  // DEFAULT_PRECISION and any of the inputs are double precision, then the
  // output precision will be double precision. Otherwise, if the desired
  // precision is DEFAULT_PRECISION and all the inputs are single precision,
  // then the output will be single precision.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkAppendPoints();
  ~vtkAppendPoints();

  // Usual data generation method
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);

  char* InputIdArrayName;
  int OutputPointsPrecision;
private:
  vtkAppendPoints(const vtkAppendPoints&);  // Not implemented.
  void operator=(const vtkAppendPoints&);  // Not implemented.
};

#endif


