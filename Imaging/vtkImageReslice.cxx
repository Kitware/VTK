/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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
#include <limits.h>
#include <float.h>
#include <math.h>
#include "vtkImageReslice.h"
#include "vtkMath.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkImageReslice* vtkImageReslice::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageReslice");
  if(ret)
    {
    return (vtkImageReslice*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageReslice;
}

//----------------------------------------------------------------------------
vtkImageReslice::vtkImageReslice()
{
  // if NULL, the main Input is used
  this->InformationInput = NULL;
  this->TransformInputSampling = 1;
  this->AutoCropOutput = 0;
  this->OutputDimensionality = 3;

  // flag to use default Spacing
  this->OutputSpacing[0] = VTK_FLOAT_MAX;
  this->OutputSpacing[1] = VTK_FLOAT_MAX;
  this->OutputSpacing[2] = VTK_FLOAT_MAX;

  // ditto
  this->OutputOrigin[0] = VTK_FLOAT_MAX;
  this->OutputOrigin[1] = VTK_FLOAT_MAX;
  this->OutputOrigin[2] = VTK_FLOAT_MAX;

  // ditto
  this->OutputExtent[0] = VTK_INT_MIN;
  this->OutputExtent[2] = VTK_INT_MIN;
  this->OutputExtent[4] = VTK_INT_MIN;

  this->OutputExtent[1] = VTK_INT_MAX;
  this->OutputExtent[3] = VTK_INT_MAX;
  this->OutputExtent[5] = VTK_INT_MAX;

  this->Wrap = 0; // don't wrap
  this->Mirror = 0; // don't mirror
  this->InterpolationMode = VTK_RESLICE_NEAREST; // no interpolation
  this->Optimization = 1; // turn off when you're paranoid 

  // default black background
  this->BackgroundColor[0] = 0;
  this->BackgroundColor[1] = 0;
  this->BackgroundColor[2] = 0;
  this->BackgroundColor[3] = 0;

  // default reslice axes are x, y, z
  this->ResliceAxesDirectionCosines[0] = 1.0;
  this->ResliceAxesDirectionCosines[1] = 0.0;
  this->ResliceAxesDirectionCosines[2] = 0.0;
  this->ResliceAxesDirectionCosines[3] = 0.0;
  this->ResliceAxesDirectionCosines[4] = 1.0;
  this->ResliceAxesDirectionCosines[5] = 0.0;
  this->ResliceAxesDirectionCosines[6] = 0.0;
  this->ResliceAxesDirectionCosines[7] = 0.0;
  this->ResliceAxesDirectionCosines[8] = 1.0;

  // default (0,0,0) axes origin
  this->ResliceAxesOrigin[0] = 0.0;
  this->ResliceAxesOrigin[1] = 0.0;
  this->ResliceAxesOrigin[2] = 0.0;

  // axes and transform are identity if set to NULL
  this->ResliceAxes = NULL;
  this->ResliceTransform = NULL;

  // cache a matrix that converts output voxel indices -> input voxel indices
  this->IndexMatrix = NULL;
  this->OptimizedTransform = NULL;
}

//----------------------------------------------------------------------------
vtkImageReslice::~vtkImageReslice()
{
  this->SetResliceTransform(NULL);
  this->SetResliceAxes(NULL);
  if (this->IndexMatrix)
    {
    this->IndexMatrix->Delete();
    }
  if (this->OptimizedTransform)
    {
    this->OptimizedTransform->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "ResliceAxes: " << this->ResliceAxes << "\n";
  if (this->ResliceAxes)
    {
    this->ResliceAxes->PrintSelf(os,indent.GetNextIndent());
    }
  this->GetResliceAxesDirectionCosines(this->ResliceAxesDirectionCosines);
  os << indent << "ResliceAxesDirectionCosines: " << 
    this->ResliceAxesDirectionCosines[0] << " " <<
    this->ResliceAxesDirectionCosines[1] << " " <<
    this->ResliceAxesDirectionCosines[2] << "\n";
  os << indent << "                             " <<
    this->ResliceAxesDirectionCosines[3] << " " <<
    this->ResliceAxesDirectionCosines[4] << " " <<
    this->ResliceAxesDirectionCosines[5] << "\n";
  os << indent << "                             " <<
    this->ResliceAxesDirectionCosines[6] << " " <<
    this->ResliceAxesDirectionCosines[7] << " " <<
    this->ResliceAxesDirectionCosines[8] << "\n";
  this->GetResliceAxesOrigin(this->ResliceAxesOrigin);
  os << indent << "ResliceAxesOrigin: " << 
    this->ResliceAxesOrigin[0] << " " <<
    this->ResliceAxesOrigin[0] << " " <<
    this->ResliceAxesOrigin[0] << "\n";
  os << indent << "ResliceTransform: " << this->ResliceTransform << "\n";
  if (this->ResliceTransform)
    {
    this->ResliceTransform->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "InformationInput: " << this->InformationInput << "\n";
  os << indent << "TransformInputSampling: " << 
    (this->TransformInputSampling ? "On\n":"Off\n");
  os << indent << "AutoCropOutput: " << 
    (this->AutoCropOutput ? "On\n":"Off\n");
  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputExtent: " << this->OutputExtent[0] << " " <<
    this->OutputExtent[1] << " " << this->OutputExtent[2] << " " <<
    this->OutputExtent[3] << " " << this->OutputExtent[4] << " " <<
    this->OutputExtent[5] << "\n";
  os << indent << "OutputDimensionality: " << 
    this->OutputDimensionality << "\n";
  os << indent << "Wrap: " << (this->Wrap ? "On\n":"Off\n");
  os << indent << "Mirror: " << (this->Mirror ? "On\n":"Off\n");
  os << indent << "InterpolationMode: " 
     << this->GetInterpolationModeAsString() << "\n";
  os << indent << "Optimization: " << (this->Optimization ? "On\n":"Off\n");
  os << indent << "BackgroundColor: " <<
    this->BackgroundColor[0] << " " << this->BackgroundColor[1] << " " <<
    this->BackgroundColor[2] << " " << this->BackgroundColor[3] << "\n";
  os << indent << "BackgroundLevel: " << this->BackgroundColor[0] << "\n";
  os << indent << "Stencil: " << this->GetStencil() << "\n";
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetStencil(vtkImageStencilData *stencil)
{
  this->vtkProcessObject::SetNthInput(1, stencil); 
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageReslice::GetStencil()
{
  if (this->NumberOfInputs < 2) 
    { 
    return NULL;
    }
  else
    {
    return (vtkImageStencilData *)(this->Inputs[1]); 
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetResliceAxesDirectionCosines(double x0, double x1, 
                                                     double x2, double y0,
                                                     double y1, double y2,
                                                     double z0, double z1,
                                                     double z2)
{
  if (!this->ResliceAxes)
    {
    this->ResliceAxes = vtkMatrix4x4::New();
    this->Modified();
    }
  this->ResliceAxes->SetElement(0,0,x0);
  this->ResliceAxes->SetElement(1,0,x1);
  this->ResliceAxes->SetElement(2,0,x2);
  this->ResliceAxes->SetElement(3,0,0);
  this->ResliceAxes->SetElement(0,1,y0);
  this->ResliceAxes->SetElement(1,1,y1);
  this->ResliceAxes->SetElement(2,1,y2);
  this->ResliceAxes->SetElement(3,1,0);
  this->ResliceAxes->SetElement(0,2,z0);
  this->ResliceAxes->SetElement(1,2,z1);
  this->ResliceAxes->SetElement(2,2,z2);
  this->ResliceAxes->SetElement(3,2,0);
}

//----------------------------------------------------------------------------
void vtkImageReslice::GetResliceAxesDirectionCosines(double xdircos[3],
                                                     double ydircos[3],
                                                     double zdircos[3])
{
  if (!this->ResliceAxes)
    {
    xdircos[0] = ydircos[1] = zdircos[2] = 1;
    xdircos[1] = ydircos[2] = zdircos[0] = 0;
    xdircos[2] = ydircos[0] = zdircos[1] = 0;
    return;
    }

  for (int i = 0; i < 3; i++) 
    {
    xdircos[i] = this->ResliceAxes->GetElement(i,0);
    ydircos[i] = this->ResliceAxes->GetElement(i,1);
    zdircos[i] = this->ResliceAxes->GetElement(i,2);
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetResliceAxesOrigin(double x, double y, double z)
{
  if (!this->ResliceAxes)
    {
    this->ResliceAxes = vtkMatrix4x4::New();
    this->Modified();
    }

  this->ResliceAxes->SetElement(0,3,x);
  this->ResliceAxes->SetElement(1,3,y);
  this->ResliceAxes->SetElement(2,3,z);
  this->ResliceAxes->SetElement(3,3,1);
}

//----------------------------------------------------------------------------
void vtkImageReslice::GetResliceAxesOrigin(double origin[3])
{
  if (!this->ResliceAxes)
    {
    origin[0] = origin[1] = origin[2] = 0;
    return;
    }

  for (int i = 0; i < 3; i++)
    {
    origin[i] = this->ResliceAxes->GetElement(i,3);
    }
}

//----------------------------------------------------------------------------
// Account for the MTime of the transform and its matrix when determining
// the MTime of the filter
unsigned long int vtkImageReslice::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;

  if ( this->ResliceTransform != NULL )
    {
    time = this->ResliceTransform->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (this->ResliceTransform->IsA("vtkHomogeneousTransform"))
      { // this is for people who directly modify the transform matrix
      time = ((vtkHomogeneousTransform *)this->ResliceTransform)
        ->GetMatrix()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }    
    }
  if ( this->ResliceAxes != NULL)
    {
    time = this->ResliceAxes->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImageReslice::ComputeInputUpdateExtents(vtkDataObject *output)
{
  this->vtkImageToImageFilter::ComputeInputUpdateExtents(output);

  vtkImageStencilData *stencil = this->GetStencil();
  if (stencil)
    {
    stencil->SetUpdateExtent(output->GetUpdateExtent());
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::ComputeInputUpdateExtent(int inExt[6], 
                                               int outExt[6])
{
  if (this->Optimization)
    {
    this->OptimizedComputeInputUpdateExtent(inExt,outExt);
    return;
    }

  if (this->ResliceTransform)
    {
    this->ResliceTransform->Update();
    if (!this->ResliceTransform->IsA("vtkHomogeneousTransform"))
      { // update the whole input extent if the transform is nonlinear
      this->GetInput()->GetWholeExtent(inExt);
      return;
      }
    }

  int i,j,k;
  float point[4],f;
  float *inSpacing,*inOrigin,*outSpacing,*outOrigin,inInvSpacing[3];

  int wrap = (this->Wrap || this->InterpolationMode != VTK_RESLICE_NEAREST);
  
  inOrigin = this->GetInput()->GetOrigin();
  inSpacing = this->GetInput()->GetSpacing();
  outOrigin = this->GetOutput()->GetOrigin();
  outSpacing = this->GetOutput()->GetSpacing();

  // save effor later: invert inSpacing
  inInvSpacing[0] = 1.0f/inSpacing[0];
  inInvSpacing[1] = 1.0f/inSpacing[1];
  inInvSpacing[2] = 1.0f/inSpacing[2];

  for (i = 0; i < 3; i++)
    {
    inExt[2*i] = VTK_INT_MAX;
    inExt[2*i+1] = VTK_INT_MIN;
    }

  // check the coordinates of the 8 corners of the output extent
  for (i = 0; i < 8; i++)  
    {
    // get output coords
    point[0] = outExt[i%2];
    point[1] = outExt[2+(i/2)%2];
    point[2] = outExt[4+(i/4)%2];

    point[0] = point[0]*outSpacing[0] + outOrigin[0];
    point[1] = point[1]*outSpacing[1] + outOrigin[1];
    point[2] = point[2]*outSpacing[2] + outOrigin[2];
    
    if (this->ResliceAxes)
      {
      point[3] = 1.0f;
      this->ResliceAxes->MultiplyPoint(point, point);
      f = 1.0f/point[3];
      point[0] *= f;
      point[1] *= f;
      point[2] *= f;
      }
    if (this->ResliceTransform)
      {
      this->ResliceTransform->TransformPoint(point, point);
      }

    point[0] = (point[0] - inOrigin[0])*inInvSpacing[0];
    point[1] = (point[1] - inOrigin[1])*inInvSpacing[1];
    point[2] = (point[2] - inOrigin[2])*inInvSpacing[2];

    // set the extent appropriately according to the interpolation mode 
    if (this->GetInterpolationMode() != VTK_RESLICE_NEAREST)
      {
      int extra = (this->GetInterpolationMode() == VTK_RESLICE_CUBIC); 
      for (j = 0; j < 3; j++) 
        {
        k = int(floor(double(point[j]))) - extra;
        if (k < inExt[2*j]) 
          {
          inExt[2*j] = k;
          }
        if (wrap)
          {
          k = int(floor(double(point[j]))) + 1 + extra;        
          }
        else
          {
          k = int(ceil(double(point[j]))) + extra;
          }        
        if (k > inExt[2*j+1])
          {
          inExt[2*j+1] = k;
          }
        }
      }
    else
      {
      for (j = 0; j < 3; j++) 
        {
        k = int(floor(double(point[j] + 0.5)));
        if (k < inExt[2*j])
          { 
          inExt[2*j] = k;
          } 
        if (k > inExt[2*j+1]) 
          {
          inExt[2*j+1] = k;
          }
        }
      }
    }

  int *wholeExtent = this->GetInput()->GetWholeExtent();
  // Clip, just to make sure we hit _some_ of the input extent
  for (i = 0; i < 3; i++)
    {
    if (inExt[2*i] < wholeExtent[2*i])
      {
      inExt[2*i] = wholeExtent[2*i];
      if (wrap)
        {
        inExt[2*i+1] = wholeExtent[2*i+1];
        }
      }
    if (inExt[2*i+1] > wholeExtent[2*i+1])
      {
      inExt[2*i+1] = wholeExtent[2*i+1];
      if (wrap)
        {
        inExt[2*i] = wholeExtent[2*i];
        }
      }
    if (inExt[2*i] > wholeExtent[2*i+1])
      {
      inExt[2*i] = wholeExtent[2*i+1];
      }
    if (inExt[2*i+1] < wholeExtent[2*i])
      {
      inExt[2*i+1] = wholeExtent[2*i];
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::GetAutoCroppedOutputBounds(vtkImageData *input,
                                                 float bounds[6])
{
  int i, j;
  float inSpacing[3], inOrigin[3];
  int inWholeExt[6];
  double f;
  double point[4];

  input->GetWholeExtent(inWholeExt);
  input->GetSpacing(inSpacing);
  input->GetOrigin(inOrigin);

  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  if (this->ResliceAxes)
    {
    vtkMatrix4x4::Invert(this->ResliceAxes, matrix);
    }
  vtkAbstractTransform* transform = NULL;
  if (this->ResliceTransform)
    {
    transform = this->ResliceTransform->GetInverse();
    }
  
  for (i = 0; i < 3; i++)
    {
    bounds[2*i] = VTK_FLOAT_MAX;
    bounds[2*i+1] = -VTK_FLOAT_MAX;
    }
    
  for (i = 0; i < 8; i++)
    {
    point[0] = inOrigin[0] + inWholeExt[i%2]*inSpacing[0];
    point[1] = inOrigin[1] + inWholeExt[2+(i/2)%2]*inSpacing[1];
    point[2] = inOrigin[2] + inWholeExt[4+(i/4)%2]*inSpacing[2];
    point[3] = 1.0f;

    if (this->ResliceTransform)
      {
      transform->TransformPoint(point,point);
      }
    matrix->MultiplyPoint(point,point);
      
    f = 1.0f/point[3];
    point[0] *= f; 
    point[1] *= f; 
    point[2] *= f;

    for (j = 0; j < 3; j++)
      {
      if (point[j] > bounds[2*j+1])
        {
        bounds[2*j+1] = point[j];
        }
      if (point[j] < bounds[2*j])
        {
        bounds[2*j] = point[j];
        }
      }
    }

  matrix->Delete();
}

//----------------------------------------------------------------------------
void vtkImageReslice::ExecuteInformation(vtkImageData *input, 
                                         vtkImageData *output) 
{
  int i,j;
  float inSpacing[3], inOrigin[3];
  int inWholeExt[6];
  float outSpacing[3], outOrigin[3];
  int outWholeExt[6];
  float maxBounds[6];

  vtkImageData *information = this->InformationInput;
  if (!information)
    {
    information = input;
    }
  if (information != input)
    {
    information->UpdateInformation();
    }

  information->GetWholeExtent(inWholeExt);
  information->GetSpacing(inSpacing);
  information->GetOrigin(inOrigin);
  
  // reslice axes matrix is identity by default
  double matrix[4][4];
  double imatrix[4][4];
  for (i = 0; i < 4; i++)
    {
    matrix[i][0] = matrix[i][1] = matrix[i][2] = matrix[i][3] = 0;
    matrix[i][i] = 1;
    imatrix[i][0] = imatrix[i][1] = imatrix[i][2] = imatrix[i][3] = 0;
    imatrix[i][i] = 1;
    }
  if (this->ResliceAxes)
    {
    vtkMatrix4x4::DeepCopy(*matrix, this->ResliceAxes);
    vtkMatrix4x4::Invert(*matrix,*imatrix);
    }

  if (this->AutoCropOutput)
    {
    this->GetAutoCroppedOutputBounds(input, maxBounds);
    }

  // pass the center of the volume through the inverse of the
  // 3x3 direction cosines matrix
  double inCenter[3];
  for (i = 0; i < 3; i++)
    {
    inCenter[i] = inOrigin[i] + \
      0.5*(inWholeExt[2*i] + inWholeExt[2*i+1])*inSpacing[i];
    }

  // the default spacing, extent and origin are the input spacing, extent
  // and origin,  transformed by the direction cosines of the ResliceAxes
  // if requested (note that the transformed output spacing will always
  // be positive)
  for (i = 0; i < 3; i++)
    {
    double s = 0;  // default output spacing 
    double d = 0;  // default linear dimension
    double e = 0;  // default extent start
    double c = 0;  // transformed center-of-volume

    if (this->TransformInputSampling)
      {
      double r = 0.0;
      for (j = 0; j < 3; j++)
        {
        c += imatrix[i][j]*(inCenter[j] - matrix[j][3]);
        double tmp = matrix[j][i]*matrix[j][i];
        s += tmp*fabs(inSpacing[j]);
        d += tmp*(inWholeExt[2*j+1] - inWholeExt[2*j])*fabs(inSpacing[j]);
        e += tmp*inWholeExt[2*j];
        r += tmp;
        } 
      s /= r;
      d /= r*sqrt(r);
      e /= r;
      }
    else
      {
      s = inSpacing[i];
      d = (inWholeExt[2*i+1] - e)*s;
      e = inWholeExt[2*i];
      }

    if (this->OutputSpacing[i] == VTK_FLOAT_MAX)
      {
      outSpacing[i] = s;
      }
    else
      {
      outSpacing[i] = this->OutputSpacing[i];
      }

    if (i >= this->OutputDimensionality)
      {
      outWholeExt[2*i] = 0;
      outWholeExt[2*i+1] = 0;
      }
    else if (this->OutputExtent[2*i] == VTK_INT_MIN ||
             this->OutputExtent[2*i+1] == VTK_INT_MAX)
      {
      if (this->AutoCropOutput)
        {
        d = maxBounds[2*i+1] - maxBounds[2*i];
        }
      outWholeExt[2*i] = int(floor(e + 0.5));
      outWholeExt[2*i+1] = int(floor(outWholeExt[2*i] + 
                                     fabs(d/outSpacing[i]) + 0.5));
      }
    else
      {
      outWholeExt[2*i] = this->OutputExtent[2*i];
      outWholeExt[2*i+1] = this->OutputExtent[2*i+1];
      }

    if (i >= this->OutputDimensionality)
      {
      outOrigin[i] = 0;
      }
    else if (this->OutputOrigin[i] == VTK_FLOAT_MAX)
      {
      if (this->AutoCropOutput)
        { // set origin so edge of extent is edge of bounds
        outOrigin[i] = maxBounds[2*i] - outWholeExt[2*i]*outSpacing[i];
        }
      else
        { // center new bounds over center of input bounds
        outOrigin[i] = c - \
          0.5*(outWholeExt[2*i] + outWholeExt[2*i+1])*outSpacing[i];
        }
      }
    else
      {
      outOrigin[i] = this->OutputOrigin[i];
      }
    }  
  
  output->SetWholeExtent(outWholeExt);
  output->SetSpacing(outSpacing);
  output->SetOrigin(outOrigin);
  output->SetScalarType(input->GetScalarType());
  output->SetNumberOfScalarComponents(input->GetNumberOfScalarComponents());

  // update information related to clipping the data
  vtkImageStencilData *stencil = this->GetStencil();
  if (stencil)
    {
    stencil->SetSpacing(outSpacing);
    stencil->SetOrigin(outOrigin);
    }
}

//----------------------------------------------------------------------------
//  Interpolation subroutines and associated code 
//----------------------------------------------------------------------------

// Three interpolation functions are supported: NearestNeighbor, Trilinear,
// and Tricubic.  These routines have the following signature:
//
//int interpolate(T *&outPtr,
//                const T *inPtr,
//                const int inExt[6],
//                const int inInc[3],
//                int numscalars,
//                const F point[3],
//                int mode,
//                const T *background)
//
// where 'T' is any arithmetic type and 'F' is a float type
//
// The result of the interpolation is put in *outPtr, and outPtr is
// incremented.

//----------------------------------------------------------------------------
// constants for different boundary-handling modes

#define VTK_RESLICE_BACKGROUND 0   // use background if out-of-bounds
#define VTK_RESLICE_WRAP       1   // wrap to opposite side of image
#define VTK_RESLICE_MIRROR     2   // mirror off of the boundary 
#define VTK_RESLICE_NULL       3   // do nothing to *outPtr if out-of-bounds

//--------------------------------------------------------------------------
// a macro to evaluate an expression for all scalar types 
#define vtkTypeCaseMacro(expression) \
      case VTK_DOUBLE: { typedef double VTK_TT; expression; } \
        break; \
      case VTK_FLOAT: { typedef float VTK_TT; expression; } \
        break; \
      case VTK_LONG: { typedef long VTK_TT; expression; } \
        break; \
      case VTK_UNSIGNED_LONG: { typedef unsigned long VTK_TT; expression; } \
        break; \
      case VTK_INT: { typedef int VTK_TT; expression; } \
        break; \
      case VTK_UNSIGNED_INT: { typedef unsigned int VTK_TT; expression; } \
        break; \
      case VTK_SHORT: { typedef short VTK_TT; expression; } \
        break; \
      case VTK_UNSIGNED_SHORT: { typedef unsigned short VTK_TT; expression; } \
        break; \
      case VTK_CHAR: { typedef char VTK_TT; expression; } \
        break; \
      case VTK_UNSIGNED_CHAR: { typedef unsigned char VTK_TT; expression; } \
        break

//--------------------------------------------------------------------------
// The 'floor' function on x86 and mips is many times slower than these
// and is used a lot in this code, optimize for different CPU architectures
static inline int vtkResliceFloor(double x)
{
#if defined mips || defined sparc
  return (int)((unsigned int)(x + 2147483648.0) - 2147483648U);
#elif defined i386 || defined _M_IX86
  unsigned int hilo[2];
  *((double *)hilo) = x + 103079215104.0;  // (2**(52-16))*1.5
  return (int)((hilo[1]<<16)|(hilo[0]>>16));
#else
  return int(floor(x));
#endif
}

static inline int vtkResliceCeil(double x)
{
  return -vtkResliceFloor(-x - 1.0) - 1;
}

static inline int vtkResliceRound(double x)
{
  return vtkResliceFloor(x + 0.5);
}

static inline int vtkResliceFloor(float x)
{
  return vtkResliceFloor((double)x);
}

static inline int vtkResliceCeil(float x)
{
  return vtkResliceCeil((double)x);
}

static inline int vtkResliceRound(float x)
{
  return vtkResliceRound((double)x);
}

// convert a double into an integer plus a fraction  
static inline int vtkResliceFloor(double x, double &f)
{
  int ix = vtkResliceFloor(x);
  f = x - ix;
  return ix;
}

// convert a float into an integer plus a fraction  
static inline int vtkResliceFloor(float x, float &f)
{
  int ix = vtkResliceFloor(x);
  f = x - ix;
  return ix;
}

//----------------------------------------------------------------------------
// rounding functions for each type, with some crazy stunts to avoid
// the use of the 'floor' function which is too slow on x86

template<class T>
static inline void vtkResliceRound(float val, T& rnd)
{
  rnd = vtkResliceRound(val);
}

template<class T>
static inline void vtkResliceRound(double val, T& rnd)
{
  rnd = vtkResliceRound(val);
}

static inline void vtkResliceRound(float val, float& rnd)
{
  rnd = val;
}

static inline void vtkResliceRound(float val, double& rnd)
{
  rnd = val;
}

static inline void vtkResliceRound(double val, float& rnd)
{
  rnd = val;
}

static inline void vtkResliceRound(double val, double& rnd)
{
  rnd = val;
}

//----------------------------------------------------------------------------
// clamping functions for each type

template <class F>
static inline void vtkResliceClamp(F val, char& clamp)
{
  if (val < VTK_CHAR_MIN)
    { 
    val = VTK_CHAR_MIN;
    }
  if (val > VTK_CHAR_MAX)
    { 
    val = VTK_CHAR_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, unsigned char& clamp)
{
  if (val < VTK_UNSIGNED_CHAR_MIN)
    { 
    val = VTK_UNSIGNED_CHAR_MIN;
    }
  if (val > VTK_UNSIGNED_CHAR_MAX)
    { 
    val = VTK_UNSIGNED_CHAR_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, short& clamp)
{
  if (val < VTK_SHORT_MIN)
    { 
    val = VTK_SHORT_MIN;
    }
  if (val > VTK_SHORT_MAX)
    { 
    val = VTK_SHORT_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, unsigned short& clamp)
{
  if (val < VTK_UNSIGNED_SHORT_MIN)
    { 
    val = VTK_UNSIGNED_SHORT_MIN;
    }
  if (val > VTK_UNSIGNED_SHORT_MAX)
    { 
    val = VTK_UNSIGNED_SHORT_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, int& clamp)
{
  if (val < VTK_INT_MIN) 
    {
    val = VTK_INT_MIN;
    }
  if (val > VTK_INT_MAX) 
    {
    val = VTK_INT_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, unsigned int& clamp)
{
  if (val < VTK_UNSIGNED_INT_MIN)
    { 
    val = VTK_UNSIGNED_INT_MIN;
    }
  if (val > VTK_UNSIGNED_INT_MAX)
    { 
    val = VTK_UNSIGNED_INT_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, long& clamp)
{
  if (val < VTK_LONG_MIN) 
    {
    val = VTK_LONG_MIN;
    }
  if (val > VTK_LONG_MAX) 
    {
    val = VTK_LONG_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, unsigned long& clamp)
{
  if (val < VTK_UNSIGNED_LONG_MIN)
    { 
    val = VTK_UNSIGNED_LONG_MIN;
    }
  if (val > VTK_UNSIGNED_LONG_MAX)
    { 
    val = VTK_UNSIGNED_LONG_MAX;
    }
  vtkResliceRound(val,clamp);
}

template <class F>
static inline void vtkResliceClamp(F val, float& clamp)
{
  clamp = val;
}

template <class F>
static inline void vtkResliceClamp(F val, double& clamp)
{
  clamp = val;
}

//----------------------------------------------------------------------------
// Perform a wrap to limit an index to [0,range).
// Ensures correct behaviour when the index is negative.
 
static inline int vtkInterpolateWrap(int num, int range)
{
  if ((num %= range) < 0)
    {
    num += range; // required for some % implementations
    } 
  return num;
}

//----------------------------------------------------------------------------
// Perform a mirror to limit an index to [0,range).
 
static inline int vtkInterpolateMirror(int num, int range)
{
  if (num < 0)
    {
    num = -num - 1;
    }
  int count = num/range;
  num %= range;
  if (count & 0x1)
    {
    num = range - num - 1;
    }
  return num;
}

//----------------------------------------------------------------------------
// Do nearest-neighbor interpolation of the input data 'inPtr' of extent 
// 'inExt' at the 'point'.  The result is placed at 'outPtr'.  
// If the lookup data is beyond the extent 'inExt', set 'outPtr' to
// the background color 'background'.  
// The number of scalar components in the data is 'numscalars'
template <class F, class T>
static
int vtkNearestNeighborInterpolation(T *&outPtr, const T *inPtr,
                                    const int inExt[6], const int inInc[3],
                                    int numscalars, const F point[3],
                                    int mode, const T *background)
{
  int inIdX0 = vtkResliceRound(point[0]) - inExt[0];
  int inIdY0 = vtkResliceRound(point[1]) - inExt[2];
  int inIdZ0 = vtkResliceRound(point[2]) - inExt[4];

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  if (inIdX0 < 0 || inIdX0 >= inExtX ||
      inIdY0 < 0 || inIdY0 >= inExtY ||
      inIdZ0 < 0 || inIdZ0 >= inExtZ)
    {
    if (mode == VTK_RESLICE_WRAP)
      {
      inIdX0 = vtkInterpolateWrap(inIdX0, inExtX);
      inIdY0 = vtkInterpolateWrap(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateWrap(inIdZ0, inExtZ);
      }
    else if (mode == VTK_RESLICE_MIRROR)
      {
      inIdX0 = vtkInterpolateMirror(inIdX0, inExtX);
      inIdY0 = vtkInterpolateMirror(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateMirror(inIdZ0, inExtZ);
      }
    else if (mode == VTK_RESLICE_BACKGROUND)
      {
      do
        {
        *outPtr++ = *background++;
        }
      while (--numscalars);
      return 0;
      }
    else
      {
      return 0;
      }
    }

  inPtr += inIdX0*inInc[0]+inIdY0*inInc[1]+inIdZ0*inInc[2];
  do
    {
    *outPtr++ = *inPtr++;
    }
  while (--numscalars);

  return 1;
} 

//----------------------------------------------------------------------------
// Do trilinear interpolation of the input data 'inPtr' of extent 'inExt'
// at the 'point'.  The result is placed at 'outPtr'.  
// If the lookup data is beyond the extent 'inExt', set 'outPtr' to
// the background color 'background'.  
// The number of scalar components in the data is 'numscalars'
template <class F, class T>
static
int vtkTrilinearInterpolation(T *&outPtr, const T *inPtr,
                              const int inExt[6], const int inInc[3],
                              int numscalars, const F point[3],
                              int mode, const T *background)
{
  F fx, fy, fz;
  int floorX = vtkResliceFloor(point[0], fx);
  int floorY = vtkResliceFloor(point[1], fy);
  int floorZ = vtkResliceFloor(point[2], fz);

  int inIdX0 = floorX - inExt[0];
  int inIdY0 = floorY - inExt[2];
  int inIdZ0 = floorZ - inExt[4];

  int inIdX1 = inIdX0 + (fx != 0);
  int inIdY1 = inIdY0 + (fy != 0);
  int inIdZ1 = inIdZ0 + (fz != 0);

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  if (inIdX0 < 0 || inIdX1 >= inExtX ||
      inIdY0 < 0 || inIdY1 >= inExtY ||
      inIdZ0 < 0 || inIdZ1 >= inExtZ)
    {
    if (mode == VTK_RESLICE_WRAP)
      {
      inIdX0 = vtkInterpolateWrap(inIdX0, inExtX);
      inIdY0 = vtkInterpolateWrap(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateWrap(inIdZ0, inExtZ);

      inIdX1 = vtkInterpolateWrap(inIdX1, inExtX);
      inIdY1 = vtkInterpolateWrap(inIdY1, inExtY);
      inIdZ1 = vtkInterpolateWrap(inIdZ1, inExtZ);
      }
    else if (mode == VTK_RESLICE_MIRROR)
      {
      inIdX0 = vtkInterpolateMirror(inIdX0, inExtX);
      inIdY0 = vtkInterpolateMirror(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateMirror(inIdZ0, inExtZ);

      inIdX1 = vtkInterpolateMirror(inIdX1, inExtX);
      inIdY1 = vtkInterpolateMirror(inIdY1, inExtY);
      inIdZ1 = vtkInterpolateMirror(inIdZ1, inExtZ);
      }
    else if (mode == VTK_RESLICE_BACKGROUND)
      {
      do
        {
        *outPtr++ = *background++;
        }
      while (--numscalars);
      return 0;
      }
    else
      {
      return 0;
      }
    }

  int factX0 = inIdX0*inInc[0];
  int factX1 = inIdX1*inInc[0];
  int factY0 = inIdY0*inInc[1];
  int factY1 = inIdY1*inInc[1];
  int factZ0 = inIdZ0*inInc[2];
  int factZ1 = inIdZ1*inInc[2];

  int i00 = factY0 + factZ0;
  int i01 = factY0 + factZ1;
  int i10 = factY1 + factZ0;
  int i11 = factY1 + factZ1;

  F rx = 1 - fx;
  F ry = 1 - fy;
  F rz = 1 - fz;

  F ryrz = ry*rz;
  F fyrz = fy*rz;
  F ryfz = ry*fz;
  F fyfz = fy*fz;

  const T *inPtr0 = inPtr + factX0;
  const T *inPtr1 = inPtr + factX1;

  do
    {
    F result = (rx*(ryrz*inPtr0[i00] + ryfz*inPtr0[i01] +
                    fyrz*inPtr0[i10] + fyfz*inPtr0[i11]) +
                fx*(ryrz*inPtr1[i00] + ryfz*inPtr1[i01] +
                    fyrz*inPtr1[i10] + fyfz*inPtr1[i11]));

    vtkResliceRound(result, *outPtr++);
    inPtr0++;
    inPtr1++;
    }
  while (--numscalars);

  return 1;
}

//----------------------------------------------------------------------------
// Do tricubic interpolation of the input data 'inPtr' of extent 'inExt' 
// at the 'point'.  The result is placed at 'outPtr'.  
// The number of scalar components in the data is 'numscalars'
// The tricubic interpolation ensures that both the intensity and
// the first derivative of the intensity are smooth across the
// image.  The first derivative is estimated using a 
// centered-difference calculation.


// helper function: set up the lookup indices and the interpolation 
// coefficients

template <class T>
void vtkTricubicInterpCoeffs(T F[4], int l, int h, T f)
{
  const static T half = T(0.5);

  int order = h - l;

  if (order == 0)
    { // no interpolation
    F[0] = 0;
    F[1] = 1;
    F[2] = 0;
    F[3] = 0;
    return;
    }
  if (order == 3)
    { // cubic interpolation
    T fm1 = f - 1;
    T fd2 = f*half;
    T ft3 = f*3; 
    F[0] = -fd2*fm1*fm1;
    F[1] = ((ft3 - 2)*fd2 - 1)*fm1;
    F[2] = -((ft3 - 4)*f - 1)*fd2;
    F[3] = f*fd2*fm1;
    return;
    }
  if (order == 1)
    { // linear interpolation
    F[0] = 0;
    F[1] = 1 - f;
    F[2] = f;
    F[3] = 0;
    return;
    }
  if (l == 0)
    { // quadratic interpolation
    T fp1 = f + 1;
    T fm1 = f - 1; 
    T fd2 = f*half;
    F[0] = fd2*fm1;
    F[1] = -fp1*fm1;
    F[2] = fp1*fd2;
    F[3] = 0;
    return;
    }
  // else
    { // quadratic interpolation
    T fm1 = f - 1;
    T fm2 = fm1 - 1;
    T fm1d2 = fm1*half;
    F[0] = 0;
    F[1] = fm1d2*fm2;
    F[2] = -f*fm2;
    F[3] = f*fm1d2;
    return;
    }
}

// tricubic interpolation
template <class F, class T>
static
int vtkTricubicInterpolation(T *&outPtr, const T *inPtr,
                             const int inExt[6], const int inInc[3],
                             int numscalars, const F point[3],
                             int mode, const T *background)
{
  F fx, fy, fz;
  int floorX = vtkResliceFloor(point[0], fx);
  int floorY = vtkResliceFloor(point[1], fy);
  int floorZ = vtkResliceFloor(point[2], fz);

  int fxIsNotZero = (fx != 0);
  int fyIsNotZero = (fy != 0);
  int fzIsNotZero = (fz != 0);

  int inIdX0 = floorX - inExt[0];
  int inIdY0 = floorY - inExt[2];
  int inIdZ0 = floorZ - inExt[4];

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  int inIncX = inInc[0];
  int inIncY = inInc[1];
  int inIncZ = inInc[2];

  int factX[4], factY[4], factZ[4];

  if (inIdX0 < 0 || (inIdX0 + fxIsNotZero) >= inExtX ||
      inIdY0 < 0 || (inIdY0 + fyIsNotZero) >= inExtY ||
      inIdZ0 < 0 || (inIdZ0 + fzIsNotZero) >= inExtZ)
    {
    if (mode != VTK_RESLICE_WRAP && mode != VTK_RESLICE_MIRROR)
      {
      if (mode == VTK_RESLICE_BACKGROUND)
        {
        do
          {
          *outPtr++ = *background++;
          }
        while (--numscalars);
        return 0;
        }
      else
        {
        return 0;
        }
      }
    }

  F fX[4], fY[4], fZ[4];
  int k1, k2, j1, j2, i1, i2;

  if (mode == VTK_RESLICE_WRAP || mode == VTK_RESLICE_MIRROR)
    {
    i1 = 0;
    i2 = 3;
    vtkTricubicInterpCoeffs(fX, i1, i2, fx);

    j1 = 1 - fyIsNotZero;
    j2 = 1 + (fyIsNotZero<<1);
    vtkTricubicInterpCoeffs(fY, j1, j2, fy);

    k1 = 1 - fzIsNotZero;
    k2 = 1 + (fzIsNotZero<<1);
    vtkTricubicInterpCoeffs(fZ, k1, k2, fz);

    if (mode == VTK_RESLICE_WRAP)
      {
      for (int i = 0; i < 4; i++)
        {
        factX[i] = vtkInterpolateWrap(inIdX0 + i - 1, inExtX)*inIncX;
        factY[i] = vtkInterpolateWrap(inIdY0 + i - 1, inExtY)*inIncY;
        factZ[i] = vtkInterpolateWrap(inIdZ0 + i - 1, inExtZ)*inIncZ;
        }
      }
    else
      {
      for (int i = 0; i < 4; i++)
        {
        factX[i] = vtkInterpolateMirror(inIdX0 + i - 1, inExtX)*inIncX;
        factY[i] = vtkInterpolateMirror(inIdY0 + i - 1, inExtY)*inIncY;
        factZ[i] = vtkInterpolateMirror(inIdZ0 + i - 1, inExtZ)*inIncZ;
        }
      }
    }
  else
    {
    // depending on whether we are at the edge of the 
    // input extent, choose the appropriate interpolation
    // method to use

    i1 = 1 - (inIdX0 > 0)*fxIsNotZero;
    j1 = 1 - (inIdY0 > 0)*fyIsNotZero;
    k1 = 1 - (inIdZ0 > 0)*fzIsNotZero;

    i2 = 1 + (1 + (inIdX0 + 2 < inExtX))*fxIsNotZero;
    j2 = 1 + (1 + (inIdY0 + 2 < inExtY))*fyIsNotZero;
    k2 = 1 + (1 + (inIdZ0 + 2 < inExtZ))*fzIsNotZero;

    vtkTricubicInterpCoeffs(fX, i1, i2, fx);
    vtkTricubicInterpCoeffs(fY, j1, j2, fy);
    vtkTricubicInterpCoeffs(fZ, k1, k2, fz);

    factX[1] = inIdX0*inIncX;
    factX[0] = factX[1] - inIncX;
    factX[2] = factX[1] + inIncX;
    factX[3] = factX[2] + inIncX;

    factY[1] = inIdY0*inIncY;
    factY[0] = factY[1] - inIncY;
    factY[2] = factY[1] + inIncY;
    factY[3] = factY[2] + inIncY;
    
    factZ[1] = inIdZ0*inIncZ;
    factZ[0] = factZ[1] - inIncZ;
    factZ[2] = factZ[1] + inIncZ;
    factZ[3] = factZ[2] + inIncZ;

    // this little bit of wierdness allows us to unroll the x loop
    if (i1 > 0)
      {
      factX[0] = factX[1];
      }
    if (i2 < 3)
      {
      factX[3] = factX[1];
      if (i2 < 2)
        {
        factX[2] = factX[1];
        }
      }
    }
  
  do // loop over components
    {
    F val = 0;
    int k = k1;
    do // loop over z
      {
      F fz = fZ[k];
      int factz = factZ[k];
      int j = j1;
      do // loop over y
        {
        F fy = fY[j];
        F fzy = fz*fy;
        int factzy = factz + factY[j];
        const T *tmpPtr = inPtr + factzy;
        // loop over x is unrolled (significant performance boost)
        val += fzy*(fX[0]*tmpPtr[factX[0]] +
                    fX[1]*tmpPtr[factX[1]] +
                    fX[2]*tmpPtr[factX[2]] +
                    fX[3]*tmpPtr[factX[3]]);
        }
      while (++j <= j2);
      }
    while (++k <= k2);

    vtkResliceClamp(val, *outPtr++);
    inPtr++;
    }
  while (--numscalars);

  return 1;
}                   

//--------------------------------------------------------------------------
// get appropriate interpolation function according to interpolation mode
// and scalar type
template<class F>
static
void vtkGetResliceInterpFunc(vtkImageReslice *self,
                             int (**interpolate)(void *&outPtr,
                                                 const void *inPtr,
                                                 const int inExt[6],
                                                 const int inInc[3],
                                                 int numscalars,
                                                 const F point[3],
                                                 int mode,
                                                 const void *background))
{
  int dataType = self->GetOutput()->GetScalarType();
  int interpolationMode = self->GetInterpolationMode();
  
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      switch (dataType)
	{
	vtkTypeCaseMacro(*((int (**)(VTK_TT *&outPtr, const VTK_TT *inPtr,
				     const int inExt[6], const int inInc[3],
				     int numscalars, const F point[3],
				     int mode,
				     const VTK_TT *background))interpolate) = \
			 &vtkNearestNeighborInterpolation);
	default:
	  interpolate = 0;
	}
      break;
    case VTK_RESLICE_LINEAR:
      switch (dataType)
	{
	vtkTypeCaseMacro(*((int (**)(VTK_TT *&outPtr, const VTK_TT *inPtr,
				     const int inExt[6], const int inInc[3],
				     int numscalars, const F point[3],
				     int mode,
				     const VTK_TT *background))interpolate) = \
			 &vtkTrilinearInterpolation);
	default:
	  interpolate = 0;
	}
      break;
    case VTK_RESLICE_CUBIC:
      switch (dataType)
	{
	vtkTypeCaseMacro(*((int (**)(VTK_TT *&outPtr, const VTK_TT *inPtr,
				     const int inExt[6], const int inInc[3],
				     int numscalars, const F point[3],
				     int mode,
				     const VTK_TT *background))interpolate) = \
			 &vtkTricubicInterpolation);
	default:
	  interpolate = 0;
	}
      break;
    default:
      interpolate = 0;
    }
}


//----------------------------------------------------------------------------
// Some helper functions for 'Execute'
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------
// pixel copy function, templated for different scalar types
template<class T>
static void vtkSetPixels(T *&outPtr, const T *inPtr, int numscalars, int n)
{
  for (int i = 0; i < n; i++)
    {
    const T *tmpPtr = inPtr;
    int m = numscalars;
    do
      {
      *outPtr++ = *tmpPtr++;
      }
    while (--m);
    }
}

// optimized for 1 scalar components
template<class T>
static void vtkSetPixels1(T *&outPtr, const T *inPtr,
                          int vtkNotUsed(numscalars), int n)
{
  T val = *inPtr;
  for (int i = 0; i < n; i++)
    {
    *outPtr++ = val;
    }
}

// get a pixel copy function that is appropriate for the data type
void vtkGetSetPixelsFunc(vtkImageReslice *self,
                         void (**setpixels)(void *&out, const void *in,
                                            int numscalars, int n))
{
  int dataType = self->GetOutput()->GetScalarType();
  int numscalars = self->GetOutput()->GetNumberOfScalarComponents();

  switch (numscalars)
    {
    case 1:
      switch (dataType)
	{
	vtkTypeCaseMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
				      int numscalars, int n))setpixels) = \
			 vtkSetPixels1);
	default:
	  setpixels = 0;
	}
    default:
      switch (dataType)
	{
	vtkTypeCaseMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
				      int numscalars, int n))setpixels) = \
			 vtkSetPixels);
	default:
	  setpixels = 0;
	}
    }
}

//----------------------------------------------------------------------------
// Convert background color from float to appropriate type
template <class T>
static
void vtkAllocBackgroundPixelT(vtkImageReslice *self,
                              T **background_ptr, int numComponents)
{
  *background_ptr = new T[numComponents];
  T *background = *background_ptr;

  for (int i = 0; i < numComponents; i++)
    {
    if (i < 4)
      {
      vtkResliceClamp(self->GetBackgroundColor()[i], background[i]);
      }
    else
      {
      background[i] = 0;
      }
    }
}

void vtkAllocBackgroundPixel(vtkImageReslice *self, void **rval, 
                             int numComponents)
{
  switch (self->GetOutput()->GetScalarType())
    {
    vtkTypeCaseMacro(vtkAllocBackgroundPixelT(self, (VTK_TT **)rval,
					      numComponents));
    }
}      

void vtkFreeBackgroundPixel(vtkImageReslice *self, void **rval)
{
  switch (self->GetOutput()->GetScalarType())
    {
    vtkTypeCaseMacro(delete [] *((VTK_TT **)rval));
    }

  *rval = 0;
}      

//----------------------------------------------------------------------------
// helper function for clipping of the output with a stencil
int vtkResliceGetNextExtent(vtkImageStencilData *stencil,
                            int &r1, int &r2, int rmin, int rmax,
                            int yIdx, int zIdx, 
                            void *&outPtr, void *background, 
                            int numscalars,
                            void (*setpixels)(void *&out,
                                              const void *in,
                                              int numscalars,
                                              int n),
                            int &iter)
{
  // trivial case if stencil is not set
  if (!stencil)
    {
    if (iter++ == 0)
      {
      r1 = rmin;
      r2 = rmax;
      return 1;
      }
    return 0;
    }

  // for clearing, start at last r2 plus 1
  int clear1 = r2 + 1;
  if (iter == 0)
    { // if no 'last time', start at rmin
    clear1 = rmin;
    }

  int rval = stencil->GetNextExtent(r1, r2, rmin, rmax, yIdx, zIdx, iter);
  int clear2 = r1 - 1;
  if (rval == 0)
    {
    clear2 = rmax;
    }

  setpixels(outPtr, background, numscalars, clear2 - clear1 + 1);

  return rval;
}

//----------------------------------------------------------------------------
// This function executes the filter for any type of data.  It is much simpler
// in structure than vtkImageResliceOptimizedExecute.
static
void vtkImageResliceExecute(vtkImageReslice *self,
                            vtkImageData *inData, void *inPtr,
                            vtkImageData *outData, void *outPtr,
                            int outExt[6], int id)
{
  int numscalars;
  int idX, idY, idZ;
  int idXmin, idXmax, iter;
  int outIncX, outIncY, outIncZ, scalarSize;
  int inExt[6], inInc[3];
  unsigned long count = 0;
  unsigned long target;
  float point[4];
  float f;
  float *inSpacing, *inOrigin, *outSpacing, *outOrigin, inInvSpacing[3];
  void *background;
  int (*interpolate)(void *&outPtr, const void *inPtr,
                     const int inExt[6], const int inInc[3],
                     int numscalars, const float point[3],
                     int mode, const void *background);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // the 'mode' species what to do with the 'pad' (out-of-bounds) area
  int mode = VTK_RESLICE_BACKGROUND;
  if (self->GetMirror())
    {
    mode = VTK_RESLICE_MIRROR;
    }
  else if (self->GetWrap())
    {
    mode = VTK_RESLICE_WRAP;
    }

  // the transformation to apply to the data
  vtkAbstractTransform *transform = self->GetResliceTransform();
  vtkMatrix4x4 *matrix = self->GetResliceAxes();

  // for conversion to data coordinates
  inOrigin = inData->GetOrigin();
  inSpacing = inData->GetSpacing();
  outOrigin = outData->GetOrigin();
  outSpacing = outData->GetSpacing();

  // save effor later: invert inSpacing
  inInvSpacing[0] = 1.0f/inSpacing[0];
  inInvSpacing[1] = 1.0f/inSpacing[1];
  inInvSpacing[2] = 1.0f/inSpacing[2];

  // find maximum input range
  inData->GetExtent(inExt);
  
  // for the progress meter
  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  scalarSize = outData->GetScalarSize();
  numscalars = inData->GetNumberOfScalarComponents();

  // allocate a voxel to copy into the background (out-of-bounds) regions
  vtkAllocBackgroundPixel(self, &background, numscalars);

  // get the appropriate functions for interpolation and pixel copying
  vtkGetResliceInterpFunc(self, &interpolate);
  vtkGetSetPixelsFunc(self, &setpixels);

  // Loop through output voxels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (id == 0) 
        { // update the progress if this is the main thread
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      
      iter = 0; // if there is a stencil, it is applied here
      while (vtkResliceGetNextExtent(self->GetStencil(), idXmin, idXmax,
                                     outExt[0], outExt[1], idY, idZ,
                                     outPtr, background, numscalars, 
                                     setpixels, iter))
        {
        for (idX = idXmin; idX <= idXmax; idX++)
          {
          // convert to data coordinates 
          point[0] = idX*outSpacing[0] + outOrigin[0];
          point[1] = idY*outSpacing[1] + outOrigin[1];
          point[2] = idZ*outSpacing[2] + outOrigin[2];

          // apply ResliceAxes matrix
          if (matrix)
            {
            point[3] = 1.0f;
            matrix->MultiplyPoint(point, point);
            f = 1.0/point[3];
            point[0] *= f;
            point[1] *= f;
            point[2] *= f;
            }

          // apply ResliceTransform
          if (transform)
            {
            transform->InternalTransformPoint(point, point);
            }
          
          // convert back to voxel indices
          point[0] = (point[0] - inOrigin[0])*inInvSpacing[0];
          point[1] = (point[1] - inOrigin[1])*inInvSpacing[1];
          point[2] = (point[2] - inOrigin[2])*inInvSpacing[2];

          // interpolate output voxel from input data set
          interpolate(outPtr, inPtr, inExt, inInc, numscalars,
                      point, mode, background);
          } 
        }
      outPtr = (void *)((char *)outPtr + outIncY*scalarSize);
      }
    outPtr = (void *)((char *)outPtr + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageReslice::ThreadedExecute(vtkImageData *inData, 
                                      vtkImageData *outData,
                                      int outExt[6], int id)
{
  if (this->Optimization)
    {
    this->OptimizedThreadedExecute(inData,outData,outExt,id);
    return;
    }

  void *inPtr = inData->GetScalarPointerForExtent(inData->GetExtent());
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
                << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
            << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }

  vtkImageResliceExecute(this, inData, inPtr, outData, outPtr, outExt, id);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// The remainder of this file is the 'optimized' version of the code.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
template <class F>
static
void vtkResliceOptimizedComputeInputUpdateExtent(vtkImageReslice *self,
                                                 int inExt[6],
                                                 int outExt[6],
                                                 F newmat[4][4])
{
  int i,j,k;
  int idX,idY,idZ;
  F xAxis[4], yAxis[4], zAxis[4], origin[4];
  F point[4],f;

  int wrap = (self->GetWrap() || 
              self->GetInterpolationMode() != VTK_RESLICE_NEAREST);

  // convert matrix from world coordinates to pixel indices
  for (i = 0; i < 4; i++)
    {
    xAxis[i] = newmat[i][0];
    yAxis[i] = newmat[i][1];
    zAxis[i] = newmat[i][2];
    origin[i] = newmat[i][3];
    }

  for (i = 0; i < 3; i++)
    {
    inExt[2*i] = VTK_INT_MAX;
    inExt[2*i+1] = VTK_INT_MIN;
    }
  
  for (i = 0; i < 8; i++)
    {
    // calculate transform using method in vtkImageResliceExecute
    idX = outExt[i%2];
    idY = outExt[2+(i/2)%2];
    idZ = outExt[4+(i/4)%2];
    
    for (j = 0; j < 4; j++) 
      {
      point[j] = origin[j] + idZ*zAxis[j];
      point[j] = point[j] + idY*yAxis[j];
      point[j] = point[j] + idX*xAxis[j];
      }
    
    f = 1/point[3];
    point[0] *= f;
    point[1] *= f;
    point[2] *= f;
    
    if (self->GetInterpolationMode() != VTK_RESLICE_NEAREST)
      {
      int extra = (self->GetInterpolationMode() == VTK_RESLICE_CUBIC); 
      for (j = 0; j < 3; j++) 
        {
        k = vtkResliceFloor(point[j])-extra;
        if (k < inExt[2*j])
          { 
          inExt[2*j] = k;
          }
        if (wrap)
          {
          k = vtkResliceFloor(point[j])+1+extra;
          }
        else
          {
          k = vtkResliceCeil(point[j])+extra;
          }
        if (k > inExt[2*j+1])
          { 
          inExt[2*j+1] = k;
          }
        }
      }
    else
      {
      for (j = 0; j < 3; j++) 
        {
        k = vtkResliceRound(point[j]);
        if (k < inExt[2*j]) 
          {
          inExt[2*j] = k;
          } 
        if (k > inExt[2*j+1])
          { 
          inExt[2*j+1] = k;
          }
        }
      }
    }

  int *wholeExtent = self->GetInput()->GetWholeExtent();
  // Clip, just to make sure we hit _some_ of the input extent
  for (i = 0; i < 3; i++)
    {
    if (inExt[2*i] < wholeExtent[2*i])
      {
      inExt[2*i] = wholeExtent[2*i];
      if (wrap)
        {
        inExt[2*i+1] = wholeExtent[2*i+1];
        }
      }
    if (inExt[2*i+1] > wholeExtent[2*i+1])
      {
      inExt[2*i+1] = wholeExtent[2*i+1];
      if (wrap)
        {
        inExt[2*i] = wholeExtent[2*i];
        }
      }
    if (inExt[2*i] > wholeExtent[2*i+1])
      {
      inExt[2*i] = wholeExtent[2*i+1];
      }
    if (inExt[2*i+1] < wholeExtent[2*i])
      {
      inExt[2*i+1] = wholeExtent[2*i];
      }
    }
}

void vtkImageReslice::OptimizedComputeInputUpdateExtent(int inExt[6], 
                                                        int outExt[6])
{
  vtkMatrix4x4 *matrix = this->GetIndexMatrix();

  if (this->GetOptimizedTransform() != NULL)
    { // update the entire input extent
    this->OptimizedTransform->Update();
    this->GetInput()->GetWholeExtent(inExt);
    return;
    }

  float newmat[4][4];
  for (int i = 0; i < 4; i++)
    {
    newmat[i][0] = matrix->GetElement(i,0);
    newmat[i][1] = matrix->GetElement(i,1);
    newmat[i][2] = matrix->GetElement(i,2);
    newmat[i][3] = matrix->GetElement(i,3);
    }
  vtkResliceOptimizedComputeInputUpdateExtent(this, inExt, outExt, newmat);
}

template<class F>
static inline
int intersectionHelper(F *point, F *axis, int *limit, int ai, int *outExt)
{
  F rd = (limit[ai]*point[3]-point[ai])
      /(axis[ai]-limit[ai]*axis[3]) + 0.5; 
    
  if (rd < outExt[0])
    { 
    return outExt[0];
    }
  else if (rd > outExt[1])
    {
    return outExt[1];
    }
  else
    {
    return int(rd);
    }
}

template <class F>
static
int intersectionLow(F *point, F *axis, int *sign,
                    int *limit, int ai, int *outExt)
{
  // approximate value of r
  int r = intersectionHelper(point,axis,limit,ai,outExt);

  F f,p;
  // move back and forth to find the point just inside the extent
  for (;;)
    {
    f = point[3]+r*axis[3];
    p = point[ai]+r*axis[ai];
    f = 1/f;
    p *= f;
    
    if ((sign[ai] < 0 && r > outExt[0] ||
         sign[ai] > 0 && r < outExt[1])
        && vtkResliceRound(p) < limit[ai])
      {
      r += sign[ai];
      }
    else
      {
      break;
      }
    }

  for (;;)
    {
    f = point[3]+(r-sign[3])*axis[3];
    p = point[ai]+(r-sign[ai])*axis[ai];
    f = 1/f;
    p *= f;
    
    if ((sign[ai] > 0 && r > outExt[0] ||
         sign[ai] < 0 && r < outExt[1])
        && vtkResliceRound(p) >= limit[ai])
      {
      r -= sign[ai];
      }
    else
      {
      break;
      }
    }

  return r;
}

// same as above, but for x = x_max
template <class F>
static
int intersectionHigh(F *point, F *axis, int *sign, 
                     int *limit, int ai, int *outExt)
{
  int r = intersectionHelper(point,axis,limit,ai,outExt);

  F f,p;
  // move back and forth to find the point just inside the extent
  for (;;)
    {
    f = point[3]+r*axis[3];
    p = point[ai]+r*axis[ai];
    f = 1/f;
    p *= f;
    
    if ((sign[ai] > 0 && r > outExt[0] ||
         sign[ai] < 0 && r < outExt[1])
        && vtkResliceRound(p) > limit[ai])
      {
      r -= sign[ai];
      }
    else
      {
      break;
      }
    }

  for (;;)
    {
    f = point[3]+(r+sign[3])*axis[3];
    p = point[ai]+(r+sign[ai])*axis[ai];
    f = 1/f;
    p *= f;
    
    if ((sign[ai] < 0 && r > outExt[0] ||
         sign[ai] > 0 && r < outExt[1])
        && vtkResliceRound(p) <= limit[ai])
      {
      r += sign[ai];
      }
    else
      {
      break;
      }
    }

  return r;
}

template <class F>
static
int isBounded(F *point, F *xAxis, int *inMin, int *inMax, int ai, int r)
{
  int bi = ai+1; 
  int ci = ai+2;
  if (bi > 2) 
    { 
    bi -= 3; // coordinate index must be 0, 1 or 2 
    } 
  if (ci > 2)
    { 
    ci -= 3;
    }
  F f = point[3]+r*xAxis[3];
  f = 1/f;
  F fbp = point[bi]+r*xAxis[bi];
  F fcp = point[ci]+r*xAxis[ci];
  fbp *= f;
  fcp *= f;

  int bp = vtkResliceRound(fbp);
  int cp = vtkResliceRound(fcp);
  
  return (bp >= inMin[bi] && bp <= inMax[bi] &&
          cp >= inMin[ci] && cp <= inMax[ci]);
}

// this huge mess finds out where the current output raster
// line intersects the input volume
void vtkResliceFindExtentHelper(int &r1, int &r2, int sign, int *outExt)
{
  if (sign < 0)
    {
    int i = r1;
    r1 = r2;
    r2 = i;
    }
  
  // bound r1,r2 within reasonable limits
  if (r1 < outExt[0]) 
    {
    r1 = outExt[0];
    }
  if (r2 > outExt[1]) 
    {
    r2 = outExt[1];
    }
  if (r1 > r2) 
    {
    r1 = outExt[0];
    r2 = outExt[0]-1;
    }
}  

template <class F>
static
void vtkResliceFindExtent(int& r1, int& r2, F *point, F *xAxis, 
                          int *inMin, int *inMax, int *outExt)
{
  int i, ix, iy, iz;
  int sign[3];
  int indx1[4],indx2[4];
  F f1,f2,p1,p2;

  // find signs of components of x axis 
  // (this is complicated due to the homogeneous coordinate)
  for (i = 0; i < 3; i++)
    {
    f1 = point[3];
    p1 = point[i];
    f1 = 1/f1;
    p1 *= f1;

    f2 = point[3]+xAxis[3];
    p2 = point[i]+xAxis[i];
    f2 = 1/f2;
    p2 *= f2;

    if (p1 <= p2)
      {
      sign[i] = 1;
      }
    else 
      {
      sign[i] = -1;
      }
    } 
  
  // order components of xAxis from largest to smallest
  
  ix = 0;
  for (i = 1; i < 3; i++)
    {
    if (((xAxis[i] < 0) ? (-xAxis[i]) : (xAxis[i])) >
        ((xAxis[ix] < 0) ? (-xAxis[ix]) : (xAxis[ix])))
      {
      ix = i;
      }
    }
  
  iy = ((ix > 1) ? ix-2 : ix+1);
  iz = ((ix > 0) ? ix-1 : ix+2);

  if (((xAxis[iy] < 0) ? (-xAxis[iy]) : (xAxis[iy])) >
      ((xAxis[iz] < 0) ? (-xAxis[iz]) : (xAxis[iz])))
    {
    i = iy;
    iy = iz;
    iz = i;
    }

  r1 = intersectionLow(point,xAxis,sign,inMin,ix,outExt);
  r2 = intersectionHigh(point,xAxis,sign,inMax,ix,outExt);
  
  // find points of intersections
  // first, find w-value for perspective (will usually be 1)
  f1 = point[3]+r1*xAxis[3];
  f2 = point[3]+r2*xAxis[3];
  f1 = 1/f1;
  f2 = 1/f2;
  
  for (i = 0; i < 3; i++)
    {
    p1 = point[i]+r1*xAxis[i];
    p2 = point[i]+r2*xAxis[i];
    p1 *= f1;
    p2 *= f2;

    indx1[i] = vtkResliceRound(p1);
    indx2[i] = vtkResliceRound(p2);
    }
  if (isBounded(point,xAxis,inMin,inMax,ix,r1))
    { // passed through x face, check opposing face
    if (isBounded(point,xAxis,inMin,inMax,ix,r2))
      {
      vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
      return;
      }
    
    if (indx2[iy] < inMin[iy])
      { // check y face
      r2 = intersectionLow(point,xAxis,sign,inMin,iy,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iy,r2))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    else if (indx2[iy] > inMax[iy])
      { // check other y face
      r2 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iy,r2))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    
    if (indx2[iz] < inMin[iz])
      { // check z face
      r2 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r2))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    else if (indx2[iz] > inMax[iz])
      { // check other z face
      r2 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r2))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    }
  
  if (isBounded(point,xAxis,inMin,inMax,ix,r2))
    { // passed through the opposite x face
    if (indx1[iy] < inMin[iy])
      { // check y face
      r1 = intersectionLow(point,xAxis,sign,inMin,iy,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iy,r1))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    else if (indx1[iy] > inMax[iy])
      { // check other y face
      r1 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iy,r1))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    
    if (indx1[iz] < inMin[iz])
      { // check z face
      r1 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r1))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    else if (indx1[iz] > inMax[iz])
      { // check other z face
      r1 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r1))
        {
        vtkResliceFindExtentHelper(r1,r2,sign[ix],outExt);
        return;
        }
      }
    }
  
  if ((indx1[iy] >= inMin[iy] && indx2[iy] < inMin[iy]) ||
      (indx1[iy] < inMin[iy] && indx2[iy] >= inMin[iy]))
    { // line might pass through bottom face
    r1 = intersectionLow(point,xAxis,sign,inMin,iy,outExt);
    if (isBounded(point,xAxis,inMin,inMax,iy,r1))
      {
      if ((indx1[iy] <= inMax[iy] && indx2[iy] > inMax[iy]) ||
          (indx1[iy] > inMax[iy] && indx2[iy] <= inMax[iy]))
        { // line might pass through top face
        r2 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
        if (isBounded(point,xAxis,inMin,inMax,iy,r2))
          {
          vtkResliceFindExtentHelper(r1,r2,sign[iy],outExt);
          return;
          }
        }
      
      if (indx1[iz] < inMin[iz] && indx2[iy] < inMin[iy] ||
          indx2[iz] < inMin[iz] && indx1[iy] < inMin[iy])
        { // line might pass through in-to-screen face
        r2 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
        if (isBounded(point,xAxis,inMin,inMax,iz,r2))
          {
          vtkResliceFindExtentHelper(r1,r2,sign[iy],outExt);
          return;
          }
        }
      else if (indx1[iz] > inMax[iz] && indx2[iy] < inMin[iy] ||
               indx2[iz] > inMax[iz] && indx1[iy] < inMin[iy])
        { // line might pass through out-of-screen face
        r2 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
        if (isBounded(point,xAxis,inMin,inMax,iz,r2))
          {
          vtkResliceFindExtentHelper(r1,r2,sign[iy],outExt);
          return;
          }
        } 
      }
    }
  
  if ((indx1[iy] <= inMax[iy] && indx2[iy] > inMax[iy]) ||
      (indx1[iy] > inMax[iy] && indx2[iy] <= inMax[iy]))
    { // line might pass through top face
    r2 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
    if (isBounded(point,xAxis,inMin,inMax,iy,r2))
      {
      if (indx1[iz] < inMin[iz] && indx2[iy] > inMax[iy] ||
          indx2[iz] < inMin[iz] && indx1[iy] > inMax[iy])
        { // line might pass through in-to-screen face
        r1 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
        if (isBounded(point,xAxis,inMin,inMax,iz,r1))
          {
          vtkResliceFindExtentHelper(r1,r2,sign[iy],outExt);
          return;
          }
        }
      else if (indx1[iz] > inMax[iz] && indx2[iy] > inMax[iy] || 
               indx2[iz] > inMax[iz] && indx1[iy] > inMax[iy])
        { // line might pass through out-of-screen face
        r1 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
        if (isBounded(point,xAxis,inMin,inMax,iz,r1))
          {
          vtkResliceFindExtentHelper(r1,r2,sign[iy],outExt);
          return;
          }
        }
      } 
    }
  
  if ((indx1[iz] >= inMin[iz] && indx2[iz] < inMin[iz]) ||
      (indx1[iz] < inMin[iz] && indx2[iz] >= inMin[iz]))
    { // line might pass through in-to-screen face
    r1 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
    if (isBounded(point,xAxis,inMin,inMax,iz,r1))
      {
      if (indx1[iz] > inMax[iz] || indx2[iz] > inMax[iz])
        { // line might pass through out-of-screen face
        r2 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
        if (isBounded(point,xAxis,inMin,inMax,iz,r2))
          {
          vtkResliceFindExtentHelper(r1,r2,sign[iz],outExt);
          return;
          }
        }
      }
    }
  
  r1 = outExt[0];
  r2 = outExt[0] - 1;
}

// application of the transform has different forms for fixed-point
// vs. floating-point
template<class F>
static inline
void vtkResliceApplyTransform(vtkAbstractTransform *newtrans,
                              F inPoint[3], F inOrigin[3],
                              F inInvSpacing[3])
{
  newtrans->InternalTransformPoint(inPoint, inPoint);
  inPoint[0] -= inOrigin[0];
  inPoint[1] -= inOrigin[1];
  inPoint[2] -= inOrigin[2];
  inPoint[0] *= inInvSpacing[0];
  inPoint[1] *= inInvSpacing[1];
  inPoint[2] *= inInvSpacing[2];
}

// The vtkOptimizedExecute() function uses an optimization which
// is conceptually simple, but complicated to implement.

// In the un-optimized version, each output voxel
// is converted into a set of look-up indices for the input data;
// then, the indices are checked to ensure they lie within the
// input data extent.

// In the optimized version below, the check is done in reverse:
// it is first determined which output voxels map to look-up indices
// within the input data extent.  Then, further calculations are
// done only for those voxels.  This means that 1) minimal work
// is done for voxels which map to regions outside fo the input
// extent (they are just set to the background color) and 2)
// the inner loops of the look-up and interpolation are
// tightened relative to the un-uptimized version. 

template <class F>
static 
void vtkOptimizedExecute(vtkImageReslice *self,
                         vtkImageData *inData, void *inPtr,
                         vtkImageData *outData, void *outPtr,
                         int outExt[6], int id, F newmat[4][4], 
                         vtkAbstractTransform *newtrans)
{
  int i, numscalars;
  int idX, idY, idZ;
  int outIncX, outIncY, outIncZ, scalarSize;
  int inExt[6];
  int inMax[3], inMin[3];
  int inInc[3];
  unsigned long count = 0;
  unsigned long target;
  int r1, r2;
  int iter, idXmin, idXmax;
  float temp[3];
  F inOrigin[3], inInvSpacing[3];
  F xAxis[4], yAxis[4], zAxis[4], origin[4]; 
  F inPoint0[4];
  F inPoint1[4];
  F inPoint[4], f;
  void *background;
  int (*interpolate)(void *&outPtr, const void *inPtr,
                     const int inExt[6], const int inInc[3],
                     int numscalars, const F point[3],
                     int mode, const void *background);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  int mode = VTK_RESLICE_BACKGROUND;
  int wrap = 0;

  if (self->GetMirror())
    {
    mode = VTK_RESLICE_MIRROR;
    wrap = 1;
    }
  else if (self->GetWrap())
    {
    mode = VTK_RESLICE_WRAP;
    wrap = 1;
    }
  
  int perspective = 0;
  if (newmat[3][0] != 0 || newmat[3][1] != 0 ||
      newmat[3][2] != 0 || newmat[3][3] != 1)
    {
    perspective = 1;
    }

  int optimizeNearest = 0;
  if (self->GetInterpolationMode() == VTK_RESLICE_NEAREST &&
      !(wrap || newtrans || perspective))
    {
    optimizeNearest = 1;
    }

  // find maximum input range
  inData->GetExtent(inExt);

  for (i = 0; i < 3; i++)
    {
    inMin[i] = inExt[2*i];
    inMax[i] = inExt[2*i+1];
    }
  
  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  scalarSize = outData->GetScalarSize();
  numscalars = inData->GetNumberOfScalarComponents();
  
  // break matrix into a set of axes plus an origin
  // (this allows us to calculate the transform Incrementally)
  for (i = 0; i < 4; i++)
    {
    xAxis[i] = newmat[i][0];
    yAxis[i] = newmat[i][1];
    zAxis[i] = newmat[i][2];
    origin[i] = newmat[i][3];
    }

  // get the input origin and spacing for conversion purposes
  inData->GetOrigin(temp);
  inOrigin[0] = F(temp[0]);
  inOrigin[1] = F(temp[1]);
  inOrigin[2] = F(temp[2]);

  inData->GetSpacing(temp);
  inInvSpacing[0] = F(1.0f/temp[0]);
  inInvSpacing[1] = F(1.0f/temp[1]);
  inInvSpacing[2] = F(1.0f/temp[2]);

  // set color for area outside of input volume extent
  vtkAllocBackgroundPixel(self, &background, numscalars);

  // Set interpolation method
  vtkGetResliceInterpFunc(self, &interpolate);
  vtkGetSetPixelsFunc(self, &setpixels);

  // Loop through output pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    inPoint0[0] = origin[0] + idZ*zAxis[0]; // incremental transform
    inPoint0[1] = origin[1] + idZ*zAxis[1]; 
    inPoint0[2] = origin[2] + idZ*zAxis[2]; 
    inPoint0[3] = origin[3] + idZ*zAxis[3]; 
    
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      inPoint1[0] = inPoint0[0] + idY*yAxis[0]; // incremental transform
      inPoint1[1] = inPoint0[1] + idY*yAxis[1];
      inPoint1[2] = inPoint0[2] + idY*yAxis[2];
      inPoint1[3] = inPoint0[3] + idY*yAxis[3];
      
      if (!id) 
        {
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      
      if (newtrans == 0 && !wrap)
        {  // find intersections of x raster line with the input extent
        vtkResliceFindExtent(r1, r2, inPoint1, xAxis, inMin, inMax, outExt);
        }
      else
        { // there is an AbstractTransform, go through whole extent
        r1 = outExt[0];
        r2 = outExt[1];
        }

      // clear pixels to left of input extent
      setpixels(outPtr, background, numscalars, r1 - outExt[0]);
        
      iter = 0;
      while (vtkResliceGetNextExtent(self->GetStencil(), idXmin, idXmax,
                                     r1, r2, idY, idZ,
                                     outPtr, background, numscalars, 
                                     setpixels, iter))
        {
        if (!optimizeNearest)
          {
          for (idX = idXmin; idX <= idXmax; idX++)
            {
            inPoint[0] = inPoint1[0] + idX*xAxis[0];
            inPoint[1] = inPoint1[1] + idX*xAxis[1];
            inPoint[2] = inPoint1[2] + idX*xAxis[2];
            if (perspective)
              { // only do perspective if necessary
              inPoint[3] = inPoint1[3] + idX*xAxis[3];
              f = 1/inPoint[3];
              inPoint[0] *= f;
              inPoint[1] *= f;
              inPoint[2] *= f;
              }
            if (newtrans)
              { // apply the AbstractTransform if there is one
              vtkResliceApplyTransform(newtrans, inPoint, inOrigin,
                                       inInvSpacing);
              }
            // call the interpolation function
            interpolate(outPtr, inPtr, inExt, inInc, numscalars,
                        inPoint, mode, background);
            }
          }
        else // optimize for nearest-neighbor interpolation
          {
          for (int idX = r1; idX <= r2; idX++)
            {
            inPoint[0] = inPoint1[0] + idX*xAxis[0];
            inPoint[1] = inPoint1[1] + idX*xAxis[1];
            inPoint[2] = inPoint1[2] + idX*xAxis[2];

            int inIdX = vtkResliceRound(inPoint[0]) - inExt[0];
            int inIdY = vtkResliceRound(inPoint[1]) - inExt[2];
            int inIdZ = vtkResliceRound(inPoint[2]) - inExt[4];
    
            void *inPtr1 = (void *)((char *)inPtr + \
                                    (inIdX*inInc[0] + 
                                     inIdY*inInc[1] +
                                     inIdZ*inInc[2])*scalarSize);

            setpixels(outPtr, inPtr1, numscalars, 1);
            }
          }
        }
      // clear pixels to right of input extent
      setpixels(outPtr, background, numscalars, outExt[1] - r2);

      outPtr = (void *)((char *)outPtr + outIncY*scalarSize);
      }
    outPtr = (void *)((char *)outPtr + outIncZ*scalarSize);
    }
  
  vtkFreeBackgroundPixel(self, &background);
}

//----------------------------------------------------------------------------
// vtkReslicePermuteExecute is specifically optimized for
// cases where the IndexMatrix has only one non-zero component
// per row, i.e. when the matrix is permutation+scale+translation.
// All of the interpolation coefficients are calculated ahead
// of time instead of on a pixel-by-pixel basis.

//----------------------------------------------------------------------------
// helper function for nearest neighbor interpolation
template<class F, class T>
static 
void vtkPermuteNearestSummation(T *&outPtr, const T *inPtr,
                                int numscalars, int n,
                                const int *iX, const F *,
                                const int *iY, const F *,
                                const int *iZ, const F *,
                                const int [3])
{
  const T *inPtr0 = inPtr + iY[0] + iZ[0];

  for (int i = 0; i < n; i++)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    int m = numscalars;
    do
      {
      *outPtr++ = *tmpPtr++;
      }
    while (--m);
    }
}

// ditto, but optimized for numscalars = 1
template<class F, class T>
static 
void vtkPermuteNearestSummation1(T *&outPtr, const T *inPtr,
                                 int, int n,
                                 const int *iX, const F *,
                                 const int *iY, const F *,
                                 const int *iZ, const F *,
                                 const int [3])
{
  const T *inPtr0 = inPtr + iY[0] + iZ[0];

  for (int i = 0; i < n; i++)
    {
    *outPtr++ = inPtr0[iX[0]];
    iX++;
    }
}

//----------------------------------------------------------------------------
// helper function for linear interpolation
template<class F, class T>
static 
void vtkPermuteTrilinearSummation(T *&outPtr, const T *inPtr,
                                  int numscalars, int n,
                                  const int *iX, const F *fX,
                                  const int *iY, const F *fY,
                                  const int *iZ, const F *fZ,
                                  const int useNearestNeighbor[3])
{
  int i00 = iY[0] + iZ[0];
  int i01 = iY[0] + iZ[1];
  int i10 = iY[1] + iZ[0];
  int i11 = iY[1] + iZ[1];

  F ry = fY[0];
  F fy = fY[1];
  F rz = fZ[0];
  F fz = fZ[1];

  F ryrz = ry*rz;
  F ryfz = ry*fz;
  F fyrz = fy*rz;
  F fyfz = fy*fz;

  if (useNearestNeighbor[0] && fy == 0 && fz == 0)
    { // no interpolation needed at all
    for (int i = 0; i < n; i++)
      {
      int t0 = iX[0];
      iX += 2;

      const T *inPtr0 = inPtr + i00 + t0;
      int m = numscalars;
      do 
        {
        *outPtr++ = *inPtr0++;
        }
      while (--m);
      }
    }
  else if (useNearestNeighbor[0] && fy == 0)
    { // only need linear z interpolation
    for (int i = 0; i < n; i++)
      {
      int t0 = iX[0];
      iX += 2;
          
      const T *inPtr0 = inPtr + t0; 
      int m = numscalars;
      do 
        {
        F result = (rz*inPtr0[i00] + fz*inPtr0[i01]);
        vtkResliceRound(result, *outPtr++);
        inPtr0++;
        }
      while (--m);
      }
    }
  else if (fz == 0)
    { // bilinear interpolation in x,y
    for (int i = 0; i < n; i++)
      {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;

      int t0 = iX[0];
      int t1 = iX[1];
      iX += 2;

      const T *inPtr0 = inPtr + t0;
      const T *inPtr1 = inPtr + t1;
      int m = numscalars;
      do 
        {
        F result = (rx*(ry*inPtr0[i00] + fy*inPtr0[i10]) +
                    fx*(ry*inPtr1[i00] + fy*inPtr1[i10]));

        vtkResliceRound(result, *outPtr++);
        inPtr0++;
        inPtr1++;
        }
      while (--m);
      }
    }
  else
    { // do full trilinear interpolation 
    for (int i = 0; i < n; i++)
      {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;
       
      int t0 = iX[0];
      int t1 = iX[1];
      iX += 2;

      const T *inPtr0 = inPtr + t0;
      const T *inPtr1 = inPtr + t1;
      int m = numscalars;
      do 
        {
        F result = (rx*(ryrz*inPtr0[i00] + ryfz*inPtr0[i01] +
                        fyrz*inPtr0[i10] + fyfz*inPtr0[i11]) +
                    fx*(ryrz*inPtr1[i00] + ryfz*inPtr1[i01] +
                        fyrz*inPtr1[i10] + fyfz*inPtr1[i11]));

        vtkResliceRound(result, *outPtr++);
        inPtr0++;
        inPtr1++;
        }
      while (--m);
      }
    }
}

//--------------------------------------------------------------------------
// helper function for tricubic interpolation
template<class F, class T>
static
void vtkPermuteTricubicSummation(T *&outPtr, const T *inPtr,
                                 int numscalars, int n,
                                 const int *iX, const F *fX,
                                 const int *iY, const F *fY,
                                 const int *iZ, const F *fZ,
                                 const int useNearestNeighbor[3])
{
  // speed things up a bit for bicubic interpolation
  int k1 = 0;
  int k2 = 3;
  if (useNearestNeighbor[2])
    {
    k1 = k2 = 1;
    }

  for (int i = 0; i < n; i++)
    {
    int iX0 = iX[0];
    int iX1 = iX[1];
    int iX2 = iX[2];
    int iX3 = iX[3];
    iX += 4;

    F fX0 = fX[0];
    F fX1 = fX[1];
    F fX2 = fX[2];
    F fX3 = fX[3];
    fX += 4;

    const T *inPtr0 = inPtr;
    int c = numscalars;
    do
      { // loop over components
      F result = 0;

      int k = k1;
      do
        { // loop over z
        F fz = fZ[k];
        if (fz != 0)
          {
          int iz = iZ[k];
	  int j = 0;
	  do
            { // loop over y
            F fy = fY[j];
            F fzy = fz*fy;
            int izy = iz + iY[j];
            const T *tmpPtr = inPtr0 + izy;
            // loop over x is unrolled (significant performance boost)
            result += fzy*(fX0*tmpPtr[iX0] +
                           fX1*tmpPtr[iX1] +
                           fX2*tmpPtr[iX2] +
                           fX3*tmpPtr[iX3]);
            }
	  while (++j <= 3);
          }
        }
      while (++k <= k2);

      vtkResliceClamp(result, *outPtr++);
      inPtr0++;
      }
    while (--c);
    }
}

//----------------------------------------------------------------------------
// get approprate summation function for different interpolation modes
// and different scalar types
template<class F>
static
void vtkGetResliceSummationFunc(vtkImageReslice *self,
                                void (**summation)(void *&out, const void *in,
                                                   int numscalars, int n,
                                                   const int *iX, const F *fX,
                                                   const int *iY, const F *fY,
                                                   const int *iZ, const F *fZ,
                                                   const int useNearest[3]),
                                int interpolationMode)
{
  int scalarType = self->GetOutput()->GetScalarType();
  
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      switch (scalarType)
	{
	vtkTypeCaseMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
				      int numscalars, int n,
				      const int *iX, const F *fX,
				      const int *iY, const F *fY,
				      const int *iZ, const F *fZ,
				      const int useNearest[3]))summation) = \
			 vtkPermuteNearestSummation);
	default:
	  summation = 0;
	}
      break;
    case VTK_RESLICE_LINEAR:
      switch (scalarType)
	{
	vtkTypeCaseMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
				      int numscalars, int n,
				      const int *iX, const F *fX,
				      const int *iY, const F *fY,
				      const int *iZ, const F *fZ,
				      const int useNearest[3]))summation) = \
			 vtkPermuteTrilinearSummation);
	default:
	  summation = 0;
	}
      break;
    case VTK_RESLICE_CUBIC:
      switch (scalarType)
	{
	vtkTypeCaseMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
				      int numscalars, int n,
				      const int *iX, const F *fX,
				      const int *iY, const F *fY,
				      const int *iZ, const F *fZ,
				      const int useNearest[3]))summation) = \
			 vtkPermuteTricubicSummation);
	default:
	  summation = 0;
	}
      break;
    default:
      summation = 0;
    }
}

