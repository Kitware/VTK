/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectionTransform.cxx
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
OF THIS EVEN, SOFTWARE IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include "vtkProjectionTransform.h"
#include "vtkPerspectiveTransformInverse.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

// Useful for viewing a double[16] as a double[4][4]
typedef double (*SqMatPtr)[4];


//----------------------------------------------------------------------------
vtkProjectionTransform* vtkProjectionTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProjectionTransform");
  if(ret)
    {
    return (vtkProjectionTransform*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProjectionTransform;
}

//----------------------------------------------------------------------------
vtkProjectionTransform::vtkProjectionTransform()
{
  this->TransformType = VTK_MATRIX4X4_TRANSFORM;
  this->PreMultiplyFlag = 1;
}

//----------------------------------------------------------------------------
vtkProjectionTransform::~vtkProjectionTransform()
{
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPerspectiveTransform::PrintSelf(os, indent);
  os << indent << (this->PreMultiplyFlag ? "PreMultiply\n" : "PostMultiply\n");
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkProjectionTransform::MakeTransform()
{
  return vtkProjectionTransform::New();
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::DeepCopy(vtkGeneralTransform *transform)
{
  if (this->TransformType != transform->GetTransformType() &&
      this->TransformType != transform->GetInverse()->GetTransformType())
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    }
  if (transform->GetTransformType() & VTK_INVERSE_TRANSFORM)
    {
    transform = ((vtkPerspectiveTransformInverse *)transform)->GetTransform(); 
    }	
  vtkProjectionTransform *t = (vtkProjectionTransform *)transform;  

  if (t == this)
    {
    return;
    }

  this->PreMultiplyFlag = t->PreMultiplyFlag;
  this->Matrix->DeepCopy(t->Matrix);
  this->Modified();
}

//----------------------------------------------------------------------------
// Sets the internal state of the ProjectionTransform to
// post multiply. All subsequent matrix
// operations will occur after those already represented
// in the current ProjectionTransformation matrix.
void vtkProjectionTransform::PostMultiply()
{
  if (this->PreMultiplyFlag != 0) 
    {
    this->PreMultiplyFlag = 0;
    this->Modified ();
    }
}

//----------------------------------------------------------------------------
// Sets the internal state of the ProjectionTransform to
// pre multiply. All subsequent matrix
// operations will occur before those already represented
// in the current ProjectionTransformation matrix.
void vtkProjectionTransform::PreMultiply()
{
  if (this->PreMultiplyFlag != 1) 
    {
    this->PreMultiplyFlag = 1;
    this->Modified ();
    }
}

//----------------------------------------------------------------------------
// Set the current matrix directly.
void vtkProjectionTransform::SetMatrix(vtkMatrix4x4 *m)
{
  this->Matrix->DeepCopy(m);
  this->Modified();
}

//----------------------------------------------------------------------------
// Set the current matrix directly.
void vtkProjectionTransform::SetMatrix(double Elements[16])
{
  this->Matrix->DeepCopy(Elements);
  this->Modified();
}

//----------------------------------------------------------------------------
// Creates an identity matrix.
void vtkProjectionTransform::Identity()
{
  this->Matrix->Identity();
  this->Modified();
}

//----------------------------------------------------------------------------
// Creates an identity matrix.
void vtkProjectionTransform::Inverse()
{
  this->Matrix->Invert();
  this->Modified();
}

//----------------------------------------------------------------------------
// Concatenates the input matrix with the current matrix.
// The setting of the PreMultiply flag determines whether the matrix
// is PreConcatenated or PostConcatenated.
void vtkProjectionTransform::Concatenate(vtkMatrix4x4 *matrix)
{
  if (this->PreMultiplyFlag) 
    {
    vtkMatrix4x4::Multiply4x4(this->Matrix, matrix, this->Matrix);
    }
  else 
    {
    vtkMatrix4x4::Multiply4x4(matrix, this->Matrix, this->Matrix);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::Concatenate(double Elements[16])
{
  if (this->PreMultiplyFlag) 
    {
    vtkMatrix4x4::Multiply4x4(*this->Matrix->Element, Elements, 
			      *this->Matrix->Element);
    }
  else 
    {
    vtkMatrix4x4::Multiply4x4(Elements, *this->Matrix->Element, 
			      *this->Matrix->Element);
    }
  this->Matrix->Modified();
  this->Modified();
}


