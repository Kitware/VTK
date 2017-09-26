/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTableToPolyData
 * @brief   filter used to convert a vtkTable to a vtkPolyData
 * consisting of vertices.
 *
 * vtkTableToPolyData is a filter used to convert a vtkTable  to a vtkPolyData
 * consisting of vertices.
*/

#ifndef vtkTableToPolyData_h
#define vtkTableToPolyData_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkTableToPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkTableToPolyData* New();
  vtkTypeMacro(vtkTableToPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the name of the column to use as the X coordinate for the points.
   */
  vtkSetStringMacro(XColumn);
  vtkGetStringMacro(XColumn);
  //@}

  //@{
  /**
   * Set the index of the column to use as the X coordinate for the points.
   */
  vtkSetClampMacro(XColumnIndex, int, 0, VTK_INT_MAX);
  vtkGetMacro(XColumnIndex, int);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetXColumn() to
   * use as the xcoordinate in case the column is a multi-component array.
   * Default is 0.
   */
  vtkSetClampMacro(XComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(XComponent, int);
  //@}

  //@{
  /**
   * Set the name of the column to use as the Y coordinate for the points.
   * Default is 0.
   */
  vtkSetStringMacro(YColumn);
  vtkGetStringMacro(YColumn);
  //@}

  //@{
  /**
   * Set the index of the column to use as the Y coordinate for the points.
   */
  vtkSetClampMacro(YColumnIndex, int, 0, VTK_INT_MAX);
  vtkGetMacro(YColumnIndex, int);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetYColumn() to
   * use as the Ycoordinate in case the column is a multi-component array.
   */
  vtkSetClampMacro(YComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(YComponent, int);
  //@}

  //@{
  /**
   * Set the name of the column to use as the Z coordinate for the points.
   * Default is 0.
   */
  vtkSetStringMacro(ZColumn);
  vtkGetStringMacro(ZColumn);
  //@}

  //@{
  /**
   * Set the index of the column to use as the Z coordinate for the points.
   */
  vtkSetClampMacro(ZColumnIndex, int, 0, VTK_INT_MAX);
  vtkGetMacro(ZColumnIndex, int);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetZColumn() to
   * use as the Zcoordinate in case the column is a multi-component array.
   */
  vtkSetClampMacro(ZComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(ZComponent, int);
  //@}

  //@{
  /**
   * Specify whether the points of the polydata are 3D or 2D. If this is set to
   * true then the Z Column will be ignored and the z value of each point on the
   * polydata will be set to 0. By default this will be off.
   */
  vtkSetMacro(Create2DPoints, bool);
  vtkGetMacro(Create2DPoints, bool);
  vtkBooleanMacro(Create2DPoints, bool);
  //@}

  //@{
  /**
   * Allow user to keep columns specified as X,Y,Z as Data arrays.
   * By default this will be off.
   */
  vtkSetMacro(PreserveCoordinateColumnsAsDataArrays, bool);
  vtkGetMacro(PreserveCoordinateColumnsAsDataArrays, bool);
  vtkBooleanMacro(PreserveCoordinateColumnsAsDataArrays, bool);
  //@}

protected:
  vtkTableToPolyData();
  ~vtkTableToPolyData() override;

  /**
   * Overridden to specify that input must be a vtkTable.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Convert input vtkTable to vtkPolyData.
   */
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector) override;

  char* XColumn;
  char* YColumn;
  char* ZColumn;
  int XColumnIndex;
  int YColumnIndex;
  int ZColumnIndex;
  int XComponent;
  int YComponent;
  int ZComponent;
  bool Create2DPoints;
  bool PreserveCoordinateColumnsAsDataArrays;
private:
  vtkTableToPolyData(const vtkTableToPolyData&) = delete;
  void operator=(const vtkTableToPolyData&) = delete;

};

#endif