//----------------------------------------------------------------------------
template <class F>
static 
void vtkPermuteNearestTable(vtkImageReslice *self, const int outExt[6],
                            const int inExt[6], const int inInc[3],
                            int clipExt[6], int **traversal, F **constants,
                            int useNearestNeighbor[3], F newmat[4][4])
{
  // set up input traversal table for nearest-neighbor interpolation  
  for (int j = 0; j < 3; j++)
    {
    int k;
    for (k = 0; k < 3; k++)
      { // set k to the element which is nonzero
      if (newmat[k][j] != 0)
        {
        break;
        }
      }

    // this is just for symmetry with Linear and Cubic
    useNearestNeighbor[j] = 1;

    int inExtK = inExt[2*k+1] - inExt[2*k] + 1;

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      int inId = vtkResliceRound((newmat[k][3] + i*newmat[k][j]));
      inId -= inExt[2*k];
      if (self->GetMirror())
        {
        inId = vtkInterpolateMirror(inId, inExtK);
        region = 1;
        }
      else if (self->GetWrap())
        {
        inId = vtkInterpolateWrap(inId, inExtK);
        region = 1;
        }
      else
        {
        if (inId < 0 || inId >= inExtK)
          {
            if (region == 1)
            { // leaving the input extent
            region = 2;
            clipExt[2*j+1] = i - 1;
            }
           }
        else 
          {
          if (region == 0)
            { // entering the input extent
            region = 1;
            clipExt[2*j] = i;
            }
          }
        }
      traversal[j][i] = inId*inInc[k];
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }
}
  
