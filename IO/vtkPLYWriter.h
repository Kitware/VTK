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

class VTK_EXPORT vtkPLYWriter : public vtkPolyDataWriter
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

  // ---Writing point data-----------

  // Description:
  // Enable the writing of point scalars as the vertex property "scalars".
  // By default, WritePointScalars is off.
  vtkSetMacro(WritePointScalars,int);
  vtkGetMacro(WritePointScalars,int);
  vtkBooleanMacro(WritePointScalars,int);
  
  // Description:
  // A lookup table can be specified in order to convert point scalars to
  // RGBA colors. Normally the scalars are written as a vertex separate
  // property called "scalars". However, if the scalars are unsigned char
  // and three or more components, then the PLY properties "red", "green",
  // and "blue" are written; or, if the scalars have an associated lookup
  // table, then the red, green, blue properties are also written. (Note that
  // data arrays (like scalars) can have an associated lookup table as well.)
  vtkSetObjectMacro(PointLookupTable,vtkScalarsToColors);
  vtkGetObjectMacro(PointLookupTable,vtkScalarsToColors);

  // ---Writing cell data-----------

  // Description:
  // Enable the writing of cell scalars as the vertex property "scalars".
  // By default, WriteCellScalars is off.
  vtkSetMacro(WriteCellScalars,int);
  vtkGetMacro(WriteCellScalars,int);
  vtkBooleanMacro(WriteCellScalars,int);
  
  // Description:
  // A lookup table can be specified in order to convert cell scalars to
  // RGBA colors. Normally the scalars are written as a vertex separate
  // property called "scalars". However, if the scalars are unsigned char
  // and three or more components, then the PLY properties "red", "green",
  // and "blue" are written; or, if the scalars have an associated lookup
  // table, then the red, green, blue properties are also written. (Note that
  // data arrays (like scalars) can have an associated lookup table as well.)
  vtkSetObjectMacro(CellLookupTable,vtkScalarsToColors);
  vtkGetObjectMacro(CellLookupTable,vtkScalarsToColors);

protected:
  vtkPLYWriter();
  ~vtkPLYWriter() {};
  vtkPLYWriter(const vtkPLYWriter&);
  void operator=(const vtkPLYWriter&);

  void WriteData();
  
  int DataByteOrder;
  
  int WritePointScalars;
  vtkScalarsToColors *PointLookupTable;
  
  int WriteCellScalars;
  vtkScalarsToColors *CellLookupTable;

};

#endif

