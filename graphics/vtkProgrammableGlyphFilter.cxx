/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableGlyphFilter.cxx
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
#include "vtkProgrammableGlyphFilter.h"
#include "vtkTransform.h"
#include "vtkVectors.h"
#include "vtkNormals.h"
#include "vtkMath.h"

// Construct object with scaling on, scaling mode is by scalar value, 
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkProgrammableGlyphFilter::vtkProgrammableGlyphFilter()
{
  this->Source = NULL;

  this->GlyphMethod = NULL;
  this->GlyphMethodArgDelete = NULL;
  this->GlyphMethodArg = NULL;

  this->Point[0] = this->Point[1] = this->Point[2] = 0.0;
  this->PointId = -1;
  this->PointData = NULL;
  
  this->ColorMode = VTK_COLOR_BY_INPUT;
}

vtkProgrammableGlyphFilter::~vtkProgrammableGlyphFilter()
{
  if ((this->GlyphMethodArg)&&(this->GlyphMethodArgDelete))
    {
    (*this->GlyphMethodArgDelete)(this->GlyphMethodArg);
    }

  this->SetSource(NULL);
}

void vtkProgrammableGlyphFilter::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkPointData *inputPD = input->GetPointData();
  vtkCellData *inputCD = input->GetCellData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkPoints *newPts, *sourcePts;
  vtkScalars *ptScalars=NULL, *cellScalars=NULL;
  vtkScalars *inPtScalars, *inCellScalars;
  int numPts = input->GetNumberOfPoints();
  vtkPointData *sourcePD;
  vtkCellData *sourceCD;
  int numSourcePts, numSourceCells, ptOffset=0, scalarOffset;
  int cellId, ptId, id, i, idx, npts;
  vtkIdList *pts=vtkIdList::New();
  vtkIdList *cellPts;
  pts->Allocate(VTK_CELL_SIZE);
  vtkCell *cell;

  // Initialize
  vtkDebugMacro(<<"Generating programmable glyphs!");
  
  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No input points to glyph");
    }

  sourcePD = this->Source->GetPointData();
  sourceCD = this->Source->GetCellData();
  numSourcePts = this->Source->GetNumberOfPoints();
  numSourceCells = this->Source->GetNumberOfCells();

  outputPD->CopyScalarsOff(); //'cause we control the coloring process
  outputCD->CopyScalarsOff();

  output->Allocate(numSourceCells*numPts,numSourceCells*numPts);
  outputPD->CopyAllocate(sourcePD, numSourcePts*numPts, numSourcePts*numPts);
  outputCD->CopyAllocate(sourceCD, numSourceCells*numPts, numSourceCells*numPts);
  newPts = vtkPoints::New();
  newPts->Allocate(numSourcePts*numPts);

  // figure out how to color the data and setup
  if ( this->ColorMode == VTK_COLOR_BY_INPUT )
    {
    if ( (inPtScalars = inputPD->GetScalars()) )
      {
      ptScalars = vtkScalars::New();
      ptScalars->Allocate(numSourcePts*numPts);
      }
    if ( (inCellScalars = inputCD->GetScalars()) )
      {
      cellScalars = vtkScalars::New();
      cellScalars->Allocate(numSourcePts*numPts);
      }
    }
  else // VTK_COLOR_BY_SOURCE
    {
    if ( (inPtScalars = sourcePD->GetScalars()) )
      {
      ptScalars = vtkScalars::New();
      ptScalars->Allocate(numSourcePts*numPts);
      }
    if ( (inCellScalars = sourceCD->GetScalars()) )
      {
      cellScalars = vtkScalars::New();
      cellScalars->Allocate(numSourcePts*numPts);
      }
    }

  // Loop over all points, invoking glyph method and Update(), then append output
  // of source to output of this filter.
  //  
  this->Updating = 1; // to prevent infinite recursion
  this->PointData = input->GetPointData();
  for (this->PointId=0; this->PointId < numPts; this->PointId++)
    {
    if ( ! (this->PointId % 10000) ) 
      {
      this->UpdateProgress ((float)this->PointId/numPts);
      if (this->GetAbortExecute()) break;
      }

    input->GetPoint(this->PointId, this->Point);
    
    if ( this->GlyphMethod ) 
      {
      (*this->GlyphMethod)(this->GlyphMethodArg);
      }
    
    if ( this->Source ) 
      {
      this->Source->Update();
    
      sourcePts = this->Source->GetPoints();
      numSourcePts = this->Source->GetNumberOfPoints();
      numSourceCells = this->Source->GetNumberOfCells();
      sourcePD = this->Source->GetPointData();
      sourceCD = this->Source->GetCellData();

      // Copy all data from source to output.
      for (ptId=0; ptId < numSourcePts; ptId++)
        {
        id = newPts->InsertNextPoint(sourcePts->GetPoint(ptId));
        outputPD->CopyData(sourcePD, ptId, id);
        }
      
      for (cellId=0; cellId < numSourceCells; cellId++)
        {
        cell = this->Source->GetCell(cellId);
        cellPts = cell->GetPointIds();
        npts = cellPts->GetNumberOfIds();
        for (pts->Reset(), i=0; i < npts; i++) 
          {
          pts->InsertId(i,cellPts->GetId(i) + ptOffset);
          }
        id = output->InsertNextCell(cell->GetCellType(),pts);
        outputCD->CopyData(sourceCD, cellId, id);
        }
      
      // If we're coloring the output with scalars, do that now
      if ( ptScalars )
        {
        for (ptId=0; ptId < numSourcePts; ptId++)
          {
          idx = (this->ColorMode == VTK_COLOR_BY_INPUT ? this->PointId : ptId);
          ptScalars->InsertNextScalar(inPtScalars->GetScalar(idx));
          }
        }
      else if ( cellScalars )
        {
        for (cellId=0; cellId < numSourceCells; cellId++)
          {
          idx = (this->ColorMode == VTK_COLOR_BY_INPUT ? this->PointId : cellId);
          cellScalars->InsertNextScalar(inCellScalars->GetScalar(idx));
          }
        }
      
      ptOffset += numSourcePts;
      
      }//if a source is available
    } //for all input points

  this->Updating = 0;
  
  pts->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  
  if ( ptScalars )
    {
    outputPD->SetScalars(ptScalars);
    ptScalars->Delete();
    }
  
  if ( cellScalars )
    {
    outputCD->SetScalars(cellScalars);
    cellScalars->Delete();
    }
  
  output->Squeeze();
}


