/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkWarpTransform - superclass for nonlinear geometric transformations
// .SECTION Description
// vtkWarpTransform provides a generic interface for nonlinear 
// warp transformations.
// .SECTION see also
// vtkThinPlateSplineTransform vtkGridTransform vtkGeneralTransform


#ifndef __vtkWarpTransform_h
#define __vtkWarpTransform_h

#include "vtkAbstractTransform.h"

class VTK_EXPORT vtkWarpTransform : public vtkAbstractTransform
{
public:

  vtkTypeMacro(vtkWarpTransform,vtkAbstractTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Invert the transformation.  Warp transformations are usually
  // inverted using an iterative technique such as Newton's method.
  // The inverse transform is usually around five or six times as
  // computationally expensive as the forward transform.
  void Inverse();

  // Description:
  // Get the inverse flag of the transformation.  This flag is
  // set to zero when the transformation is first created, and
  // is flipped each time Inverse() is called.
  vtkGetMacro(InverseFlag,int);

  // Description:
  // Set the tolerance for inverse transformation.
  // The default is 0.001.
  vtkSetMacro(InverseTolerance,double);
  vtkGetMacro(InverseTolerance,double);

  // Description:
  // Set the maximum number of iterations for the inverse
  // transformation.  The default is 500, but usually only 
  // 2 to 5 iterations are used.  The inversion method
  // is fairly robust, and it should converge for nearly all smooth
  // transformations that do not fold back on themselves.
  vtkSetMacro(InverseIterations,int);
  vtkGetMacro(InverseIterations,int);

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);
  void InternalTransformPoint(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation, as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
				   float derivative[3][3]);
  void InternalTransformDerivative(const double in[3], double out[3],
				   double derivative[3][3]);

  //BTX
  // Description:
  // Do not use these methods.  They exists only as a work-around for
  // internal templated functions (I really didn't want to make the
  // Forward/Inverse methods public, is there a decent work around
  // for this sort of thing?)
  void TemplateTransformPoint(const float in[3], float out[3]) {
    this->ForwardTransformPoint(in,out); }; 
  void TemplateTransformPoint(const double in[3], double out[3]) {
    this->ForwardTransformPoint(in,out); }; 
  void TemplateTransformPoint(const float in[3], float out[3],
			      float derivative[3][3]) {
    this->ForwardTransformDerivative(in,out,derivative); }; 
  void TemplateTransformPoint(const double in[3], double out[3],
			      double derivative[3][3]) {
    this->ForwardTransformDerivative(in,out,derivative); }; 
  void TemplateTransformInverse(const float in[3], float out[3]) {
    this->InverseTransformPoint(in,out); }; 
  void TemplateTransformInverse(const double in[3], double out[3]) {
    this->InverseTransformPoint(in,out); }; 
  void TemplateTransformInverse(const float in[3], float out[3],
				float derivative[3][3]) {
    this->InverseTransformDerivative(in,out,derivative); }; 
  void TemplateTransformInverse(const double in[3], double out[3],
				double derivative[3][3]) {
    this->InverseTransformDerivative(in,out,derivative); }; 
  //ETX

protected:
  vtkWarpTransform();
  ~vtkWarpTransform();
  vtkWarpTransform(const vtkWarpTransform&);
  void operator=(const vtkWarpTransform&);

  // Description:
  // If the InverseFlag is set to 0, then a call to InternalTransformPoint
  // results in a call to ForwardTransformPoint. 
  virtual void ForwardTransformPoint(const float in[3], float out[3]) = 0;
  virtual void ForwardTransformPoint(const double in[3], double out[3]) = 0;

  // Description:
  // Calculate the forward transform as well as the derivative. 
  virtual void ForwardTransformDerivative(const float in[3], float out[3],
					  float derivative[3][3]) = 0;
  virtual void ForwardTransformDerivative(const double in[3], double out[3],
					  double derivative[3][3]) = 0;

  // Description:
  // If the InverseFlag is set to 1, then a call to InternalTransformPoint
  // results in a call to InverseTransformPoint.  The inverse transformation
  // is calculated from using Newton's method.
  virtual void InverseTransformPoint(const float in[3], float out[3]);
  virtual void InverseTransformPoint(const double in[3], double out[3]);

  // Description:
  // Calculate the inverse transform as well as the derivative of the
  // forward transform (that's correct: the derivative of the
  // forward transform, not of the inverse transform)
  virtual void InverseTransformDerivative(const float in[3], float out[3],
					  float derivative[3][3]);
  virtual void InverseTransformDerivative(const double in[3], double out[3],
					  double derivative[3][3]);

  int InverseFlag;
  int InverseIterations;
  double InverseTolerance;
};

#endif





