/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleElevationFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkSimpleElevationFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkSimpleElevationFilter* vtkSimpleElevationFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSimpleElevationFilter");
  if(ret)
    {
    return (vtkSimpleElevationFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSimpleElevationFilter;
}


// Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
// range is (0,1).
vtkSimpleElevationFilter::vtkSimpleElevationFilter()
{
  this->Vector[0] = 0.0;
  this->Vector[1] = 0.0;
  this->Vector[2] = 1.0;
}

// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vtkSimpleElevationFilter::Execute()
{
  int i, numPts;
  vtkScalars *newScalars;
  float s, x[3];
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  // Initialize
  //
  vtkDebugMacro(<<"Generating elevation scalars!");

 // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    //vtkErrorMacro(<< "No input!");
    return;
    }

  // Allocate
  //
  newScalars = vtkScalars::New();
  newScalars->SetNumberOfScalars(numPts);

  // Set up 1D parametric system
  //
  if ( vtkMath::Dot(this->Vector,this->Vector) == 0.0)
    {
    vtkErrorMacro(<< "Bad vector, using (0,0,1)");
    this->Vector[0] = this->Vector[1] = 0.0; this->Vector[2] = 1.0;
    }

  // Compute dot product
  //
  for (i=0; i<numPts; i++)
    {
    if ( ! (i % 10000) ) 
      {
      this->UpdateProgress ((float)i/numPts);
      if (this->GetAbortExecute())
	{
	break;
	}
      }

    input->GetPoint(i,x);
    s = vtkMath::Dot(this->Vector,x);
    newScalars->SetScalar(i,s);
    }

  // Update self
  //
  output->GetPointData()->CopyScalarsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkSimpleElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Vector: (" << this->Vector[0] << ", "
     << this->Vector[1] << ", " << this->Vector[2] << ")\n";
}
