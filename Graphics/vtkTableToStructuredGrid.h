/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTableToStructuredGrid - converts vtkTable to a vtkStructuredGrid.
// .SECTION Description
// vtkTableToStructuredGrid is a filter that converts an input
// vtkTable to a vtkStructuredGrid. It provides API to select columns to use as
// points in the output structured grid. The specified dimensions of the output
// (specified using SetWholeExtent()) must match the number of rows in the input
// table.

#ifndef __vtkTableToStructuredGrid_h
#define __vtkTableToStructuredGrid_h

#include "vtkStructuredGridAlgorithm.h"

class vtkTable;

class VTK_GRAPHICS_EXPORT vtkTableToStructuredGrid : public vtkStructuredGridAlgorithm
{
public:
  static vtkTableToStructuredGrid* New();
  vtkTypeMacro(vtkTableToStructuredGrid, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the whole extents for the image to produce. The size of the image
  // must match the number of rows in the input table.
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

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
  vtkTableToStructuredGrid();
  ~vtkTableToStructuredGrid();

  int Convert(vtkTable*, vtkStructuredGrid*, int extent[6]);

  // Description:
  // Overridden to specify that input must be a vtkTable.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Convert input vtkTable to vtkStructuredGrid.
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  // Description:
  // Request information -- pass whole extent to the pipeline.
  virtual int RequestInformation(vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector);

  char* XColumn;
  char* YColumn;
  char* ZColumn;
  int XComponent;
  int YComponent;
  int ZComponent;
  int WholeExtent[6];

private:
  vtkTableToStructuredGrid(const vtkTableToStructuredGrid&); // Not implemented.
  void operator=(const vtkTableToStructuredGrid&); // Not implemented.
//ETX
};

#endif


