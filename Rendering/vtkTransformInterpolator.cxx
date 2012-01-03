/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransformInterpolator.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkProp3D.h"
#include "vtkTupleInterpolator.h"
#include "vtkQuaternionInterpolator.h"
#include <list>

vtkStandardNewMacro(vtkTransformInterpolator);

// PIMPL STL encapsulation for list of transforms, and list of
// quaternions. This just keeps track of all the data the user specifies,
// which is later dumped into the interpolators.
struct vtkQTransform
{
  double Time;
  double P[3];
  double S[3];
  double Q[4];

  vtkQTransform()
    {
      this->Time = 0.0;
      this->P[0] = this->P[1] = this->P[2] = 0.0;
      this->S[0] = this->S[1] = this->S[2] = 0.0;
      this->Q[0] = this->Q[1] = this->Q[2] = this->Q[3] = 0.0;
    }
  vtkQTransform(double t, vtkTransform *xform)
    {
      this->Time = t;
      if ( xform )
        {
        xform->GetPosition(this->P);
        xform->GetScale(this->S);
        xform->GetOrientationWXYZ(this->Q); //Rotation (in degrees) around unit vector
        }
      else
        {
        this->P[0] = this->P[1] = this->P[2] = 0.0;
        this->S[0] = this->S[1] = this->S[2] = 0.0;
        this->Q[0] = this->Q[1] = this->Q[2] = this->Q[3] = 0.0;
        }
    }
};

// The list is arranged in increasing order in T
class vtkTransformList : public std::list<vtkQTransform> {};
typedef vtkTransformList::iterator TransformListIterator;


//----------------------------------------------------------------------------
vtkTransformInterpolator::vtkTransformInterpolator()
{
  // Set up the interpolation
  this->InterpolationType = INTERPOLATION_TYPE_SPLINE;

  // Spline interpolation
  this->PositionInterpolator = vtkTupleInterpolator::New();
  this->ScaleInterpolator = vtkTupleInterpolator::New();
  this->RotationInterpolator = vtkQuaternionInterpolator::New();
  
  // Quaternion interpolation
  this->TransformList = new vtkTransformList;
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkTransformInterpolator::~vtkTransformInterpolator()
{
  delete this->TransformList;

  if ( this->PositionInterpolator )
    {
    this->PositionInterpolator->Delete();
    }
  if ( this->ScaleInterpolator )
    {
    this->ScaleInterpolator->Delete();
    }
  if ( this->RotationInterpolator )
    {
    this->RotationInterpolator->Delete();
    }
}

//----------------------------------------------------------------------------
unsigned long vtkTransformInterpolator::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long posMTime, scaleMTime, rotMTime;
  
  if ( this->PositionInterpolator )
    {
    posMTime = this->PositionInterpolator->GetMTime();
    mTime = (posMTime > mTime ? posMTime : mTime);
    }
  if ( this->ScaleInterpolator )
    {
    scaleMTime = this->ScaleInterpolator->GetMTime();
    mTime = (scaleMTime > mTime ? scaleMTime : mTime);
    }
  if ( this->RotationInterpolator )
    {
    rotMTime = this->RotationInterpolator->GetMTime();
    mTime = (rotMTime > mTime ? rotMTime : mTime);
    }

  return mTime;
}


//----------------------------------------------------------------------------
int vtkTransformInterpolator::GetNumberOfTransforms()
{
  return static_cast<int>(this->TransformList->size());
}


//----------------------------------------------------------------------------
double vtkTransformInterpolator::GetMinimumT()
{
  if ( this->TransformList->empty() )
    {
    return -VTK_LARGE_FLOAT;
    }
  else
    {
    return this->TransformList->front().Time;
    }
}


//----------------------------------------------------------------------------
double vtkTransformInterpolator::GetMaximumT()
{
  if ( this->TransformList->empty() )
    {
    return VTK_LARGE_FLOAT;
    }
  else
    {
    return this->TransformList->back().Time;
    }
}


