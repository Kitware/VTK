/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph3D.cxx
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
#include "vtkGlyph3D.h"
#include "vtkTransform.h"
#include "vtkVectors.h"
#include "vtkNormals.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

//------------------------------------------------------------------------
vtkGlyph3D* vtkGlyph3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGlyph3D");
  if(ret)
    {
    return (vtkGlyph3D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGlyph3D;
}

// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkGlyph3D::vtkGlyph3D()
{
  this->Scaling = 1;
  this->ColorMode = VTK_COLOR_BY_SCALE;
  this->ScaleMode = VTK_SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->VectorMode = VTK_USE_VECTOR;
  this->Clamping = 0;
  this->IndexMode = VTK_INDEXING_OFF;
  this->NumberOfRequiredInputs = 1;
}

vtkGlyph3D::~vtkGlyph3D()
{
}

void vtkGlyph3D::Execute()
{
  vtkPointData *pd;
  vtkScalars *inScalars;
  vtkVectors *inVectors;
  int requestedGhostLevel;
  unsigned char* inGhostLevels=0;
  vtkNormals *inNormals, *sourceNormals = NULL;
  vtkDataArray *newScalarsData = NULL, *inScalarsData = NULL;
  int numPts, numSourcePts, numSourceCells;
  int inPtId, i, index;
  vtkPoints *sourcePts = NULL;
  vtkPoints *newPts;
  vtkScalars *newScalars=NULL;
  vtkVectors *newVectors=NULL;
  vtkNormals *newNormals=NULL;
  float *x, *v = NULL, vNew[3], s = 0.0, vMag = 0.0, value;
  vtkTransform *trans = vtkTransform::New();
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts;
  vtkIdList *pts;
  int haveVectors, haveNormals, ptIncr, cellId;
  float scalex,scaley,scalez, den;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkDataSet *input = this->GetInput();
  int numberOfSources = this->GetNumberOfSources();
  vtkPolyData *defaultSource = NULL;
  
  vtkDebugMacro(<<"Generating glyphs");

  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  pd = input->GetPointData();
  inScalars = pd->GetScalars();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();

  vtkDataArray* temp = 0;
  if (pd)
    {
    temp = pd->GetArray("vtkGhostLevels");
    }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
    {
    vtkDebugMacro("No appropriate ghost levels field available.");
    }
  else
    {
    inGhostLevels = ((vtkUnsignedCharArray*)temp)->GetPointer(0);
    }

  requestedGhostLevel = output->GetUpdateGhostLevel();
  
  
  numPts = input->GetNumberOfPoints();
  if (numPts < 1)
    {
    vtkDebugMacro(<<"No points to glyph!");
    return;
    }

  // Check input for consistency
  //
  if ( (den = this->Range[1] - this->Range[0]) == 0.0 )
    {
    den = 1.0;
    }
  if ( this->VectorMode != VTK_VECTOR_ROTATION_OFF &&
       ((this->VectorMode == VTK_USE_VECTOR && inVectors != NULL) ||
        (this->VectorMode == VTK_USE_NORMAL && inNormals != NULL)) )
    {
    haveVectors = 1;
    }
  else
    {
    haveVectors = 0;
    }

  if ( (this->IndexMode == VTK_INDEXING_BY_SCALAR && !inScalars) ||
       (this->IndexMode == VTK_INDEXING_BY_VECTOR &&
       ((!inVectors && this->VectorMode == VTK_USE_VECTOR) ||
        (!inNormals && this->VectorMode == VTK_USE_NORMAL))) )
    {
    if ( this->GetSource(0) == NULL )
      {
      vtkErrorMacro(<<"Indexing on but don't have data to index with");
      pts->Delete();
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
  outputPD->CopyScalarsOff();
  outputPD->CopyVectorsOff();
  outputPD->CopyNormalsOff();

  if (!this->GetSource(0))
    {
    defaultSource = vtkPolyData::New();
    defaultSource->Allocate();
    vtkPoints *defaultPoints = vtkPoints::New();
    defaultPoints->Allocate(6);
    defaultPoints->InsertNextPoint(0, 0, 0);
    defaultPoints->InsertNextPoint(1, 0, 0);
    vtkIdType defaultPointIds[2];
    defaultPointIds[0] = 0;
    defaultPointIds[1] = 1;
    defaultSource->SetPoints(defaultPoints);
    defaultSource->InsertNextCell(VTK_LINE, 2, defaultPointIds);
    defaultSource->SetUpdateExtent(0, 1, 0);
    this->SetSource(defaultSource);
    defaultSource->Delete();
    defaultSource = NULL;
    defaultPoints->Delete();
    defaultPoints = NULL;
    }
  
  if ( this->IndexMode != VTK_INDEXING_OFF )
    {
    pd = NULL;
    numSourcePts = numSourceCells = 0;
    haveNormals = 1;
    for (numSourcePts=numSourceCells=i=0; i < numberOfSources; i++)
      {
      if ( this->GetSource(i) != NULL )
        {
        numSourcePts += this->GetSource(i)->GetNumberOfPoints();
        numSourceCells += this->GetSource(i)->GetNumberOfCells();
        if ( !(sourceNormals = this->GetSource(i)->GetPointData()->GetNormals()) )
          {
          haveNormals = 0;
          }
        }
      }
    }
  else
    {
    sourcePts = this->GetSource(0)->GetPoints();
    numSourcePts = sourcePts->GetNumberOfPoints();
    numSourceCells = this->GetSource(0)->GetNumberOfCells();

    sourceNormals = this->GetSource(0)->GetPointData()->GetNormals();
    if ( sourceNormals )
      {
      haveNormals = 1;
      }
    else
      {
      haveNormals = 0;
      }

    // Prepare to copy output.
    pd = this->GetSource(0)->GetPointData();
    outputPD->CopyAllocate(pd,numPts*numSourcePts);
    }

  newPts = vtkPoints::New();
  newPts->Allocate(numPts*numSourcePts);
  if ( this->ColorMode == VTK_COLOR_BY_SCALAR && inScalars )
    {
    newScalars = (vtkScalars *) inScalars->MakeObject ();
    newScalars->Allocate(numPts*numSourcePts);
    newScalarsData = newScalars->GetData();
    inScalarsData = inScalars->GetData();
    newScalarsData->SetName(inScalarsData->GetName());
    }
  else if ( (this->ColorMode == VTK_COLOR_BY_SCALE) && inScalars)
    {
    newScalars = vtkScalars::New();
    newScalars->Allocate(numPts*numSourcePts);
    newScalars->GetData()->SetName("GlyphScale");
    }
  else if ( (this->ColorMode == VTK_COLOR_BY_VECTOR) && haveVectors)
    {
    newScalars = vtkScalars::New();
    newScalars->Allocate(numPts*numSourcePts);
    newScalars->GetData()->SetName("VectorMagnitude");
    }
  if ( haveVectors )
    {
    newVectors = vtkVectors::New();
    newVectors->Allocate(numPts*numSourcePts);
    newVectors->GetData()->SetName("GlyphVector");
    }
  if ( haveNormals )
    {
    newNormals = vtkNormals::New();
    newNormals->Allocate(numPts*numSourcePts);
    newNormals->GetData()->SetName("Normals");
    }

  // Setting up for calls to PolyData::InsertNextCell()
  output->Allocate(3*numPts*numSourceCells,numPts*numSourceCells);

  // Traverse all Input points, transforming Source points and copying
  // point attributes.
  //
  ptIncr=0;
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    scalex = scaley = scalez = 1.0;
    if ( ! (inPtId % 10000) )
      {
      this->UpdateProgress ((float)inPtId/numPts);
      if (this->GetAbortExecute())
        {
              break;
        }
      }

    // Get the scalar and vector data
    if ( inScalars )
      {
      s = inScalars->GetScalar(inPtId);
      if ( this->ScaleMode == VTK_SCALE_BY_SCALAR ||
                this->ScaleMode == VTK_DATA_SCALING_OFF )
        {
        scalex = scaley = scalez = s;
        }
      }
    
    if ( haveVectors )
      {
      if ( this->VectorMode == VTK_USE_NORMAL )
        {
        v = inNormals->GetNormal(inPtId);
        }
      else
        {
        v = inVectors->GetVector(inPtId);
        }
      vMag = vtkMath::Norm(v);
      if ( this->ScaleMode == VTK_SCALE_BY_VECTORCOMPONENTS )
        {
        scalex = v[0];
        scaley = v[1];
        scalez = v[2];
        }
      else if ( this->ScaleMode == VTK_SCALE_BY_VECTOR )
        {
        scalex = scaley = scalez = vMag;
        }
      }
    
    // Clamp data scale if enabled
    if ( this->Clamping )
      {
      scalex = (scalex < this->Range[0] ? this->Range[0] :
                (scalex > this->Range[1] ? this->Range[1] : scalex));
      scalex = (scalex - this->Range[0]) / den;
      scaley = (scaley < this->Range[0] ? this->Range[0] :
                (scaley > this->Range[1] ? this->Range[1] : scaley));
      scaley = (scaley - this->Range[0]) / den;
      scalez = (scalez < this->Range[0] ? this->Range[0] :
                (scalez > this->Range[1] ? this->Range[1] : scalez));
      scalez = (scalez - this->Range[0]) / den;
      }
    
    // Compute index into table of glyphs
    if ( this->IndexMode == VTK_INDEXING_OFF )
      {
      index = 0;
      }
    else 
      {
      if ( this->IndexMode == VTK_INDEXING_BY_SCALAR )
        {
        value = s;
        }
      else
        {
        value = vMag;
        }
      
      index = (int) ((float)(value - this->Range[0]) * numberOfSources / den);
      index = (index < 0 ? 0 :
              (index >= numberOfSources ? (numberOfSources-1) : index));
      
      if ( this->GetSource(index) != NULL )
        {
        sourcePts = this->GetSource(index)->GetPoints();
        sourceNormals = this->GetSource(index)->GetPointData()->GetNormals();
        numSourcePts = sourcePts->GetNumberOfPoints();
        numSourceCells = this->GetSource(index)->GetNumberOfCells();
        }
      }
    
    // Make sure we're not indexing into empty glyph
    if ( this->GetSource(index) == NULL )
      {
      continue;
      }

    // Check ghost points.
    // If we are processing a piece, we do not want to duplicate 
    // glyphs on the borders.  The corrct check here is:
    // ghostLevel > 0.  I am leaving this over glyphing here because
    // it make a nice example (sphereGhost.tcl) to show the 
    // point ghost levels with the glyph filter.  I am not certain 
    // of the usefullness of point ghost levels over 1, but I will have
    // to think about it.
    if (inGhostLevels && inGhostLevels[inPtId] > requestedGhostLevel)
      {
      continue;
      }
    
    // Now begin copying/transforming glyph
    trans->Identity();
    
    // Copy all topology (transformation independent)
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->GetSource(index)->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (pts->Reset(), i=0; i < npts; i++) 
        {
        pts->InsertId(i,cellPts->GetId(i) + ptIncr);
        }
      output->InsertNextCell(cell->GetCellType(),pts);
      }
    
    // translate Source to Input point
    x = input->GetPoint(inPtId);
    trans->Translate(x[0], x[1], x[2]);
    
    if ( haveVectors )
      {
      // Copy Input vector
      for (i=0; i < numSourcePts; i++) 
        {
        newVectors->InsertVector(i+ptIncr, v);
        }
      if (this->Orient && (vMag > 0.0))
        {
        // if there is no y or z component
        if ( v[1] == 0.0 && v[2] == 0.0 )
          {
          if (v[0] < 0) //just flip x if we need to
            {
            trans->RotateWXYZ(180.0,0,1,0);
            }
          }
        else
          {
          vNew[0] = (v[0]+vMag) / 2.0;
          vNew[1] = v[1] / 2.0;
          vNew[2] = v[2] / 2.0;
          trans->RotateWXYZ((float)180.0,vNew[0],vNew[1],vNew[2]);
          }
        }
      }
    
    // determine scale factor from scalars if appropriate
    if ( inScalars )
      {
      // Copy scalar value
      if (this->ColorMode == VTK_COLOR_BY_SCALE)
        {
        for (i=0; i < numSourcePts; i++) 
          {
          newScalars->InsertScalar(i+ptIncr, scalex); // = scaley = scalez
          }
        }
      else if (this->ColorMode == VTK_COLOR_BY_SCALAR)
        {
              for (i=0; i < numSourcePts; i++)
          {
          outputPD->CopyTuple(inScalarsData, newScalarsData, inPtId, ptIncr+i);
          }
        }
      }
    if (haveVectors && this->ColorMode == VTK_COLOR_BY_VECTOR)
      {
      for (i=0; i < numSourcePts; i++) 
        {
        newScalars->InsertScalar(i+ptIncr, vMag);
        }
      }
    
    // scale data if appropriate
    if ( this->Scaling )
      {
      if ( this->ScaleMode == VTK_DATA_SCALING_OFF )
        {
        scalex = scaley = scalez = this->ScaleFactor;
        }
      else
        {
        scalex *= this->ScaleFactor;
        scaley *= this->ScaleFactor;
        scalez *= this->ScaleFactor;
        }
      
      if ( scalex == 0.0 )
        {
        scalex = 1.0e-10;
        }
      if ( scaley == 0.0 )
        {
        scaley = 1.0e-10;
        }
      if ( scalez == 0.0 )
        {
        scalez = 1.0e-10;
        }
      trans->Scale(scalex,scaley,scalez);
      }
    
    // multiply points and normals by resulting matrix
    trans->TransformPoints(sourcePts,newPts);
    
    if ( haveNormals )
      {
      trans->TransformNormals(sourceNormals,newNormals);
      }
    
    // Copy point data from source (if possible)
    if ( pd ) 
      {
      for (i=0; i < numSourcePts; i++)
        {
        outputPD->CopyData(pd,i,ptIncr+i);
        }
      }
    ptIncr += numSourcePts;
    } 
  
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
  trans->Delete();
  pts->Delete();
}

//----------------------------------------------------------------------------
// Since indexing determines size of outputs, EstimatedWholeMemorySize is
// truly an estimate.  Ignore Indexing (although for a best estimate we
// should average the size of the sources instead of using 0).
void vtkGlyph3D::ExecuteInformation()
{
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }
}