//----------------------------------------------------------------------------
template <class F>
static 
void vtkPermuteLinearTable(vtkImageReslice *self, const int outExt[6],
                           const int inExt[6], const int inInc[3],
                           int clipExt[6], int **traversal, F **constants,
                           int useNearestNeighbor[3], F newmat[4][4])
{
  // set up input traversal table for linear interpolation  
  for (int j = 0; j < 3; j++)
    {
    int k;
    for (k = 0; k < 3; k++)
      { // set k to the element which is nonzero
      if (newmat[k][j] != 0)
        {
        break;
        }
      }

    // do the output pixels lie exactly on top of the input pixels?
    useNearestNeighbor[j] = (newmat[k][j] == vtkResliceFloor(newmat[k][j]) &&
                             newmat[k][3] == vtkResliceFloor(newmat[k][3]));
    
    int inExtK = inExt[2*k+1] - inExt[2*k] + 1;

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];
      F f;
      int trunc = vtkResliceFloor(point,f);
      constants[j][2*i] = 1 - f;
      constants[j][2*i+1] = f;
      
      int fIsNotZero = (f != 0);
      int inId0 = trunc - inExt[2*k];
      int inId1 = inId0 + fIsNotZero;

      if (self->GetMirror())
        {
        inId0 = vtkInterpolateMirror(inId0, inExtK);
        inId1 = vtkInterpolateMirror(inId1, inExtK);
        region = 1;
        }
      else if (self->GetWrap())
        {
        inId0 = vtkInterpolateWrap(inId0, inExtK);
        inId1 = vtkInterpolateWrap(inId1, inExtK);
        region = 1;
        }
      else if (inId0 < 0 || inId1 >= inExtK)
        {
        if (region == 1)
          { // leaving the input extent
          region = 2;
          clipExt[2*j+1] = i - 1;
          }
        }
      else 
        {
        if (region == 0)
          { // entering the input extent
          region = 1;
          clipExt[2*j] = i;           
          }
        }
      traversal[j][2*i] = inId0*inInc[k];
      traversal[j][2*i+1] = inId1*inInc[k];
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }
}

