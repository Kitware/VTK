/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkPolyDataMapper2D - draw vtkPolyData onto image plane
// .SECTION Description
// vtkPolyDataMapper2D is a mapper that renders 3D polygonal data 
// (vtkPolyData) onto the 2D image plane (i.e., the renderer's viewport).
// The 3D data is transformed into 2D data by ignoring the z-coordinate
// of the 3D points in vtkPolyData.

// .SECTION See Also
// vtkMapper2D vtkActor2D

#ifndef __vtkPolyDataMapper2D_h
#define __vtkPolyDataMapper2D_h


#include "vtkMapper2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkLookupTable.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkPolyDataMapper2D : public vtkMapper2D
{
public:
  vtkPolyDataMapper2D();
  ~vtkPolyDataMapper2D();
  const char *GetClassName() {return "vtkPolyDataMapper2D";};
  static vtkPolyDataMapper2D *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Draw the polydata to the screen.  This function is implemented in
  // the device dependent subclasses.
  virtual void Render(vtkViewport*, vtkActor2D*) {};

  // Description:
  // Set the input to the mapper.  
  vtkGetObjectMacro(Input, vtkPolyData);
  vtkSetObjectMacro(Input, vtkPolyData);


  // Description:
  // Specify a lookup table for the mapper to use.
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
  // Specify range in terms of scalar minimum and maximum (smin,smax). These
  // values are used to map scalars into lookup table.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Calculate and return the colors for the input. After invoking this
  // method, use GetColor() on the scalar to get the scalar values. This
  // method may return NULL if no color information is available.
  vtkScalars *GetColors();


  // Description:
  // Overload standard modified time function. If lookup table is modified,
  // then this object is modified as well.
  virtual unsigned long GetMTime();


protected:
  vtkPolyData* Input;
  vtkScalars *Colors;
  vtkLookupTable *LookupTable;
  int ScalarVisibility;
  vtkTimeStamp BuildTime;
  float ScalarRange[2];
  int ColorMode;

};


#endif

