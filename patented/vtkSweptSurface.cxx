/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSweptSurface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 5,542,036
    "Implicit Modeling of Swept Volumes and Swept Surfaces"
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Mike Silver
        GE Medical Systems
        16705 West Lincoln Ave., 
        NB 900
        New Berlin, WI, 53151
        Phone:1-414-827-3400 
    for more information.

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
#include "vtkSweptSurface.h"
#include "vtkActor.h"
#include "vtkVoxel.h"
#include "vtkMath.h"

// Description:
// Construct object with SampleDimensions = (50,50,50), FillValue = 
// VTK_LARGE_FLOAT, ModelBounds=(0,0,0,0,0,0) (i.e, bounds will be
// computed automatically), and Capping turned on.
vtkSweptSurface::vtkSweptSurface()
{
  this->ModelBounds[0] = 0.0; //compute automatically
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->NumberOfInterpolationSteps = 0;
  this->MaximumNumberOfInterpolationSteps = VTK_LARGE_INTEGER;
  this->FillValue = VTK_LARGE_FLOAT;
  this->Transforms = NULL;
  this->Capping = 1;
}

void vtkSweptSurface::SetModelBounds(float xmin, float xmax, float ymin, 
                                     float ymax, float zmin, float zmax)
{
  float bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->SetModelBounds(bounds);
}

void vtkSweptSurface::Execute()
{
  int i, numPts, numOutPts;
  vtkPointData *pd, *outPD;
  vtkScalars *inScalars, *newScalars;
  float inSpacing[3], inOrigin[3];
  int inDim[3];
  int numSteps, stepNum;
  int numTransforms, transNum;
  vtkActor a; //use the actor to do position/orientation stuff
  vtkTransform *transform1, *transform2, t;
  float time;
  float position[3], position1[3], position2[3];
  float orient[3], orient1[3], orient2[3];
  float origin[3], spacing[3], bbox[24];
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;

  vtkDebugMacro(<<"Creating swept surface");

  // make sure there is input
  pd = this->Input->GetPointData();
  outPD = output->GetPointData();
  
  inScalars = pd->GetScalars();
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 ||
  inScalars == NULL )
    {
    vtkErrorMacro(<<"No input scalars defined!");
    return;
    }

  // check that path is defined
  if ( this->Transforms == NULL )
    {
    vtkErrorMacro(<<"No path defined!");
    return;
    }

  if ( (numTransforms=this->Transforms->GetNumberOfItems()) < 2 )
    {
    vtkErrorMacro(<<"At least two transforms are required to define path!");
    return;
    }

  output->SetDimensions(this->SampleDimensions);
  this->ComputeBounds(origin, spacing, bbox);

  input->GetDimensions(inDim);
  input->GetSpacing(inSpacing);
  input->GetOrigin(inOrigin);
//
// Allocate data.  Scalar "type" is same as input.
//
  numOutPts = this->SampleDimensions[0] * this->SampleDimensions[1] * 
              this->SampleDimensions[2];
  newScalars = inScalars->MakeObject(numOutPts);
  newScalars->SetNumberOfScalars(numOutPts);
  for (i = 0; i < numOutPts; i++) newScalars->SetScalar(i,this->FillValue);
