/*=========================================================================

  Program:   Visualization Library
  Module:    SweptSur.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "SweptSur.hh"
#include "Actor.hh"
#include "Voxel.hh"

// Description:
// Construct object with SampleDimensions = (50,50,50), FillValue = 
// LARGE_FLOAT, ModelBounds=(0,0,0,0,0,0) (i.e, bounds will be
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

  this->Interpolation = 0;
  this->FillValue = LARGE_FLOAT;
  this->Transforms = NULL;
  this->Capping = 1;
}

// Description:
// Define the volume (in world coordinates) in which the sampling is to occur.
// Make sure that the volume is large enough to accomodate the motion of the
// geometry along the path. If the model bounds are set to all zero values, the
// model bounds will be computed automatically from the input and path.
void vtkSweptSurface::SetModelBounds(float *bounds)
{
  vtkSweptSurface::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vtkSweptSurface::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  if (this->ModelBounds[0] != xmin || this->ModelBounds[1] != xmax ||
  this->ModelBounds[2] != ymin || this->ModelBounds[3] != ymax ||
  this->ModelBounds[4] != zmin || this->ModelBounds[5] != zmax )
    {
    float length;

    this->Modified();
    this->ModelBounds[0] = xmin;
    this->ModelBounds[1] = xmax;
    this->ModelBounds[2] = ymin;
    this->ModelBounds[3] = ymax;
    this->ModelBounds[4] = zmin;
    this->ModelBounds[5] = zmax;

    this->Origin[0] = xmin;
    this->Origin[1] = ymin;
    this->Origin[2] = zmin;

    if ( (length = xmax - xmin) == 0.0 ) length = 1.0;
    this->AspectRatio[0] = 1.0;
    this->AspectRatio[1] = (ymax - ymin) / length;
    this->AspectRatio[2] = (zmax - zmin) / length;
    }
}

void vtkSweptSurface::Execute()
{
  int i, numPts, numOutPts;
  float ar[3];
  vtkPointData *pd;
  vtkScalars *inScalars, *newScalars;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  float inAr[3], inOrigin[3];
  int inDim[3];
  int numSteps, stepNum;
  int numTransforms, transNum;
  vtkActor a;
  vtkTransform *t1, *t2;
  float time;
  float position[3], position1[3], position2[3];
  float orientation[3], orientation1[3], orientation2[3];

  vtkDebugMacro(<<"Creating swept surface");
  this->Initialize();

  // make sure there is input
  pd = this->Input->GetPointData();
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

  this->SetDimensions(this->SampleDimensions);

  // if bounds are not specified, compute bounds from path
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5])
    {
    this->ComputeBounds();
    }

  input->GetDimensions(inDim);
  input->GetAspectRatio(inAr);
  input->GetOrigin(inOrigin);
//
// Allocate data.  Scalar "type" is same as input.
//
  numOutPts = this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2];
  newScalars = inScalars->MakeObject(numOutPts);
  for (i = 0; i < numOutPts; i++) newScalars->SetScalar(i,this->FillValue);
//
// Sample data at each point in path
//
  this->Transforms->InitTraversal();
  t2 = this->Transforms->GetNextItem();
  t2->Push();
  t2->Inverse();

  for (transNum=0; transNum < (numTransforms-1); transNum++)
    {
    vtkDebugMacro(<<"Injecting between transforms "<< transNum+1 <<" and "
                  << transNum+2);
    t1 = t2;
    t2 = this->Transforms->GetNextItem();
    t2->Push();
    t2->Inverse();
//
// Loop over all points (i.e., voxels), transform into input coordinate system,
// and obtain interpolated value. Then perform union operation.  
//
    if ( this->Interpolation > 0 ) numSteps = this->Interpolation;
    else if ( this->Interpolation < 0 ) numSteps = 1;
    else numSteps = this->ComputeNumberOfSteps(t1,t2);

    for (stepNum=0; stepNum < numSteps; stepNum++)
      {
      // linearly interpolate position and orientation
      time = stepNum / numSteps;

      for (i=0; i<3; i++)
        {
        position1[i] = position2[i];
        orientation1[i] = orientation2[i];
        }
      t2->GetPosition(position2[0], position2[1], position2[2]);
      t2->GetOrientation(orientation2[0], orientation2[1], orientation2[2]);

      for (i=0; i<3; i++)
        {
        position[i] = position1[i] + time*(position2[i] - position1[i]);
        orientation[i] = orientation1[i] + time*(orientation2[i] - orientation1[i]);
        }

      a.SetPosition(position);
      a.SetOrientation(orientation);
      this->SampleInput(a.GetMatrix(), inDim, inOrigin, inAr,
                        inScalars, newScalars);
      }

    t1->Pop();
    }

  //finish off last step
  a.SetPosition(position2);
  a.SetOrientation(orientation2);
  this->SampleInput(a.GetMatrix(), inDim, inOrigin, inAr,
                    inScalars, newScalars);

  t2->Pop();

  // Update ourselves and release memory
  this->PointData.SetScalars(newScalars);
  newScalars->Delete();
}

void vtkSweptSurface::SampleInput(vtkMatrix4x4& m, int inDim[3], 
                                 float inOrigin[3], float inAr[3], 
                                 vtkScalars *inScalars, vtkScalars *outScalars)
{
  int i, j, k, ii;
  int inSliceSize=inDim[0]*inDim[1];
  int sliceSize=this->SampleDimensions[0]*this->SampleDimensions[1];
  float x[3], loc[3], newScalar, scalar;
  static vtkVoxel voxel;
  static vtkIdList idList(8);
  static vtkFloatScalars voxelScalars(8);
  int kOffset, jOffset, dim[3], idx;
  float xTrans[3], weights[8];

  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    kOffset = k*sliceSize;
    x[2] = this->Origin[2] + k * this->AspectRatio[2];
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      jOffset = j*this->SampleDimensions[0];
      x[1] = this->Origin[1] + k * this->AspectRatio[1];
      for (i=0; i<this->SampleDimensions[0]; i++)
        {
        x[0] = this->Origin[0] + k * this->AspectRatio[0];

        // transform into local space
        m.PointMultiply(x,xTrans);

        // determine which voxel point falls in.
        for (ii=0; ii<3; ii++)
          {
          loc[ii] = (xTrans[ii]-inOrigin[ii]) / inAr[ii];
          dim[ii] = (int)loc[ii];
          }

        //check and make sure point is inside
        if ( loc[0] >= 0.0 && loc[1] >= 0.0 && loc[2] >= 0.0 &&
        dim[0] < inDim[0] && dim[1] < inDim[1] && dim[2] < inDim[2] )
          {
          //get scalar values
          idx = dim[0] + dim[1]*inDim[0] + dim[2]*inSliceSize;
          idList.SetId(0,idx);
          idList.SetId(1,idx+1);
          idList.SetId(2,idx+1 + inDim[0]);
          idList.SetId(3,idx + inDim[0]);
          idList.SetId(0,idx + sliceSize);
          idList.SetId(1,idx+1 + sliceSize);
          idList.SetId(2,idx+1 + inDim[0] + sliceSize);
          idList.SetId(3,idx + inDim[0] + sliceSize);

          inScalars->GetScalars(idList,voxelScalars);

          for (ii=0; ii<3; ii++) loc[ii] = loc[ii] - dim[ii];

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
  unsigned long int mtime=this->MTime;
  unsigned long int transMtime;
  vtkTransform *t;

  for (this->Transforms->InitTraversal(); t = this->Transforms->GetNextItem(); )
    {
    transMtime = t->GetMTime();
    if ( transMtime > mtime ) mtime = transMtime;
    }

  return mtime;
}


// compute model bounds from geometry and path
void vtkSweptSurface::ComputeBounds()
{
}

// based on both path and bounding box of input, compute the number of 
// steps between the specified transforms
int vtkSweptSurface::ComputeNumberOfSteps(vtkTransform *t1, vtkTransform *t2)
{
  return 1;
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
      s->SetScalar(i+j*this->SampleDimensions[1], this->FillValue);

  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+j*this->SampleDimensions[1], this->FillValue);

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
  vtkStructuredPointsToStructuredPointsFilter::PrintSelf(os,indent);

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

