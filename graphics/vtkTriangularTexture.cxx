/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTexture.cxx
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
#include "vtkTriangularTexture.h"
#include "vtkAGraymap.h"
#include "vtkMath.h"

// Description:
// Instantiate object with XSize and YSize = 64; the texture pattern =1
// (opaque at centroid); and the scale factor set to 1.0.

vtkTriangularTexture::vtkTriangularTexture()
{
  this->XSize = this->YSize = 64;
  this->TexturePattern = 1;
  this->ScaleFactor = 1.0;
}

static void OpaqueAtElementCentroid (int XSize, int YSize, float ScaleFactor, vtkAGraymap *newScalars)
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
      if (distToV2 < dist) dist = distToV2;
      distToV3 = vtkMath::Distance2BetweenPoints (point, v3);
      if (distToV3 < dist) dist = distToV3;

      opacity = sqrt(dist) * ScaleFactor;
      if (opacity < .5) opacity = 0.0;
      if (opacity > .5) opacity = 1.0;
      AGrayValue[1] = (unsigned char) (opacity * 255);
      newScalars->InsertNextAGrayValue (AGrayValue);
      } 	
    }
}

static void OpaqueAtVertices (int XSize, int YSize, float ScaleFactor, 
                              vtkAGraymap *newScalars)
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
      if (distToV2 < dist) dist = distToV2;
      distToV3 = vtkMath::Distance2BetweenPoints (point, v3);
      if (distToV3 < dist) dist = distToV3;

      opacity = sqrt(dist) * ScaleFactor;
      if (opacity < .5) opacity = 0.0;
      if (opacity > .5) opacity = 1.0;
      opacity = 1.0 - opacity;
      AGrayValue[1] = (unsigned char) (opacity * 255);
      newScalars->InsertNextAGrayValue (AGrayValue);
      } 	
    }
}

void vtkTriangularTexture::Execute()
{
  int numPts;
  vtkAGraymap *newScalars;
  vtkStructuredPoints *output = this->GetOutput();
  
  if ( (numPts = this->XSize * this->YSize) < 1 )
    {
    vtkErrorMacro(<<"Bad texture (xsize,ysize) specification!");
    return;
    }

  output->SetDimensions(this->XSize,this->YSize,1);
  newScalars = vtkAGraymap::New();
  newScalars->Allocate(numPts);

  switch (this->TexturePattern) 
    {
    case 1: // opaque at element vertices
        OpaqueAtVertices (this->XSize, this->YSize, this->ScaleFactor, 
                          newScalars);
	break;

    case 2: // opaque at element centroid
        OpaqueAtElementCentroid (this->XSize, this->YSize, this->ScaleFactor, 
                                 newScalars);
	break;

    case 3: // opaque in rings around vertices
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

