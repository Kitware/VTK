/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformStructuredPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkTransformStructuredPoints.h"

// Description:
// Construct object to use input dimensions as sample dimensions,
// and to compute bounds automatically from input. Fill value is set
// to large positive integer.
vtkTransformStructuredPoints::vtkTransformStructuredPoints()
{
  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->SampleDimensions[0] = 0; // use input dimensions by default
  this->SampleDimensions[1] = 0;
  this->SampleDimensions[2] = 0;

  this->FillValue = VTK_LARGE_FLOAT;

  this->Transform = NULL;
}

void vtkTransformStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToStructuredPointsFilter::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Fill Value:" << this->FillValue << "\n";

  os << indent << "Transform:" << this->Transform << "\n";
}

// Description:
// Define pre-transformed size of structured point set.
void vtkTransformStructuredPoints::SetModelBounds(float *bounds)
{
  vtkTransformStructuredPoints::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vtkTransformStructuredPoints::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  if (this->ModelBounds[0] != xmin || this->ModelBounds[1] != xmax ||
  this->ModelBounds[2] != ymin || this->ModelBounds[3] != ymax ||
  this->ModelBounds[4] != zmin || this->ModelBounds[5] != zmax )
    {
    this->Modified();
    this->ModelBounds[0] = xmin;
    this->ModelBounds[1] = xmax;
    this->ModelBounds[2] = ymin;
    this->ModelBounds[3] = ymax;
    this->ModelBounds[4] = zmin;
    this->ModelBounds[5] = zmax;
    }
}

void vtkTransformStructuredPoints::Execute()
{
  int i, numPts, numOutPts;
  int *dimIn, *dim;
  float *originIn, *aspectIn, ar[3];
  vtkPointData *pd;
  vtkScalars *inScalars, *outScalars;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;


  vtkDebugMacro(<<"Transforming points");

  // make sure there is input
  pd = this->Input->GetPointData();
  inScalars = pd->GetScalars();
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 ||
  inScalars == NULL )
    {
    vtkErrorMacro(<<"No data to transform!");
    return;
    }

  // Get origin, aspect ratio and dimensions from input
  dimIn = input->GetDimensions();
  originIn = input->GetOrigin();
  aspectIn = input->GetAspectRatio();

  // if dimensions are not specified, use input's dimensions
  if (this->SampleDimensions[0] <= 1 || this->SampleDimensions[1] <= 1 || 
  this->SampleDimensions[2] <= 1)
    {
    output->SetDimensions(dimIn);
    dim = dimIn;
    
    }
  // otherwise use the specified dimensions
  else 
    {
    output->SetDimensions(this->SampleDimensions);
    dim = this->SampleDimensions;
    
    }

  // if bounds are not specified, use input's aspect ratio and origin
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5])
    {
    output->SetAspectRatio(aspectIn);
    output->SetOrigin(originIn);
    }
  // otherwise, calculate them from bounds
  else 
    {
    output->SetOrigin(this->ModelBounds[0], this->ModelBounds[1], 
                    this->ModelBounds[2]);
    for (i=0; i<3; i++) 
      ar[i] = (this->ModelBounds[2*i+1]-this->ModelBounds[2*i]) /
              (dim[i] - 1);

    output->SetAspectRatio(ar);
    }

  // Allocate data.  Scalar type is same as input.
  numOutPts = dim[0] * dim[1] * dim[2];
  outScalars = inScalars->MakeObject(numOutPts);
  for (i = 0; i < numOutPts; i++) outScalars->SetScalar(i,this->FillValue);

  // Loop over all output voxels, transforming and then resampling from input.
  // Need to get inverse transformation matrix to perform transformation.






  // Update ourselves
  output->GetPointData()->SetScalars(outScalars);

  outScalars->Delete();
}


unsigned long int vtkTransformStructuredPoints::GetMTime()
{
  unsigned long int mtime = 0;

  if ( this->Transform ) mtime = this->Transform->GetMTime();
  if (mtime > this->MTime) return mtime;
  return this->MTime;
}


