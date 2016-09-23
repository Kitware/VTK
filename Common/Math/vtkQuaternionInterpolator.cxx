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
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkQuaternion.h"
#include "vtkQuaternionInterpolator.h"
#include <vector>

vtkStandardNewMacro(vtkQuaternionInterpolator);

//----------------------------------------------------------------------------
// PIMPL STL encapsulation for list of quaternions. The list is sorted on
// the spline parameter T (or Time) using a STL list.
// Here we define a quaternion class that includes extra information including
// a unit quaternion representation.
struct TimedQuaternion
{
  double Time;
  vtkQuaterniond Q;     //VTK's quaternion: unit rotation axis with angles in degrees

  TimedQuaternion()
    : Q(0.0)
  {
    this->Time = 0.0;
  }
  TimedQuaternion(double t, vtkQuaterniond q)
  {
    this->Time = t;
    this->Q = q;
  }
};

// The list is arranged in increasing order in T
class vtkQuaternionList : public std::vector<TimedQuaternion> {};
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
  return static_cast<int>(this->QuaternionList->size());
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
  vtkQuaterniond quat(q);
  this->AddQuaternion(t, quat);
}

//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::AddQuaternion(double t,
                                              const vtkQuaterniond& q)
{
  int size = static_cast<int>(this->QuaternionList->size());

  // Check special cases: t at beginning or end of list
  if ( size <= 0 || t < this->QuaternionList->front().Time )
  {
    this->QuaternionList->insert(this->QuaternionList->begin(),TimedQuaternion(t,q));
    return;
  }
  else if ( t > this->QuaternionList->back().Time )
  {
    this->QuaternionList->push_back(TimedQuaternion(t,q));
    return;
  }
  else if ( size == 1 && t == this->QuaternionList->front().Time )
  {
    this->QuaternionList->front() = TimedQuaternion(t,q);
    return;
  }

  // Okay, insert in sorted order
  QuaternionListIterator iter = this->QuaternionList->begin();
  QuaternionListIterator nextIter = iter + 1;
  for (int i=0; i < (size-1); i++, ++iter, ++nextIter)
  {
    if ( t == iter->Time )
    {
      (*iter) = TimedQuaternion(t,q); //overwrite
      break;
    }
    else if ( t > iter->Time && t < nextIter->Time )
    {
      this->QuaternionList->insert(nextIter, TimedQuaternion(t,q));
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
void vtkQuaternionInterpolator::InterpolateQuaternion(double t, double q[4])
{
  vtkQuaterniond quat(q);
  this->InterpolateQuaternion(t, quat);
  for (int i = 0; i < 4; ++i)
  {
    q[i] = quat[i];
  }
}

//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::InterpolateQuaternion(double t,
                                                      vtkQuaterniond& q)
{
  // The quaternion may be clamped if it is outside the range specified
  if ( t <= this->QuaternionList->front().Time )
  {
    TimedQuaternion &Q = this->QuaternionList->front();
    q = Q.Q;
    return;
  }

  else if ( t >= this->QuaternionList->back().Time )
  {
    TimedQuaternion &Q = this->QuaternionList->front();
    q = Q.Q;
    return;
  }

  // Depending on the interpolation type we do the right thing.
  // The code above guarantees that there are at least two quaternions defined.
  int numQuats = this->GetNumberOfQuaternions();
  if ( this->InterpolationType == INTERPOLATION_TYPE_LINEAR || numQuats < 3 )
  {
    QuaternionListIterator iter = this->QuaternionList->begin();
    QuaternionListIterator nextIter = iter + 1;
    for ( ; nextIter != this->QuaternionList->end(); ++iter, ++nextIter)
    {
      if ( iter->Time <= t && t <= nextIter->Time )
      {
        double T = (t - iter->Time) / (nextIter->Time - iter->Time);
        q = iter->Q.Slerp(T,nextIter->Q);
        break;
      }
    }
  }//if linear quaternion interpolation

  else // this->InterpolationType == INTERPOLATION_TYPE_SPLINE
  {
    QuaternionListIterator iter = this->QuaternionList->begin();
    QuaternionListIterator nextIter = iter + 1;
    QuaternionListIterator iter0, iter1, iter2, iter3;

    //find the interval
    double T=0.0;
    int i;
    for (i=0; nextIter != this->QuaternionList->end(); ++iter, ++nextIter, ++i)
    {
      if ( iter->Time <= t && t <= nextIter->Time )
      {
        T = (t - iter->Time) / (nextIter->Time - iter->Time);
        break;
      }
    }

    vtkQuaterniond ai, bi, qc, qd;
    if ( i == 0 ) //initial interval
    {
      iter1 = iter;
      iter2 = nextIter;
      iter3 = nextIter + 1;

      ai = iter1->Q.Normalized(); //just duplicate first quaternion
      vtkQuaterniond q1 = iter1->Q.Normalized();
      bi = q1.InnerPoint(iter2->Q.Normalized(), iter3->Q.Normalized());
    }
    else if ( i == (numQuats-2) ) //final interval
    {
      iter0 = iter - 1;
      iter1 = iter;
      iter2 = nextIter;

      vtkQuaterniond q0 = iter0->Q.Normalized();
      ai = q0.InnerPoint(iter1->Q.Normalized(), iter2->Q.Normalized());

      bi = iter2->Q.Normalized(); //just duplicate last quaternion
    }
    else //in a middle interval somewhere
    {
      iter0 = iter - 1;
      iter1 = iter;
      iter2 = nextIter;
      iter3 = nextIter + 1;

      vtkQuaterniond q0 = iter0->Q.Normalized();
      ai = q0.InnerPoint(iter1->Q.Normalized(), iter2->Q.Normalized());

      vtkQuaterniond q1 = iter1->Q.Normalized();
      bi = q1.InnerPoint(iter2->Q.Normalized(), iter3->Q.Normalized());
    }

    // These three Slerp operations implement a Squad interpolation
    vtkQuaterniond q1 = iter1->Q.Normalized();
    qc = q1.Slerp(T,iter2->Q.Normalized());
    qd = ai.Slerp(T,bi);
    q = qc.Slerp(2.0*T*(1.0-T),qd);
    q.NormalizeWithAngleInDegrees();
  }

  return;
}

//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "QuaternionList: " << this->QuaternionList->size()
     << " quaternions to interpolate\n";

  os << indent << "InterpolationType: "
     << (this->InterpolationType == INTERPOLATION_TYPE_LINEAR ?
         "Linear\n" : "Spline\n");
}



