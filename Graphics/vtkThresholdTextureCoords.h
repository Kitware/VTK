/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTextureCoords.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThresholdTextureCoords - compute 1D, 2D, or 3D texture coordinates based on scalar threshold
// .SECTION Description
// vtkThresholdTextureCoords is a filter that generates texture coordinates for
// any input dataset type given a threshold criterion. The criterion can take 
// three forms: 1) greater than a particular value (ThresholdByUpper()); 
// 2) less than a particular value (ThresholdByLower(); or 3) between two 
// values (ThresholdBetween(). If the threshold criterion is satisfied, 
// the "in" texture coordinate will be set (this can be specified by the
// user). If the threshold criterion is not satisfied the "out" is set.

// .SECTION Caveats
// There is a texture map - texThres.vtk - that can be used in conjunction
// with this filter. This map defines a "transparent" region for texture 
// coordinates 0<=r<0.5, and an opaque full intensity map for texture 
// coordinates 0.5<r<=1.0. There is a small transition region for r=0.5.

// .SECTION See Also
// vtkThreshold vtkThresholdPoints vtkTextureMapToPlane vtkTextureMapToSphere
// vtkTextureMapToCylinder vtkTextureMapToBox

#ifndef __vtkThresholdTextureCoords_h
#define __vtkThresholdTextureCoords_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkThresholdTextureCoords : public vtkDataSetToDataSetFilter
{
public:
  static vtkThresholdTextureCoords *New();
  vtkTypeRevisionMacro(vtkThresholdTextureCoords,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Criterion is cells whose scalars are less than lower threshold.
  void ThresholdByLower(float lower);

  // Description:
  // Criterion is cells whose scalars are less than upper threshold.
  void ThresholdByUpper(float upper);

  // Description:
  // Criterion is cells whose scalars are between lower and upper thresholds.
  void ThresholdBetween(float lower, float upper);

  // Description:
  // Return the upper and lower thresholds.
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

  // Description:
  // Set the desired dimension of the texture map.
  vtkSetClampMacro(TextureDimension,int,1,3);
  vtkGetMacro(TextureDimension,int);

  // Description:
  // Set the texture coordinate value for point satisfying threshold criterion.
  vtkSetVector3Macro(InTextureCoord,float);
  vtkGetVectorMacro(InTextureCoord,float,3);

  // Description:
  // Set the texture coordinate value for point NOT satisfying threshold
  //  criterion.
  vtkSetVector3Macro(OutTextureCoord,float);
  vtkGetVectorMacro(OutTextureCoord,float,3);

protected:
  vtkThresholdTextureCoords();
  ~vtkThresholdTextureCoords() {};

  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  int TextureDimension;

  float InTextureCoord[3];
  float OutTextureCoord[3];

  //BTX
  int (vtkThresholdTextureCoords::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
private:
  vtkThresholdTextureCoords(const vtkThresholdTextureCoords&);  // Not implemented.
  void operator=(const vtkThresholdTextureCoords&);  // Not implemented.
};

#endif
