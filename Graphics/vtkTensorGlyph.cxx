/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorGlyph.cxx
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
#include "vtkTensorGlyph.h"
#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//----------------------------------------------------------------------------
vtkTensorGlyph* vtkTensorGlyph::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTensorGlyph");
  if(ret)
    {
    return (vtkTensorGlyph*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTensorGlyph;
}

// Construct object with scaling on and scale factor 1.0. Eigenvalues are 
// extracted, glyphs are colored with input scalar data, and logarithmic
// scaling is turned off.
vtkTensorGlyph::vtkTensorGlyph()
{
  this->Scaling = 1;
  this->ScaleFactor = 1.0;
  this->ExtractEigenvalues = 1;
  this->ColorGlyphs = 1;
  this->ClampScaling = 0;
  this->MaxScaleFactor = 100;
}

vtkTensorGlyph::~vtkTensorGlyph()
{
  this->SetSource(NULL);
}

void vtkTensorGlyph::Execute()
{
  vtkDataArray *inTensors;
  float *tensor;
  vtkDataArray *inScalars;
  vtkIdType numPts, numSourcePts, numSourceCells, inPtId, i;
  int j;
  vtkPoints *sourcePts;
  vtkDataArray *sourceNormals;
  vtkCellArray *sourceCells, *cells;  
  vtkPoints *newPts;
  vtkFloatArray *newScalars=NULL;
  vtkFloatArray *newNormals=NULL;
  float *x, s;
  vtkTransform *trans = vtkTransform::New();
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts;
  vtkIdType *pts = new vtkIdType[this->GetSource()->GetMaxCellSize()];
  vtkIdType ptIncr, cellId;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  float *m[3], w[3], *v[3];
  float m0[3], m1[3], m2[3];
  float v0[3], v1[3], v2[3];
  float xv[3], yv[3], zv[3];
  float maxScale;
  vtkPointData *pd, *outPD;
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  // set up working matrices
  m[0] = m0; m[1] = m1; m[2] = m2; 
  v[0] = v0; v[1] = v1; v[2] = v2; 

  vtkDebugMacro(<<"Generating tensor glyphs");

  pd = input->GetPointData();
  outPD = output->GetPointData();
  inTensors = pd->GetActiveTensors();
  inScalars = pd->GetActiveScalars();
  numPts = input->GetNumberOfPoints();

  if ( !inTensors || numPts < 1 )
    {
    vtkErrorMacro(<<"No data to glyph!");
    return;
    }
  //
  // Allocate storage for output PolyData
  //
  sourcePts = this->GetSource()->GetPoints();
  numSourcePts = sourcePts->GetNumberOfPoints();
  numSourceCells = this->GetSource()->GetNumberOfCells();

  newPts = vtkPoints::New();
  newPts->Allocate(numPts*numSourcePts);

  // Setting up for calls to PolyData::InsertNextCell()
  if ( (sourceCells=this->GetSource()->GetVerts())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numPts*sourceCells->GetSize());
    output->SetVerts(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->GetSource()->GetLines())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numPts*sourceCells->GetSize());
    output->SetLines(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->GetSource()->GetPolys())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numPts*sourceCells->GetSize());
    output->SetPolys(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->GetSource()->GetStrips())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numPts*sourceCells->GetSize());
    output->SetStrips(cells);
    cells->Delete();
    }

  // only copy scalar data through
  pd = this->GetSource()->GetPointData();
  if ( inScalars &&  this->ColorGlyphs ) 
    {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(numPts*numSourcePts);
    }
  else
    {
    outPD->CopyAllOff();
    outPD->CopyScalarsOn();
    outPD->CopyAllocate(pd,numPts*numSourcePts);
    }
  if ( (sourceNormals = pd->GetActiveNormals()) )
    {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3*numPts*numSourcePts);
    }
  //
  // First copy all topology (transformation independent)
  //
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->GetSource()->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (i=0; i < npts; i++)
	{
	pts[i] = cellPts->GetId(i) + ptIncr;
	}
      output->InsertNextCell(cell->GetCellType(),npts,pts);
      }
    }
  //
  // Traverse all Input points, transforming glyph at Source points
  //
  trans->PreMultiply();

  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    
    trans->Identity();

    // translate Source to Input point
    x = input->GetPoint(inPtId);
    trans->Translate(x[0], x[1], x[2]);

    tensor = inTensors->GetTuple(inPtId);

    // compute orientation vectors and scale factors from tensor
    if ( this->ExtractEigenvalues ) // extract appropriate eigenfunctions
      {
      for (j=0; j<3; j++)
	{
        for (i=0; i<3; i++)
	  {
          m[i][j] = tensor[i+3*j];
	  }
	}
      vtkMath::Jacobi(m, w, v);

      //copy eigenvectors
      xv[0] = v[0][0]; xv[1] = v[1][0]; xv[2] = v[2][0];
      yv[0] = v[0][1]; yv[1] = v[1][1]; yv[2] = v[2][1];
      zv[0] = v[0][2]; zv[1] = v[1][2]; zv[2] = v[2][2];
      }
    else //use tensor columns as eigenvectors
      {
      for (i=0; i<3; i++)
        {
        xv[i] = tensor[i];
        yv[i] = tensor[i+3]; 
        zv[i] = tensor[i+6];
        }
      w[0] = vtkMath::Normalize(xv);
      w[1] = vtkMath::Normalize(yv);
      w[2] = vtkMath::Normalize(zv);
      }

    // compute scale factors
    w[0] *= this->ScaleFactor;
    w[1] *= this->ScaleFactor;
    w[2] *= this->ScaleFactor;
    
    if ( this->ClampScaling )
      {
      for (maxScale=0.0, i=0; i<3; i++)
	{
        if ( maxScale < fabs(w[i]) )
	  {
	  maxScale = fabs(w[i]);
	  }
	}
      if ( maxScale > this->MaxScaleFactor )
        {
        maxScale = this->MaxScaleFactor / maxScale;
        for (i=0; i<3; i++)
	  {
          w[i] *= maxScale; //preserve overall shape of glyph
	  }
        }
      }

    // normalized eigenvectors rotate object
    matrix->Element[0][0] = xv[0];
    matrix->Element[0][1] = yv[0];
    matrix->Element[0][2] = zv[0];
    matrix->Element[1][0] = xv[1];
    matrix->Element[1][1] = yv[1];
    matrix->Element[1][2] = zv[1];
    matrix->Element[2][0] = xv[2];
    matrix->Element[2][1] = yv[2];
    matrix->Element[2][2] = zv[2];
    trans->Concatenate(matrix);

    // make sure scale is okay (non-zero) and scale data
    for (maxScale=0.0, i=0; i<3; i++)
      {
      if ( w[i] > maxScale )
	{
	maxScale = w[i];
	}
      }
    if ( maxScale == 0.0 )
      {
      maxScale = 1.0;
      }
    for (i=0; i<3; i++)
      {
      if ( w[i] == 0.0 )
	{
	w[i] = maxScale * 1.0e-06;
	}
      }
    trans->Scale(w[0], w[1], w[2]);

    // multiply points (and normals if available) by resulting matrix
    trans->TransformPoints(sourcePts,newPts);
    if ( newNormals )
      {
      trans->TransformNormals(sourceNormals,newNormals);
      }

    // Copy point data from source
    if ( inScalars && this->ColorGlyphs ) 
      {
      s = inScalars->GetComponent(inPtId, 0);
      for (i=0; i < numSourcePts; i++) 
	{
        newScalars->InsertTuple(ptIncr+i, &s);
	}
      }
    else
      {
      for (i=0; i < numSourcePts; i++) 
	{
        outPD->CopyData(pd,i,ptIncr+i);
	}
      }
    }
  vtkDebugMacro(<<"Generated " << numPts <<" tensor glyphs");
  //
  // Update output and release memory
  //
  delete [] pts;

  output->SetPoints(newPts);
  newPts->Delete();

  if ( newScalars )
    {
    outPD->SetScalars(newScalars);
    newScalars->Delete();
    }

  if ( newNormals )
    {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    }

  output->Squeeze();
  trans->Delete();
  matrix->Delete();
}

void vtkTensorGlyph::SetSource(vtkPolyData *source)
{
  this->vtkProcessObject::SetNthInput(1, source);
}

vtkPolyData *vtkTensorGlyph::GetSource()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  return (vtkPolyData *)(this->Inputs[1]);
  
}

void vtkTensorGlyph::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Source: " << this->GetSource() << "\n";
  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Extract Eigenvalues: " << (this->ExtractEigenvalues ? "On\n" : "Off\n");
  os << indent << "Color Glyphs: " << (this->ColorGlyphs ? "On\n" : "Off\n");
  os << indent << "Clamp Scaling: " << (this->ClampScaling ? "On\n" : "Off\n");
  os << indent << "Max Scale Factor: " << this->MaxScaleFactor << "\n";
}
