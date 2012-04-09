/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeneralTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkGeneralTransform);

//----------------------------------------------------------------------------
vtkGeneralTransform::vtkGeneralTransform()
{
  this->Input = NULL;

  // most of the functionality is provided by the concatenation
  this->Concatenation = vtkTransformConcatenation::New();

  // the stack will be allocated the first time Push is called
  this->Stack = NULL;
}

//----------------------------------------------------------------------------
vtkGeneralTransform::~vtkGeneralTransform()
{
  this->SetInput(NULL);

  if (this->Concatenation)
    {
    this->Concatenation->Delete();
    }
  if (this->Stack)
    {
    this->Stack->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "InverseFlag: " << this->GetInverseFlag() << "\n";
  os << indent << "NumberOfConcatenatedTransforms: " <<
    this->GetNumberOfConcatenatedTransforms() << "\n";
  if (this->GetNumberOfConcatenatedTransforms() != 0)
    {
    int n = this->GetNumberOfConcatenatedTransforms();
    for (int i = 0; i < n; i++)
      {
      vtkAbstractTransform *t = this->GetConcatenatedTransform(i);
      os << indent << "    " << i << ": " << t->GetClassName() << " at " <<
         t << "\n";
      }
    }
}

//------------------------------------------------------------------------
// Pass the point through each transform in turn
template<class T2, class T3>
void vtkConcatenationTransformPoint(vtkAbstractTransform *input,
                                    vtkTransformConcatenation *concat,
                                    T2 point[3], T3 output[3])
{
  output[0] = point[0];
  output[1] = point[1];
  output[2] = point[2];

  int i = 0;
  int nTransforms = concat->GetNumberOfTransforms();
  int nPreTransforms = concat->GetNumberOfPreTransforms();

  // push point through the PreTransforms
  for (; i < nPreTransforms; i++)
    {
    concat->GetTransform(i)->InternalTransformPoint(output,output);
    }

  // push point though the Input, if present
  if (input)
    {
    if (concat->GetInverseFlag())
      {
      input = input->GetInverse();
      }
    input->InternalTransformPoint(output,output);
    }

  // push point through PostTransforms
  for (; i < nTransforms; i++)
    {
    concat->GetTransform(i)->InternalTransformPoint(output,output);
    }
}

//----------------------------------------------------------------------------
// Pass the point through each transform in turn,
// concatenate the derivatives.
template<class T2, class T3, class T4>
void vtkConcatenationTransformDerivative(
  vtkAbstractTransform *input,
  vtkTransformConcatenation *concat,
  T2 point[3], T3 output[3],
  T4 derivative[3][3])
{
  T4 matrix[3][3];

  output[0] = point[0];
  output[1] = point[1];
  output[2] = point[2];

  vtkMath::Identity3x3(derivative);

  int i = 0;
  int nTransforms = concat->GetNumberOfTransforms();
  int nPreTransforms = concat->GetNumberOfPreTransforms();

  // push point through the PreTransforms
  for (; i < nPreTransforms; i++)
    {
    concat->GetTransform(i)->InternalTransformDerivative(output,output,matrix);
    vtkMath::Multiply3x3(matrix,derivative,derivative);
    }

  // push point though the Input, if present
  if (input)
    {
    if (concat->GetInverseFlag())
      {
      input = input->GetInverse();
      }
    input->InternalTransformDerivative(output,output,matrix);
    vtkMath::Multiply3x3(matrix,derivative,derivative);
    }

  // push point through PostTransforms
  for (; i < nTransforms; i++)
    {
    concat->GetTransform(i)->InternalTransformDerivative(output,output,matrix);
    vtkMath::Multiply3x3(matrix,derivative,derivative);
    }
}

//------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformPoint(const float input[3],
                                                 float output[3])
{
  vtkConcatenationTransformPoint(this->Input,this->Concatenation,input,output);
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformPoint(const double input[3],
                                                 double output[3])
{
  vtkConcatenationTransformPoint(this->Input,this->Concatenation,input,output);
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformDerivative(const float input[3],
                                                      float output[3],
                                                      float derivative[3][3])
{
  vtkConcatenationTransformDerivative(this->Input,this->Concatenation,
                                      input,output,derivative);
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformDerivative(const double input[3],
                                                      double output[3],
                                                      double derivative[3][3])
{
  vtkConcatenationTransformDerivative(this->Input,this->Concatenation,
                                      input,output,derivative);
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalDeepCopy(vtkAbstractTransform *gtrans)
{
  vtkGeneralTransform *transform =
    static_cast<vtkGeneralTransform *>(gtrans);

  // copy the input
  this->SetInput(transform->Input);

  // copy the concatenation
  this->Concatenation->DeepCopy(transform->Concatenation);

  // copy the stack
  if (transform->Stack)
    {
    if (this->Stack == NULL)
      {
      this->Stack = vtkTransformConcatenationStack::New();
      }
    this->Stack->DeepCopy(transform->Stack);
    }
  else
    {
    if (this->Stack)
      {
      this->Stack->Delete();
      this->Stack = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalUpdate()
{
  // update the input
  if (this->Input)
    {
    if (this->Concatenation->GetInverseFlag())
      {
      this->Input->GetInverse()->Update();
      }
    else
      {
      this->Input->Update();
      }
    }

  // update the concatenation
  int nTransforms = this->Concatenation->GetNumberOfTransforms();
  for (int i = 0; i < nTransforms; i++)
    {
    this->Concatenation->GetTransform(i)->Update();
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::Concatenate(vtkAbstractTransform *transform)
{
  if (transform->CircuitCheck(this))
    {
    vtkErrorMacro("Concatenate: this would create a circular reference.");
    return;
    }
  this->Concatenation->Concatenate(transform);
  this->Modified();
};

//----------------------------------------------------------------------------
void vtkGeneralTransform::SetInput(vtkAbstractTransform *input)
{
  if (this->Input == input)
    {
    return;
    }
  if (input && input->CircuitCheck(this))
    {
    vtkErrorMacro("SetInput: this would create a circular reference.");
    return;
    }
  if (this->Input)
    {
    this->Input->Delete();
    }
  this->Input = input;
  if (this->Input)
    {
    this->Input->Register(this);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkGeneralTransform::CircuitCheck(vtkAbstractTransform *transform)
{
  if (this->vtkAbstractTransform::CircuitCheck(transform) ||
      (this->Input && this->Input->CircuitCheck(transform)))
    {
    return 1;
    }

  int n = this->Concatenation->GetNumberOfTransforms();
  for (int i = 0; i < n; i++)
    {
    if (this->Concatenation->GetTransform(i)->CircuitCheck(transform))
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkGeneralTransform::MakeTransform()
{
  return vtkGeneralTransform::New();
}

//----------------------------------------------------------------------------
unsigned long vtkGeneralTransform::GetMTime()
{
  unsigned long mtime = this->vtkAbstractTransform::GetMTime();
  unsigned long mtime2;

  if (this->Input)
    {
    mtime2 = this->Input->GetMTime();
    if (mtime2 > mtime)
      {
      mtime = mtime2;
      }
    }
  mtime2 = this->Concatenation->GetMaxMTime();
  if (mtime2 > mtime)
    {
    return mtime2;
    }
  return mtime;
}



