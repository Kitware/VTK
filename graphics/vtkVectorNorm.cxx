/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.cxx
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
#include "vtkVectorNorm.h"

// Construct with normalize flag off.
vtkVectorNorm::vtkVectorNorm()
{
  this->Normalize = 0;
  this->AttributeMode = VTK_ATTRIBUTE_MODE_DEFAULT;
}

void vtkVectorNorm::Execute()
{
  int i, numVectors, computePtScalars=1, computeCellScalars=1;
  vtkScalars *newScalars;
  float *v, s, maxScalar;
  vtkVectors *ptVectors, *cellVectors;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();

  // Initialize
  vtkDebugMacro(<<"Computing norm of vectors!");

  ptVectors = pd->GetVectors();
  cellVectors = cd->GetVectors();
  if (!ptVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_CELL_DATA)
    {
    computePtScalars = 0;
    }

  if (!cellVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
    {
    computeCellScalars = 0;
    }

  if ( !computeCellScalars && !computePtScalars )
    {
    vtkErrorMacro(<< "No vector norm to compute!");
    return;
    }

  // Allocate / operate on point data
  if ( computePtScalars )
    {
    numVectors = ptVectors->GetNumberOfVectors();
    newScalars = vtkScalars::New();
    newScalars->SetNumberOfScalars(numVectors);

    for (maxScalar=0.0, i=0; i < numVectors; i++)
      {
      v = ptVectors->GetVector(i);
      s = sqrt((double)v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
      if ( s > maxScalar ) maxScalar = s;
      newScalars->SetScalar(i,s);

      if ( ! (i % 20000) ) 
        {
        vtkDebugMacro(<<"Computing point vector norm #" << i);
        this->UpdateProgress (0.5*i/numVectors);
        }
      }

    // If necessary, normalize
    if ( this->Normalize && maxScalar > 0.0 )
      {
      for (i=0; i < numVectors; i++)
        {
        s = newScalars->GetScalar(i);
        s /= maxScalar;
        newScalars->SetScalar(i,s);
        }
      }

    outPD->SetScalars(newScalars);
    newScalars->Delete();
    }//if computing point scalars

  // Allocate / operate on cell data
  if ( computeCellScalars )
    {
    numVectors = cellVectors->GetNumberOfVectors();
    newScalars = vtkScalars::New();
    newScalars->SetNumberOfScalars(numVectors);

    for (maxScalar=0.0, i=0; i < numVectors; i++)
      {
      v = cellVectors->GetVector(i);
      s = sqrt((double)v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
      if ( s > maxScalar ) maxScalar = s;
      newScalars->SetScalar(i,s);
      if ( ! (i % 20000) ) 
        {
        vtkDebugMacro(<<"Computing cell vector norm #" << i);
        this->UpdateProgress (0.5*i/numVectors+0.5);
        }
      }

    // If necessary, normalize
    if ( this->Normalize && maxScalar > 0.0 )
      {
      for (i=0; i < numVectors; i++)
        {
        s = newScalars->GetScalar(i);
        s /= maxScalar;
        newScalars->SetScalar(i,s);
        }
      }

    outCD->SetScalars(newScalars);
    newScalars->Delete();
    }//if computing cell scalars

  // Pass appropriate data through to output
  outPD->PassNoReplaceData(pd);
  outCD->PassNoReplaceData(cd);
}

// Return the method for generating scalar data as a string.
char *vtkVectorNorm::GetAttributeModeAsString(void)
{
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
    {
    return "Default";
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else 
    {
    return "UseCellData";
    }
}

void vtkVectorNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Normalize: " << (this->Normalize ? "On\n" : "Off\n");
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() 
     << endl;
}

