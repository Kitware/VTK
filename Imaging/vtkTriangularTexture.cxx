/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTexture.cxx
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
#include "vtkTriangularTexture.h"
#include "vtkMath.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkTriangularTexture* vtkTriangularTexture::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTriangularTexture");
  if(ret)
    {
    return (vtkTriangularTexture*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTriangularTexture;
}




// Instantiate object with XSize and YSize = 64; the texture pattern =1
// (opaque at centroid); and the scale factor set to 1.0.

vtkTriangularTexture::vtkTriangularTexture()
{
  this->XSize = this->YSize = 64;
  this->TexturePattern = 1;
  this->ScaleFactor = 1.0;
}

static void OpaqueAtElementCentroid (int XSize, int YSize, float ScaleFactor, 
				     vtkUnsignedCharArray *newScalars)
{
  int i, j;
  float opacity;
  float point[3];
  float XScale = XSize + 1.0;
  float YScale = YSize + 1.0;
  unsigned char AGrayValue[2];
  float dist, distToV2, distToV3;
  float v1[3] = {0.0, 0.0, 0.0};
  float v2[3] = {1.0, 0.0, 0.0};
  float v3[3] = {0.5, sqrt(3.0)/2.0, 0.0};

  point[2] = 0.0;
  AGrayValue[0] = AGrayValue[1] = 255;

  for (j = 0; j < YSize; j++)
    {
    for (i = 0; i < XSize; i++) 
      {
      point[0] = i / XScale;
      point[1] = j / YScale;
      dist = vtkMath::Distance2BetweenPoints (point, v1);
      distToV2 = vtkMath::Distance2BetweenPoints (point, v2);
      if (distToV2 < dist)
	{
	dist = distToV2;
	}
      distToV3 = vtkMath::Distance2BetweenPoints (point, v3);
      if (distToV3 < dist)
	{
	dist = distToV3;
	}

      opacity = sqrt(dist) * ScaleFactor;
      if (opacity < .5)
	{
	opacity = 0.0;
	}
      if (opacity > .5)
	{
	opacity = 1.0;
	}
      AGrayValue[1] = (unsigned char) (opacity * 255);
      newScalars->InsertNextValue (AGrayValue[0]);
      newScalars->InsertNextValue (AGrayValue[1]);
      } 	
    }
}

static void OpaqueAtVertices (int XSize, int YSize, float ScaleFactor, 
                              vtkUnsignedCharArray *newScalars)
{
  int i, j;
  float opacity;
  float point[3];
  float XScale = XSize + 1.0;
  float YScale = YSize + 1.0;
  unsigned char AGrayValue[2];
  float dist, distToV2, distToV3;
  float v1[3] = {0.0, 0.0, 0.0};
  float v2[3] = {1.0, 0.0, 0.0};
  float v3[3] = {0.5, sqrt(3.0)/2.0, 0.0};

  point[2] = 0.0;
  AGrayValue[0] = AGrayValue[1] = 255;

  for (j = 0; j < YSize; j++) 
    {
    for (i = 0; i < XSize; i++) 
      {
      point[0] = i / XScale;
      point[1] = j / YScale;
      dist = vtkMath::Distance2BetweenPoints (point, v1);
      distToV2 = vtkMath::Distance2BetweenPoints (point, v2);
      if (distToV2 < dist)
	{
	dist = distToV2;
	}
      distToV3 = vtkMath::Distance2BetweenPoints (point, v3);
      if (distToV3 < dist)
	{
	dist = distToV3;
	}

      opacity = sqrt(dist) * ScaleFactor;
      if (opacity < .5)
	{
	opacity = 0.0;
	}
      if (opacity > .5)
	{
	opacity = 1.0;
	}
      opacity = 1.0 - opacity;
      AGrayValue[1] = (unsigned char) (opacity * 255);
      newScalars->InsertNextValue (AGrayValue[0]);
      newScalars->InsertNextValue (AGrayValue[1]);
      } 	
    }
}

void vtkTriangularTexture::Execute()
{
  int numPts;
  vtkUnsignedCharArray *newScalars;
  vtkUnsignedCharArray *data;
  vtkStructuredPoints *output = this->GetOutput();
  
  if ( (numPts = this->XSize * this->YSize) < 1 )
    {
    vtkErrorMacro(<<"Bad texture (xsize,ysize) specification!");
    return;
    }

  output->SetDimensions(this->XSize,this->YSize,1);
  newScalars = vtkUnsignedCharArray::New();
  newScalars->SetNumberOfComponents(2);
  newScalars->Allocate(2*numPts);

  switch (this->TexturePattern) 
    {
    case 1: // opaque at element vertices
      OpaqueAtVertices (this->XSize, this->YSize, this->ScaleFactor, newScalars);
      break;

    case 2: // opaque at element centroid
      OpaqueAtElementCentroid (this->XSize, this->YSize, this->ScaleFactor, newScalars);
      break;

    case 3: // opaque in rings around vertices
      vtkErrorMacro(<<"Opaque vertex rings not implemented");
      break;
    }

//
// Update the output data
//
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkTriangularTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "XSize:" << this->XSize << "\n";
  os << indent << "YSize:" << this->YSize << "\n";

  os << indent << "Texture Pattern:" << this->TexturePattern << "\n";

  os << indent << "Scale Factor:" << this->ScaleFactor << "\n";
}

