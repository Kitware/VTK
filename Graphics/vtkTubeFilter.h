/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTubeFilter.h
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
// .NAME vtkTubeFilter - filter that generates tubes around lines
// .SECTION Description
// vtkTubeFilter is a filter that generates a tube around each input line. 
// The tubes are made up of triangle strips and rotate around the tube with
// the rotation of the line normals. (If no normals are present, they are
// computed automatically.) The radius of the tube can be set to vary with 
// scalar or vector value. If the radius varies with scalar value the radius
// is linearly adjusted. If the radius varies with vector value, a mass
// flux preserving variation is used. The number of sides for the tube also 
// can be specified. You can also specify which of the sides are visible. This
// is useful for generating interesting striping effects.
//
// This filter is typically used to create thick or dramatic lines. Another
// common use is to combine this filter with vtkStreamLine to generate
// streamtubes.

// .SECTION Caveats
// The number of tube sides must be greater than 3. If you wish to use fewer
// sides (i.e., a ribbon), use vtkRibbonFilter.
//
// The input line must not have duplicate points, or normals at points that
// are parallel to the incoming/outgoing line segments. (Duplicate points
// can be removed with vtkCleanPolyData.)

// .SECTION See Also
// vtkRibbonFilter vtkStreamLine

#ifndef __vtkTubeFilter_h
#define __vtkTubeFilter_h

#include "vtkPolyDataToPolyDataFilter.h"

#define VTK_VARY_RADIUS_OFF 0
#define VTK_VARY_RADIUS_BY_SCALAR 1
#define VTK_VARY_RADIUS_BY_VECTOR 2

class VTK_EXPORT vtkTubeFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkTubeFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with radius 0.5, radius variation turned off, the number 
  // of sides set to 3, and radius factor of 10.
  static vtkTubeFilter *New();

  // Description:
  // Set the minimum tube radius (minimum because the tube radius may vary).
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Turn on/off the variation of tube radius with scalar value.
  vtkSetClampMacro(VaryRadius,int,
                   VTK_VARY_RADIUS_OFF,VTK_VARY_RADIUS_BY_VECTOR);
  vtkGetMacro(VaryRadius,int);
  void SetVaryRadiusToVaryRadiusOff()
    {this->SetVaryRadius(VTK_VARY_RADIUS_OFF);};
  void SetVaryRadiusToVaryRadiusByScalar()
    {this->SetVaryRadius(VTK_VARY_RADIUS_BY_SCALAR);};
  void SetVaryRadiusToVaryRadiusByVector()
    {this->SetVaryRadius(VTK_VARY_RADIUS_BY_VECTOR);};
  const char *GetVaryRadiusAsString();

  // Description:
  // Set the number of sides for the tube. At a minimum, number of sides is 3.
  vtkSetClampMacro(NumberOfSides,int,3,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfSides,int);

  // Description:
  // Set the maximum tube radius in terms of a multiple of the minimum radius.
  vtkSetMacro(RadiusFactor,float);
  vtkGetMacro(RadiusFactor,float);

  // Description:
  // Set the default normal to use if no normals are supplied, and the
  // DefaultNormalOn is set.
  vtkSetVector3Macro(DefaultNormal,float);
  vtkGetVectorMacro(DefaultNormal,float,3);

  // Description:
  // Set a boolean to control whether to use default normals.
  // DefaultNormalOn is set.
  vtkSetMacro(UseDefaultNormal,int);
  vtkGetMacro(UseDefaultNormal,int);
  vtkBooleanMacro(UseDefaultNormal,int);

  // Description:
  // Turn on/off whether to cap the ends with polygons.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

  // Description:
  // Control the striping of the tubes. If OnRatio is greater than 1,
  // then every nth tube side is turned on, beginning with the Offset
  // side.
  vtkSetClampMacro(OnRatio,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Control the striping of the tubes. The offset sets the
  // first tube side that is visible. Offset is generally used with
  // OnRatio to create nifty striping effects.
  vtkSetClampMacro(Offset,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(Offset,int);

protected:
  vtkTubeFilter();
  ~vtkTubeFilter() {};
  vtkTubeFilter(const vtkTubeFilter&);
  void operator=(const vtkTubeFilter&);

  // Usual data generation method
  void Execute();

  float Radius; //minimum radius of tube
  int VaryRadius; //controls radius variation
  int NumberOfSides; //number of sides to create tube
  float RadiusFactor; //maxium allowablew radius
  float DefaultNormal[3];
  int UseDefaultNormal;
  int Capping;
  int OnRatio; //control the generation of the sides of the tube
  int Offset;  //control the generation of the sides
  
};

// Description:
// Return the method of varying tube radius descriptive character string.
inline const char *vtkTubeFilter::GetVaryRadiusAsString(void)
{
  if ( this->VaryRadius == VTK_VARY_RADIUS_OFF )
    {
    return "VaryRadiusOff";
    }
  else if ( this->VaryRadius == VTK_VARY_RADIUS_BY_SCALAR ) 
    {
    return "VaryRadiusByScalar";
    }
  else 
    {
    return "VaryRadiusByVector";
    }
}


#endif
