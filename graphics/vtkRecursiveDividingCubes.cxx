/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRecursiveDividingCubes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkRecursiveDividingCubes.h"
#include "vtkMath.h"
#include "vtkVoxel.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"


//------------------------------------------------------------------------------
vtkRecursiveDividingCubes* vtkRecursiveDividingCubes::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRecursiveDividingCubes");
  if(ret)
    {
    return (vtkRecursiveDividingCubes*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRecursiveDividingCubes;
}




vtkRecursiveDividingCubes::vtkRecursiveDividingCubes()
{
  this->Value = 0.0;
  this->Distance = 0.1;
  this->Increment = 1;
  this->Count = 0;
  this->Voxel = vtkVoxel::New();
}

vtkRecursiveDividingCubes::~vtkRecursiveDividingCubes()
{
  this->Voxel->Delete();
}

static float X[3]; //origin of current voxel
static float Spacing[3]; //spacing of current voxel
static float Normals[8][3]; //voxel normals
static vtkPoints *NewPts; //points being generated
static vtkFloatArray *NewNormals; //points being generated
static vtkCellArray *NewVerts; //verts being generated

void vtkRecursiveDividingCubes::Execute()
{
  int i, j, k;
  vtkIdType idx;
  vtkDataArray *inScalars;
  vtkIdList *voxelPts;
  float origin[3];
  int dim[3], jOffset, kOffset, sliceSize;
  int above, below, vertNum;
  vtkImageData *input= this->GetInput();
  vtkPolyData *output= this->GetOutput();
  vtkFloatArray *voxelScalars;

  vtkDebugMacro(<< "Executing recursive dividing cubes...");
  //
  // Initialize self; check input; create output objects
  //
  this->Count = 0;

  // make sure we have scalar data
  if ( ! (inScalars = input->GetPointData()->GetActiveScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to contour");
    return;
    }

  // just deal with volumes
  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro("Bad input: only treats 3D structured point datasets");
    return;
    }
  input->GetDimensions(dim);
  input->GetSpacing(Spacing);
  input->GetOrigin(origin);

  // creating points
  NewPts = vtkPoints::New();
  NewPts->Allocate(50000,100000);
  NewNormals = vtkFloatArray::New();
  NewNormals->SetNumberOfComponents(3);
  NewNormals->Allocate(50000,100000);
  NewVerts = vtkCellArray::New();
  NewVerts->Allocate(50000,100000);
  NewVerts->InsertNextCell(0); //temporary cell count

  voxelPts = vtkIdList::New();
  voxelPts->Allocate(8); 
  voxelPts->SetNumberOfIds(8);

  voxelScalars = vtkFloatArray::New();
  voxelScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
  voxelScalars->Allocate(8*inScalars->GetNumberOfComponents());
  
  //
  // Loop over all cells checking to see which straddle the specified value. 
  // Since we know that we are working with a volume, can create 
  // appropriate data directly.
  //
  sliceSize = dim[0] * dim[1];
  for ( k=0; k < (dim[2]-1); k++)
    {
    kOffset = k*sliceSize;
    X[2] = origin[2] + k*Spacing[2];

    for ( j=0; j < (dim[1]-1); j++)
      {
      jOffset = j*dim[0];
      X[1] = origin[1] + j*Spacing[1];

      for ( i=0; i < (dim[0]-1); i++)
        {
        idx  = i + jOffset + kOffset;
        X[0] = origin[0] + i*Spacing[0];

        // get point ids of this voxel
        voxelPts->SetId(0, idx);
        voxelPts->SetId(1, idx + 1);
        voxelPts->SetId(2, idx + dim[0]);
        voxelPts->SetId(3, idx + dim[0] + 1);
        voxelPts->SetId(4, idx + sliceSize);
        voxelPts->SetId(5, idx + sliceSize + 1);
        voxelPts->SetId(6, idx + sliceSize + dim[0]);
        voxelPts->SetId(7, idx + sliceSize + dim[0] + 1);

        // get scalars of this voxel
        inScalars->GetTuples(voxelPts,voxelScalars);

        // loop over 8 points of voxel to check if cell straddles value
        for ( above=below=0, vertNum=0; vertNum < 8; vertNum++ )
          {
          if ( voxelScalars->GetComponent(vertNum,0) >= this->Value )
	    {
            above = 1;
	    }
          else if ( voxelScalars->GetComponent(vertNum,0) < this->Value )
	    {
            below = 1;
	    }

          if ( above && below ) // recursively generate points
            { //compute voxel normals and subdivide
            input->GetPointGradient(i,j,k, inScalars, Normals[0]);
            input->GetPointGradient(i+1,j,k, inScalars, Normals[1]);
            input->GetPointGradient(i,j+1,k, inScalars, Normals[2]);
            input->GetPointGradient(i+1,j+1,k, inScalars, Normals[3]);
            input->GetPointGradient(i,j,k+1, inScalars, Normals[4]);
            input->GetPointGradient(i+1,j,k+1, inScalars, Normals[5]);
            input->GetPointGradient(i,j+1,k+1, inScalars, Normals[6]);
            input->GetPointGradient(i+1,j+1,k+1, inScalars, Normals[7]);

            this->SubDivide(X, Spacing, voxelScalars->GetPointer(0));
            }
          }
        }
      }
    }

  voxelPts->Delete();
  voxelScalars->Delete();
  NewVerts->UpdateCellCount(NewPts->GetNumberOfPoints());
  vtkDebugMacro(<< "Created " << NewPts->GetNumberOfPoints() << " points");
  //
  // Update ourselves and release memory
  //
  output->SetPoints(NewPts);
  NewPts->Delete();

  output->SetVerts(NewVerts);
  NewVerts->Delete();

  output->GetPointData()->SetNormals(NewNormals);
  NewNormals->Delete();

  output->Squeeze();
}

static int ScalarInterp[8][8] = {{0,8,12,24,16,22,20,26},
                                 {8,1,24,13,22,17,26,21},
                                 {12,24,2,9,20,26,18,23},
                                 {24,13,9,3,26,21,23,19},
                                 {16,22,20,26,4,10,14,25},
                                 {22,17,26,21,10,5,25,15},
                                 {20,26,18,23,14,25,6,11},
                                 {26,21,23,19,25,15,11,7}};

#define VTK_POINTS_PER_POLY_VERTEX 10000

void vtkRecursiveDividingCubes::SubDivide(float origin[3], float h[3], 
                                          float values[8])
{
  int i;
  float hNew[3];

  for (i=0; i<3; i++)
    {
    hNew[i] = h[i] / 2.0;
    }

  // if subdivided far enough, create point and end termination
  if ( h[0] < this->Distance && h[1] < this->Distance && h[2] < this->Distance )
    {
    vtkIdType id;
    float x[3], n[3];
    float p[3], w[8];

    for (i=0; i <3; i++)
      {
      x[i] = origin[i] + hNew[i];
      }

    if ( ! (this->Count++ % this->Increment) ) //add a point
      {
      id = NewPts->InsertNextPoint(x);
      NewVerts->InsertCellPoint(id);
      for (i=0; i<3; i++)
	{
	p[i] = (x[i] - X[i]) / Spacing[i];
	}
      this->Voxel->InterpolationFunctions(p,w);
      for (n[0]=n[1]=n[2]=0.0, i=0; i<8; i++)
	{
	n[0] += Normals[i][0]*w[i];
	n[1] += Normals[i][1]*w[i];
	n[2] += Normals[i][2]*w[i];
	}
      vtkMath::Normalize(n);
      NewNormals->InsertTuple(id,n);

      if ( !(NewPts->GetNumberOfPoints() % VTK_POINTS_PER_POLY_VERTEX) )
        {
        vtkDebugMacro(<<"point# "<<NewPts->GetNumberOfPoints());
        }
      }
    
    return;
    }

  // otherwise, create eight sub-voxels and recurse
  else
    {
    int j, k, idx, above, below, ii;
    float x[3];
    float newValues[8];
    float s[27], scalar;

    for (i=0; i<8; i++)
      {
      s[i] = values[i];
      }

    s[8] = (s[0] + s[1]) / 2.0; // edge verts
    s[9] = (s[2] + s[3]) / 2.0;
    s[10] = (s[4] + s[5]) / 2.0;
    s[11] = (s[6] + s[7]) / 2.0;
    s[12] = (s[0] + s[2]) / 2.0;
    s[13] = (s[1] + s[3]) / 2.0;
    s[14] = (s[4] + s[6]) / 2.0;
    s[15] = (s[5] + s[7]) / 2.0;
    s[16] = (s[0] + s[4]) / 2.0;
    s[17] = (s[1] + s[5]) / 2.0;
    s[18] = (s[2] + s[6]) / 2.0;
    s[19] = (s[3] + s[7]) / 2.0;

    s[20] = (s[0] + s[2] + s[4] + s[6]) / 4.0; // face verts
    s[21] = (s[1] + s[3] + s[5] + s[7]) / 4.0;
    s[22] = (s[0] + s[1] + s[4] + s[5]) / 4.0;
    s[23] = (s[2] + s[3] + s[6] + s[7]) / 4.0;
    s[24] = (s[0] + s[1] + s[2] + s[3]) / 4.0;
    s[25] = (s[4] + s[5] + s[6] + s[7]) / 4.0;

    s[26] = (s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6] + s[7]) / 8.0; //middle

    for (k=0; k < 2; k++)
      {
      x[2] = origin[2] +  k*hNew[2];

      for (j=0; j < 2; j++)
        {
        x[1] = origin[1] +  j*hNew[1];

        for (i=0; i < 2; i++)
          {
          idx = i + j*2 + k*4;
          x[0] = origin[0] +  i*hNew[0];

          for (above=below=0,ii=0; ii<8; ii++)
            {
            scalar = s[ScalarInterp[idx][ii]];

            if ( scalar >= this->Value )
	      {
	      above = 1;
	      }
            else if ( scalar < this->Value )
	      {
	      below = 1;
	      }

            newValues[ii] = scalar;
            }

          if ( above && below )
	    {
            this->SubDivide(x, hNew, newValues);
	    }
          }
        }
      }
    }
}

void vtkRecursiveDividingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Value: " << this->Value << "\n";
  os << indent << "Distance: " << this->Distance << "\n";
  os << indent << "Increment: " << this->Increment << "\n";
}


