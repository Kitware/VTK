/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBlankStructuredGrid
 * @brief   translate point attribute data into a blanking field
 *
 *
 * vtkBlankStructuredGrid is a filter that sets the blanking field in a
 * vtkStructuredGrid dataset. The blanking field is set by examining a
 * specified point attribute data array (e.g., scalars) and converting
 * values in the data array to either a "1" (visible) or "0" (blanked) value
 * in the blanking array. The values to be blanked are specified by giving
 * a min/max range. All data values in the data array indicated and laying
 * within the range specified (inclusive on both ends) are translated to
 * a "off" blanking value.
 *
 * @sa
 * vtkStructuredGrid
*/

#ifndef vtkBlankStructuredGrid_h
#define vtkBlankStructuredGrid_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkBlankStructuredGrid : public vtkStructuredGridAlgorithm
{
public:
  static vtkBlankStructuredGrid *New();
  vtkTypeMacro(vtkBlankStructuredGrid,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the lower data value in the data array specified which will be
   * converted into a "blank" (or off) value in the blanking array.
   */
  vtkSetMacro(MinBlankingValue,double);
  vtkGetMacro(MinBlankingValue,double);
  //@}

  //@{
  /**
   * Specify the upper data value in the data array specified which will be
   * converted into a "blank" (or off) value in the blanking array.
   */
  vtkSetMacro(MaxBlankingValue,double);
  vtkGetMacro(MaxBlankingValue,double);
  //@}

  //@{
  /**
   * Specify the data array name to use to generate the blanking
   * field. Alternatively, you can specify the array id. (If both are set,
   * the array name takes precedence.)
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Specify the data array id to use to generate the blanking
   * field. Alternatively, you can specify the array name. (If both are set,
   * the array name takes precedence.)
   */
  vtkSetMacro(ArrayId,int);
  vtkGetMacro(ArrayId,int);
  //@}

  //@{
  /**
   * Specify the component in the data array to use to generate the blanking
   * field.
   */
  vtkSetClampMacro(Component,int,0,VTK_INT_MAX);
  vtkGetMacro(Component,int);
  //@}

protected:
  vtkBlankStructuredGrid();
  ~vtkBlankStructuredGrid() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  double MinBlankingValue;
  double MaxBlankingValue;
  char  *ArrayName;
  int   ArrayId;
  int   Component;

private:
  vtkBlankStructuredGrid(const vtkBlankStructuredGrid&) = delete;
  void operator=(const vtkBlankStructuredGrid&) = delete;
};

#endif
