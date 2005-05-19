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

vtkCxxRevisionMacro(vtkQuaternionInterpolator, "1.3");
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
inline void vtkQuaternionInterpolator::Add(double q0[4], double q1[4], double q[4])
{
  q[0] = q0[0] + q1[0];
  q[1] = q0[1] + q1[1];
  q[2] = q0[2] + q1[2];
  q[3] = q0[3] + q1[3];
}

//----------------------------------------------------------------------------
inline void vtkQuaternionInterpolator::Product(double q0[4], double q1[4],
                                               double q[4])
{
  q[0] = q0[0]*q1[0] - q0[1]*q1[1] - q0[2]*q1[2] - q0[3]*q1[3];
  q[1] = q0[0]*q1[1] + q0[1]*q1[0] + q0[2]*q1[3] - q0[3]*q1[2];
  q[2] = q0[0]*q1[2] - q0[1]*q1[3] + q0[2]*q1[0] + q0[3]*q1[1];
  q[3] = q0[0]*q1[3] + q0[1]*q1[2] - q0[2]*q1[1] + q0[3]*q1[0];
}

//----------------------------------------------------------------------------
inline void vtkQuaternionInterpolator::Conjugate(double q[4], double qConj[4])
{
  qConj[0] =  q[0];
  qConj[1] = -q[1];
  qConj[2] = -q[2];
  qConj[3] = -q[3];
}

//----------------------------------------------------------------------------
inline double vtkQuaternionInterpolator::Norm(double q[4])
{
  return (q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
}

//----------------------------------------------------------------------------
inline void vtkQuaternionInterpolator::Inverse(double q[4], double qInv[4])
{
  vtkQuaternionInterpolator::Conjugate(q,qInv);
  double norm = vtkQuaternionInterpolator::Norm(q);
  qInv[0] /= norm;
  qInv[1] /= norm;
  qInv[2] /= norm;
  qInv[3] /= norm;
}


//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::Log(double q[4], double qLog[4])
{
  qLog[0] = qLog[1] = qLog[2] = qLog[3] = 0.0;
  if ( q[1] == 0.0 && q[2] == 0.0 && q[3] == 0.0 ) //real valued
    {
    if (q[0] > 0)
      {
      qLog[0] = log(q[0]);
      }
    else if (q[0] < 0)
      {
      qLog[0] = log(-q[0]);
      qLog[1] = 1.0; //arbitrary
      }
    else
      {
      ; //leave NULL
      }
    }

  else //has complex part
    {
    double l = sqrt(q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    double r = sqrt(l*l + q[0]*q[0]);
    double theta = atan2(l, q[0]);
    double t = theta / l;
    qLog[0] = log(r);
    qLog[1] = t * q[1];
    qLog[2] = t * q[2];
    qLog[3] = t * q[3];
    }
}

//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::Exp(double q[4], double qExp[4])
{
  qExp[0] = qExp[1] = qExp[2] = qExp[3] = 0.0;
  if ( q[1] == 0.0 && q[2] == 0.0 && q[3] == 0.0 ) //real valued
    {
    qExp[0] = exp(q[0]);
    }
  else
    {
    double l = sqrt(q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    double s = sin(l);
    double c = cos(l);
    double e = exp(q[0]);
    double t = e * s / l;
    qExp[0] = e * c;
    qExp[1] = t * q[1];
    qExp[2] = t * q[2];
    qExp[3] = t * q[3];
    }
}

//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::InnerPoint( double q0[4], double q1[4], 
                                            double q2[4], double q[4] )
{
  double qInv[4], qL[4], qR[4];
  vtkQuaternionInterpolator::Inverse(q1,qInv);
  vtkQuaternionInterpolator::Product(qInv,q2,qL);
  vtkQuaternionInterpolator::Product(qInv,q0,qR);
  
  q[0] = q1[0] * exp(-(log(qL[0]) + log(qR[0])) / 4.0);
  q[1] = q1[1] * exp(-(log(qL[1]) + log(qR[1])) / 4.0);
  q[2] = q1[2] * exp(-(log(qL[2]) + log(qR[2])) / 4.0);
  q[3] = q1[3] * exp(-(log(qL[3]) + log(qR[3])) / 4.0);
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
        this->Slerp(T,iter->Q,nextIter->Q,q);
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
    
    if ( i == 0 ) //initial interval
      {
      iter0 = iter;
      iter1 = iter;
      iter2 = nextIter;
      iter3 = nextIter + 1;
      }
    else if ( i == (numQuats-2) ) //final interval
      {
      iter0 = iter - 1;
      iter1 = iter;
      iter2 = nextIter;
      iter3 = nextIter;
      }
    else //in a middle interval somewhere
      {
      iter0 = iter - 1;
      iter1 = iter;
      iter2 = nextIter;
      iter3 = nextIter + 1;
      }
    
    double ai[4], bi[4], qc[4], qd[4];
    this->InnerPoint(iter0->Q,iter1->Q,iter2->Q,ai);
    this->InnerPoint(iter1->Q,iter2->Q,iter3->Q,bi);

    // These three Slerp operations implement a Squad interpolation
    this->Slerp(T,iter1->Q,iter2->Q,qc);
    this->Slerp(T,ai,bi,qd);
    this->Slerp(2.0*T*(1.0-T),qc,qd,q);
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



