/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanPolyData.cxx
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
#include "vtkCleanPolyData.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkCleanPolyData* vtkCleanPolyData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCleanPolyData");
  if (ret)
    {
    return (vtkCleanPolyData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCleanPolyData;
}

//---------------------------------------------------------------------------
// Construct object with initial Tolerance of 0.0
vtkCleanPolyData::vtkCleanPolyData()
{
  this->ToleranceIsAbsolute  = 0;
  this->Tolerance            = 0.0;
  this->AbsoluteTolerance    = 1.0;
  this->ConvertPolysToLines  = 1;
  this->ConvertLinesToPoints = 1;
  this->ConvertStripsToPolys = 1;
  this->Locator = NULL;
}

//--------------------------------------------------------------------------
vtkCleanPolyData::~vtkCleanPolyData()
{
  ReleaseLocator();
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::OperateOnPoint(float in[3], float out[3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::OperateOnBounds(float in[6], float out[6])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
  out[3] = in[3];
  out[4] = in[4];
  out[5] = in[5];
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::Execute()
{
  vtkPolyData  *input = this->GetInput();
  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }
  vtkPoints    *inPts = input->GetPoints();
  int numPts = input->GetNumberOfPoints();
  //
  vtkDebugMacro(<<"Beginning PolyData clean");
  if ( (numPts<1) || (inPts == NULL ) )
    {
    vtkErrorMacro(<<"No data to Operate On!");
    return;
    }
  vtkIdType *updatedPts = new vtkIdType[input->GetMaxCellSize()];

  int numNewPts;
  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(numPts);

  // we'll be needing these
  int inCellID, newId;
  int i;
  vtkIdType ptId;
  int npts;
  vtkIdType *pts;
  float x[3];
  float newx[3];

  vtkCellArray *inVerts  = input->GetVerts(),  *newVerts  = NULL;
  vtkCellArray *inLines  = input->GetLines(),  *newLines  = NULL;
  vtkCellArray *inPolys  = input->GetPolys(),  *newPolys  = NULL;
  vtkCellArray *inStrips = input->GetStrips(), *newStrips = NULL;

  vtkPointData *inputPD = input->GetPointData();
  vtkCellData  *inputCD = input->GetCellData();

  // We must be careful to 'operate' on the bounds of the locator so
  // that all inserted points lie inside it
  this->CreateDefaultLocator();
  if (this->ToleranceIsAbsolute) 
    {
    this->Locator->SetTolerance(this->AbsoluteTolerance);
    } 
  else 
    {
    this->Locator->SetTolerance(this->Tolerance*input->GetLength());
    }
  float originalbounds[6], mappedbounds[6];
  input->GetBounds(originalbounds);
  this->OperateOnBounds(originalbounds,mappedbounds);
  this->Locator->InitPointInsertion(newPts, mappedbounds);

  vtkPolyData  *output   = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData  *outputCD = output->GetCellData();
  outputPD->CopyAllocate(inputPD);
  outputCD->CopyAllocate(inputCD);

  // Celldata needs to be copied correctly. If a poly is converted to
  // a line, or a line to a point, then using a CellCounter will not
  // do, as the cells should be ordered verts, lines, polys,
  // strips. We need to maintain seperate cell data lists so we can
  // copy them all correctly. Tedious but easy to implement. We can
  // use outputCD for vertex cell data, then add the rest at the end.
  vtkCellData  *outLineData = NULL;
  vtkCellData  *outPolyData = NULL;
  vtkCellData  *outStrpData = NULL;
  int vertIDcounter = 0, lineIDcounter = 0;
  int polyIDcounter = 0, strpIDcounter = 0;

  // Begin to adjust topology.
  //
  // Vertices are renumbered and we remove duplicates
  inCellID = 0;
  if ( !this->GetAbortExecute() && inVerts->GetNumberOfCells() > 0 ) 
    {
    newVerts = vtkCellArray::New();
    newVerts->Allocate(inVerts->GetSize());

    vtkDebugMacro(<<"Starting Verts "<<inCellID);
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); 
         inCellID++) 
      {
      for ( numNewPts=0, i=0; i < npts; i++ ) 
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( this->Locator->InsertUniquePoint(newx, ptId) ) 
          {
          updatedPts[numNewPts++] = ptId;
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        }
      if ( numNewPts > 0 ) 
        {
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Vertex ID fault in vertex test");
          }
        vertIDcounter++;
      }
    }
  }
  this->UpdateProgress(0.25);

  // lines reduced to one point are eliminated or made into verts
  if ( !this->GetAbortExecute() && inLines->GetNumberOfCells() > 0 ) 
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(inLines->GetSize());
    outLineData = vtkCellData::New();
    outLineData->CopyAllocate(inputCD);
    //
    vtkDebugMacro(<<"Starting Lines "<<inCellID);
    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); inCellID++)
      {
      for ( numNewPts=0, i=0; i<npts; i++ ) 
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( this->Locator->InsertUniquePoint(newx, ptId) ) 
          {
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        if ( i == 0 || ptId != updatedPts[numNewPts-1] ) 
          {
          updatedPts[numNewPts++] = ptId;
          }
        }
      if ( numNewPts>1 ) 
        {
        newId = newLines->InsertNextCell(numNewPts,updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Line ID fault in line test");
          }
        lineIDcounter++;
        } 
      else if ( (numNewPts==1) && this->ConvertLinesToPoints ) 
        {
        if (!newVerts) 
          {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
          }
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Vertex ID fault in line test");
          }
        vertIDcounter++;
        }
      }
    vtkDebugMacro(<<"Removed " 
             << inLines->GetNumberOfCells() - newLines->GetNumberOfCells() 
             << " lines");
    }
  this->UpdateProgress(0.50);

  // polygons reduced to two points or less are either eliminated
  // or converted to lines or points if enabled
  if ( !this->GetAbortExecute() && inPolys->GetNumberOfCells() > 0 ) 
    {
    newPolys = vtkCellArray::New();
    newPolys->Allocate(inPolys->GetSize());
    outPolyData = vtkCellData::New();
    outPolyData->CopyAllocate(inputCD);

    vtkDebugMacro(<<"Starting Polys "<<inCellID);
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); inCellID++)
      {
      for ( numNewPts=0, i=0; i<npts; i++ ) 
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( this->Locator->InsertUniquePoint(newx, ptId) ) 
          {
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        if ( i == 0 || ptId != updatedPts[numNewPts-1] ) 
          {
          updatedPts[numNewPts++] = ptId;
          }
        } //for points in polygon
      if ( numNewPts>2 && updatedPts[0] == updatedPts[numNewPts-1] ) 
        {
        numNewPts--;
        }
      if ( numNewPts > 2 ) 
        {
        newId = newPolys->InsertNextCell(numNewPts,updatedPts);
        outPolyData->CopyData(inputCD, inCellID, newId);
        if (polyIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Poly ID fault in poly test");
          }
        polyIDcounter++;
        } 
      else if ( (numNewPts==2) && this->ConvertPolysToLines ) 
        {
        if (!newLines) 
          {
          newLines = vtkCellArray::New();
          newLines->Allocate(5);
          outLineData = vtkCellData::New();
          outLineData->CopyAllocate(inputCD);
          }
        newId = newLines->InsertNextCell(numNewPts,updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Line ID fault in poly test");
          }
        lineIDcounter++;
        } 
      else if ( (numNewPts==1) &&
                this->ConvertPolysToLines &&
                this->ConvertLinesToPoints) 
        {
        if (!newVerts) 
          {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
          }
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Vertex ID fault in poly test");
          }
        vertIDcounter++;
        }
      }
    vtkDebugMacro(<<"Removed " 
           << inPolys->GetNumberOfCells() - newPolys->GetNumberOfCells() 
           << " polys");
    }
  this->UpdateProgress(0.75);

  // triangle strips can reduced to polys/lines/points etc
  if ( !this->GetAbortExecute() && inStrips->GetNumberOfCells() > 0 ) 
    {
    newStrips = vtkCellArray::New();
    newStrips->Allocate(inStrips->GetSize());
    outStrpData = vtkCellData::New();
    outStrpData->CopyAllocate(inputCD);

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); 
         inCellID++) 
      {
      for ( numNewPts=0, i=0; i < npts; i++ ) 
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( this->Locator->InsertUniquePoint(newx, ptId) ) 
          {
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        if ( i == 0 || ptId != updatedPts[numNewPts-1] ) 
          {
          updatedPts[numNewPts++] = ptId;
          }
        }
      if ( numNewPts > 3 ) 
        {
        newId = newStrips->InsertNextCell(numNewPts,updatedPts);
        outStrpData->CopyData(inputCD, inCellID, newId);
        if (strpIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Strip ID fault in strip test");
          }
        strpIDcounter++;
        } 
      else if ( (numNewPts==3) && this->ConvertStripsToPolys ) 
        {
        if (!newPolys) 
          {
          newPolys = vtkCellArray::New();
          newPolys->Allocate(5);
          outPolyData = vtkCellData::New();
          outPolyData->CopyAllocate(inputCD);
          }
        newId = newPolys->InsertNextCell(numNewPts,updatedPts);
        outPolyData->CopyData(inputCD, inCellID, newId);
        if (polyIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Poly ID fault in strip test");
          }
        polyIDcounter++;
        } 
      else if ( (numNewPts==2) && this->ConvertPolysToLines ) 
        {
        if (!newLines) 
          {
          newLines = vtkCellArray::New();
          newLines->Allocate(5);
          outLineData = vtkCellData::New();
          outLineData->CopyAllocate(inputCD);
          }
        newId = newLines->InsertNextCell(numNewPts,updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Line ID fault in strip test");
          }
        lineIDcounter++;
        } 
      else if ( (numNewPts==1) &&
                this->ConvertPolysToLines &&
                this->ConvertLinesToPoints) 
        {
        if (!newVerts) 
          {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
          }
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter!=newId) 
          {
          vtkErrorMacro(<<"Vertex ID fault in strip test");
          }
        vertIDcounter++;
        }
      }
    vtkDebugMacro(<<"Removed " 
              << inStrips->GetNumberOfCells() - newStrips->GetNumberOfCells() 
              << " strips");
    }

  vtkDebugMacro(<<"Removed " 
                << numPts - newPts->GetNumberOfPoints() << " points");

  // Update ourselves and release memory
  //
  delete [] updatedPts;
  this->Locator->Initialize(); //release memory.

  // Now transfer all CellData from Lines/Polys/Strips into final
  // Cell data output
  int CombinedCellID = vertIDcounter;
  if (newLines) 
    {
    for (i=0; i<lineIDcounter; i++, CombinedCellID++) 
      {
      outputCD->CopyData(outLineData, i, CombinedCellID);
      }
    outLineData->Delete();
    }
  if (newPolys) 
    {
    for (i=0; i<polyIDcounter; i++, CombinedCellID++) 
      {
      outputCD->CopyData(outPolyData, i, CombinedCellID);
      }
    outPolyData->Delete();
    }
  if (newStrips) 
    {
    for (i=0; i<strpIDcounter; i++, CombinedCellID++) 
      {
      outputCD->CopyData(outStrpData, i, CombinedCellID);
      }
    outStrpData->Delete();
    }

  output->SetPoints(newPts);
  newPts->Squeeze();
  newPts->Delete();
  if (newVerts) 
    {
    newVerts->Squeeze();
    output->SetVerts(newVerts);
    newVerts->Delete();
    }
  if (newLines) 
    {
    newLines->Squeeze();
    output->SetLines(newLines);
    newLines->Delete();
    }
  if (newPolys) 
    {
    newPolys->Squeeze();
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  if (newStrips) 
    {
    newStrips->Squeeze();
    output->SetStrips(newStrips);
    newStrips->Delete();
    }
}

