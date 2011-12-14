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
#include <vector>

#define VTKQUATERNIONINTERPOLATOR_TOLERNCE 1e-6

vtkStandardNewMacro(vtkQuaternionInterpolator);

//----------------------------------------------------------------------------
// PIMPL STL encapsulation for list of quaternions. The list is sorted on
// the spline paramter T (or Time) using a STL list.
// Here we define a quaternion class that includes extra information including
// a unit quaternion representation.
struct vtkQuaternion
{
  double Time;
  double Q[4];     //VTK's quaternion: unit rotation axis with angles in degrees
  double QUnit[4]; //Unit quaternion (i.e., normalized)

  vtkQuaternion()
    {
      this->Time = 0.0;
      this->Q[0] = this->Q[1] = this->Q[2] = this->Q[3] = 0.0;
      this->QUnit[0] = this->QUnit[1] = this->QUnit[2] = this->QUnit[3] = 0.0;
    }
  vtkQuaternion(double t, double q[4])
    {
      this->Time = t;
      this->Q[0] = this->QUnit[0] = q[0];
      this->Q[1] = this->QUnit[1] = q[1];
      this->Q[2] = this->QUnit[2] = q[2];
      this->Q[3] = this->QUnit[3] = q[3];

      // determine theta, sin(theta), cos(theta) for unit quaternion
      this->QUnit[0] = vtkMath::RadiansFromDegrees( this->QUnit[0] ); //convert to radians
      vtkQuaternion::Normalize( this->QUnit );
    }
  static void Add(double q0[4], double q1[4], double q[4])
    {
      q[0] = q0[0] + q1[0];
      q[1] = q0[1] + q1[1];
      q[2] = q0[2] + q1[2];
      q[3] = q0[3] + q1[3];
    }
  static void Product(double q0[4], double q1[4], double q[4])
    {
      q[0] = q0[0]*q1[0] - q0[1]*q1[1] - q0[2]*q1[2] - q0[3]*q1[3];
      q[1] = q0[0]*q1[1] + q0[1]*q1[0] + q0[2]*q1[3] - q0[3]*q1[2];
      q[2] = q0[0]*q1[2] - q0[1]*q1[3] + q0[2]*q1[0] + q0[3]*q1[1];
      q[3] = q0[0]*q1[3] + q0[1]*q1[2] - q0[2]*q1[1] + q0[3]*q1[0];
    }
  static void Conjugate(double q[4], double qConj[4])
    {
      qConj[0] =  q[0];
      qConj[1] = -q[1];
      qConj[2] = -q[2];
      qConj[3] = -q[3];
    }
  static void Inverse(double q[4], double qInv[4])
    {
      vtkQuaternion::Conjugate(q,qInv);
      double norm2 = vtkQuaternion::Norm2(q);
      if ( norm2 != 0.0 )
        {
        qInv[0] /= norm2;
        qInv[1] /= norm2;
        qInv[2] /= norm2;
        qInv[3] /= norm2;
        }
    }
  static double Norm2(double q[4])
    {
      return (q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    }
  static double Normalize(double q[4])
    {
      double norm = sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
      if ( norm != 0.0 )
        {
        q[0] /= norm;
        q[1] /= norm;
        q[2] /= norm;
        q[3] /= norm;
        }
      return norm;
    }
  // convert a unit quaternion to a VTK quaternion (angle in degrees;unit axis)
  static void UnitToVTK(double q[4])
    {
      double vNorm = sqrt(q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
      if ( vNorm != 0.0 )
        {
        q[0] /= vNorm;
        q[1] /= vNorm;
        q[2] /= vNorm;
        q[3] /= vNorm;
        }
      q[0] = vtkMath::DegreesFromRadians( q[0] );
    }
  // compute unit vector where q is a unit quaternion
  static void UnitVector(double q[4], double &theta, double &sinTheta,
                         double &cosTheta, double v[3])
    {
      double norm = sqrt(q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
      v[0] = q[1]/norm;
      v[1] = q[2]/norm;
      v[2] = q[3]/norm;
      int maxI = (q[1] > q[2] ? (q[1] > q[3] ? 1 : 3) : (q[2] > q[3] ? 2 : 3));
      if (q[maxI] != 0.0 )
        {
        sinTheta = q[maxI] / v[maxI-1];
        theta = asin(sinTheta);
        cosTheta = cos(theta);
        }
    }
  // log(q) where q is a unit (normalized) quaternion
  static void UnitLog(double q[4], double qLog[4])
    {
      double theta = 0.0;
      double sinTheta = 0.0;
      double cosTheta = 1.0;
      double v[3];
      v[0] = 0.0;
      v[1] = 0.0;
      v[2] = 0.0;

      vtkQuaternion::UnitVector(q,theta,sinTheta,cosTheta,v);
      qLog[0] = 0.0;
      qLog[1] = theta * v[0];
      qLog[2] = theta * v[1];
      qLog[3] = theta * v[2];
    }
  // exp(q) where q is a unit quaternion
  static void UnitExp(double q[4], double qExp[4])
    {
      double theta = 0.0;
      double sinTheta = 0.0;
      double cosTheta = 1.0;
      double v[3];
      v[0] = 0.0;
      v[1] = 0.0;
      v[2] = 0.0;

      vtkQuaternion::UnitVector(q,theta,sinTheta,cosTheta,v);
      qExp[0] = cosTheta;
      qExp[1] = sinTheta * v[0];
      qExp[2] = sinTheta * v[1];
      qExp[3] = sinTheta * v[2];
    }
};

// The list is arranged in increasing order in T
class vtkQuaternionList : public std::vector<vtkQuaternion> {};
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
  int size = static_cast<int>(this->QuaternionList->size());

  // Check special cases: t at beginning or end of list
  if ( size <= 0 || t < this->QuaternionList->front().Time )
    {
    this->QuaternionList->insert(this->QuaternionList->begin(),vtkQuaternion(t,q));
    return;
    }
  else if ( t > this->QuaternionList->back().Time )
    {
    this->QuaternionList->push_back(vtkQuaternion(t,q));
    return;
    }
  else if ( size == 1 && t == this->QuaternionList->front().Time )
    {
    this->QuaternionList->front() = vtkQuaternion(t,q);
    return;
    }

  // Okay, insert in sorted order
  QuaternionListIterator iter = this->QuaternionList->begin();
  QuaternionListIterator nextIter = iter + 1;
  for (int i=0; i < (size-1); i++, ++iter, ++nextIter)
    {
    if ( t == iter->Time )
      {
      (*iter) = vtkQuaternion(t,q); //overwrite
      break;
      }
    else if ( t > iter->Time && t < nextIter->Time )
      {
      this->QuaternionList->insert(nextIter, vtkQuaternion(t,q));
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
  const double dot = vtkMath::Dot(q0+1,q1+1);
  double t1, t2;

  // To avoid division by zero, perform a linear interpolation (LERP), if our
  // quarternions are nearly in the same direction, otherwise resort
  // to spherical linear interpolation. In the limiting case (for small
  // angles), SLERP is equivalent to LERP.

  if ((1.0 - fabs(dot)) < VTKQUATERNIONINTERPOLATOR_TOLERNCE)
    {
    t1 = 1.0-t;
    t2 = t;
    }
  else
    {
    const double theta = acos( dot );
    t1 = sin((1.0-t)*theta)/sin(theta);
    t2 = sin(t*theta)/sin(theta);
    }

  q[0] = q0[0]*t1 + q1[0]*t2;
  q[1] = q0[1]*t1 + q1[1]*t2;
  q[2] = q0[2]*t1 + q1[2]*t2;
  q[3] = q0[3]*t1 + q1[3]*t2;
}


//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::InnerPoint(double q0[4], double q1[4],
                                           double q2[4], double q[4])
{
   double qInv[4], qL[4], qR[4];
   vtkQuaternion::Inverse(q1,qInv);
   vtkQuaternion::Product(qInv,q2,qL);
   vtkQuaternion::Product(qInv,q0,qR);

   double qLLog[4], qRLog[4], qSum[4], qExp[4];
   vtkQuaternion::UnitLog(qL, qLLog);
   vtkQuaternion::UnitLog(qR, qRLog);
   vtkQuaternion::Add(qLLog,qRLog,qSum);
   qSum[1] /= -4.0;
   qSum[2] /= -4.0;
   qSum[3] /= -4.0;
   vtkQuaternion::UnitExp(qSum,qExp);
   vtkQuaternion::Product(q1,qExp,q);
}


//----------------------------------------------------------------------------
void vtkQuaternionInterpolator::InterpolateQuaternion(double t, double q[4])
{
  // The quaternion may be clamped if it is outside the range specified
  if ( t <= this->QuaternionList->front().Time )
    {
    vtkQuaternion &Q = this->QuaternionList->front();
    q[0] = Q.Q[0]; q[1] = Q.Q[1]; q[2] = Q.Q[2]; q[3] = Q.Q[3];
    return;
    }

  else if ( t >= this->QuaternionList->back().Time )
    {
    vtkQuaternion &Q = this->QuaternionList->back();
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

    double ai[4], bi[4], qc[4], qd[4];
    if ( i == 0 ) //initial interval
      {
      iter1 = iter;
      iter2 = nextIter;
      iter3 = nextIter + 1;

      ai[0] = iter1->QUnit[0]; //just duplicate first quaternion
      ai[1] = iter1->QUnit[1];
      ai[2] = iter1->QUnit[2];
      ai[3] = iter1->QUnit[3];

      this->InnerPoint(iter1->QUnit, iter2->QUnit, iter3->QUnit, bi);
      }
    else if ( i == (numQuats-2) ) //final interval
      {
      iter0 = iter - 1;
      iter1 = iter;
      iter2 = nextIter;

      this->InnerPoint(iter0->QUnit, iter1->QUnit, iter2->QUnit, ai);

      bi[0] = iter2->QUnit[0]; //just duplicate last quaternion
      bi[1] = iter2->QUnit[1];
      bi[2] = iter2->QUnit[2];
      bi[3] = iter2->QUnit[3];
      }
    else //in a middle interval somewhere
      {
      iter0 = iter - 1;
      iter1 = iter;
      iter2 = nextIter;
      iter3 = nextIter + 1;
      this->InnerPoint(iter0->QUnit, iter1->QUnit, iter2->QUnit, ai);
      this->InnerPoint(iter1->QUnit, iter2->QUnit, iter3->QUnit, bi);
      }

    // These three Slerp operations implement a Squad interpolation
    this->Slerp(T,iter1->QUnit,iter2->QUnit,qc);
    this->Slerp(T,ai,bi,qd);
    this->Slerp(2.0*T*(1.0-T),qc,qd,q);
    vtkQuaternion::UnitToVTK(q);
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



