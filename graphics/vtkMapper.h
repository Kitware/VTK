/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkMapper - abstract class specifies interface to map data to graphics primitives
// .SECTION Description
// vtkMapper is an abstract class to specify interface between data and 
// graphics primitives. Subclasses of vtkMapper map data through a 
// lookuptable and control the creation of rendering primitives that
// interface to the graphics library. The mapping can be controlled by 
// supplying a lookup table and specifying a scalar range to map data
// through.
//
// There are several important control mechanisms affecting the behavior of
// this object. The ScalarVisibility flag controls whether scalar data (if
// any) controls the color of the associated actor(s) that refer to the
// mapper. The ScalarMode ivar is used to determine whether scalar point data
// or cell data is used to color the object. By default, point data scalars
// are used unless there are none, in which cell scalars are used. Or you can
// explicitly control whether to use point or cell scalar data. Finally, the
// mapping of scalars through the lookup table varies depending on the
// setting of the ColorMode flag. See the documentation for the appropriate
// methods for an explanation.
//
// Another important feature of this class is whether to use immediate mode
// rendering (ImmediateModeRenderingOn) or display list rendering
// (ImmediateModeRenderingOff). If display lists are used, a data structure
// is constructed (generally in the rendering library) which can then be
// rapidly traversed and rendered by the rendering library. The disadvantage
// of display lists is that they require additionally memory which may affect
// the perfomance of the system.

// .SECTION See Also
// vtkDataSetMapper vtkPolyDataMapper

#ifndef __vtkMapper_h
#define __vtkMapper_h

#include "vtkAbstractMapper3D.h"
#include "vtkScalarsToColors.h"
#include "vtkDataSet.h"

#define VTK_SCALAR_MODE_DEFAULT 0
#define VTK_SCALAR_MODE_USE_POINT_DATA 1
#define VTK_SCALAR_MODE_USE_CELL_DATA 2

class vtkWindow;
class vtkRenderer;
class vtkActor;

class VTK_EXPORT vtkMapper : public vtkAbstractMapper3D
{
public:
  vtkTypeMacro(vtkMapper,vtkAbstractMapper3D);
  void PrintSelf(vtkOstream& os, vtkIndent indent);

  // Description:
  // Make a shallow copy of this mapper.
  void ShallowCopy(vtkMapper *m);

  // Description:
  // Overload standard modified time function. If lookup table is modified,
  // then this object is modified as well.
  unsigned long GetMTime();

  // Description:
  // Method initiates the mapping process. Generally sent by the actor 
  // as each frame is rendered.
  virtual void Render(vtkRenderer *ren, vtkActor *a) = 0;

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};

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
  // (ColorModeToDefault), scalars that are unsigned char types are treated
  // as colors, and NOT mapped through the lookup table, while everything
  // else is.  Setting ColorModeToMapScalars means that all scalar data will
  // be mapped through the lookup table. Setting ColorModeToLuminance means
  // that scalars will be converted to luminance (gray values) using the
  // luminance equation . (The ColorMode ivar is used with vtkScalars to map
  // scalar data to colors. See vtkScalars::InitColorTraversal() for more
  // information.)
  vtkSetMacro(ColorMode,int);
  vtkGetMacro(ColorMode,int);
  void SetColorModeToDefault() {
    this->SetColorMode(VTK_COLOR_MODE_DEFAULT);};
  void SetColorModeToMapScalars() {
    this->SetColorMode(VTK_COLOR_MODE_MAP_SCALARS);};
  void SetColorModeToLuminance() {
    this->SetColorMode(VTK_COLOR_MODE_LUMINANCE);};

  // Description:
  // Return the method of coloring scalar data.
  char *GetColorModeAsString();

  // Description:
  // Turn on/off flag to control whether data is rendered using
  // immediate mode or note. Immediate mode rendering
  // tends to be slower but it can handle larger datasets.
  // The default value is immediate mode off. If you are 
  // having problems rendering a large dataset you might
  // want to consider using imediate more rendering.
  vtkSetMacro(ImmediateModeRendering,int);
  vtkGetMacro(ImmediateModeRendering,int);
  vtkBooleanMacro(ImmediateModeRendering,int);

  // Description:
  // Turn on/off flag to control whether data is rendered using
  // immediate mode or note. Immediate mode rendering
  // tends to be slower but it can handle larger datasets.
  // The default value is immediate mode off. If you are 
  // having problems rendering a large dataset you might
  // want to consider using imediate more rendering.
  static void SetGlobalImmediateModeRendering(int val);
  static void GlobalImmediateModeRenderingOn() {
    vtkMapper::SetGlobalImmediateModeRendering(1);};
  static void GlobalImmediateModeRenderingOff() {
    vtkMapper::SetGlobalImmediateModeRendering(0);};
  static int  GetGlobalImmediateModeRendering();

  // Description:
  // Specify range in terms of scalar minimum and maximum (smin,smax). These
  // values are used to map scalars into lookup table.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Update the input to the Mapper.
  virtual void Update();

  // Description:
  // Calculate and return the colors for the input. After invoking this
  // method, use GetColor() on the scalar to get the scalar values. This
  // method may return NULL if no color information is available.
  vtkScalars *GetColors();
  
  // Description:
  // Control how the filter works with scalar point data and cell attribute
  // data.  By default (ScalarModeToDefault), the filter will use point data,
  // and if no point data is available, then cell data is used. Alternatively
  // you can explicitly set the filter to use point data
  // (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
  vtkSetMacro(ScalarMode,int);
  vtkGetMacro(ScalarMode,int);
  void SetScalarModeToDefault() {
    this->SetScalarMode(VTK_SCALAR_MODE_DEFAULT);};
  void SetScalarModeToUsePointData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA);};
  void SetScalarModeToUseCellData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_DATA);};

  // Description:
  // Return the method for obtaining scalar data.
  char *GetScalarModeAsString();

  // Description:
  // Return bounding box (array of six floats) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual float *GetBounds();
  virtual void GetBounds(float bounds[6]) 
    {this->vtkAbstractMapper3D::GetBounds(bounds);};

  // Description:
  // This instance variable is used by vtkLODActor to determine which
  // mapper to use.  It is an estimate of the time necessary to render.
  // Setting the render time does not modify the mapper.
  void SetRenderTime(float time) {this->RenderTime = time;}
  vtkGetMacro(RenderTime, float);

  //BTX
  // Description:
  // Get the input as a vtkDataSet.  This method is overridden in
  // the specialized mapper classes to return more specific data types.
  vtkDataSet *GetInput();
  //ETX

  // Description:
  // Get the input as a vtkDataSet, instead of as a more specialized
  // data type.  This method is provided for use in the wrapper languages,
  // C++ programmers should use GetInput() instead.
  vtkDataSet *GetInputAsDataSet() { return this->GetInput(); };

protected:
  vtkMapper();
  ~vtkMapper();
  vtkMapper(const vtkMapper&) {};
  void operator=(const vtkMapper&) {};

  vtkScalars *Colors;

  vtkScalarsToColors *LookupTable;
  int ScalarVisibility;
  vtkTimeStamp BuildTime;
  float ScalarRange[2];
  int ImmediateModeRendering;
  int ColorMode;
  int ScalarMode;

  float RenderTime;

};

#endif