//------------------------------------------------------------------------
// Specify a spatial locator for speeding the search process. By
// default an instance of vtkLocator is used.
void vtkCleanPolyData::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator)
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }

    this->Locator = locator;
    this->Modified();
}

//--------------------------------------------------------------------------
// Method manages creation of locators. It takes into account the potential
// change of tolerance (zero to non-zero).
void vtkCleanPolyData::CreateDefaultLocator() 
{
  float tol = 0.0;
  if (this->ToleranceIsAbsolute) 
    {
    tol = this->AbsoluteTolerance;
    } 
  else 
    {
    if (this->GetInput())
      {
      tol = this->Tolerance*this->GetInput()->GetLength();
      }
    else
      {
      tol = this->Tolerance;
      }
    }

  if ( this->Locator == NULL) 
    {
    if (tol==0.0) 
      {
      this->Locator = vtkMergePoints::New();
      } 
    else 
      {
      this->Locator = vtkPointLocator::New();
      }
    } 
  else 
    {
    // lets check the tolerance wasn't changed from zero to non zero
    if ((tol>0.0) && (this->GetLocator()->GetTolerance()==0.0)) 
      {
      this->ReleaseLocator();
      this->Locator = vtkPointLocator::New();
      }
    }
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::ReleaseLocator(void) 
{
  if ( this->Locator ) 
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::PrintSelf(ostream& os, vtkIndent indent) 
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
  os << indent << "ToleranceIsAbsolute: "
     << this->ToleranceIsAbsolute << "\n";
  os << indent << "Tolerance: "
     << this->Tolerance << "\n";
  os << indent << "AbsoluteTolerance: "
     << this->AbsoluteTolerance << "\n";
  os << indent << "ConvertPolysToLines: "
     << this->ConvertPolysToLines << "\n";
  os << indent << "ConvertLinesToPoints: "
     << this->ConvertLinesToPoints << "\n";
  os << indent << "ConvertStripsToPolys: "
     << this->ConvertStripsToPolys << "\n";
  if ( this->Locator ) 
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else 
    {
    os << indent << "Locator: (none)\n";
    }
}

//--------------------------------------------------------------------------
unsigned long int vtkCleanPolyData::GetMTime() 
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;
  if ( this->Locator != NULL ) 
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}
