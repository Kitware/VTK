/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTable.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkLookupTable - map scalar values into colors or colors to scalars; generate color table
// .SECTION Description
// vtkLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The color table can be created by direct 
// insertion of color values, or by specifying  hue, saturation, value, and 
// alpha range and generating a table.
//
// This class is designed as a base class for derivation by other classes. 
// The Build(), MapValue(), and SetTableRange() methods are virtual and may 
// require overloading in subclasses.
// .SECTION Caveats
// vtkLookupTable is a reference counted object. Therefore, you should 
// always use operator "new" to construct new objects. This procedure will
// avoid memory problems (see text).
// .SECTION See Also
// vtkLogLookupTable vtkWindowLevelLookupTable

#ifndef __vtkLookupTable_h
#define __vtkLookupTable_h

#include "vtkReferenceCount.h"
#include "vtkAPixmap.h"

class VTK_EXPORT vtkLookupTable : public vtkReferenceCount
{
public:
  vtkLookupTable(int sze=256, int ext=256);
  int Allocate(int sz=256, int ext=256);
  virtual void Build();
  static vtkLookupTable *New() {return new vtkLookupTable;};
  char *GetClassName() {return "vtkLookupTable";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of colors in the lookup table. Use this method before
  // building the lookup table. Use SetNumberOfTableValues() to change the
  // table size after the lookup table has been built.
  vtkSetClampMacro(NumberOfColors,int,8, 65536);
  vtkGetMacro(NumberOfColors,int);

  void SetTableRange(float r[2]); // can't use macro 'cause don't want modified
  virtual void SetTableRange(float min, float max);
  vtkGetVectorMacro(TableRange,float,2);

  // Description:
  // Set the range in hue (using automatic generation). Hue ranges 
  // between (0,1).
  vtkSetVector2Macro(HueRange,float);
  vtkGetVectorMacro(HueRange,float,2);

  // Description:
  // Set the range in saturation (using automatic generation). Saturation 
  // ranges between (0,1).
  vtkSetVector2Macro(SaturationRange,float);
  vtkGetVectorMacro(SaturationRange,float,2);

  // Description:
  // Set the range in value (using automatic generation). Value ranges 
  // between (0,1).
  vtkSetVector2Macro(ValueRange,float);
  vtkGetVectorMacro(ValueRange,float,2);

  // Description:
  // Set the range in alpha (using automatic generation). Alpha ranges from 
  // (0,1).
  vtkSetVector2Macro(AlphaRange,float);
  vtkGetVectorMacro(AlphaRange,float,2);

  virtual unsigned char *MapValue(float v);

  void SetNumberOfTableValues(int number);
  void SetTableValue (int indx, float rgba[4]);
  void SetTableValue (int indx, float r, float g, float b, float a=1.0);

  float *GetTableValue (int id);
  void GetTableValue (int id, float rgba[4]);

  unsigned char *GetPointer(const int id);
  unsigned char *WritePointer(const int id, const int number);

protected:
  int NumberOfColors;
  vtkAPixmap Table;  
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];
  float AlphaRange[2];
  vtkTimeStamp InsertTime;
  vtkTimeStamp BuildTime;
};

// Description:
// Get pointer to color table data. Format is array of unsigned char
// r-g-b-a-r-g-b-a...
inline unsigned char *vtkLookupTable::GetPointer(const int id)
{
  return this->Table.GetPointer(id);
}
// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the location you 
// wish to write into; number is the number of rgba values to write.
inline unsigned char *vtkLookupTable::WritePointer(const int id, const int number)
{
  return this->Table.WritePointer(id,number);
}

#endif