//----------------------------------------------------------------------------
template <class F>
static 
void vtkPermuteCubicTable(vtkImageReslice *self, const int outExt[6],
                          const int inExt[6], const int inInc[3],
                          int clipExt[6], int **traversal, F **constants,
                          int useNearestNeighbor[3], F newmat[4][4])
{
  // set up input traversal table for cubic interpolation  
  for (int j = 0; j < 3; j++)
    {
    int k;
    for (k = 0; k < 3; k++)
      { // set k to the element which is nonzero
      if (newmat[k][j] != 0)
        {
        break;
        }
      }

    // do the output pixels lie exactly on top of the input pixels?
    useNearestNeighbor[j] = (newmat[k][j] == vtkResliceFloor(newmat[k][j]) &&
                             newmat[k][3] == vtkResliceFloor(newmat[k][3]));

    int inExtK = inExt[2*k+1] - inExt[2*k] + 1;

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];
      F f;
      int trunc = vtkResliceFloor(point, f);
      int fIsNotZero = (f != 0);
      int inId[4];
      inId[1] = trunc - inExt[2*k];
      inId[0] = inId[1] - 1;
      inId[2] = inId[1] + 1;
      inId[3] = inId[1] + 2;

      int low = 1 - fIsNotZero;
      int high = 1 + 2*fIsNotZero;
      if (self->GetMirror())
        {
        inId[0] = vtkInterpolateMirror(inId[0], inExtK);
        inId[1] = vtkInterpolateMirror(inId[1], inExtK);
        inId[2] = vtkInterpolateMirror(inId[2], inExtK);
        inId[3] = vtkInterpolateMirror(inId[3], inExtK);
        region = 1;
        }
      else if (self->GetWrap())
        {
        inId[0] = vtkInterpolateWrap(inId[0], inExtK);
        inId[1] = vtkInterpolateWrap(inId[1], inExtK);
        inId[2] = vtkInterpolateWrap(inId[2], inExtK);
        inId[3] = vtkInterpolateWrap(inId[3], inExtK);
        region = 1;
        }      
      else
        {
        if (inId[1] < 0 || inId[1] + fIsNotZero >= inExtK)
          {
          if (region == 1)
            { // leaving the input extent
            region = 2;
            clipExt[2*j+1] = i - 1;
            }
          }
        else 
          {
          if (region == 0)
            { // entering the input extent
            region = 1;
            clipExt[2*j] = i;           
            }
          }
        low  = 1 - (inId[0] >= 0)*fIsNotZero;
        high = 1 + (1 + (inId[3] < inExtK))*fIsNotZero;
        }
      vtkTricubicInterpCoeffs(&constants[j][4*i], low, high, f);

      // set default values
      int l;
      for (l = 0; l < 4; l++)
        {
        traversal[j][4*i+l] = inId[1]*inInc[k];
        }
      for (l = low; l <= high; l++)
        { 
        traversal[j][4*i+l] = inId[l]*inInc[k];
        }
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }
}

