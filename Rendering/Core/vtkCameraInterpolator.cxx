/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraInterpolator.h"
#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkTransform.h"
#include "vtkTupleInterpolator.h"
#include <list>

vtkStandardNewMacro(vtkCameraInterpolator);

// PIMPL STL encapsulation for list of cameras. This just keeps track of all
// the data the user specifies, which is later dumped into the interpolators.
struct vtkICamera
{
  double Time;  //Parameter t
  double P[3];  //Position
  double FP[3]; //Focal point
  double VUP[3];//ViewUp
  double CR[2]; //Clipping range
  double VA[1]; //View angle
  double PS[1]; //Parallel scale

  vtkICamera()
    {
      this->Time = 0.0;
      this->P[0] = this->P[1] = this->P[2] = 0.0;
      this->FP[0] = this->FP[1] = this->FP[2] = 0.0;
      this->VUP[0] = this->VUP[1] = this->VUP[2] = 0.0;
      this->CR[0] = 1.0; this->CR[0] = 1000.0;
      this->VA[0] = 30.0;
      this->PS[0] = 1.0;
    }
  vtkICamera(double t, vtkCamera *camera)
    {
      this->Time = t;
      if ( camera )
        {
        camera->GetPosition(this->P);
        camera->GetFocalPoint(this->FP);
        camera->GetViewUp(this->VUP);
        camera->GetClippingRange(this->CR);
        this->VA[0] = camera->GetViewAngle();
        this->PS[0] = camera->GetParallelScale();
        }
      else
        {
        this->P[0] = this->P[1] = this->P[2] = 0.0;
        this->FP[0] = this->FP[1] = this->FP[2] = 0.0;
        this->VUP[0] = this->VUP[1] = this->VUP[2] = 0.0;
        this->CR[0] = 1.0; this->CR[0] = 1000.0;
        this->VA[0] = 30.0;
        this->PS[0] = 1.0;
        }
    }
};

// The list is arranged in increasing order in T
class vtkCameraList : public std::list<vtkICamera> {};
typedef vtkCameraList::iterator CameraListIterator;


//----------------------------------------------------------------------------
vtkCameraInterpolator::vtkCameraInterpolator()
{
  // Set up the interpolation
  this->InterpolationType = INTERPOLATION_TYPE_SPLINE;

  // Spline interpolation
  this->PositionInterpolator = vtkTupleInterpolator::New();
  this->FocalPointInterpolator = vtkTupleInterpolator::New();
  this->ViewUpInterpolator = vtkTupleInterpolator::New();
  this->ViewAngleInterpolator = vtkTupleInterpolator::New();
  this->ParallelScaleInterpolator = vtkTupleInterpolator::New();
  this->ClippingRangeInterpolator = vtkTupleInterpolator::New();

  // Track the important camera parameters
  this->CameraList = new vtkCameraList;
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkCameraInterpolator::~vtkCameraInterpolator()
{
  delete this->CameraList;

  this->SetPositionInterpolator(NULL);
  this->SetFocalPointInterpolator(NULL);
  this->SetViewUpInterpolator(NULL);
  this->SetViewAngleInterpolator(NULL);
  this->SetParallelScaleInterpolator(NULL);
  this->SetClippingRangeInterpolator(NULL);
}

//----------------------------------------------------------------------------
unsigned long vtkCameraInterpolator::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long posMTime, fpMTime, vupMTime, vaMTime, psMTime, crMTime;

  if ( this->PositionInterpolator )
    {
    posMTime = this->PositionInterpolator->GetMTime();
    mTime = (posMTime > mTime ? posMTime : mTime);
    }
  if ( this->FocalPointInterpolator )
    {
    fpMTime = this->FocalPointInterpolator->GetMTime();
    mTime = (fpMTime > mTime ? fpMTime : mTime);
    }
  if ( this->ViewUpInterpolator )
    {
    vupMTime = this->ViewUpInterpolator->GetMTime();
    mTime = (vupMTime > mTime ? vupMTime : mTime);
    }
  if ( this->ViewAngleInterpolator )
    {
    vaMTime = this->ViewAngleInterpolator->GetMTime();
    mTime = (vaMTime > mTime ? vaMTime : mTime);
    }
  if ( this->ParallelScaleInterpolator )
    {
    psMTime = this->ParallelScaleInterpolator->GetMTime();
    mTime = (psMTime > mTime ? psMTime : mTime);
    }
  if ( this->ClippingRangeInterpolator )
    {
    crMTime = this->ClippingRangeInterpolator->GetMTime();
    mTime = (crMTime > mTime ? crMTime : mTime);
    }

  return mTime;
}


//----------------------------------------------------------------------------
int vtkCameraInterpolator::GetNumberOfCameras()
{
  return static_cast<int>(this->CameraList->size());
}