// Override update method because execution can branch two ways (via Input 
// and Source).
void vtkProgrammableGlyphFilter::Update()
{
  // make sure input is available
  if ( this->Input == NULL || this->Source == NULL )
    {
    vtkErrorMacro(<< "No input or source...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  if ( this->Source ) this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime || 
  (this->Source && this->Source->GetMTime() > this->ExecuteTime) || 
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() ) this->Input->ForceUpdate();
    if ( this->Source && this->Source->GetDataReleased() ) 
      this->Source->ForceUpdate();

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    // reset AbortExecute flag and Progress
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute ) this->UpdateProgress(1.0);
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->Source && this->Source->ShouldIReleaseData() ) 
    this->Source->ReleaseData();
}

// Specify function to be called before object executes.
void vtkProgrammableGlyphFilter::SetGlyphMethod(void (*f)(void *), void *arg)
{
  if ( f != this->GlyphMethod || arg != this->GlyphMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->GlyphMethodArg)&&(this->GlyphMethodArgDelete))
      {
      (*this->GlyphMethodArgDelete)(this->GlyphMethodArg);
      }
    this->GlyphMethod = f;
    this->GlyphMethodArg = arg;
    this->Modified();
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkProgrammableGlyphFilter::SetGlyphMethodArgDelete(void (*f)(void *))
{
  if ( f != this->GlyphMethodArgDelete)
    {
    this->GlyphMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Return the method of coloring as a descriptive character string.
char *vtkProgrammableGlyphFilter::GetColorModeAsString(void)
{
  if ( this->ColorMode == VTK_COLOR_BY_INPUT )
    {
    return "ColorByInput";
    }
  else 
    {
    return "ColorBySource";
    }
}

void vtkProgrammableGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Source: " << (void *)this->Source << "\n";

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;
  os << indent << "Point Id: " << this->PointId << "\n";
  os << indent << "Point: " << this->Point[0]
                    << ", " << this->Point[1]
                    << ", " << this->Point[2] << "\n";
  if (this->PointData)
    {
    os << indent << "PointData: " << this->PointData << "\n";
    }
  else
    {
    os << indent << "PointData: (not defined)\n";
    }

  if ( this->GlyphMethod )
    {
    os << indent << "Glyph Method defined\n";
    }
  else
    {
    os << indent << "No Glyph Method\n";
    }

}
