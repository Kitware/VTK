/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkPolyDataMapper2D - draw vtkPolyData onto the image plane
// .SECTION Description
// vtkPolyDataMapper2D is a mapper that renders 3D polygonal data 
// (vtkPolyData) onto the 2D image plane (i.e., the renderer's viewport).
// By default, the 3D data is transformed into 2D data by ignoring the 
// z-coordinate of the 3D points in vtkPolyData, and taking the x-y values 
// as local display values (i.e., pixel coordinates). Alternatively, you
// can provide a vtkCoordinate object that will transform the data into
// local display coordinates (use the vtkCoordinate::SetCoordinateSystem()
// methods to indicate which coordinate system you are transforming the
// data from).

// .SECTION See Also
// vtkMapper2D vtkActor2D

#ifndef __vtkPolyDataMapper2D_h
#define __vtkPolyDataMapper2D_h


#include "vtkMapper2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkScalarsToColors.h"
#include "vtkPolyData.h"

#define VTK_GET_ARRAY_BY_ID 0
#define VTK_GET_ARRAY_BY_NAME 1

class VTK_EXPORT vtkPolyDataMapper2D : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkPolyDataMapper2D,vtkMapper2D);
  static vtkPolyDataMapper2D *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the input to the mapper.  
  vtkSetObjectMacro(Input, vtkPolyData);
  vtkGetObjectMacro(Input, vtkPolyData);

  // Description:
  // Specify a lookup table for the mapper to use.
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available with the scalar data.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Turn on/off flag to control whether scalar data is used to color objects.
  vtkSetMacro(ScalarVisibility,int);
  vtkGetMacro(ScalarVisibility,int);
  vtkBooleanMacro(ScalarVisibility,int);

  // Description:
  // Control how the scalar data is mapped to colors.  By default
  // (ColorModeToDefault), unsigned char scalars are treated as colors, and
  // NOT mapped through the lookup table, while everything else is. Setting
  // ColorModeToMapScalars means that all scalar data will be mapped through
  // the lookup table.  (Note that for multi-component scalars, the
  // particular component to use for mapping can be specified using the
  // ColorByArrayComponent() method.)
  vtkSetMacro(ColorMode,int);
  vtkGetMacro(ColorMode,int);
  void SetColorModeToDefault() 
    {this->SetColorMode(VTK_COLOR_MODE_DEFAULT);};
  void SetColorModeToMapScalars() 
    {this->SetColorMode(VTK_COLOR_MODE_MAP_SCALARS);};

  // Description:
  // Return the method of coloring scalar data.
  const char *GetColorModeAsString();

  // Description:
  // Control whether the mapper sets the lookuptable range based on its
  // own ScalarRange, or whether it will use the LookupTable ScalarRange
  // regardless of it's own setting. By default the Mapper is allowed to set
  // the LookupTable range, but users who are sharing LookupTables between
  // mappers/actors will probably wish to force the mapper to use the
  // LookupTable unchanged.
  vtkSetMacro(UseLookupTableScalarRange,int);
  vtkGetMacro(UseLookupTableScalarRange,int);
  vtkBooleanMacro(UseLookupTableScalarRange,int);

  // Description:
  // Specify range in terms of scalar minimum and maximum (smin,smax). These
  // values are used to map scalars into lookup table. Has no effect when
  // UseLookupTableScalarRange is true.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Control how the filter works with scalar point data and cell attribute
  // data.  By default (ScalarModeToDefault), the filter will use point data,
  // and if no point data is available, then cell data is used. Alternatively
  // you can explicitly set the filter to use point data
  // (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
  // You can also choose to get the scalars from an array in point field
  // data (ScalarModeToUsePointFieldData) or cell field data
  // (ScalarModeToUseCellFieldData).  If scalars are coming from a field
  // data array, you must call ColorByArrayComponent before you call
  // GetColors.
  vtkSetMacro(ScalarMode,int);
  vtkGetMacro(ScalarMode,int);
  void SetScalarModeToDefault() {
    this->SetScalarMode(VTK_SCALAR_MODE_DEFAULT);};
  void SetScalarModeToUsePointData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA);};
  void SetScalarModeToUseCellData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_DATA);};
  void SetScalarModeToUsePointFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);};
  void SetScalarModeToUseCellFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);};
  
  // Description:
  // Choose which component of which field data array to color by.
  void ColorByArrayComponent(int arrayNum, int component);
  void ColorByArrayComponent(char* arrayName, int component);
  
  // Description:
  // Get the array name or number and component to color by.
  char* GetArrayName() { return this->ArrayName; }
  int GetArrayId() { return this->ArrayId; }
  int GetArrayAccessMode() { return this->ArrayAccessMode; }
  int GetArrayComponent() { return this->ArrayComponent; }

  // Description:
  // Overload standard modified time function. If lookup table is modified,
  // then this object is modified as well.
  virtual unsigned long GetMTime();

  // Description:
  // Specify a vtkCoordinate object to be used to transform the vtkPolyData
  // point coordinates. By default (no vtkCoordinate specified), the point 
  // coordinates are taken as local display coordinates.
  vtkSetObjectMacro(TransformCoordinate, vtkCoordinate);
  vtkGetObjectMacro(TransformCoordinate, vtkCoordinate);

  // Description:
  // Map the scalars (if there are any scalars and ScalarVisibility is on)
  // through the lookup table, returning an unsigned char RGBA array. This is
  // typically done as part of the rendering process. The alpha parameter 
  // allows the blending of the scalars with an additional alpha (typically
  // which comes from a vtkActor, etc.)
  vtkUnsignedCharArray *MapScalars(float alpha);
  
  // Description:
  // Make a shallow copy of this mapper.
  void ShallowCopy(vtkPolyDataMapper2D *m);

  // Description:
  // Calculate and return the colors for the input. After invoking this
  // method, use GetColor() on the scalar to get the scalar values. This
  // method may return NULL if no color information is available. (This
  // method is obsolete; use MapScalars() instead.)
  vtkScalars *GetColors();

protected:
  vtkPolyDataMapper2D();
  ~vtkPolyDataMapper2D();
  vtkPolyDataMapper2D(const vtkPolyDataMapper2D&);
  void operator=(const vtkPolyDataMapper2D&);

  vtkPolyData* Input;

  vtkScalars *Scalars;
  vtkUnsignedCharArray *Colors;

  vtkScalarsToColors *LookupTable;
  int ScalarVisibility;
  vtkTimeStamp BuildTime;
  float ScalarRange[2];
  int UseLookupTableScalarRange;
  int ColorMode;
  int ScalarMode;
  
  vtkCoordinate *TransformCoordinate;

  // for coloring by a component of a field data array
  int ArrayId;
  char ArrayName[256];
  int ArrayComponent;
  int ArrayAccessMode;
};


#endif