//----------------------------------------------------------------------------
double vtkCameraInterpolator::GetMinimumT()
{
  if ( this->CameraList->empty() )
    {
    return -VTK_LARGE_FLOAT;
    }
  else
    {
    return this->CameraList->front().Time;
    }
}


//----------------------------------------------------------------------------
double vtkCameraInterpolator::GetMaximumT()
{
  if ( this->CameraList->empty() )
    {
    return VTK_LARGE_FLOAT;
    }
  else
    {
    return this->CameraList->back().Time;
    }
}


//----------------------------------------------------------------------------
void vtkCameraInterpolator::Initialize()
{
  this->CameraList->clear();
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::AddCamera(double t, vtkCamera *camera)
{
  int size = static_cast<int>(this->CameraList->size());

  // Check special cases: t at beginning or end of list
  if ( size <= 0 || t < this->CameraList->front().Time )
    {
    this->CameraList->push_front(vtkICamera(t,camera));
    return;
    }
  else if ( t > this->CameraList->back().Time )
    {
    this->CameraList->push_back(vtkICamera(t,camera));
    return;
    }
  else if ( size == 1 && t == this->CameraList->back().Time )
    {
    this->CameraList->front() = vtkICamera(t,camera);
    return;
    }

  // Okay, insert in sorted order
  CameraListIterator iter = this->CameraList->begin();
  CameraListIterator nextIter = ++(this->CameraList->begin());
  for (int i=0; i < (size-1); i++, ++iter, ++nextIter)
    {
    if ( t == iter->Time )
      {
      (*iter) = vtkICamera(t,camera);
      }
    else if ( t > iter->Time && t < nextIter->Time )
      {
      this->CameraList->insert(nextIter, vtkICamera(t,camera));
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::RemoveCamera(double t)
{
  if ( t < this->CameraList->front().Time ||
       t > this->CameraList->back().Time )
    {
    return;
    }

  CameraListIterator iter = this->CameraList->begin();
  for ( ; iter->Time != t && iter != this->CameraList->end(); ++iter )
    {
    }
  if ( iter != this->CameraList->end() )
    {
    this->CameraList->erase(iter);
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::SetPositionInterpolator(vtkTupleInterpolator *pi)
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
void vtkCameraInterpolator::SetFocalPointInterpolator(vtkTupleInterpolator *fpi)
{
  if ( this->FocalPointInterpolator != fpi )
    {
    if ( this->FocalPointInterpolator != NULL )
      {
      this->FocalPointInterpolator->Delete();
      }
    this->FocalPointInterpolator = fpi;
    if ( this->FocalPointInterpolator != NULL )
      {
      this->FocalPointInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::SetViewUpInterpolator(vtkTupleInterpolator *vupi)
{
  if ( this->ViewUpInterpolator != vupi )
    {
    if ( this->ViewUpInterpolator != NULL )
      {
      this->ViewUpInterpolator->Delete();
      }
    this->ViewUpInterpolator = vupi;
    if ( this->ViewUpInterpolator != NULL )
      {
      this->ViewUpInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::SetClippingRangeInterpolator(vtkTupleInterpolator *cri)
{
  if ( this->ClippingRangeInterpolator != cri )
    {
    if ( this->ClippingRangeInterpolator != NULL )
      {
      this->ClippingRangeInterpolator->Delete();
      }
    this->ClippingRangeInterpolator = cri;
    if ( this->ClippingRangeInterpolator != NULL )
      {
      this->ClippingRangeInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::SetParallelScaleInterpolator(vtkTupleInterpolator *psi)
{
  if ( this->ParallelScaleInterpolator != psi )
    {
    if ( this->ParallelScaleInterpolator != NULL )
      {
      this->ParallelScaleInterpolator->Delete();
      }
    this->ParallelScaleInterpolator = psi;
    if ( this->ParallelScaleInterpolator != NULL )
      {
      this->ParallelScaleInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::SetViewAngleInterpolator(vtkTupleInterpolator *vai)
{
  if ( this->ViewAngleInterpolator != vai )
    {
    if ( this->ViewAngleInterpolator != NULL )
      {
      this->ViewAngleInterpolator->Delete();
      }
    this->ViewAngleInterpolator = vai;
    if ( this->ViewAngleInterpolator != NULL )
      {
      this->ViewAngleInterpolator->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::InitializeInterpolation()
{
  if ( this->CameraList->empty() )
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
    if ( !this->FocalPointInterpolator )
      {
      this->FocalPointInterpolator = vtkTupleInterpolator::New();
      }
    if ( !this->ViewUpInterpolator )
      {
      this->ViewUpInterpolator = vtkTupleInterpolator::New();
      }
    if ( !this->ClippingRangeInterpolator )
      {
      this->ClippingRangeInterpolator = vtkTupleInterpolator::New();
      }
    if ( !this->ParallelScaleInterpolator )
      {
      this->ParallelScaleInterpolator = vtkTupleInterpolator::New();
      }
    if ( !this->ViewAngleInterpolator )
      {
      this->ViewAngleInterpolator = vtkTupleInterpolator::New();
      }

    this->PositionInterpolator->Initialize();
    this->FocalPointInterpolator->Initialize();
    this->ViewUpInterpolator->Initialize();
    this->ClippingRangeInterpolator->Initialize();
    this->ParallelScaleInterpolator->Initialize();
    this->ViewAngleInterpolator->Initialize();

    this->PositionInterpolator->SetNumberOfComponents(3);
    this->FocalPointInterpolator->SetNumberOfComponents(3);
    this->ViewUpInterpolator->SetNumberOfComponents(3);
    this->ClippingRangeInterpolator->SetNumberOfComponents(2);
    this->ParallelScaleInterpolator->SetNumberOfComponents(1);
    this->ViewAngleInterpolator->SetNumberOfComponents(1);

    if ( this->InterpolationType == INTERPOLATION_TYPE_LINEAR )
      {
      this->PositionInterpolator->SetInterpolationTypeToLinear();
      this->FocalPointInterpolator->SetInterpolationTypeToLinear();
      this->ViewUpInterpolator->SetInterpolationTypeToLinear();
      this->ClippingRangeInterpolator->SetInterpolationTypeToLinear();
      this->ParallelScaleInterpolator->SetInterpolationTypeToLinear();
      this->ViewAngleInterpolator->SetInterpolationTypeToLinear();
      }
    else if ( this->InterpolationType == INTERPOLATION_TYPE_SPLINE )
      {
      this->PositionInterpolator->SetInterpolationTypeToSpline();
      this->FocalPointInterpolator->SetInterpolationTypeToSpline();
      this->ViewUpInterpolator->SetInterpolationTypeToSpline();
      this->ClippingRangeInterpolator->SetInterpolationTypeToSpline();
      this->ParallelScaleInterpolator->SetInterpolationTypeToSpline();
      this->ViewAngleInterpolator->SetInterpolationTypeToSpline();
      }
    else
      {
      ; //manual override, user manipulates interpolators directly
      }

    // Okay, now we can load the interpolators with data
    CameraListIterator iter = this->CameraList->begin();
    for ( ; iter != this->CameraList->end(); ++iter)
      {
      this->PositionInterpolator->AddTuple(iter->Time,iter->P);
      this->FocalPointInterpolator->AddTuple(iter->Time,iter->FP);
      this->ViewUpInterpolator->AddTuple(iter->Time,iter->VUP);
      this->ClippingRangeInterpolator->AddTuple(iter->Time,iter->CR);
      this->ViewAngleInterpolator->AddTuple(iter->Time,iter->VA);
      this->ParallelScaleInterpolator->AddTuple(iter->Time,iter->PS);
      }

    this->Initialized = 1;
    this->InitializeTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::InterpolateCamera(double t, vtkCamera *camera)
{
  if ( this->CameraList->empty() )
    {
    return;
    }

  // Make sure the camera and this class are initialized properly
  this->InitializeInterpolation();

  // Evaluate the interpolators
  if ( t < this->CameraList->front().Time )
    {
    t = this->CameraList->front().Time;
    }

  else if ( t > this->CameraList->back().Time )
    {
    t = this->CameraList->back().Time;
    }

  double P[3],FP[3],VUP[3],CR[2],VA[1],PS[1];
  this->PositionInterpolator->InterpolateTuple(t,P);
  this->FocalPointInterpolator->InterpolateTuple(t,FP);
  this->ViewUpInterpolator->InterpolateTuple(t,VUP);
  this->ClippingRangeInterpolator->InterpolateTuple(t,CR);
  this->ViewAngleInterpolator->InterpolateTuple(t,VA);
  this->ParallelScaleInterpolator->InterpolateTuple(t,PS);

  camera->SetPosition(P);
  camera->SetFocalPoint(FP);
  camera->SetViewUp(VUP);
  camera->SetClippingRange(CR);
  camera->SetViewAngle(VA[0]);
  camera->SetParallelScale(PS[0]);
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are " << this->GetNumberOfCameras()
     << " cameras to be interpolated\n";

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

  os << indent << "Focal Point Interpolator: ";
  if ( this->FocalPointInterpolator )
    {
    os << this->FocalPointInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }

  os << indent << "View Up Interpolator: ";
  if ( this->ViewUpInterpolator )
    {
    os << this->ViewUpInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }

  os << indent << "Clipping Range Interpolator: ";
  if ( this->ClippingRangeInterpolator )
    {
    os << this->ClippingRangeInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }

  os << indent << "View Angle Interpolator: ";
  if ( this->ViewAngleInterpolator )
    {
    os << this->ViewAngleInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }

  os << indent << "Parallel Scale Interpolator: ";
  if ( this->ParallelScaleInterpolator )
    {
    os << this->ParallelScaleInterpolator << "\n";
    }
  else
    {
    os << "(null)\n";
    }
}



