/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph3D.cxx
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
#include "vtkGlyph3D.h"
#include "vtkTransform.h"
#include "vtkFloatVectors.h"
#include "vtkFloatNormals.h"
#include "vtkMath.h"

// Description
// Construct object with scaling on, scaling mode is by scalar value, 
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkGlyph3D::vtkGlyph3D()
{
  this->NumberOfSources = 1;
  this->Source = new vtkPolyData *[1]; this->Source[0] = NULL;
  this->Scaling = 1;
  this->ScaleMode = VTK_SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->VectorMode = VTK_USE_VECTOR;
  this->Clamping = 0;
  this->IndexMode = VTK_INDEXING_OFF;
}

vtkGlyph3D::~vtkGlyph3D()
{
  if ( this->Source ) delete [] this->Source;
}

void vtkGlyph3D::Execute()
{
  vtkPointData *pd;
  vtkScalars *inScalars;
  vtkVectors *inVectors;
  vtkNormals *inNormals, *sourceNormals;
  int numPts, numSourcePts, numSourceCells;
  int inPtId, i, index;
  vtkPoints *sourcePts;
  vtkFloatPoints *newPts;
  vtkFloatScalars *newScalars=NULL;
  vtkFloatVectors *newVectors=NULL;
  vtkFloatNormals *newNormals=NULL;
  float *x, *v, vNew[3], s, vMag, value;
  vtkTransform trans;
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts;
  vtkIdList pts(VTK_CELL_SIZE);
  int haveVectors, haveNormals, ptIncr, cellId;
  float scale = 1.0;
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
  // Check input for consistency
  //
  if ( (den = this->Range[1] - this->Range[0]) == 0.0 ) den = 1.0;
    
  if ( (this->VectorMode == VTK_USE_VECTOR && inVectors != NULL) ||
  (this->VectorMode == VTK_USE_NORMAL && inNormals != NULL) )
    haveVectors = 1;
  else
    haveVectors = 0;

  if ( (this->IndexMode == VTK_INDEXING_BY_SCALAR && !inScalars) ||
  (this->IndexMode == VTK_INDEXING_BY_VECTOR && 
  ((!inVectors && this->VectorMode == VTK_USE_VECTOR) ||
  (!inNormals && this->VectorMode == VTK_USE_NORMAL))) )
    {
    if ( this->Source[0] == NULL )
      {
      vtkErrorMacro(<<"Indexing on but don't have data to index with");
      return;
      }
    else
      {
      vtkWarningMacro(<<"Turning indexing off: no data to index with");
      this->IndexMode = VTK_INDEXING_OFF;
      }
    }
    
  // Allocate storage for output PolyData
  //
  if ( this->IndexMode != VTK_INDEXING_OFF )
    {
    numSourcePts = numSourceCells = 0;
    haveNormals = 1;
    for (numSourcePts=numSourceCells=i=0; i < this->NumberOfSources; i++)
      {
      if ( this->Source[i] != NULL )
        {
        numSourcePts += this->Source[i]->GetNumberOfPoints();
        numSourceCells += this->Source[i]->GetNumberOfCells();
        if ( !(sourceNormals = this->Source[i]->GetPointData()->GetNormals()) )
          haveNormals = 0;
        }
      }
    }
  else
    {
    sourcePts = this->Source[0]->GetPoints();
    numSourcePts = sourcePts->GetNumberOfPoints();
    numSourceCells = this->Source[0]->GetNumberOfCells();

    sourceNormals = this->Source[0]->GetPointData()->GetNormals();
    if ( sourceNormals ) haveNormals = 1;
    else haveNormals = 0;
    }

  newPts = vtkFloatPoints::New();
  newPts->Allocate(numPts*numSourcePts);
  if ( inScalars )
    {
    newScalars = vtkFloatScalars::New();
    newScalars->Allocate(numPts*numSourcePts);
    }
  if ( haveVectors )
    {
    newVectors = vtkFloatVectors::New();
    newVectors->Allocate(numPts*numSourcePts);
    }
  if ( haveNormals )
    {
    newNormals = vtkFloatNormals::New();
    newNormals->Allocate(numPts*numSourcePts);
    }

  // Setting up for calls to PolyData::InsertNextCell()
  output->Allocate(numPts*numSourceCells,numPts);
    
  // Prepare to copy output
  outputPD->CopyScalarsOff();
  outputPD->CopyVectorsOff();
  outputPD->CopyNormalsOff();
  outputPD->CopyAllocate(pd,numPts*numSourcePts);

  //
  // Traverse all Input points, transforming Source points and copying 
  // point attributes.
  //
  for (ptIncr=0, inPtId=0; inPtId < numPts; inPtId++, ptIncr += numSourcePts)
    {

    // Get the scalar and vector data
    if ( inScalars ) 
      {
      s = inScalars->GetScalar(inPtId);
      if ( this->ScaleMode == VTK_SCALE_BY_SCALAR || this->ScaleMode == VTK_DATA_SCALING_OFF )
        scale = s;
      }

    if ( haveVectors )
      {
      if ( this->VectorMode == VTK_USE_NORMAL ) v = inNormals->GetNormal(inPtId);
      else v = inVectors->GetVector(inPtId);
      vMag = vtkMath::Norm(v);
      if ( this->ScaleMode == VTK_SCALE_BY_VECTOR ) scale = vMag;
      }

    // Clamp data scale if enabled
    if ( this->Clamping )
      {
      scale = (scale < this->Range[0] ? this->Range[0] :
              (scale > this->Range[1] ? this->Range[1] : scale));
      scale = (scale - this->Range[0]) / den;
      }

    // Compute index into table of glyphs
    if ( this->IndexMode == VTK_INDEXING_OFF )
      {
      index = 0;
      }
    else 
      {
      if ( this->IndexMode == VTK_INDEXING_BY_SCALAR ) value = s;
      else value = vMag;

      index = (int) ((float)(value - this->Range[0]) * (this->NumberOfSources-1) / den);
      index = (index < 0 ? 0 : 
              (index >= this->NumberOfSources ? (this->NumberOfSources-1) : index));

      if ( this->Source[index] != NULL )
        {
        sourcePts = this->Source[index]->GetPoints();
        sourceNormals = this->Source[index]->GetPointData()->GetNormals();
        numSourcePts = sourcePts->GetNumberOfPoints();
        numSourceCells = this->Source[index]->GetNumberOfCells();
        }
      }

    // Make sure we're not indexing into empty glyph
    if ( this->Source[index] == NULL ) continue;

    // Now begin copying/transforming glyph
    trans.Identity();

    // Copy all topology (transformation independent)
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->Source[index]->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (pts.Reset(), i=0; i < npts; i++) pts.InsertId(i,cellPts->GetId(i) + ptIncr);
      output->InsertNextCell(cell->GetCellType(),pts);
      }

    // translate Source to Input point
    x = this->Input->GetPoint(inPtId);
    trans.Translate(x[0], x[1], x[2]);

    if ( haveVectors )
      {
      // Copy Input vector
      for (i=0; i < numSourcePts; i++) 
        newVectors->InsertVector(i+ptIncr, v);
          
      if (this->Orient && (vMag > 0.0)) 
        {
        // if there is no y or z component
        if ( v[1] == 0.0 && v[2] == 0.0 )
          {
          if (v[0] < 0) //just flip x if we need to
            {
            trans.RotateWXYZ(180.0,0,1,0);
            }
          }
        else
         {
         vNew[0] = (v[0]+vMag) / 2.0;
         vNew[1] = v[1] / 2.0;
         vNew[2] = v[2] / 2.0;
         trans.RotateWXYZ(180.0,vNew[0],vNew[1],vNew[2]);
         }
        }
      }

    // determine scale factor from scalars if appropriate
    if ( inScalars )
      {
      // Copy scalar value
      for (i=0; i < numSourcePts; i++) 
        newScalars->InsertScalar(i+ptIncr, scale);
      }

    // scale data if appropriate
    if ( this->Scaling )
      {
      if ( this->ScaleMode == VTK_DATA_SCALING_OFF ) scale = this->ScaleFactor;
      else scale *= this->ScaleFactor;

      if ( scale == 0.0 ) scale = 1.0e-10;
      trans.Scale(scale,scale,scale);
      }

    // multiply points and normals by resulting matrix
    trans.MultiplyPoints(sourcePts,newPts);
    if ( haveNormals ) trans.MultiplyNormals(sourceNormals,newNormals);

    // Copy point data from source
    for (i=0; i < numSourcePts; i++) outputPD->CopyData(pd,i,ptIncr+i);
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
  int i;
  unsigned long int mtime, latest;

  // make sure input is available
  for (i=0; i<this->NumberOfSources; i++)
    if ( this->Source[i] != NULL ) 
      break;

  if ( this->Input == NULL || (this->IndexMode == VTK_INDEXING_OFF && i != 0) ||
  i >= this->NumberOfSources )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  for (i=0; i<this->NumberOfSources; i++)
    {
    if ( this->Source[i] != NULL ) this->Source[i]->Update();
    }
  this->Updating = 0;

  // get source modified time
  for (i=0; i<this->NumberOfSources; i++)
    {
    latest = 0;
    if ( this->Source[i] != NULL ) 
      {
      mtime = this->Source[i]->GetMTime();
      if ( mtime > latest ) latest = mtime;
      }
    }

  if (this->Input->GetMTime() > this->ExecuteTime || latest > this->ExecuteTime || 
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() ) this->Input->ForceUpdate();
    for (i=0; i<this->NumberOfSources; i++)
      {
      if ( this->Source[i] != NULL && this->Source[i]->GetDataReleased() ) 
        this->Source[i]->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  for (i=0; i<this->NumberOfSources; i++)
    {
    if ( this->Source[i] != NULL && this->Source[i]->ShouldIReleaseData() ) 
        this->Source[i]->ReleaseData();
    }
}

// Description:
// Set the number of source objects in the glyph table. This should be
// done prior to specifying more than one source.
void vtkGlyph3D::SetNumberOfSources(int num)
{
  if ( num < 1 ) num = 1;
  this->Source = new vtkPolyData *[num];
  this->NumberOfSources = num;
}

// Description:
// Specify a source object at a specified table location.
void vtkGlyph3D::SetSource(int id, vtkPolyData *pd)
{
  if ( id < 0 || id >= this->NumberOfSources )
    {
    vtkErrorMacro(<<"Specify index between (0,NumberOfSources-1)");
    return;
    }

  this->Source[id] = pd;
}

// Description:
// Get a pointer to a source object at a specified table location.
vtkPolyData *vtkGlyph3D::GetSource(int id)
{
  if ( id < 0 || id >= this->NumberOfSources )
    {
    vtkErrorMacro(<<"Trying to retrieve undefiend source");
    return NULL;
    }
  else
    {
    return this->Source[id];
    }
}

void vtkGlyph3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  if ( this->NumberOfSources < 2 )
    {
    if ( this->Source[0] != NULL ) os << indent << "Source: (" << this->Source[0] << ")\n";
    else os << indent << "Source: (none)\n";
    }
  else
    {
    os << indent << "A table of " << this->NumberOfSources << " glyphs has been defined\n";
    }

  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");

  os << indent << "Scale Mode: ";
  if ( this->ScaleMode == VTK_SCALE_BY_SCALAR ) os << "Scale by scalar\n";
  else if ( this->ScaleMode == VTK_SCALE_BY_VECTOR ) os << "Scale by vector\n";
  else os << "Data scaling is turned off\n";

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Clamping: " << (this->Clamping ? "On\n" : "Off\n");
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");
  os << indent << "Orient Mode: " << (this->VectorMode == VTK_USE_VECTOR ? 
                                       "Orient by vector\n" : "Orient by normal\n");
  os << indent << "Index Mode: ";
  if ( this->IndexMode == VTK_INDEXING_BY_SCALAR )
    os << "Index by scalar value\n";
  else if ( this->IndexMode == VTK_INDEXING_BY_VECTOR )
    os << "Index by vector value\n";
  else
    os << "Indexing off\n";
}