//----------------------------------------------------------------------------
// Check to see if we can do nearest-neighbor instead of linear or cubic.  
// This check only works on permutation+scale+translation matrices.
template <class F>
static inline int vtkCanUseNearestNeighbor(F matrix[4][4], int outExt[6])
{
  // loop through dimensions
  for (int i = 0; i < 3; i++)
    {
    int j;
    for (j = 0; j < 3; j++)
      {
      if (matrix[i][j] != 0)
        {
        break;
        }
      }
    F x = matrix[i][j];
    F y = matrix[i][3];
    if (outExt[2*j] == outExt[2*j+1])
      {
      y += x*outExt[2*i];
      x = 0;
      }
    if (vtkResliceFloor(x) != x || vtkResliceFloor(y) != y)
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// the ReslicePermuteExecute path is taken when the output slices are
// orthogonal to the input slices
template <class F>
static void vtkReslicePermuteExecute(vtkImageReslice *self,
                                     vtkImageData *inData, void *inPtr,
                                     vtkImageData *outData, void *outPtr,
                                     int outExt[6], int id, F newmat[4][4])
{
  int outInc[3], scalarSize, numscalars;
  int inExt[6], inInc[3], clipExt[6];
  int *traversal[3];
  F *constants[3];
  int useNearestNeighbor[3];
  int i;
  
  // find maximum input range
  self->GetInput()->GetExtent(inExt);

  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outInc[0], outInc[1], outInc[2]);
  scalarSize = outData->GetScalarSize();
  numscalars = inData->GetNumberOfScalarComponents();

  for (i = 0; i < 3; i++)
    {
    clipExt[2*i] = outExt[2*i];
    clipExt[2*i+1] = outExt[2*i+1];
    }

  int interpolationMode = self->GetInterpolationMode();
  if (vtkCanUseNearestNeighbor(newmat, outExt))
    {
    interpolationMode = VTK_RESLICE_NEAREST;
    }

  // the step size is the number of coefficients per dimension
  int step = 1;
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      step = 1;
      break;
    case VTK_RESLICE_LINEAR:
      step = 2;
      break;
    case VTK_RESLICE_CUBIC:
      step = 4;
      break;
    }

  // allocate the interpolation tables
  for (i = 0; i < 3; i++)
    {
    int outExtI = outExt[2*i+1] - outExt[2*i] + 1; 

    traversal[i] = new int[outExtI*step];
    traversal[i] -= step*outExt[2*i];
    constants[i] = new F[outExtI*step];
    constants[i] -= step*outExt[2*i];
    }

  // fill in the interpolation tables
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      vtkPermuteNearestTable(self, outExt, inExt, inInc, clipExt,
                             traversal, constants, 
                             useNearestNeighbor, newmat);
      break;
    case VTK_RESLICE_LINEAR:
      vtkPermuteLinearTable(self, outExt, inExt, inInc, clipExt,
                            traversal, constants, 
                            useNearestNeighbor, newmat);
      break;
    case VTK_RESLICE_CUBIC:
      vtkPermuteCubicTable(self, outExt, inExt, inInc, clipExt,
                           traversal, constants, 
                           useNearestNeighbor, newmat);
      break;
    }

  // get type-specific functions
  void (*summation)(void *&out, const void *in, int numscalars, int n,
                    const int *iX, const F *fX,
                    const int *iY, const F *fY,
                    const int *iZ, const F *fZ,
                    const int useNearestNeighbor[3]);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);
  vtkGetResliceSummationFunc(self, &summation, interpolationMode);
  vtkGetSetPixelsFunc(self, &setpixels);

  // set color for area outside of input volume extent
  void *background;
  vtkAllocBackgroundPixel(self, &background, numscalars);

  // for tracking progress
  unsigned long count = 0;
  unsigned long target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Loop through output pixels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    int idZ0 = idZ*step;

    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      int idY0 = idY*step;

      if (id == 0) 
        { // track progress if we are main thread
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }

      // do extent check
      if (idZ < clipExt[4] || idZ > clipExt[5] ||
          idY < clipExt[2] || idY > clipExt[3])
        { // just clear, we're completely outside
        setpixels(outPtr, background, numscalars, outExt[1] - outExt[0] + 1);
        }
      else
        {
        // clear pixels to left of input extent
        setpixels(outPtr, background, numscalars, clipExt[0] - outExt[0]);

        int iter = 0;
        int idXmin, idXmax;
        while (vtkResliceGetNextExtent(self->GetStencil(), idXmin, idXmax,
                                       clipExt[0], clipExt[1], idY, idZ,
                                       outPtr, background, numscalars,
                                       setpixels, iter))
          {
          int idX0 = idXmin*step;

          summation(outPtr, inPtr, numscalars, idXmax - idXmin + 1,
                    &traversal[0][idX0], &constants[0][idX0],
                    &traversal[1][idY0], &constants[1][idY0],
                    &traversal[2][idZ0], &constants[2][idZ0],
                    useNearestNeighbor);
          }

        // clear pixels to right of input extent
        setpixels(outPtr, background, numscalars, outExt[1] - clipExt[1]);
        }

      outPtr = (void *)((char *)outPtr + outInc[1]*scalarSize);
      }
    outPtr = (void *)((char *)outPtr + outInc[2]*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);

  for (i = 0; i < 3; i++)
    {
    traversal[i] += step*outExt[2*i];
    constants[i] += step*outExt[2*i];
    delete [] traversal[i];
    delete [] constants[i];
    }
}

