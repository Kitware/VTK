/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalar.h
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
// .NAME vtkWarpScalar - deform geometry with scalar data
// .SECTION Description
// vtkWarpScalar is a filter that modifies point coordinates by moving
// points along point normals by the scalar amount times the scale factor.
// Useful for creating carpet or x-y-z plots.
//
// If normals are not present in data, the Normal instance variable will
// be used as the direction along which to warp the geometry. If normals are
// present but you would like to use the Normal instance variable, set the 
// UseNormal boolean to true.
//
// If XYPlane boolean is set true, then the z-value is considered to be 
// a scalar value (still scaled by scale factor), and the displacement is
// along the z-axis. If scalars are also present, these are copied through
// and can be used to color the surface.

#ifndef __vtkWarpScalar_h
#define __vtkWarpScalar_h

#include "vtkPointSetToPointSetFilter.h"

class VTK_EXPORT vtkWarpScalar : public vtkPointSetToPointSetFilter
{
public:
  vtkWarpScalar();
  static vtkWarpScalar *New() {return new vtkWarpScalar;};
  const char *GetClassName() {return "vtkWarpScalar";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Turn on/off use of user specified normal. If on, data normals
  // will be ignored and instance variable Normal will be used instead.
  vtkSetMacro(UseNormal,int);
  vtkGetMacro(UseNormal,int);
  vtkBooleanMacro(UseNormal,int);

  // Description:
  // Normal (i.e., direction) along which to warp geometry. Only used
  // if UseNormal boolean set to true or no normals available in data.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Turn on/off flag specifying that input data is x-y plane. If x-y plane,
  // then the z value is used to warp the surface in the z-axis direction 
  // (times the scale factor) and scalars are used to color the surface.
  vtkSetMacro(XYPlane,int);
  vtkGetMacro(XYPlane,int);
  vtkBooleanMacro(XYPlane,int);

protected:
  void Execute();

  float ScaleFactor;
  int UseNormal;
  float Normal[3];
  int XYPlane;

  //BTX
  float *(vtkWarpScalar::*PointNormal)(int id, vtkNormals *normals);
  float *DataNormal(int id, vtkNormals *normals=NULL);
  float *InstanceNormal(int id, vtkNormals *normals=NULL);
  float *ZNormal(int id, vtkNormals *normals=NULL);
  //ETX
};

#endif