//----------------------------------------------------------------------------
void vtkTransformInterpolator::Initialize()
{
  this->TransformList->clear();
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::AddTransform(double t, vtkTransform *xform)
{
  int size = static_cast<int>(this->TransformList->size());

  // Check special cases: t at beginning or end of list
  if ( size <= 0 || t < this->TransformList->front().Time )
    {
    this->TransformList->push_front(vtkQTransform(t,xform));
    return;
    }
  else if ( t > this->TransformList->back().Time )
    {
    this->TransformList->push_back(vtkQTransform(t,xform));
    return;
    }
  else if ( size == 1 && t == this->TransformList->back().Time )
    {
    this->TransformList->front() = vtkQTransform(t,xform);
    return;
    }

  // Okay, insert in sorted order
  TransformListIterator iter = this->TransformList->begin();
  TransformListIterator nextIter = ++(this->TransformList->begin());
  for (int i=0; i < (size-1); i++, ++iter, ++nextIter)
    {
    if ( t == iter->Time )
      {
      (*iter) = vtkQTransform(t,xform);
      }
    else if ( t > iter->Time && t < nextIter->Time )
      {
      this->TransformList->insert(nextIter, vtkQTransform(t,xform));
      }
    }
  
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::AddTransform(double t, vtkMatrix4x4 *matrix)
{
  vtkTransform *xform = vtkTransform::New();
  xform->SetMatrix(matrix);
  this->AddTransform(t,xform);
  xform->Delete();
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::AddTransform(double t, vtkProp3D *prop3D)
{
  this->AddTransform(t,prop3D->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::RemoveTransform(double t)
{
  if ( t < this->TransformList->front().Time ||
       t > this->TransformList->back().Time )
    {
    return;
    }
  
  TransformListIterator iter = this->TransformList->begin();
  for ( ; iter->Time != t && iter != this->TransformList->end(); ++iter )
    {
    }
  if ( iter != this->TransformList->end() )
    {
    this->TransformList->erase(iter);
    }
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::SetPositionInterpolator(vtkTupleInterpolator *pi)
{
  if ( this->PositionInterpolator != pi )
    {
    if ( this->PositionInterpolator != NULL )
      {
      this->PositionInterpolator->Delete();
      }
    this->PositionInterpolator = pi;
    if ( this->PositionInterpolator != NULL )
      {
      this->PositionInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::SetScaleInterpolator(vtkTupleInterpolator *si)
{
  if ( this->ScaleInterpolator != si )
    {
    if ( this->ScaleInterpolator != NULL )
      {
      this->ScaleInterpolator->Delete();
      }
    this->ScaleInterpolator = si;
    if ( this->ScaleInterpolator != NULL )
      {
      this->ScaleInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::SetRotationInterpolator(vtkQuaternionInterpolator *ri)
{
  if ( this->RotationInterpolator != ri )
    {
    if ( this->RotationInterpolator != NULL )
      {
      this->RotationInterpolator->Delete();
      }
    this->RotationInterpolator = ri;
    if ( this->RotationInterpolator != NULL )
      {
      this->RotationInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::InitializeInterpolation()
{
  if ( this->TransformList->empty() )
    {
    return;
    }

  // Set up the interpolators if we need to
  if ( !this->Initialized || this->GetMTime() > this->InitializeTime )
    {
    if ( !this->PositionInterpolator )
      {
      this->PositionInterpolator = vtkTupleInterpolator::New();
      }
    if ( !this->ScaleInterpolator )
      {
      this->ScaleInterpolator = vtkTupleInterpolator::New();
      }
    if ( !this->RotationInterpolator )
      {
      this->RotationInterpolator = vtkQuaternionInterpolator::New();
      }
    
    this->PositionInterpolator->Initialize();
    this->ScaleInterpolator->Initialize();
    this->RotationInterpolator->Initialize();

    this->PositionInterpolator->SetNumberOfComponents(3);
    this->ScaleInterpolator->SetNumberOfComponents(3);

    if ( this->InterpolationType == INTERPOLATION_TYPE_LINEAR )
      {
      this->PositionInterpolator->SetInterpolationTypeToLinear();
      this->ScaleInterpolator->SetInterpolationTypeToLinear();
      this->RotationInterpolator->SetInterpolationTypeToLinear();
      }
    else if ( this->InterpolationType == INTERPOLATION_TYPE_SPLINE )
      {
      this->PositionInterpolator->SetInterpolationTypeToSpline();
      this->ScaleInterpolator->SetInterpolationTypeToSpline();
      this->RotationInterpolator->SetInterpolationTypeToSpline();
      }
    else
      {
      ; //manual override, user manipulates interpolators directly
      }
    
    // Okay, now we can load the interpolators with data
    TransformListIterator iter = this->TransformList->begin();
    for ( ; iter != this->TransformList->end(); ++iter)
      {
      this->PositionInterpolator->AddTuple(iter->Time,iter->P);
      this->ScaleInterpolator->AddTuple(iter->Time,iter->S);
      this->RotationInterpolator->AddQuaternion(iter->Time,iter->Q);
      }
    
    this->Initialized = 1;
    this->InitializeTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::InterpolateTransform(double t, 
                                                    vtkTransform *xform)
{
  if ( this->TransformList->empty() )
    {
    return;
    }

  // Make sure the xform and this class are initialized properly
  xform->Identity();
  this->InitializeInterpolation();
  
  // Evaluate the interpolators 
  if ( t < this->TransformList->front().Time )
    {
    t = this->TransformList->front().Time;
    }

  else if ( t > this->TransformList->back().Time )
    {
    t = this->TransformList->back().Time;
    }

  double P[3],S[3],Q[4];
  this->PositionInterpolator->InterpolateTuple(t,P);
  this->ScaleInterpolator->InterpolateTuple(t,S);
  this->RotationInterpolator->InterpolateQuaternion(t,Q);
  
  xform->Translate(P);
  xform->RotateWXYZ(Q[0],Q+1);
  xform->Scale(S);
}

//----------------------------------------------------------------------------
void vtkTransformInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are " << this->GetNumberOfTransforms()
     << " transforms to be interpolated\n";

  os << indent << "Interpolation Type: ";
  if ( this->InterpolationType == INTERPOLATION_TYPE_LINEAR )
    {
    os << "Linear\n";
    }
  else if ( this->InterpolationType == INTERPOLATION_TYPE_SPLINE )
    {
    os << "Spline\n";
    }
  else //if ( this->InterpolationType == INTERPOLATION_TYPE_MANUAL )
    {
    os << "Manual\n";
    }
  
  os << indent << "Position Interpolator: ";
  if ( this->PositionInterpolator )
    {
    os << this->PositionInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }

  os << indent << "Scale Interpolator: ";
  if ( this->ScaleInterpolator )
    {
    os << this->ScaleInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }

  os << indent << "Rotation Interpolator: ";
  if ( this->RotationInterpolator )
    {
    os << this->RotationInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }
}



