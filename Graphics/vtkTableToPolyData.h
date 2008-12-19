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
// .NAME vtkTableToPolyData - filter used to convert a vtkTable to a vtkPolyData
// consisting of vertices.
// .SECTION Description
// vtkTableToPolyData is a filter used to convert a vtkTable  to a vtkPolyData
// consisting of vertices.

#ifndef __vtkTableToPolyData_h
#define __vtkTableToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkTableToPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkTableToPolyData* New();
  vtkTypeRevisionMacro(vtkTableToPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of the column to use as the X coordinate for the points. 
  vtkSetStringMacro(XColumn);
  vtkGetStringMacro(XColumn);

  // Description:
  // Specify the component for the column specified using SetXColumn() to
  // use as the xcoordinate in case the column is a multi-component array.
  // Default is 0.
  vtkSetClampMacro(XComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(XComponent, int);

  // Description:
  // Set the name of the column to use as the Y coordinate for the points. 
  // Default is 0.
  vtkSetStringMacro(YColumn);
  vtkGetStringMacro(YColumn);

  // Description:
  // Specify the component for the column specified using SetYColumn() to
  // use as the Ycoordinate in case the column is a multi-component array.
  vtkSetClampMacro(YComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(YComponent, int);

  // Description:
  // Set the name of the column to use as the Z coordinate for the points.
  // Default is 0.
  vtkSetStringMacro(ZColumn);
  vtkGetStringMacro(ZColumn);

  // Description:
  // Specify the component for the column specified using SetZColumn() to
  // use as the Zcoordinate in case the column is a multi-component array.
  vtkSetClampMacro(ZComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(ZComponent, int);

//BTX
protected:
  vtkTableToPolyData();
  ~vtkTableToPolyData();

  // Description:
  // Overridden to specify that input must be a vtkTable.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Convert input vtkTable to vtkPolyData.
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  char* XColumn;
  char* YColumn;
  char* ZColumn;
  int XComponent;
  int YComponent;
  int ZComponent;
private:
  vtkTableToPolyData(const vtkTableToPolyData&); // Not implemented.
  void operator=(const vtkTableToPolyData&); // Not implemented.
//ETX
};

#endif