// Set the number of source objects in the glyph table. This should be
// done prior to specifying more than one source.
void vtkGlyph3D::SetNumberOfSources(int num)
{
  // one more because input has index 0.
  this->SetNumberOfInputs(num+1);
}

int vtkGlyph3D::GetNumberOfSources()
{
  // one less because input has index 0.
  return this->NumberOfInputs - 1;
}

// Specify a source object at a specified table location.
void vtkGlyph3D::SetSource(int id, vtkPolyData *pd)
{
  if (id < 0)
    {
    vtkErrorMacro("Bad index " << id << " for source.");
    return;
    }
  this->vtkProcessObject::SetNthInput(id + 1, pd);
}

// Get a pointer to a source object at a specified table location.
vtkPolyData *vtkGlyph3D::GetSource(int id)
{
  if ( id < 0 || id >= this->GetNumberOfSources() )
    {
    return NULL;
    }
  else
    {
    return (vtkPolyData *)this->Inputs[id+1];
    }
}

void vtkGlyph3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;

  if ( this->GetNumberOfSources() < 2 )
    {
    if ( this->GetSource(0) != NULL )
      {
      os << indent << "Source: (" << this->GetSource(0) << ")\n";
      }
    else
      {
      os << indent << "Source: (none)\n";
      }
    }
  else
    {
    os << indent << "A table of " << this->GetNumberOfSources() << " glyphs has been defined\n";
    }

  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
  
  os << indent << "Scale Mode: ";
  if ( this->ScaleMode == VTK_SCALE_BY_SCALAR )
    {
    os << "Scale by scalar\n";
    }
  else if ( this->ScaleMode == VTK_SCALE_BY_VECTOR )
    {
    os << "Scale by vector\n";
    }
  else
    {
    os << "Data scaling is turned off\n";
    }

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Clamping: " << (this->Clamping ? "On\n" : "Off\n");
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");
  os << indent << "Orient Mode: " << (this->VectorMode == VTK_USE_VECTOR ? 
                                       "Orient by vector\n" : "Orient by normal\n");
  os << indent << "Index Mode: ";
  if ( this->IndexMode == VTK_INDEXING_BY_SCALAR )
    {
    os << "Index by scalar value\n";
    }
  else if ( this->IndexMode == VTK_INDEXING_BY_VECTOR )
    {
    os << "Index by vector value\n";
    }
  else
    {
    os << "Indexing off\n";
    }
}

void vtkGlyph3D::ComputeInputUpdateExtents( vtkDataObject *output )
{
  vtkPolyData *outPd;

  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  output = output;
  outPd = this->GetOutput();
  if (this->GetSource())
    {
    this->GetSource()->SetUpdateExtent(0, 1, 0);
    }
  this->GetInput()->SetUpdateExtent(outPd->GetUpdatePiece(), 
                                    outPd->GetUpdateNumberOfPieces(),
                                    outPd->GetUpdateGhostLevel());
  this->GetInput()->RequestExactExtentOn();
}