//
// Sample data at each point in path
//
  this->Transforms->InitTraversal();
  transform2 = this->Transforms->GetNextItem();
  transform2->GetInverse(t.GetMatrix());

  t.GetPosition(position2[0], position2[1], position2[2]);
  t.GetOrientation(orient2[0], orient2[1], orient2[2]);

  for (transNum=0; transNum < (numTransforms-1); transNum++)
    {
    transform1 = transform2;
    transform2 = this->Transforms->GetNextItem();
    transform2->GetInverse(t.GetMatrix());
//
// Loop over all points (i.e., voxels), transform into input coordinate system,
// and obtain interpolated value. Then perform union operation.  
//
    if ( this->NumberOfInterpolationSteps > 0 ) 
      numSteps = this->NumberOfInterpolationSteps;
    else if ( this->NumberOfInterpolationSteps < 0 ) 
      numSteps = 1;
    else 
      numSteps = this->ComputeNumberOfSteps(transform1,transform2,bbox);

    numSteps = (numSteps > this->MaximumNumberOfInterpolationSteps ?
                this->MaximumNumberOfInterpolationSteps : numSteps);

    for (i=0; i<3; i++)
      {
      position1[i] = position2[i];
      orient1[i] = orient2[i];
      }
    t.GetPosition(position2[0], position2[1], position2[2]);
    t.GetOrientation(orient2[0], orient2[1], orient2[2]);

    vtkDebugMacro(<<"Injecting " << numSteps << " steps between transforms "
                  << transNum <<" and "<< transNum+1);
    for (stepNum=0; stepNum < numSteps; stepNum++)
      {
      // linearly interpolate position and orientation
      time = (float) stepNum / numSteps;

      for (i=0; i<3; i++)
        {
        position[i] = position1[i] + time*(position2[i] - position1[i]);
        orient[i] = orient1[i] + time*(orient2[i] - orient1[i]);
        }

      a.SetPosition(position);
      a.SetOrientation(orient);
      this->SampleInput(a.vtkProp::GetMatrix(), inDim, inOrigin, inSpacing,
                        inScalars, newScalars);
      }
    }

  //finish off last step
  a.SetPosition(position2);
  a.SetOrientation(orient2);
  this->SampleInput(a.vtkProp::GetMatrix(), inDim, inOrigin, inSpacing,
                    inScalars, newScalars);

  // Update ourselves and release memory
  outPD->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkSweptSurface::SampleInput(vtkMatrix4x4& m, int inDim[3], 
                                 float inOrigin[3], float inSpacing[3], 
                                 vtkScalars *inScalars, vtkScalars *outScalars)
{
  int i, j, k, ii;
  int inSliceSize=inDim[0]*inDim[1];
  int sliceSize=this->SampleDimensions[0]*this->SampleDimensions[1];
  float x[4], loc[3], newScalar, scalar;
  static vtkVoxel voxel;
  static vtkIdList idList(8); idList.SetNumberOfIds(8);
  static vtkFloatScalars voxelScalars(8); voxelScalars.SetNumberOfScalars(8);
  int kOffset, jOffset, ijk[3], idx;
  float xTrans[4], weights[8];
  static vtkTransform t;
  float *origin, *spacing;

  origin = ((vtkStructuredPoints *)this->Output)->GetOrigin();
  spacing = ((vtkStructuredPoints *)this->Output)->GetSpacing();

  t.SetMatrix(m);
  x[3] = 1.0; //homogeneous coordinates

  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    kOffset = k*sliceSize;
    x[2] = origin[2] + k * spacing[2];
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      jOffset = j*this->SampleDimensions[0];
      x[1] = origin[1] + j * spacing[1];
      for (i=0; i<this->SampleDimensions[0]; i++)
        {
        x[0] = origin[0] + i * spacing[0];

        // transform into local space
        t.MultiplyPoint(x,xTrans);
        if ( xTrans[3] != 0.0 ) for (ii=0; ii<3; ii++) xTrans[ii] /= xTrans[3];

        // determine which voxel point falls in.
        for (ii=0; ii<3; ii++)
          {
          loc[ii] = (xTrans[ii]-inOrigin[ii]) / inSpacing[ii];
          ijk[ii] = (int)loc[ii];
          }

        //check and make sure point is inside
        if ( loc[0] >= 0.0 && loc[1] >= 0.0 && loc[2] >= 0.0 &&
	     (ijk[0] < inDim[0] - 1) && 
	     (ijk[1] < inDim[1] - 1) && 
	     (ijk[2] < inDim[2] - 1))
          {
          //get scalar values
          idx = ijk[0] + ijk[1]*inDim[0] + ijk[2]*inSliceSize;
          idList.SetId(0,idx);
          idList.SetId(1,idx+1);
          idList.SetId(2,idx + inDim[0]);
          idList.SetId(3,idx+1 + inDim[0]);
          idList.SetId(4,idx + inSliceSize);
          idList.SetId(5,idx+1 + inSliceSize);
          idList.SetId(6,idx + inDim[0] + inSliceSize);
          idList.SetId(7,idx+1 + inDim[0] + inSliceSize);

          inScalars->GetScalars(idList,voxelScalars);

          for (ii=0; ii<3; ii++) loc[ii] = loc[ii] - ijk[ii];

          voxel.InterpolationFunctions(loc,weights);

          for (newScalar=0.0, ii=0; ii<8; ii++) 
            newScalar += voxelScalars.GetScalar(ii) * weights[ii];

          scalar = outScalars->GetScalar((idx=i+jOffset+kOffset));
          if ( newScalar < scalar )  //union operation
            outScalars->SetScalar(idx,newScalar);
          }
        }
      }
    }
}

