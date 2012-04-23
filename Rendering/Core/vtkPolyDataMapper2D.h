/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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


#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper2D.h"

class vtkCoordinate;
class vtkPolyData;
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTKRENDERINGCORE_EXPORT vtkPolyDataMapper2D : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkPolyDataMapper2D,vtkMapper2D);
  static vtkPolyDataMapper2D *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input to the mapper.
  void SetInputData(vtkPolyData *in);
  vtkPolyData *GetInput();

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
  void SetColorModeToDefault();
  void SetColorModeToMapScalars();

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
  vtkSetVector2Macro(ScalarRange,double);
  vtkGetVectorMacro(ScalarRange,double,2);

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
  virtual void SetTransformCoordinate(vtkCoordinate*);
  vtkGetObjectMacro(TransformCoordinate, vtkCoordinate);

  // Description:
  // Specify whether or not rounding to integers the transformed points when
  // TransformCoordinate is set. By default, it does not use double precision.
  vtkGetMacro(TransformCoordinateUseDouble,bool);
  vtkSetMacro(TransformCoordinateUseDouble,bool);
  vtkBooleanMacro(TransformCoordinateUseDouble,bool);

  // Description:
  // Map the scalars (if there are any scalars and ScalarVisibility is on)
  // through the lookup table, returning an unsigned char RGBA array. This is
  // typically done as part of the rendering process. The alpha parameter
  // allows the blending of the scalars with an additional alpha (typically
  // which comes from a vtkActor, etc.)
  vtkUnsignedCharArray *MapScalars(double alpha);

  // Description:
  // Make a shallow copy of this mapper.
  void ShallowCopy(vtkAbstractMapper *m);

protected:
  vtkPolyDataMapper2D();
  ~vtkPolyDataMapper2D();

  vtkUnsignedCharArray *Colors;

  vtkScalarsToColors *LookupTable;
  int ScalarVisibility;
  vtkTimeStamp BuildTime;
  double ScalarRange[2];
  int UseLookupTableScalarRange;
  int ColorMode;
  int ScalarMode;

  vtkCoordinate *TransformCoordinate;
  bool TransformCoordinateUseDouble;

  virtual int FillInputPortInformation(int, vtkInformation*);

  // for coloring by a component of a field data array
  int ArrayId;
  char ArrayName[256];
  int ArrayComponent;
  int ArrayAccessMode;
private:
  vtkPolyDataMapper2D(const vtkPolyDataMapper2D&);  // Not implemented.
  void operator=(const vtkPolyDataMapper2D&);  // Not implemented.
};


#endif

