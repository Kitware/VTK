/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerspectiveTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPerspectiveTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <stdlib.h>

vtkStandardNewMacro(vtkPerspectiveTransform);

//----------------------------------------------------------------------------
vtkPerspectiveTransform::vtkPerspectiveTransform()
{
  this->Input = NULL;

  // most of the functionality is provided by the concatenation
  this->Concatenation = vtkTransformConcatenation::New();

  // the stack will be allocated the first time Push is called
  this->Stack = NULL;
}

//----------------------------------------------------------------------------
vtkPerspectiveTransform::~vtkPerspectiveTransform()
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
void vtkPerspectiveTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Update();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "InverseFlag: " << this->GetInverseFlag() << "\n";
  os << indent << "NumberOfConcatenatedTransforms: " <<
    this->GetNumberOfConcatenatedTransforms() << "\n";
  if (this->GetNumberOfConcatenatedTransforms() != 0)
    {
    int n = this->GetNumberOfConcatenatedTransforms();
    for (int i = 0; i < n; i++)
      {
      vtkHomogeneousTransform *t = this->GetConcatenatedTransform(i);
      os << indent << "    " << i << ": " << t->GetClassName() << " at " <<
         t << "\n";
      }
    }
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::Concatenate(vtkHomogeneousTransform *transform)
{
  if (transform->CircuitCheck(this))
    {
    vtkErrorMacro("Concatenate: this would create a circular reference.");
    return; 
    }
  this->Concatenation->Concatenate(transform); 
  this->Modified(); 
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::SetInput(vtkHomogeneousTransform *input)
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
int vtkPerspectiveTransform::CircuitCheck(vtkAbstractTransform *transform)
{
  if (this->vtkHomogeneousTransform::CircuitCheck(transform) ||
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
vtkAbstractTransform *vtkPerspectiveTransform::MakeTransform()
{
  return vtkPerspectiveTransform::New();
}

//----------------------------------------------------------------------------
unsigned long vtkPerspectiveTransform::GetMTime()
{
  unsigned long mtime = this->vtkHomogeneousTransform::GetMTime();
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

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::InternalDeepCopy(vtkAbstractTransform *gtrans)
{
  vtkPerspectiveTransform *transform =
    static_cast<vtkPerspectiveTransform *>(gtrans);

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

  // defer to superclass
  this->vtkHomogeneousTransform::InternalDeepCopy(transform);
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::InternalUpdate()
{
  // copy matrix from input
  if (this->Input)
    {
    this->Matrix->DeepCopy(this->Input->GetMatrix());
    // if inverse flag is set, invert the matrix
    if (this->Concatenation->GetInverseFlag())
      {
      this->Matrix->Invert();
      }
    }
  else
  // no input, start with identity
    {
    this->Matrix->Identity();
    }

  int i;
  int nTransforms = this->Concatenation->GetNumberOfTransforms();
  int nPreTransforms = this->Concatenation->GetNumberOfPreTransforms();

  // concatenate PreTransforms 
  for (i = nPreTransforms-1; i >= 0; i--)
    {
    vtkHomogeneousTransform *transform = 
      static_cast<vtkHomogeneousTransform *>(this->Concatenation->GetTransform(i));
    vtkMatrix4x4::Multiply4x4(this->Matrix,transform->GetMatrix(),
                              this->Matrix);
    }

  // concatenate PostTransforms
  for (i = nPreTransforms; i < nTransforms; i++)
    {
    vtkHomogeneousTransform *transform = 
      static_cast<vtkHomogeneousTransform *>(this->Concatenation->GetTransform(i));
    vtkMatrix4x4::Multiply4x4(transform->GetMatrix(),this->Matrix,
                              this->Matrix);
    }
}  

//----------------------------------------------------------------------------
// Utility for adjusting the window range to a new one.  Usually the
// previous range was ([-1,+1],[-1,+1]) as per Ortho and Frustum, and you
// are mapping to the display coordinate range ([0,width-1],[0,height-1]).
void vtkPerspectiveTransform::AdjustViewport(double oldXMin, double oldXMax, 
                                             double oldYMin, double oldYMax,
                                             double newXMin, double newXMax, 
                                             double newYMin, double newYMax)
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = (newXMax - newXMin)/(oldXMax - oldXMin);
  matrix[1][1] = (newYMax - newYMin)/(oldYMax - oldYMin);

  matrix[0][3] = (newXMin*oldXMax - newXMax*oldXMin)/(oldXMax - oldXMin);
  matrix[1][3] = (newYMin*oldYMax - newYMax*oldYMin)/(oldYMax - oldYMin);

  this->Concatenate(*matrix);
}  

//----------------------------------------------------------------------------
// Utility for adjusting the min/max range of the Z buffer.  Usually
// the oldZMin, oldZMax are [-1,+1] as per Ortho and Frustum, and
// you are mapping the Z buffer to a new range.
void vtkPerspectiveTransform::AdjustZBuffer(double oldZMin, double oldZMax,
                                            double newZMin, double newZMax)
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[2][2] = (newZMax - newZMin)/(oldZMax - oldZMin);
  matrix[2][3] = (newZMin*oldZMax - newZMax*oldZMin)/(oldZMax - oldZMin);

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
// The orthographic perspective maps [xmin,xmax], [ymin,ymax], [-znear,-zfar]
// to [-1,+1], [-1,+1], [-1,+1].
// From the OpenGL Programmer's guide, 2nd Ed.
void vtkPerspectiveTransform::Ortho(double xmin, double xmax,
                                    double ymin, double ymax,
                                    double znear, double zfar)
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = 2/(xmax - xmin);
  matrix[1][1] = 2/(ymax - ymin);
  matrix[2][2] = -2/(zfar - znear);
  
  matrix[0][3] = -(xmin + xmax)/(xmax - xmin);
  matrix[1][3] = -(ymin + ymax)/(ymax - ymin);
  matrix[2][3] = -(znear + zfar)/(zfar - znear);

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
// The frustrum perspective maps a frustum with the front plane at -znear
// which has extent [xmin,xmax],[ymin,ymax] and a back plane at -zfar
// to [-1,+1], [-1,+1], [-1,+1].
// From the OpenGL Programmer's guide, 2nd Ed.
void vtkPerspectiveTransform::Frustum(double xmin, double xmax,
                                      double ymin, double ymax,
                                      double znear, double zfar)
{
  double matrix[4][4];

  matrix[0][0] =  2*znear/(xmax - xmin);
  matrix[1][0] =  0;
  matrix[2][0] =  0;
  matrix[3][0] =  0;

  matrix[0][1] =  0;
  matrix[1][1] =  2*znear/(ymax - ymin);
  matrix[2][1] =  0;
  matrix[3][1] =  0;

  matrix[0][2] =  (xmin + xmax)/(xmax - xmin);
  matrix[1][2] =  (ymin + ymax)/(ymax - ymin);
  matrix[2][2] = -(znear + zfar)/(zfar - znear);
  matrix[3][2] = -1;

  matrix[0][3] =  0;
  matrix[1][3] =  0;
  matrix[2][3] = -2*znear*zfar/(zfar - znear);
  matrix[3][3] =  0;

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
// For convenience, an easy way to set up a symmetrical frustum.
void vtkPerspectiveTransform::Perspective(double angle, double aspect,
                                          double znear, double zfar)
{
  double ymax =  tan( vtkMath::RadiansFromDegrees( angle ) / 2 ) * znear;
  double ymin = -ymax; 

  double xmax =  ymax*aspect;
  double xmin = -xmax;

  this->Frustum(xmin, xmax, ymin, ymax, znear, zfar);
}

//----------------------------------------------------------------------------
// The Shear method can be used after Perspective to create correct
// perspective views for use with head-tracked stereo on a flat, fixed
// (i.e. not head-mounted) viewing screen.
//
// You must measure the eye position relative to the center of the
// RenderWindow (or the center of the screen, if the window is
// full-screen).  The following applies:  +x is 'right', +y is 'up',
// and zplane is the distance from screen to the eye.
//
// Here is some info on how to set this up properly:
//
//  - Decide on a real-world-coords to virtual-world-coords conversion
//    factor that is appropriate for the scene you are viewing.
//  - The screen is the focal plane, the near clipping plane lies in 
//    front of the screen and far clipping plane lies behind.  
//    Measure the (x,y,z) displacent from the center of the screen to 
//    your eye.  Scale these by the factor you chose.
//  - After you have scaled x, y, and z call Shear(x/z,y/z,z).
//
// We're not done yet!
//
//  - When you set up the view using SetupCamera(), the camera should
//    be placed the same distance from the screen as your eye, but so
//    that it looks at the screen straight-on.  I.e. it must lie along
//    the ray perpendicular to the screen which passes through the center
//    of screen (i.e. the center of the screen, in world coords, corresponds
//    to the focal point).  Whatever 'z' you used in Shear(), the 
//    camera->focalpoint distance should be the same.   If you are 
//    wondering why you don't set the camera position to be the eye
//    position, don't worry -- the combination of SetupCamera() and
//    an Oblique() shear about the focal plane does precisely that.
//  
//  - When you set up the view frustum using Perspective(),
//    set the angle to  2*atan(0.5*height/z)  where 'height' is
//    the height of your screen multiplied by the real-to-virtual
//    scale factor.  Don't forget to convert the angle to degrees.
//  - Though it is not absolutely necessary, you might want to 
//    keep your near and far clipping planes at constant distances
//    from the focal point.  By default, they are set up relative
//    to the camera position.
//
//  The order in which you apply the transformations, in 
//  PreMultiply mode, is:
//    1) Perspective(),  2) Shear(),  3) SetupCamera()
//
// Take the above advice with a grain of salt... I've never actually
// tried any of this except for with pencil & paper.  Looks good on
// paper, though!
void vtkPerspectiveTransform::Shear(double dxdz, double dydz, double zplane)
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  // everything is negative because the position->focalpoint vector
  // is in the -z direction, hence z distances along that vector
  // are negative.

  // shear according to the eye position relative to the screen
  matrix[0][2] = -dxdz;
  matrix[1][2] = -dydz;

  // shift so that view rays converge in the focal plane
  matrix[0][3] = -zplane*dxdz;
  matrix[1][3] = -zplane*dydz;

  // concatenate with the current matrix
  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
// For convenience -- this is sufficient for most people's stereo needs.
// Set the angle to negative for left eye, positive for right eye.
void vtkPerspectiveTransform::Stereo(double angle, double focaldistance)
{
  double dxdz = tan( vtkMath::RadiansFromDegrees( angle ) );

  this->Shear(dxdz, 0.0, focaldistance);
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::SetupCamera(const double position[3],
                                          const double focalPoint[3],
                                          const double viewUp[3])
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  // the view directions correspond to the rows of the rotation matrix,
  // so we'll make the connection explicit
  double *viewSideways =    matrix[0];
  double *orthoViewUp =     matrix[1];
  double *viewPlaneNormal = matrix[2]; 

  // set the view plane normal from the view vector
  viewPlaneNormal[0] = position[0] - focalPoint[0];
  viewPlaneNormal[1] = position[1] - focalPoint[1];
  viewPlaneNormal[2] = position[2] - focalPoint[2];
  vtkMath::Normalize(viewPlaneNormal);

  // orthogonalize viewUp and compute viewSideways
  vtkMath::Cross(viewUp,viewPlaneNormal,viewSideways);
  vtkMath::Normalize(viewSideways);
  vtkMath::Cross(viewPlaneNormal,viewSideways,orthoViewUp);

  // translate by the vector from the position to the origin
  double delta[4];
  delta[0] = -position[0];
  delta[1] = -position[1];
  delta[2] = -position[2];
  delta[3] = 0.0; // yes, this should be zero, not one

  vtkMatrix4x4::MultiplyPoint(*matrix,delta,delta);

  matrix[0][3] = delta[0]; 
  matrix[1][3] = delta[1]; 
  matrix[2][3] = delta[2]; 

  // apply the transformation
  this->Concatenate(*matrix);
}

void vtkPerspectiveTransform::SetupCamera(double p0, double p1, double p2,
                                          double fp0, double fp1, double fp2,
                                          double vup0, double vup1, double vup2)
{
  double p[3], fp[3], vup[3];
  p[0] = p0; 
  p[1] = p1; 
  p[2] = p2;
  fp[0] = fp0; 
  fp[1] = fp1; 
  fp[2] = fp2;
  vup[0] = vup0; 
  vup[1] = vup1; 
  vup[2] = vup2;

  this->SetupCamera(p, fp, vup);
}
  