unsigned long int vtkSweptSurface::GetMTime()
{
  unsigned long mtime=vtkStructuredPointsFilter::GetMTime();
  unsigned long int transMtime;
  vtkTransform *t;

  for (this->Transforms->InitTraversal(); 
       (t = this->Transforms->GetNextItem()); )
    {
    transMtime = t->GetMTime();
    if ( transMtime > mtime ) mtime = transMtime;
    }

  return mtime;
}


// compute model bounds from geometry and path
void vtkSweptSurface::ComputeBounds(float origin[3], float spacing[3], float bbox[24])
{
  int i, j, k, ii, idx, dim;
  float *bounds;

  // Compute eight points of bounding box (used later)
  bounds = this->Input->GetBounds();

  for (idx=0, k=4; k<6; k++) 
    {
    for (j=2; j<4; j++) 
      {
      for (i=0; i<2; i++) 
        {
        bbox[idx++] = bounds[i];
        bbox[idx++] = bounds[j];
        bbox[idx++] = bounds[k];
        }
      }
    }

  // if bounds are not specified, compute bounds from path
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5])
    {
    int numTransforms, transNum;
    float xmin[3], xmax[3], x[4], xTrans[4], h;
    float position[3], orient[3], position1[3], orient1[3];
    float position2[3], orient2[3];
    vtkActor a;
    vtkTransform t, t2, *transform1, *transform2;

    xmin[0] = xmin[1] = xmin[2] = VTK_LARGE_FLOAT;
    xmax[0] = xmax[1] = xmax[2] = -VTK_LARGE_FLOAT;
        
    numTransforms = this->Transforms->GetNumberOfItems();

    this->Transforms->InitTraversal();
    transform2 = this->Transforms->GetNextItem();
    transform2->GetMatrix(t.GetMatrix());
    t.GetPosition(position2[0], position2[1], position2[2]);
    t.GetOrientation(orient2[0], orient2[1], orient2[2]);

    // Initialize process with initial transformed position of input
    x[3] = 1.0;
    for (i=0; i<8; i++)
      {
      x[0] = bbox[i*3]; x[1] = bbox[i*3+1]; x[2] = bbox[i*3+2]; 
      t.MultiplyPoint(x,xTrans);
      if ( xTrans[3] != 0.0 ) for (ii=0; ii<3; ii++) xTrans[ii] /= xTrans[3];
      for (j=0; j<3; j++)
        {
        if (xTrans[j] < xmin[j]) xmin[j] = xTrans[j];
        if (xTrans[j] > xmax[j]) xmax[j] = xTrans[j];
        }
      }

    for (transNum=0; transNum < (numTransforms-1); transNum++)
      {
      transform1 = transform2;
      transform2 = this->Transforms->GetNextItem();
      transform2->GetMatrix(t.GetMatrix());
      for (i=0; i<3; i++)
        {
        position1[i] = position2[i];
        orient1[i] = orient2[i];
        }
      t.GetPosition(position2[0], position2[1], position2[2]);
      t.GetOrientation(orient2[0], orient2[1], orient2[2]);

      // Sample inbetween matrices to compute better bounds. 
      // Use 4 steps (arbitrary),
      h = 0.25;
      for (k=1; k <= 4; k++ ) 
        {
        for (i=0; i<3; i++) //linear interpolation
          {
          position[i] = position1[i] + k*h*(position2[i] - position1[i]);
          orient[i] = orient1[i] + k*h*(orient2[i] - orient1[i]);
          }

        a.SetPosition(position);
        a.SetOrientation(orient);
        a.GetMatrix(t2.GetMatrix());

        for (i=0; i<8; i++) //loop over eight corners of bounding box
          {
          x[0] = bbox[i*3]; x[1] = bbox[i*3+1]; x[2] = bbox[i*3+2]; 
          t2.MultiplyPoint(x,xTrans);
          if ( xTrans[3] != 0.0 ) 
            for (ii=0; ii<3; ii++) xTrans[ii] /= xTrans[3];
          for (j=0; j<3; j++)
            {
            if (xTrans[j] < xmin[j]) xmin[j] = xTrans[j];
            if (xTrans[j] > xmax[j]) xmax[j] = xTrans[j];
            }
          }
        }
      }
    // adjust bounds larger (2.5%) to make sure data lies within volume
    for (i=0; i<3; i++)
      {
      spacing[i] = (xmax[i]-xmin[i]);
      h = 0.0125 * spacing[i];
      xmin[i] -= h;
      xmax[i] += h;
      origin[i] = xmin[i];
      if ( (dim=this->SampleDimensions[i]) <= 1 ) dim=2;
      if ( (spacing[i]=spacing[i]/(dim-1)) == 0.0 ) spacing[i] = 1.0;
      }
    vtkDebugMacro(<<"Computed model bounds as (" << xmin[0] << "," << xmax[0]
                  << ", " << xmin[1] << "," << xmax[1]
                  << ", " << xmin[2] << "," << xmax[2] << ")");
    } //if compute model bounds

  else // else use model bounds specified
    {
    for (i=0; i<3; i++)
      {
      origin[i] = this->ModelBounds[2*i];
      if ( (dim=this->SampleDimensions[i]) <= 1 ) dim = 2;
      spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
              / (dim - 1);
      if ( spacing[i] == 0.0 ) spacing[i] = 1.0;
      }
    }

  // set output
  ((vtkStructuredPoints *)(this->Output))->SetOrigin(origin);
  ((vtkStructuredPoints *)(this->Output))->SetSpacing(spacing);
}

