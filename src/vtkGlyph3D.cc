/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph3D.cc
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
#include "vtkGlyph3D.hh"
#include "vtkTransform.hh"
#include "vtkFloatVectors.hh"
#include "vtkFloatNormals.hh"
#include "vtkMath.hh"

// Description
// Construct object with scaling on, scaling mode is by scalar value, 
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping is turned off.
vtkGlyph3D::vtkGlyph3D()
{
  this->Source = NULL;
  this->Scaling = 1;
  this->ScaleMode = VTK_SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->VectorMode = VTK_USE_VECTOR;
  this->Clamping = 0;
}

vtkGlyph3D::~vtkGlyph3D()
{
}

void vtkGlyph3D::Execute()
{
  vtkPointData *pd;
  vtkScalars *inScalars;
  vtkVectors *inVectors;
  vtkNormals *inNormals, *sourceNormals;
  int numPts, numSourcePts, numSourceCells;
  int inPtId, i;
  vtkPoints *sourcePts;
  vtkCellArray *sourceCells, *cells;
  vtkFloatPoints *newPts;
  vtkFloatScalars *newScalars=NULL;
  vtkFloatVectors *newVectors=NULL;
  vtkFloatNormals *newNormals=NULL;
  float *x, *v, vNew[3];
  vtkTransform trans;
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts;
  vtkIdList pts(VTK_CELL_SIZE);
  int orient, scaleSource, ptIncr, cellId;
  float scale = 1;
  float den;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  vtkDebugMacro(<<"Generating glyphs");

  pd = this->Input->GetPointData();
  inScalars = pd->GetScalars();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();

  numPts = this->Input->GetNumberOfPoints();
//
// Allocate storage for output PolyData
//
  sourcePts = this->Source->GetPoints();
  numSourcePts = sourcePts->GetNumberOfPoints();
  numSourceCells = this->Source->GetNumberOfCells();
  sourceNormals = this->Source->GetPointData()->GetNormals();

  newPts = new vtkFloatPoints(numPts*numSourcePts);
  if (inScalars != NULL) 
    newScalars = new vtkFloatScalars(numPts*numSourcePts);
  if (inVectors != NULL || inNormals != NULL ) 
    newVectors = new vtkFloatVectors(numPts*numSourcePts);
  if (sourceNormals != NULL) 
    newNormals = new vtkFloatNormals(numPts*numSourcePts);

  // Setting up for calls to PolyData::InsertNextCell()
  if ( (sourceCells=this->Source->GetVerts())->GetNumberOfCells() > 0 )
    {
    cells = new vtkCellArray(numPts*sourceCells->GetSize());
    output->SetVerts(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->Source->GetLines())->GetNumberOfCells() > 0 )
    {
    cells = new vtkCellArray(numPts*sourceCells->GetSize());
    output->SetLines(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->Source->GetPolys())->GetNumberOfCells() > 0 )
    {
    cells = new vtkCellArray(numPts*sourceCells->GetSize());
    output->SetPolys(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->Source->GetStrips())->GetNumberOfCells() > 0 )
    {
    cells = new vtkCellArray(numPts*sourceCells->GetSize());
    output->SetStrips(cells);
    cells->Delete();
    }
//
// Copy (input scalars) to (output scalars) and either (input vectors or
// normals) to (output vectors). All other point attributes are copied 
// from Source.
//
  pd = this->Source->GetPointData();
  outputPD->CopyScalarsOff();
  outputPD->CopyVectorsOff();
  outputPD->CopyNormalsOff();
  outputPD->CopyAllocate(pd,numPts*numSourcePts);
//
// First copy all topology (transformation independent)
//
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->Source->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (pts.Reset(), i=0; i < npts; i++) 
        pts.InsertId(i,cellPts->GetId(i) + ptIncr);
      output->InsertNextCell(cell->GetCellType(),pts);
      }
    }
