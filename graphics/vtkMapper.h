/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkMapper - abstract class specifies interface to map data to graphics primitives
// .SECTION Description
// vtkMapper is an abstract class to specify interface between data and 
// graphics primitives. Subclasses of vtkMapper map data through a 
// lookuptable and control the creation of rendering primitives that
// interface to the graphics library. The mapping can be controlled by 
// supplying a lookup table and specifying a scalar range to map data
// through.

// .SECTION See Also
// vtkDataSetMapper vtkPolyDataMapper

#ifndef __vtkMapper_h
#define __vtkMapper_h

#include "vtkProcessObject.h"
#include "vtkLookupTable.h"
#include "vtkDataSet.h"

class vtkRenderer;
class vtkActor;

class VTK_EXPORT vtkMapper : public vtkProcessObject
{
public:
  vtkMapper();
  ~vtkMapper();
  const char *GetClassName() {return "vtkMapper";};
  void PrintSelf(ostream& os, vtkIndent indent);
  void operator=(const vtkMapper& m);

  unsigned long int GetMTime();

  // Description:
  // Method initiates the mapping process. Generally sent by the actor 
  // as each frame is rendered.
  virtual void Render(vtkRenderer *ren, vtkActor *a) = 0;

  void SetLookupTable(vtkLookupTable *lut);
  void SetLookupTable(vtkLookupTable& lut) {this->SetLookupTable(&lut);};
  vtkLookupTable *GetLookupTable();

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
  void SetColorModeToDefault() 
    {this->SetColorMode(VTK_COLOR_MODE_DEFAULT);};
  void SetColorModeToMapScalars() 
    {this->SetColorMode(VTK_COLOR_MODE_MAP_SCALARS);};
  void SetColorModeToLuminance() 
    {this->SetColorMode(VTK_COLOR_MODE_LUMINANCE);};
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
  static void GlobalImmediateModeRenderingOn() 
  {vtkMapper::SetGlobalImmediateModeRendering(1);};
  static void GlobalImmediateModeRenderingOff() 
  {vtkMapper::SetGlobalImmediateModeRendering(0);};
  static int  GetGlobalImmediateModeRendering();

  // Description:
  // Specify range in terms of scalar minimum and maximum (smin,smax). These
  // values are used to map scalars into lookup table.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Return bounding box of data in terms of (xmin,xmax, ymin,ymax, zmin,zmax).
  // Used in the rendering process to automatically create a camera in the 
  // proper initial configuration.
  virtual float *GetBounds() = 0;

  float *GetCenter();

  // Description:
  // Update the network connected to this mapper.
  virtual void Update();

  virtual vtkDataSet *GetInput() {return this->Input;};

  // Description:
  // Calculate and return the colors for the input. After invoking this
  // method, use GetColor() on the scalar to get the scalar values. This
  // method may return NULL if no color information is available.
  vtkScalars *GetColors();
  
protected:
  vtkDataSet *Input;
  vtkScalars *Colors;

  vtkLookupTable *LookupTable;
  int ScalarVisibility;
  vtkTimeStamp BuildTime;
  float ScalarRange[2];
  int SelfCreatedLookupTable;
  int ImmediateModeRendering;
  int ColorMode;
  
};

#endif


