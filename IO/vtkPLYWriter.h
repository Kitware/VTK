/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Mike Dresser MD/PhD
             Director of Core Facility for Imaging
             Program in Molecular and Cell Biology
             Oklahoma Medical Research Foundation


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPLYWriter - write Stanford PLY file format
// .SECTION Description
// vtkPLYWriter writes polygonal data in Stanford University PLY format
// (see http://graphics.stanford.edu/data/3Dscanrep/). The data can be
// written in either binary (little or big endian) or ASCII representation. 

// .SECTION Caveats
// PLY does not handle big endian versus little endian correctly. Also,
// this class is compiled into VTK only if the PLY library is found
// during the make process (using CMake).

// .SECTION See Also
// vtkPLYReader

#ifndef __vtkPLYWriter_h
#define __vtkPLYWriter_h

#include "vtkPolyDataWriter.h"
#include "vtkPolyData.h"
#include "vtkScalarsToColors.h"

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
  vtkSetObjectMacro(LookupTable,vtkScalarsToColors);
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