//
// Traverse all Input points, transforming Source points and copying 
// point attributes.
//
  if ( (this->VectorMode == VTK_USE_VECTOR && inVectors != NULL) ||
  (this->VectorMode == VTK_USE_NORMAL && inNormals != NULL) )
    orient = 1;
  else
    orient = 0;
    
  if ( this->Scaling && 
  ((this->ScaleMode == VTK_SCALE_BY_SCALAR && inScalars != NULL) ||
  (this->ScaleMode == VTK_SCALE_BY_VECTOR && (inVectors || inNormals))) )
    scaleSource = 1;
  else
    scaleSource = 0;

  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    
    trans.Identity();

    // translate Source to Input point
    x = this->Input->GetPoint(inPtId);
    trans.Translate(x[0], x[1], x[2]);

    if ( orient )
      {
      if ( this->VectorMode == VTK_USE_NORMAL )
        v = inNormals->GetNormal(inPtId);
      else
        v = inVectors->GetVector(inPtId);
      scale = vtkMath::Norm(v);

      // Copy Input vector
      for (i=0; i < numSourcePts; i++) 
        newVectors->InsertVector(ptIncr+i,v);
          
      if (this->Orient && (scale > 0)) 
        {
	// if there is no y or z component
	if (v[1] == 0 && v[2] == 0)
	  {
	  // just flip x if we need to
	  if (v[0] < 0)
	    {
	    trans.RotateWXYZ(180.0,0,1,0);
	    }
	  }
	else
	  {
	  vNew[0] = (v[0]+scale) / 2.0;
	  vNew[1] = v[1] / 2.0;
	  vNew[2] = v[2] / 2.0;
	  trans.RotateWXYZ(180.0,vNew[0],vNew[1],vNew[2]);
	  }
        }
      }

    // determine scale factor from scalars if appropriate
    if ( inScalars != NULL )
      {
      // Copy Input scalar
      if ( this->ScaleMode == VTK_SCALE_BY_SCALAR )
        {
	scale = inScalars->GetScalar(inPtId);
	}
      if (this->Clamping)
	{
        if ( (den = this->Range[1] - this->Range[0]) == 0.0 ) den = 1.0;
	
        scale = (scale < this->Range[0] ? this->Range[0] :
                 (scale > this->Range[1] ? this->Range[1] : scale));
        scale = (scale - this->Range[0]) / den;
        }

      for (i=0; i < numSourcePts; i++) 
        newScalars->InsertScalar(ptIncr+i,scale);
      }

    // scale data if appropriate
    if ( scaleSource )
      {
      scale *= this->ScaleFactor;
      if ( scale == 0.0 ) scale = 1.0e-10;
      trans.Scale(scale,scale,scale);
      }

    // multiply points and normals by resulting matrix
    trans.MultiplyPoints(sourcePts,newPts);
    if ( sourceNormals != NULL ) 
      trans.MultiplyNormals(sourceNormals,newNormals);

    // Copy point data from source
    for (i=0; i < numSourcePts; i++) 
      outputPD->CopyData(pd,i,ptIncr+i);
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPts);
  newPts->Delete();

  if (newScalars)
    {
    outputPD->SetScalars(newScalars);
    newScalars->Delete();
    }

  if (newVectors)
    {
    outputPD->SetVectors(newVectors);
    newVectors->Delete();
    }

  if (newNormals)
    {
    outputPD->SetNormals(newNormals);
    newNormals->Delete();
    }

  output->Squeeze();
}

// Description:
// Override update method because execution can branch two ways (via Input 
// and Source).
void vtkGlyph3D::Update()
{
  // make sure input is available
  if ( this->Input == NULL || this->Source == NULL )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime || 
  this->Source->GetMTime() > this->ExecuteTime || 
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() ) this->Input->ForceUpdate();
    if ( this->Source->GetDataReleased() ) this->Source->ForceUpdate();

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->Source->ShouldIReleaseData() ) this->Source->ReleaseData();
}

void vtkGlyph3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Source: " << this->Source << "\n";
  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
  os << indent << "Scale Mode: " << (this->ScaleMode == VTK_SCALE_BY_SCALAR ? "Scale by scalar\n" : "Scale by vector\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");

  os << indent << "Orient Mode: " << (this->VectorMode == VTK_USE_VECTOR ? "Orient by vector\n" : "Orient by normal\n");
}

