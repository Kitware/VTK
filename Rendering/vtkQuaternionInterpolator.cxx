/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuaternionInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuaternionInterpolator.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkQuaternionInterpolator, "1.1");
vtkStandardNewMacro(vtkQuaternionInterpolator);

// PIMPL STL encapsulation for list of quaternions
struct vtkQ
{
  double Time;
  double Q[4];

  vtkQ()
    {
      this->Time = 0.0;
      this->Q[0] = this->Q[1] = this->Q[2] = this->Q[3] = 0.0;
    }
  vtkQ(double t, double q[4])
    {
      this->Time = t;
      this->Q[0] = q[0];
      this->Q[1] = q[1];
      this->Q[2] = q[2];
      this->Q[3] = q[3];
    }
};

// The list is arranged in increasing order in T
class vtkQuaternionList : public vtkstd::vector<vtkQ> {};
typedef vtkQuaternionList::iterator QuaternionListIterator;

//----------------------------------------------------------------------------
vtkQuaternionInterpolator::vtkQuaternionInterpolator()
{
  // Set up the interpolation
  this->QuaternionList = new vtkQuaternionList;
  this->InterpolationType = INTERPOLATION_TYPE_SPLINE;
}

//----------------------------------------------------------------------------
vtkQuaternionInterpolator::~vtkQuaternionInterpolator()
{
  this->Initialize();
  delete this->QuaternionList;
}

//----------------------------------------------------------------------------
int vtkQuaternionInterpolator::GetNumberOfQuaternions()
{
    return this->QuaternionList->size();
}


//----------------------------------------------------------------------------
double vtkQuaternionInterpolator::GetMinimumT()
{
  if (this->QuaternionList->size() > 0)
    {
    return this->QuaternionList->front().Time;
    }
  else
    {
    return 0.0;
    }
}


//----------------------------------------------------------------------------
double vtkQuaternionInterpolator::GetMaximumT()
{
  if (this->QuaternionList->size() > 0)
    {
    return this->QuaternionList->back().Time;
    }
  else
    {
    return 0.0;
    }
}


//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::Initialize()
{
  // Wipe out old data
  this->QuaternionList->clear();
}


//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::AddQuaternion(double t, double q[4])
{
  int size = this->QuaternionList->size();

  // Check special cases: t at beginning or end of list
  if ( size <= 0 || t < this->QuaternionList->front().Time )
    {
    this->QuaternionList->insert(this->QuaternionList->begin(),vtkQ(t,q));
    return;
    }
  else if ( t > this->QuaternionList->back().Time )
    {
    this->QuaternionList->push_back(vtkQ(t,q));
    return;
    }
  else if ( size == 1 && t == this->QuaternionList->front().Time )
    {
    this->QuaternionList->front() = vtkQ(t,q);
    return;
    }

  // Okay, insert in sorted order
  QuaternionListIterator iter = this->QuaternionList->begin();
  QuaternionListIterator nextIter = ++(this->QuaternionList->begin());
  for (int i=0; i < (size-1); i++, ++iter, ++nextIter)
    {
    if ( t == iter->Time )
      {
      (*iter) = vtkQ(t,q); //overwrite
      break;
      }
    else if ( t > iter->Time && t < nextIter->Time )
      {
      this->QuaternionList->insert(nextIter, vtkQ(t,q));
      break;
      }
    }//for not in the right spot
  
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::RemoveQuaternion(double t)
{
  if ( t < this->QuaternionList->front().Time ||
       t > this->QuaternionList->back().Time )
    {
    return;
    }

  QuaternionListIterator iter = this->QuaternionList->begin();
  for ( ; iter->Time != t && iter != this->QuaternionList->end(); ++iter )
    {
    }
  if ( iter != this->QuaternionList->end() )
    {
    this->QuaternionList->erase(iter);
    }
  
  this->Modified();
}

//----------------------------------------------------------------------------
//Interpolate using spherical linear interpolation between the quaternions q0
//and q1 to produce the output q. The parametric coordinate t is [0,1] and
//lies between (q0,q1).
void vtkQuaternionInterpolator::Slerp(double t, double q0[4], double q1[4], 
                                      double q[4])
{
  double theta = acos( vtkMath::Dot(q0+1,q1+1) );
  double t1 = sin((1.0-t)*theta)/sin(theta);
  double t2 = sin(t*theta)/sin(theta);
  q[0] = q0[0]*t1 + q1[0]*t2;
  q[1] = q0[1]*t1 + q1[1]*t2;
  q[2] = q0[2]*t1 + q1[2]*t2;
  q[3] = q0[3]*t1 + q1[3]*t2;
}


//----------------------------------------------------------------------------
//Interpolate using spherical cubic spline interpolation (using the method of
//quadrilaters) between the quaternions 
void vtkQuaternionInterpolator::Squad(double t, double qm[4], double q0[4], 
                                      double q1[4], double qp[4], double q[4])
{
}


//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::InterpolateQuaternion(double t, double q[4])
{
  // The quaternion may be clamped if it is outside the range specified
  if ( t <= this->QuaternionList->front().Time )
    {
    vtkQ &Q = this->QuaternionList->front();
    q[0] = Q.Q[0]; q[1] = Q.Q[1]; q[2] = Q.Q[2]; q[3] = Q.Q[3];
    return;
    }

  else if ( t >= this->QuaternionList->back().Time )
    {
    vtkQ &Q = this->QuaternionList->back();
    q[0] = Q.Q[0]; q[1] = Q.Q[1]; q[2] = Q.Q[2]; q[3] = Q.Q[3];
    return;
    }

  // Depending on the interpolation type we do the right thing.
  // The code above guarantees that there are at least two quaternions defined.
  if ( this->InterpolationType == INTERPOLATION_TYPE_LINEAR ||
       this->GetNumberOfQuaternions() < 4 )
    {
    QuaternionListIterator iter = this->QuaternionList->begin();
    QuaternionListIterator nextIter = ++(this->QuaternionList->begin());
    for ( ; nextIter != this->QuaternionList->end(); ++iter, ++nextIter)
      {
      if ( iter->Time <= t && t <= nextIter->Time )
        {
        double T = (t - iter->Time) / (nextIter->Time - iter->Time); 
        this->Slerp(T,iter->Q,nextIter->Q,q);
        break;
        }
      }
    }//if linear quaternion interpolation
  
  else // this->InterpolationType == INTERPOLATION_TYPE_SPLINE
    {
    }

  return;
}

//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are " << this->GetNumberOfQuaternions()
     << " quaternions to be interpolated\n";
  
  os << indent << "Interpolation Type: " 
     << (this->InterpolationType == INTERPOLATION_TYPE_LINEAR ?
         "Linear\n" : "Spline\n");
}