// based on both path and bounding box of input, compute the number of 
// steps between the specified transforms
int vtkSweptSurface::ComputeNumberOfSteps(vtkTransform *t1, vtkTransform *t2, 
                                          float bbox[24])
{
  float x[4], xTrans1[4], xTrans2[4];
  float dist2, maxDist2;
  float h, *spacing;
  int numSteps, i, j;
  
  // Compute maximum distance between points.
  x[3] = 1.0;
  for (maxDist2=0.0, i=0; i<8; i++)
    {
    for (j=0; j<3; j++) x[j] = bbox[3*i+j];
    t1->MultiplyPoint(x,xTrans1);
    if ( xTrans1[3] != 0.0 ) for (j=0; j<3; j++) xTrans1[j] /= xTrans1[3];

    t2->MultiplyPoint(xTrans1,xTrans2);
    if ( xTrans2[3] != 0.0 ) for (j=0; j<3; j++) xTrans2[j] /= xTrans2[3];

    dist2 = vtkMath::Distance2BetweenPoints((float *)xTrans1,(float *)xTrans2);
    if ( dist2 > maxDist2 ) maxDist2 = dist2;
    }

  // use magic factor to convert to nuumber of steps. Takes into account
  // rotation (assuming maximum 90 degrees), data spacing of output, and 
  // effective size of voxel.
  spacing = ((vtkStructuredPoints *)this->Output)->GetSpacing();
  h = sqrt(spacing[0]*spacing[0] + spacing[1]*spacing[1] + 
           spacing[2]*spacing[2]) / 2.0;
  numSteps = (int) ((double)(1.414 * sqrt((double)maxDist2)) / h);

  if ( numSteps <= 0 ) return 1;
  else return numSteps;
}

void vtkSweptSurface::Cap(vtkFloatScalars *s)
{
  int i,j,k;
  int idx;
  int d01=this->SampleDimensions[0]*this->SampleDimensions[1];

// i-j planes
  k = 0;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+j*this->SampleDimensions[0], this->FillValue);

  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+j*this->SampleDimensions[0], this->FillValue);

// j-k planes
  i = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(j*this->SampleDimensions[0]+k*d01, this->FillValue);

  i = this->SampleDimensions[0] - 1;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(i+j*this->SampleDimensions[0]+k*d01, this->FillValue);

// i-k planes
  j = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+k*d01, this->FillValue);

  j = this->SampleDimensions[1] - 1;
  idx = j*this->SampleDimensions[0];
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+k*d01, this->FillValue);

}

void vtkSweptSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsFilter::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Fill Value:" << this->FillValue << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");

  if ( this->Transforms )
    {
    os << indent << "Number of Transforms: " << this->Transforms->GetNumberOfItems() << "\n";
    }
  else
    {
    os << indent << "No transform defined!\n";
    }
}

