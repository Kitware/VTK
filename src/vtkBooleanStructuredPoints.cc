/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanStructuredPoints.cc
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
#include "vtkBooleanStructuredPoints.hh"

// Description:
// Construct with sample resolution of (50,50,50) and automatic 
// computation of sample bounds. Initial boolean operation is union.
vtkBooleanStructuredPoints::vtkBooleanStructuredPoints()
{
  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->OperationType = VTK_UNION_OPERATOR;
  // this->Operator = this->Union;
  this->Output = new vtkStructuredPoints;
  this->Output->SetSource(this);
}

// Description:
// Add another structured point set to the list of objects to boolean.
void vtkBooleanStructuredPoints::AddInput(vtkStructuredPoints *sp)
{
  if ( ! this->InputList.IsItemPresent(sp) )
    {
    this->Modified();
    this->InputList.AddItem(sp);
    }
}

// Description:
// Remove an object from the list of objects to boolean.
void vtkBooleanStructuredPoints::RemoveInput(vtkStructuredPoints *sp)
{
  if ( this->InputList.IsItemPresent(sp) )
    {
    this->Modified();
    this->InputList.RemoveItem(sp);
    }
}

void vtkBooleanStructuredPoints::Update()
{
  unsigned long int mtime, dsMtime;
  vtkDataSet *ds;

  // make sure input is available or output has been created
  if ( this->InputList.GetNumberOfItems() < 1 && 
  this->Output->GetPointData()->GetScalars() == NULL )
    {
    vtkErrorMacro(<< "No input...or appended data...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, this->InputList.InitTraversal(); 
       (ds = this->InputList.GetNextItem()); )
    {
    ds->Update();
    dsMtime = ds->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  this->Updating = 0;

  if ( mtime > this->ExecuteTime || this->GetMTime() > this->ExecuteTime )
    {
    for (this->InputList.InitTraversal(); (ds=this->InputList.GetNextItem());)
      {
      if ( ds->GetDataReleased() ) ds->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  for (this->InputList.InitTraversal(); (ds = this->InputList.GetNextItem()); )
    if ( ds->ShouldIReleaseData() ) ds->ReleaseData();
}

// Initialize object prior to performing Boolean operations
void vtkBooleanStructuredPoints::InitializeBoolean()
{
  vtkScalars *inScalars=NULL;
  vtkScalars *newScalars;
  vtkStructuredPoints *sp;
  float *bounds;
  int numPts;
  int i, j;
  vtkStructuredPoints *output = this->GetOutput();
  float tempf[3];
  
  output->SetDimensions(this->SampleDimensions);
  numPts = output->GetNumberOfPoints();

  // If ModelBounds unset, use input, else punt
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    if ( this->InputList.GetNumberOfItems() > 0 )
      {
       this->ModelBounds[0] = this->ModelBounds[2] = 
	 this->ModelBounds[4] = VTK_LARGE_FLOAT;
       this->ModelBounds[1] = this->ModelBounds[3] = 
	 this->ModelBounds[5] = -VTK_LARGE_FLOAT;
      for (this->InputList.InitTraversal(); 
	   (sp = this->InputList.GetNextItem()); )
        {
        bounds = sp->GetBounds();
        for (j=0; j < 3; j++)
          {
          if ( bounds[2*j] < this->ModelBounds[2*j] )
            this->ModelBounds[2*j] = bounds[2*j];

          if ( bounds[2*j+1] > this->ModelBounds[2*j+1] )
            this->ModelBounds[2*j+1] = bounds[2*j+1];
          }
        }
      } 
    else
      {
      this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = 0.0;
      this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = 1000.0;
      }
    }

  // Update origin and aspect ratio
  for (i=0; i<3; i++)
    {
    tempf[i] = this->ModelBounds[2*i];
    }
  output->SetOrigin(tempf);
  for (i=0; i<3; i++)
    {
    tempf[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
    }
  output->SetAspectRatio(tempf);
  
  // Create output scalar (same type as input)
  if ( this->InputList.GetNumberOfItems() > 0 )
    {
    this->InputList.InitTraversal(); sp = this->InputList.GetNextItem();
    inScalars = sp->GetPointData()->GetScalars();
    }

  if ( inScalars != NULL )
    {
    newScalars = inScalars->MakeObject(numPts); //copy
    }
  else
    {
    newScalars = new vtkFloatScalars(numPts);
    }
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

// Perform boolean operations on input volumes.
void vtkBooleanStructuredPoints::Execute()
{
  vtkStructuredPoints *sp;

  this->InitializeBoolean();

  for (this->InputList.InitTraversal(); (sp = this->InputList.GetNextItem()); )
    {
    this->Append(sp);
    }
}

// Description:
// Perform boolean operations by appending to current output data.
void vtkBooleanStructuredPoints::Append(vtkStructuredPoints *sp)
{
  vtkScalars *currentScalars, *inScalars;
  float *inBounds;
  float *destBounds;
  int i,j,k;
  float *in_aspect;
  float inX,inY,inZ;
  int inI,inJ,inK;
  int inKval,inJval;
  int destKval,destJval;
  int *inDimensions;
  vtkStructuredPoints *output = (vtkStructuredPoints *)this->Output;
  float *aspectRatio = output->GetAspectRatio();
  
  if ((currentScalars = this->Output->GetPointData()->GetScalars()) == NULL )
    {
    this->InitializeBoolean();
    currentScalars = this->Output->GetPointData()->GetScalars();
    }

  inScalars = sp->GetPointData()->GetScalars();

  inBounds = sp->GetBounds();
  destBounds = this->GetModelBounds();
  in_aspect = sp->GetAspectRatio();
  inDimensions = sp->GetDimensions();

  // now perform operation on data
  switch (this->OperationType)
    {
    case VTK_UNION_OPERATOR :
      {
      // for each cell
      for (k = 0; k < this->SampleDimensions[2]; k++)
	{
	inZ = destBounds[4] + k*aspectRatio[2];
	inK = int ((float)(inZ - inBounds[4])/in_aspect[2]);
	if ((inK >= 0)&&(inK < inDimensions[2]))
	  {
	  inKval = inK*inDimensions[0]*inDimensions[1];
	  destKval = k*this->SampleDimensions[0]*this->SampleDimensions[1];
	  for (j = 0; j < this->SampleDimensions[1]; j++)
	    {
	    inY = destBounds[2] + j*aspectRatio[1];
	    inJ = (int) ((float)(inY - inBounds[2])/in_aspect[1]);
	    if ((inJ >= 0)&&(inJ < inDimensions[1]))
	      {
	      inJval = inJ*inDimensions[0];
	      destJval = j*this->SampleDimensions[0];
	      for (i = 0; i < this->SampleDimensions[0]; i++)
		{
		inX = destBounds[0] + i*aspectRatio[0];
		inI = (int) ((float)(inX - inBounds[0])/in_aspect[0]);
		if ((inI >= 0)&&(inI < inDimensions[0]))
		  {
		  if (inScalars->GetScalar(inKval+inJval+inI))
		    {
		    currentScalars->SetScalar(destKval + destJval+i,1);
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
      break;
    }
}

// Description:
// Set the i-j-k dimensions on which to perform boolean operation.
void vtkBooleanStructuredPoints::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vtkBooleanStructuredPoints::SetSampleDimensions(int dim[3])
{
  int i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<0 || dim[1]<0 || dim[2]<0 )
      {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];

    this->Modified();
    }
}

// Description:
// Set the size of the volume on which to perform the sampling.
void vtkBooleanStructuredPoints::SetModelBounds(float *bounds)
{
  vtkBooleanStructuredPoints::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vtkBooleanStructuredPoints::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  vtkStructuredPoints *output = this->GetOutput();
  
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

    output->SetOrigin(xmin,ymin,zmin);

    if ( (length = xmax - xmin) == 0.0 ) length = 1.0;
    output->SetAspectRatio(1.0,(ymax - ymin) / length,(zmax - zmin) / length);
    }
}

void vtkBooleanStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFilter::PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList.PrintSelf(os,indent.GetNextIndent());
}

