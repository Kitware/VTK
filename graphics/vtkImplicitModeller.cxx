/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitModeller.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include "vtkImplicitModeller.h"
#include "vtkFloatScalars.h"

// Description:
// Construct with sample dimensions=(50,50,50), and so that model bounds are
// automatically computed from the input. Capping is turned on with CapValue
// equal to a large positive number.
vtkImplicitModeller::vtkImplicitModeller()
{
  this->MaximumDistance = 0.075;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Capping = 1;
  this->CapValue = sqrt(VTK_LARGE_FLOAT) / 3.0;

  this->DataAppended = 0;
  this->AdjustBounds = 1;
  this->AdjustDistance = 0.0125;
}

void vtkImplicitModeller::SetModelBounds(float xmin, float xmax, float ymin, 
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

// Description:
// Initialize the filter for appending data. You must invoke the
// StartAppend() method before doing successive Appends(). It's also a
// good idea to manually specify the model bounds; otherwise the input
// bounds for the data will be used.
void vtkImplicitModeller::StartAppend()
{
  int numPts;
  vtkFloatScalars *newScalars;
  int i;
  float maxDistance;

  vtkDebugMacro(<< "Initializing data");
  this->DataAppended = 1;

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
           * this->SampleDimensions[2];
  newScalars = vtkFloatScalars::New(); 
  newScalars->SetNumberOfScalars(numPts);
  maxDistance = this->CapValue * this->CapValue;//sqrt taken later
  for (i=0; i<numPts; i++) newScalars->SetScalar(i,maxDistance);

  this->GetOutput()->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

// Description:
// Append a data set to the existing output. To use this function,
// you'll have to invoke the StartAppend() method before doing
// successive appends. It's also a good idea to specify the model
// bounds; otherwise the input model bounds is used. When you've
// finished appending, use the EndAppend() method.
void vtkImplicitModeller::Append(vtkDataSet *input)
{
  int cellNum, i, j, k;
  float *bounds, adjBounds[6];
  vtkCell *cell;
  float maxDistance, pcoords[3];
  vtkFloatScalars *newScalars;
  int idx, subId;
  int min[3], max[3];
  float x[3], prevDistance2, distance2;
  int jkFactor;
  float closestPoint[3];
  vtkStructuredPoints *output = this->GetOutput();
  float *Spacing;
  float *origin;
  float *weights=new float[input->GetMaxCellSize()];
  
  vtkDebugMacro(<< "Appending data");

  // Get the output scalars
  newScalars = (vtkFloatScalars *) (output->GetPointData()->GetScalars());

  output->SetDimensions(this->GetSampleDimensions());
  maxDistance = this->ComputeModelBounds();
  Spacing = output->GetSpacing();
  origin = output->GetOrigin();
  
  //
  // Traverse all cells; computing distance function on volume points.
  //
  for (cellNum=0; cellNum < input->GetNumberOfCells(); cellNum++)
    {
    cell = input->GetCell(cellNum);
    bounds = cell->GetBounds();
    for (i=0; i<3; i++)
      {
      adjBounds[2*i] = bounds[2*i] - maxDistance;
      adjBounds[2*i+1] = bounds[2*i+1] + maxDistance;
      }

    // compute dimensional bounds in data set
    for (i=0; i<3; i++)
      {
      min[i] = (int) ((float)(adjBounds[2*i] - origin[i]) / 
                      Spacing[i]);
      max[i] = (int) ((float)(adjBounds[2*i+1] - origin[i]) / 
                      Spacing[i]);
      if (min[i] < 0) min[i] = 0;
      if (max[i] >= this->SampleDimensions[i]) max[i] = this->SampleDimensions[i] - 1;
      }

    jkFactor = this->SampleDimensions[0]*this->SampleDimensions[1];
    for (k = min[2]; k <= max[2]; k++) 
      {
      x[2] = Spacing[2] * k + origin[2];
      for (j = min[1]; j <= max[1]; j++)
        {
        x[1] = Spacing[1] * j + origin[1];
        for (i = min[0]; i <= max[0]; i++) 
          {
          x[0] = Spacing[0] * i + origin[0];
          idx = jkFactor*k + this->SampleDimensions[0]*j + i;
          prevDistance2 = newScalars->GetScalar(idx);

          // union combination of distances
          if ( cell->EvaluatePosition(x, closestPoint, subId, pcoords, 
          distance2, weights) != -1 && distance2 < prevDistance2 )
            newScalars->SetScalar(idx,distance2);
          }
        }
      }
    }

  delete [] weights;
}

// Description:
// Method completes the append process.
void vtkImplicitModeller::EndAppend()
{
  vtkFloatScalars *newScalars;
  int i, numPts;
  float distance2;

  vtkDebugMacro(<< "End append");

  newScalars = (vtkFloatScalars *) (this->GetOutput()->GetPointData()->GetScalars());
  numPts = newScalars->GetNumberOfScalars();
//
// Run through scalars and take square root
//
  for (i=0; i<numPts; i++)
    {
    distance2 = newScalars->GetScalar(i);
    newScalars->SetScalar(i,sqrt(distance2));
    }
//
// If capping is turned on, set the distances of the outside faces of the volume
// to the CapValue.
//
  if ( this->Capping )
    {
    this->Cap(newScalars);
    }

  this->ExecuteTime.Modified();
  this->SetDataReleased(0);
}

void vtkImplicitModeller::Execute()
{
  vtkDebugMacro(<< "Executing implicit model");

  this->StartAppend();
  this->Append(this->Input);
  this->EndAppend();
}

// Description:
// Special update methods handles possibility of appending data.
void vtkImplicitModeller::Update()
{
  // make sure input is available
  if ( !this->Input && ! this->DataAppended)
    {
    vtkErrorMacro(<< "No input...or data appended...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  if ( this->Input )
    {
    this->DataAppended = 0;

    this->Updating = 1;
    this->Input->Update();
    this->Updating = 0;

    if (this->Input->GetMTime() > this->ExecuteTime ||
    this->GetMTime() > this->ExecuteTime )
      {
      if ( this->Input->GetDataReleased() )
        {
        this->Input->ForceUpdate();
        }

      if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
      this->Output->Initialize(); //clear output
      this->Execute();
      this->ExecuteTime.Modified();
      this->SetDataReleased(0);
      if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
      }

    if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
    }
}

// Description:
// Compute ModelBounds from input geometry.
float vtkImplicitModeller::ComputeModelBounds()
{
  float *bounds, maxDist, h[3];
  int i;
  vtkStructuredPoints *output=this->GetOutput();
  float tempf[3];
  
  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    bounds = this->Input->GetBounds();
    }
  else
    {
    bounds = this->ModelBounds;
    }

  for (maxDist=0.0, i=0; i<3; i++)
    if ( (h[i]=(bounds[2*i+1] - bounds[2*i])) > maxDist )
      maxDist = bounds[2*i+1] - bounds[2*i];

  maxDist *= this->MaximumDistance;

  // adjust bounds so model fits strictly inside (only if not set previously)
  if ( this->AdjustBounds )
    {
    for (i=0; i<3; i++)
      {
      this->ModelBounds[2*i] = bounds[2*i] - h[i]*this->AdjustDistance;
      this->ModelBounds[2*i+1] = bounds[2*i+1] + h[i]*this->AdjustDistance;
      }
    }

  // Set volume origin and data spacing
  output->SetOrigin(this->ModelBounds[0],this->ModelBounds[2],
		    this->ModelBounds[4]);
  
  for (i=0; i<3; i++)
    {
    tempf[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
    }
  output->SetSpacing(tempf);

  return maxDist;  
}

// Description:
// Set the i-j-k dimensions on which to sample the distance function.
void vtkImplicitModeller::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vtkImplicitModeller::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }

    for (dataDim=0, i=0; i<3 ; i++) if (dim[i] > 1) dataDim++;

    if ( dataDim  < 3 )
      {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];

    this->Modified();
    }
}

void vtkImplicitModeller::Cap(vtkFloatScalars *s)
{
  int i,j,k;
  int idx;
  int d01=this->SampleDimensions[0]*this->SampleDimensions[1];

// i-j planes
  k = 0;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+j*this->SampleDimensions[0], this->CapValue);

  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+j*this->SampleDimensions[0], this->CapValue);

// j-k planes
  i = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(j*this->SampleDimensions[0]+k*d01, this->CapValue);

  i = this->SampleDimensions[0] - 1;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(i+j*this->SampleDimensions[0]+k*d01, this->CapValue);

// i-k planes
  j = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+k*d01, this->CapValue);

  j = this->SampleDimensions[1] - 1;
  idx = j*this->SampleDimensions[0];
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+k*d01, this->CapValue);

}

void vtkImplicitModeller::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToStructuredPointsFilter::PrintSelf(os,indent);

  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";
  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";
}
