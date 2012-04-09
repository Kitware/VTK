/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPLYWriter - write Stanford PLY file format
// .SECTION Description
// vtkPLYWriter writes polygonal data in Stanford University PLY format
// (see http://graphics.stanford.edu/data/3Dscanrep/). The data can be
// written in either binary (little or big endian) or ASCII representation.
// As for PointData and CellData, vtkPLYWriter cannot handle normals or
// vectors. It only handles RGB PointData and CellData. You need to set the
// name of the array (using SetName for the array and SetArrayName for the
// writer). If the array is not a vtkUnsignedCharArray with 3 components,
// you need to specify a vtkLookupTable to map the scalars to RGB.

// .SECTION Caveats
// PLY does not handle big endian versus little endian correctly.

// .SECTION See Also
// vtkPLYReader

#ifndef __vtkPLYWriter_h
#define __vtkPLYWriter_h

#include "vtkPolyDataWriter.h"

class vtkScalarsToColors;
class vtkDataSetAttributes;

#define VTK_LITTLE_ENDIAN 0
#define VTK_BIG_ENDIAN    1

#define VTK_COLOR_MODE_DEFAULT 0
#define VTK_COLOR_MODE_UNIFORM_CELL_COLOR 1
#define VTK_COLOR_MODE_UNIFORM_POINT_COLOR 2
#define VTK_COLOR_MODE_UNIFORM_COLOR 3
#define VTK_COLOR_MODE_OFF 4


class VTK_IO_EXPORT vtkPLYWriter : public vtkPolyDataWriter
{
public:
  static vtkPLYWriter *New();
  vtkTypeMacro(vtkPLYWriter,vtkPolyDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If the file type is binary, then the user can specify which
  // byte order to use (little versus big endian).
  vtkSetClampMacro(DataByteOrder,int,VTK_LITTLE_ENDIAN,VTK_BIG_ENDIAN);
  vtkGetMacro(DataByteOrder,int);
  void SetDataByteOrderToBigEndian()
    {this->SetDataByteOrder(VTK_BIG_ENDIAN);}
  void SetDataByteOrderToLittleEndian()
    {this->SetDataByteOrder(VTK_LITTLE_ENDIAN);}

  // Description:
  // These methods enable the user to control how to add color into the PLY
  // output file. The default behavior is as follows. The user provides the
  // name of an array and a component number. If the type of the array is
  // three components, unsigned char, then the data is written as three
  // separate "red", "green" and "blue" properties. If the type is not
  // unsigned char, and a lookup table is provided, then the array/component
  // are mapped through the table to generate three separate "red", "green"
  // and "blue" properties in the PLY file. The user can also set the
  // ColorMode to specify a uniform color for the whole part (on a vertex
  // colors, face colors, or both. (Note: vertex colors or cell colors may be
  // written, depending on where the named array is found. If points and
  // cells have the arrays with the same name, then both colors will be
  // written.)
  vtkSetMacro(ColorMode,int);
  vtkGetMacro(ColorMode,int);
  void SetColorModeToDefault() 
    {this->SetColorMode(VTK_COLOR_MODE_DEFAULT);}
  void SetColorModeToUniformCellColor() 
    {this->SetColorMode(VTK_COLOR_MODE_UNIFORM_CELL_COLOR);}
  void SetColorModeToUniformPointColor() 
    {this->SetColorMode(VTK_COLOR_MODE_UNIFORM_POINT_COLOR);}
  void SetColorModeToUniformColor() //both cells and points are colored
    {this->SetColorMode(VTK_COLOR_MODE_UNIFORM_COLOR);}
  void SetColorModeToOff() //No color information is written
    {this->SetColorMode(VTK_COLOR_MODE_OFF);}
  
  // Description:
  // Specify the array name to use to color the data.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  
  // Description:
  // Specify the array component to use to color the data.
  vtkSetClampMacro(Component,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(Component,int);

  // Description:
  // A lookup table can be specified in order to convert data arrays to
  // RGBA colors.
  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);
  
  // Description:
  // Set the color to use when using a uniform color (either point or cells,
  // or both). The color is specified as a triplet of three unsigned chars
  // between (0,255). This only takes effect when the ColorMode is set to
  // uniform point, uniform cell, or uniform color.
  vtkSetVector3Macro(Color,unsigned char);
  vtkGetVector3Macro(Color,unsigned char);

protected:
  vtkPLYWriter();
  ~vtkPLYWriter();

  void WriteData();
  unsigned char *GetColors(vtkIdType num, vtkDataSetAttributes *dsa);
  
  int DataByteOrder;
  char *ArrayName;
  int Component;
  int ColorMode;
  vtkScalarsToColors *LookupTable;
  unsigned char Color[3];

private:
  vtkPLYWriter(const vtkPLYWriter&);  // Not implemented.
  void operator=(const vtkPLYWriter&);  // Not implemented.
};

#endif

