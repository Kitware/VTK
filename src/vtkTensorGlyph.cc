/*=========================================================================

  Program:   Visualization Library
  Module:    vtkTensorGlyph.cc
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
#include "vtkTensorGlyph.hh"
#include "vtkTransform.hh"
#include "vtkFloatVectors.hh"
#include "vtkFloatNormals.hh"
#include "vtkMath.hh"

// Description
// Construct object with scaling on and scale factor 1.0. Eigenvalues are 
// extracted, glyphs are colored with input scalar data, and logarithmic
// scaling is turned off.
vtkTensorGlyph::vtkTensorGlyph()
{
  this->Source = NULL;
  this->Scaling = 1;
  this->ScaleFactor = 1.0;
  this->ExtractEigenvalues = 1;
  this->ColorGlyphs = 1;
  this->ClampScaling = 0;
  this->MaxScaleFactor = 100;
}

void vtkTensorGlyph::Execute()
{
  vtkTensors *inTensors;
  vtkTensor *tensor;
  vtkScalars *inScalars;
  int numPts, numSourcePts, numSourceCells;
  int inPtId, i, j;
  vtkPoints *sourcePts;
  vtkNormals *sourceNormals;
  vtkCellArray *sourceCells, *cells;  
  vtkFloatPoints *newPts;
  vtkFloatScalars *newScalars=NULL;
  vtkFloatNormals *newNormals=NULL;
  float *x, s;
  vtkTransform trans;
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts, *pts=new int[this->Source->GetMaxCellSize()];
  int ptIncr, cellId;
  vtkMath math;
  vtkMatrix4x4 matrix;
  float *m[3], w[3], *v[3];
  float m0[3], m1[3], m2[3];
  float v0[3], v1[3], v2[3];
  float xv[3], yv[3], zv[3];
  float maxScale;
  vtkPointData *pd, *outPD;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  
  // set up working matrices
  m[0] = m0; m[1] = m1; m[2] = m2; 
  v[0] = v0; v[1] = v1; v[2] = v2; 

  vtkDebugMacro(<<"Generating tensor glyphs");

  pd = this->Input->GetPointData();
  outPD = output->GetPointData();
  inTensors = pd->GetTensors();
  inScalars = pd->GetScalars();
  numPts = this->Input->GetNumberOfPoints();

  if ( !inTensors || numPts < 1 )
    {
    vtkErrorMacro(<<"No data to glyph!");
    return;
    }
//
// Allocate storage for output PolyData
//
  sourcePts = this->Source->GetPoints();
  numSourcePts = sourcePts->GetNumberOfPoints();
  numSourceCells = this->Source->GetNumberOfCells();

  newPts = new vtkFloatPoints(numPts*numSourcePts);

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

  // only copy scalar data through
  pd = this->Source->GetPointData();
  if ( inScalars &&  this->ColorGlyphs ) 
    newScalars = new vtkFloatScalars(numPts*numSourcePts);
  else
    {
    outPD->CopyAllOff();
    outPD->CopyScalarsOn();
    outPD->CopyAllocate(pd,numPts*numSourcePts);
    }
  if ( sourceNormals = pd->GetNormals() )
    newNormals = new vtkFloatNormals(numPts*numSourcePts);
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
      for (i=0; i < npts; i++) pts[i] = cellPts->GetId(i) + ptIncr;
      output->InsertNextCell(cell->GetCellType(),npts,pts);
      }
    }
//
// Traverse all Input points, transforming glyph at Source points
//
  trans.PreMultiply();

  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    
    trans.Identity();

    // translate Source to Input point
    x = this->Input->GetPoint(inPtId);
    trans.Translate(x[0], x[1], x[2]);

    tensor = inTensors->GetTensor(inPtId);

    // compute orientation vectors and scale factors from tensor
    if ( this->ExtractEigenvalues ) // extract appropriate eigenfunctions
      {
      for (j=0; j<3; j++)
        for (i=0; i<3; i++)
          m[i][j] = tensor->GetComponent(i,j);

      math.Jacobi(m, w, v);

      //copy eigenvectors
      xv[0] = v[0][0]; xv[1] = v[1][0]; xv[2] = v[2][0];
      yv[0] = v[0][1]; yv[1] = v[1][1]; yv[2] = v[2][1];
      zv[0] = v[0][2]; zv[1] = v[1][2]; zv[2] = v[2][2];
      }
    else //use tensor columns as eigenvectors
      {
      for (i=0; i<3; i++)
        {
        xv[i] = tensor->GetComponent(i,0);
        yv[i] = tensor->GetComponent(i,1);
        zv[i] = tensor->GetComponent(i,2);
        }
      w[0] = math.Normalize(xv);
      w[1] = math.Normalize(yv);
      w[2] = math.Normalize(zv);
      }

    // compute scale factors
    w[0] *= this->ScaleFactor;
    w[1] *= this->ScaleFactor;
    w[2] *= this->ScaleFactor;
    
    if ( this->ClampScaling )
      {
      for (maxScale=0.0, i=0; i<3; i++)
        if ( maxScale < fabs(w[i]) ) maxScale = fabs(w[i]);

      if ( maxScale > this->MaxScaleFactor )
        {
        maxScale = this->MaxScaleFactor / maxScale;
        for (i=0; i<3; i++)
          w[i] *= maxScale; //preserve overall shape of glyph
        }
      }

    // normalized eigenvectors rotate object
    matrix.Element[0][0] = xv[0];
    matrix.Element[0][1] = yv[0];
    matrix.Element[0][2] = zv[0];
    matrix.Element[1][0] = xv[1];
    matrix.Element[1][1] = yv[1];
    matrix.Element[1][2] = zv[1];
    matrix.Element[2][0] = xv[2];
    matrix.Element[2][1] = yv[2];
    matrix.Element[2][2] = zv[2];
    trans.Concatenate(matrix);

    // make sure scale is okay (non-zero) and scale data
    for (maxScale=0.0, i=0; i<3; i++)
      if ( w[i] > maxScale ) maxScale = w[i];
    if ( maxScale == 0.0 ) maxScale = 1.0;
    for (i=0; i<3; i++)
      if ( w[i] == 0.0 ) w[i] = maxScale * 1.0e-06;
    trans.Scale(w[0], w[1], w[2]);

    // multiply points (and normals if available) by resulting matrix
    trans.MultiplyPoints(sourcePts,newPts);
    if ( newNormals )
      trans.MultiplyNormals(sourceNormals,newNormals);

    // Copy point data from source
    if ( inScalars && this->ColorGlyphs ) 
      {
      s = inScalars->GetScalar(inPtId);
      for (i=0; i < numSourcePts; i++) 
        newScalars->InsertScalar(ptIncr+i, s);
      }
    else
      {
      for (i=0; i < numSourcePts; i++) 
        outPD->CopyData(pd,i,ptIncr+i);
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
}

// Description:
// Override update method because execution can branch two ways (via Input 
// and Source objects).
void vtkTensorGlyph::Update()
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

  if ( this->Input->GetMTime() > this->ExecuteTime || 
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

void vtkTensorGlyph::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Source: " << this->Source << "\n";
  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Extract Eigenvalues: " << (this->ExtractEigenvalues ? "On\n" : "Off\n");
  os << indent << "Color Glyphs: " << (this->ColorGlyphs ? "On\n" : "Off\n");
  os << indent << "Clamp Scaling: " << (this->ClampScaling ? "On\n" : "Off\n");
  os << indent << "Max Scale Factor: " << this->MaxScaleFactor << "\n";
}