//----------------------------------------------------------------------------
// check a matrix to ensure that it is a permutation+scale+translation
// matrix

template <class F>
static int vtkIsPermutationMatrix(F matrix[4][4])
{
  for (int i = 0; i < 3; i++)
    {
    if (matrix[3][i] != 0)
      {
      return 0;
      }
    }
  if (matrix[3][3] != 1)
    {
    return 0;
    }
  for (int j = 0; j < 3; j++)
    {
    int k = 0;
    for (int i = 0; i < 3; i++)
      {
      if (matrix[i][j] != 0)
        {
        k++;
        }
      }
    if (k != 1)
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// check a matrix to see whether it is the identity matrix

static int vtkIsIdentityMatrix(vtkMatrix4x4 *matrix)
{
  static double identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  int i,j;

  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      if (matrix->GetElement(i,j) != identity[4*i+j])
        {
        return 0;
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// The transform matrix supplied by the user converts output coordinates
// to input coordinates.  
// To speed up the pixel lookup, the following function provides a
// matrix which converts output pixel indices to input pixel indices.
//
// This will also concatenate the ResliceAxes and the ResliceTransform
// if possible (if the ResliceTransform is a 4x4 matrix transform).
// If it does, this->OptimizedTransform will be set to NULL, otherwise
// this->OptimizedTransform will be equal to this->ResliceTransform.

vtkMatrix4x4 *vtkImageReslice::GetIndexMatrix()
{
  // first verify that we have to update the matrix
  if (this->IndexMatrix == NULL)
    {
    this->IndexMatrix = vtkMatrix4x4::New();
    }

  int isIdentity = 0;
  float inOrigin[3];
  float inSpacing[3];
  float outOrigin[3];
  float outSpacing[3];

  this->GetInput()->GetSpacing(inSpacing);
  this->GetInput()->GetOrigin(inOrigin);
  this->GetOutput()->GetSpacing(outSpacing);
  this->GetOutput()->GetOrigin(outOrigin);  
  
  vtkTransform *transform = vtkTransform::New();
  vtkMatrix4x4 *inMatrix = vtkMatrix4x4::New();
  vtkMatrix4x4 *outMatrix = vtkMatrix4x4::New();

  if (this->OptimizedTransform)
    {
    this->OptimizedTransform->Delete();
    }
  this->OptimizedTransform = NULL;

  if (this->ResliceAxes)
    {
    transform->SetMatrix(this->GetResliceAxes());
    }
  if (this->ResliceTransform)
    {
    if (this->ResliceTransform->IsA("vtkHomogeneousTransform"))
      {
      transform->PostMultiply();
      transform->Concatenate(((vtkHomogeneousTransform *)
                              this->ResliceTransform)->GetMatrix());
      }
    else
      {
      this->ResliceTransform->Register(this);
      this->OptimizedTransform = this->ResliceTransform;
      }
    }
  
  // check to see if we have an identity matrix
  isIdentity = vtkIsIdentityMatrix(transform->GetMatrix());

  // the outMatrix takes OutputData indices to OutputData coordinates,
  // the inMatrix takes InputData coordinates to InputData indices
  for (int i = 0; i < 3; i++) 
    {
    if ((this->OptimizedTransform == NULL && 
         (inSpacing[i] != outSpacing[i] || inOrigin[i] != outOrigin[i])) ||
        (this->OptimizedTransform != NULL &&
         (inSpacing[i] != 1.0f || inOrigin[i] != 0.0f))) 
      {
      isIdentity = 0;
      }
    inMatrix->Element[i][i] = 1.0f/inSpacing[i];
    inMatrix->Element[i][3] = -inOrigin[i]/inSpacing[i];
    outMatrix->Element[i][i] = outSpacing[i];
    outMatrix->Element[i][3] = outOrigin[i];
    }
  this->GetOutput()->GetOrigin(outOrigin); 

  if (!isIdentity)
    {
    transform->PreMultiply();
    transform->Concatenate(outMatrix);
    if (this->OptimizedTransform == NULL)
      {
      transform->PostMultiply();
      transform->Concatenate(inMatrix);
      }
    }

  transform->GetMatrix(this->IndexMatrix);
  
  transform->Delete();
  inMatrix->Delete();
  outMatrix->Delete();

  return this->IndexMatrix;
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageReslice::OptimizedThreadedExecute(vtkImageData *inData, 
                                               vtkImageData *outData,
                                               int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(inData->GetExtent());
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
                      << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }

  // change transform matrix so that instead of taking 
  // input coords -> output coords it takes output indices -> input indices
  vtkMatrix4x4 *matrix = this->IndexMatrix;

  // get the portion of the transformation that remains apart from
  // the IndexMatrix
  vtkAbstractTransform *newtrans = this->OptimizedTransform;

  float newmat[4][4];
  for (int i = 0; i < 4; i++)
    {
    newmat[i][0] = matrix->GetElement(i,0);
    newmat[i][1] = matrix->GetElement(i,1);
    newmat[i][2] = matrix->GetElement(i,2);
    newmat[i][3] = matrix->GetElement(i,3);
    }
  
  if (vtkIsPermutationMatrix(newmat) && newtrans == NULL)
    {
    vtkReslicePermuteExecute(this, inData, inPtr, outData, outPtr,
                             outExt, id, newmat);
    }
  else
    {
    vtkOptimizedExecute(this, inData, inPtr, outData, outPtr, outExt,
                        id, newmat, newtrans);
    }
}

