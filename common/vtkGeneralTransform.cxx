/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkGeneralTransform.h"
#include "vtkGeneralTransformInverse.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
void vtkGeneralTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);

  os << indent << "TransformType: " << this->TransformType << "\n";
  os << indent << "AutoUpdate: " << (this->AutoUpdate ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
// If the subclass has not defined a 'double' transform method, then
// use the 'float' transform method instead.  
void vtkGeneralTransform::TransformPoint(const double in[3], double out[3])
{
  float point[3];
  point[0] = in[0];
  point[1] = in[1];
  point[2] = in[2];
  this->TransformPoint(point,point);
  out[0] = point[0];
  out[1] = point[1];
  out[2] = point[2];
}

//----------------------------------------------------------------------------
float *vtkGeneralTransform::TransformFloatPoint(float x, 
						float y, 
						float z)
{
  this->InternalFloatPoint[0] = x;
  this->InternalFloatPoint[1] = y;
  this->InternalFloatPoint[2] = z;
  this->TransformPoint(this->InternalFloatPoint,this->InternalFloatPoint);
  return this->InternalFloatPoint;
}

//----------------------------------------------------------------------------
double *vtkGeneralTransform::TransformDoublePoint(double x,
						  double y,
						  double z)
{
  this->InternalDoublePoint[0] = x;
  this->InternalDoublePoint[1] = y;
  this->InternalDoublePoint[2] = z;
  this->TransformPoint(this->InternalDoublePoint,this->InternalDoublePoint);
  return this->InternalDoublePoint;
}

//----------------------------------------------------------------------------
// If the subclass has not defined a TransformPoints method, then
// call the TransformPoint method for each point.
void vtkGeneralTransform::TransformPoints(vtkPoints *in, vtkPoints *out)
{
  int i;
  int n = in->GetNumberOfPoints();
  float outPnt[3];
  float *inPnt;

  for (i = 0; i < n; i++)
    {
    inPnt = in->GetPoint(i);
    this->TransformPoint(inPnt,outPnt);
    out->InsertNextPoint(outPnt);
    }
}

//----------------------------------------------------------------------------
// If the subclass has not defined a TransformNormals method, then
// this method attempts to approximate the normals. This is only
// a second-order approximation and it is quite expensive, so if you can 
// override this with a precise analytic calculation, you should.
void vtkGeneralTransform::TransformNormals(vtkPoints *inPts, 
					   vtkPoints *outPts,
					   vtkNormals *inNms, 
					   vtkNormals *outNms)
{
  int i;
  int n = inPts->GetNumberOfPoints();
  float offset1[3];
  float offset2[3];
  float point1[3];
  float point2[3];
  float point3[3];
  float point4[3];
  float outNormal[3];
  float outNormalP[3];
  float outNormalN[3];
  float *inPoint,*outPoint,*inNormal;
  float r,f;

  for (i = 0; i < n; i++)
    {
    inNormal = inNms->GetNormal(i);
    inPoint = inPts->GetPoint(i);
    outPoint = outPts->GetPoint(i);

    // Set the scale to use in the
    // calculation of the output normal.
    f = 0.1;

    // find two vectors which are perpendicular to the normal
    vtkMath::Perpendiculars(inNormal,offset1,offset2,0);
    
    // use these vectors to find the four vertices of a
    // square which is perpendicular to the normal
    point1[0] = inPoint[0] + f*offset1[0]; 
    point1[1] = inPoint[1] + f*offset1[1]; 
    point1[2] = inPoint[2] + f*offset1[2]; 

    point2[0] = inPoint[0] + f*offset2[0]; 
    point2[1] = inPoint[1] + f*offset2[1]; 
    point2[2] = inPoint[2] + f*offset2[2];

    point3[0] = inPoint[0] - f*offset1[0]; 
    point3[1] = inPoint[1] - f*offset1[1]; 
    point3[2] = inPoint[2] - f*offset1[2]; 

    point4[0] = inPoint[0] - f*offset2[0]; 
    point4[1] = inPoint[1] - f*offset2[1]; 
    point4[2] = inPoint[2] - f*offset2[2];

    // transform the square
    this->TransformPoint(point1,point1);
    this->TransformPoint(point2,point2);
    this->TransformPoint(point3,point3);
    this->TransformPoint(point4,point4);

    // find new offsets
    offset1[0] = point1[0] - outPoint[0];
    offset1[1] = point1[1] - outPoint[1];
    offset1[2] = point1[2] - outPoint[2];

    offset2[0] = point2[0] - outPoint[0];
    offset2[1] = point2[1] - outPoint[1];
    offset2[2] = point2[2] - outPoint[2];

    // find the new normal from positive offsets
    vtkMath::Cross(offset1,offset2,outNormalP);
    r = sqrt(outNormalP[0]*outNormalP[0] + 
	     outNormalP[1]*outNormalP[1] + 
	     outNormalP[2]*outNormalP[2]);
    outNormalP[0] /= r;
    outNormalP[1] /= r;
    outNormalP[2] /= r;

    // find negative offsets
    offset1[0] = point3[0] - outPoint[0];
    offset1[1] = point3[1] - outPoint[1];
    offset1[2] = point3[2] - outPoint[2];

    offset2[0] = point4[0] - outPoint[0];
    offset2[1] = point4[1] - outPoint[1];
    offset2[2] = point4[2] - outPoint[2];

    // find the new normal from negative offsets
    vtkMath::Cross(offset1,offset2,outNormalN);
    r = sqrt(outNormalN[0]*outNormalN[0] + 
	     outNormalN[1]*outNormalN[1] + 
	     outNormalN[2]*outNormalN[2]);
    outNormalN[0] /= r;
    outNormalN[1] /= r;
    outNormalN[2] /= r;
    
    // Take the average (this is equivalent to spherical interpolation)
    outNormal[0] = outNormalP[0] + outNormalN[0];
    outNormal[1] = outNormalP[1] + outNormalN[1];
    outNormal[2] = outNormalP[2] + outNormalN[2];
    
    r = sqrt(outNormal[0]*outNormal[0] + 
	     outNormal[1]*outNormal[1] + 
	     outNormal[2]*outNormal[2]);

    outNormal[0] /= r;
    outNormal[1] /= r;
    outNormal[2] /= r;
    
    outNms->InsertNextNormal(outNormal);
    }
}

//----------------------------------------------------------------------------
// If the subclass has not defined a TransformVectors method, then
// transform the vectors by transforming the two end points.
void vtkGeneralTransform::TransformVectors(vtkPoints *inPts, 
					   vtkPoints *outPts,
					   vtkVectors *inVrs, 
					   vtkVectors *outVrs)
{
  int i;
  int n = inPts->GetNumberOfPoints();
  float tmp[3];
  float *point,*vec;

  for (i = 0; i < n; i++)
    {
    vec = inVrs->GetVector(i);
    point = inPts->GetPoint(i);
    tmp[0] = vec[0] + point[0];
    tmp[1] = vec[1] + point[1];
    tmp[2] = vec[2] + point[2];
    this->TransformPoint(tmp,tmp);
    point = outPts->GetPoint(i);
    tmp[0] -= point[0];
    tmp[1] -= point[1];
    tmp[2] -= point[2];
    outVrs->InsertNextVector(tmp);
    }
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkGeneralTransform::GetInverse()
{
  if (this->MyInverse == NULL)
    {
    this->MyInverse = vtkGeneralTransformInverse::New();
    // we create a circular reference here, it is dealt
    // in UnRegister
    this->MyInverse->SetOriginalTransform(this);
    }
  return this->MyInverse;
}

//----------------------------------------------------------------------------
// update: do nothing in base class
void vtkGeneralTransform::Update()
{
}

//----------------------------------------------------------------------------
// need to handle the circular reference between a transform and its
// inverse
void vtkGeneralTransform::UnRegister(vtkObject *o)
{
  if (this->InUnRegister)
    {
    this->ReferenceCount--;
    return;
    }

  // 'this' is reference by this->MyInverse
  if (this->ReferenceCount == 2 && this->MyInverse &&
      this->MyInverse->GetReferenceCount() == 1 &&
      this->MyInverse->GetOriginalTransform() == this)
    { // break the cycle
    this->InUnRegister = 1;
    this->MyInverse->Delete();
    this->MyInverse = NULL;
    this->InUnRegister = 0;
    }

  this->vtkObject::UnRegister(o);
}




