/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometryFilter.cxx
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
#include "vtkGeometryFilter.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkPyramid.h"
#include "vtkUnsignedCharArray.h"


//----------------------------------------------------------------------------
vtkGeometryFilter* vtkGeometryFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGeometryFilter");
  if(ret)
    {
    return (vtkGeometryFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGeometryFilter;
}




// Construct with all types of clipping turned off.
vtkGeometryFilter::vtkGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = VTK_LARGE_INTEGER;

  this->CellMinimum = 0;
  this->CellMaximum = VTK_LARGE_INTEGER;

  this->Extent[0] = -VTK_LARGE_FLOAT;
  this->Extent[1] = VTK_LARGE_FLOAT;
  this->Extent[2] = -VTK_LARGE_FLOAT;
  this->Extent[3] = VTK_LARGE_FLOAT;
  this->Extent[4] = -VTK_LARGE_FLOAT;
  this->Extent[5] = VTK_LARGE_FLOAT;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;

  this->Merging = 1;
  this->Locator = NULL;
}

vtkGeometryFilter::~vtkGeometryFilter()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(float xMin, float xMax, float yMin,
                                     float yMax, float zMin, float zMax)
{
  float extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(float extent[6])
{
  int i;

  if ( extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
       extent[2] != this->Extent[2] || extent[3] != this->Extent[3] ||
       extent[4] != this->Extent[4] || extent[5] != this->Extent[5] )
    {
    this->Modified();
    for (i=0; i<3; i++)
      {
      if ( extent[2*i+1] < extent[2*i] )
        {
        extent[2*i+1] = extent[2*i];
        }
      this->Extent[2*i] = extent[2*i];
      this->Extent[2*i+1] = extent[2*i+1];
      }
    }
}

void vtkGeometryFilter::Execute()
{
  int cellId, i, j, newCellId;
  vtkDataSet *input= this->GetInput();
  int numPts=input->GetNumberOfPoints();
  int numCells=input->GetNumberOfCells();
  char *cellVis;
  vtkGenericCell *cell;
  vtkCell *face;
  float *x;
  vtkIdList *ptIds;
  vtkIdList *cellIds;
  vtkIdList *pts;
  vtkPoints *newPts;
  int ptId;
  int npts;
  vtkIdType pt;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  int allVisible;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  // ghost cell stuff
  unsigned char  updateLevel = (unsigned char)(output->GetUpdateGhostLevel());
  unsigned char  *cellGhostLevels = NULL;
  
  if (numCells == 0)
    {
    return;
    }

  switch (input->GetDataObjectType())
    {
    case  VTK_POLY_DATA:
      this->PolyDataExecute();
      return;
    case  VTK_UNSTRUCTURED_GRID:
      this->UnstructuredGridExecute();
      return;
    case VTK_STRUCTURED_GRID:      
      this->StructuredGridExecute();
      return;
    }

  vtkDataArray* temp = 0;
  if (cd)
    {
    temp = cd->GetArray("vtkGhostLevels");
    }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
    {
    vtkDebugMacro("No appropriate ghost levels field available.");
    }
  else
    {
    cellGhostLevels = ((vtkUnsignedCharArray*)temp)->GetPointer(0);
    }
  
  cellIds = vtkIdList::New();
  pts = vtkIdList::New();

  vtkDebugMacro(<<"Executing geometry filter");

  cell = vtkGenericCell::New();

  if ( (!this->CellClipping) && (!this->PointClipping) && 
  (!this->ExtentClipping) )
    {
    allVisible = 1;
    cellVis = NULL;
    }
  else
    {
    allVisible = 0;
    cellVis = new char[numCells];
    }

  // Mark cells as being visible or not
  //
  if ( ! allVisible )
    {
    for(cellId=0; cellId < numCells; cellId++)
      {
      if ( this->CellClipping && cellId < this->CellMinimum ||
      cellId > this->CellMaximum )
        {
        cellVis[cellId] = 0;
        }
      else
        {
        input->GetCell(cellId,cell);
        ptIds = cell->GetPointIds();
        for (i=0; i < ptIds->GetNumberOfIds(); i++) 
          {
          ptId = ptIds->GetId(i);
          x = input->GetPoint(ptId);

          if ( (this->PointClipping && (ptId < this->PointMinimum ||
          ptId > this->PointMaximum) ) ||
          (this->ExtentClipping && 
          (x[0] < this->Extent[0] || x[0] > this->Extent[1] ||
          x[1] < this->Extent[2] || x[1] > this->Extent[3] ||
          x[2] < this->Extent[4] || x[2] > this->Extent[5] )) )
            {
            cellVis[cellId] = 0;
            break;
            }
          }
        if ( i >= ptIds->GetNumberOfIds() )
          {
          cellVis[cellId] = 1;
          }
        }
      }
    }

  // Allocate
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts,numPts/2);
  output->Allocate(4*numCells,numCells/2);
  outputPD->CopyAllocate(pd,numPts,numPts/2);
  outputCD->CopyAllocate(cd,numCells,numCells/2);

  if ( this->Merging )
    {
    if ( this->Locator == NULL )
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion (newPts, input->GetBounds());
    }

  // Traverse cells to extract geometry
  //
  int abort=0;
  int progressInterval = numCells/20 + 1;
  for(cellId=0; cellId < numCells && !abort; cellId++)
    {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress ((float)cellId/numCells);
      abort = this->GetAbortExecute();
      }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhostLevels && cellGhostLevels[cellId] > updateLevel)
      { // Do not create surfaces in outer ghost cells.
      continue;
      }
    
    if ( allVisible || cellVis[cellId] )
      {
      input->GetCell(cellId,cell);
      switch (cell->GetCellDimension())
        {
        // create new points and then cell
        case 0: case 1: case 2:
          
          npts = cell->GetNumberOfPoints();
          pts->Reset();
          for ( i=0; i < npts; i++)
            {
            ptId = cell->GetPointId(i);
            x = input->GetPoint(ptId);

            if ( this->Merging && this->Locator->InsertUniquePoint(x, pt) )
              {
              outputPD->CopyData(pd,ptId,pt);
              }
            else if (!this->Merging)
              {
              pt = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd,ptId,pt);
              }
            pts->InsertId(i,pt);
            }
          newCellId = output->InsertNextCell(cell->GetCellType(), pts);
          outputCD->CopyData(cd,cellId,newCellId);
          break;

        case 3:
          for (j=0; j < cell->GetNumberOfFaces(); j++)
            {
            face = cell->GetFace(j);
            input->GetCellNeighbors(cellId, face->PointIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 || 
            (!allVisible && !cellVis[cellIds->GetId(0)]) )
              {
              npts = face->GetNumberOfPoints();
              pts->Reset();
              for ( i=0; i < npts; i++)
                {
                ptId = face->GetPointId(i);
                x = input->GetPoint(ptId);
                if (this->Merging && this->Locator->InsertUniquePoint(x, pt) )
                  {
                  outputPD->CopyData(pd,ptId,pt);
                  }
                else if (!this->Merging)
                  {
                  pt = newPts->InsertNextPoint(x);
                  outputPD->CopyData(pd,ptId,pt);
                  }
                pts->InsertId(i,pt);
                }
              newCellId = output->InsertNextCell(face->GetCellType(), pts);
              outputCD->CopyData(cd,cellId,newCellId);
              }
            }
          break;

        } //switch
      } //if visible
    } //for all cells

  vtkDebugMacro(<<"Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  cell->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  //free storage
  if (!this->Merging && this->Locator)
    {
    this->Locator->Initialize(); 
    }
  output->Squeeze();

  cellIds->Delete();
  pts->Delete();
  if ( cellVis )
    {
    delete [] cellVis;
    }
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkGeometryFilter::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
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

void vtkGeometryFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}

unsigned long int vtkGeometryFilter::GetMTime()
{
  unsigned long mTime=this-> vtkDataSetToPolyDataFilter::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}


void vtkGeometryFilter::PolyDataExecute()
{
  vtkPolyData *input= (vtkPolyData *)this->GetInput();
  int i, cellId;
  int allVisible;
  vtkIdType npts;
  vtkIdType *pts;
  vtkPoints *p = input->GetPoints();
  int numCells=input->GetNumberOfCells();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  int newCellId, visible, type, ptId;
  float *x;
  // ghost cell stuff
  unsigned char  updateLevel = (unsigned char)(output->GetUpdateGhostLevel());
  unsigned char  *cellGhostLevels = 0;
  
  vtkDebugMacro(<<"Executing geometry filter for poly data input");

  vtkDataArray* temp = 0;
  if (cd)
    {
    temp = cd->GetArray("vtkGhostLevels");
    }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
    {
    vtkDebugMacro("No appropriate ghost levels field available.");
    }
  else
    {
    cellGhostLevels = ((vtkUnsignedCharArray*)temp)->GetPointer(0);
    }
  
  if ( (!this->CellClipping) && (!this->PointClipping) &&
       (!this->ExtentClipping) )
    {
    allVisible = 1;
    }
  else
    {
    allVisible = 0;
    }

  if ( allVisible ) //just pass input to output
    {
    output->CopyStructure(input);
    outputPD->PassData(pd);
    outputCD->PassData(cd);
    return;
    }

  // Always pass point data
  output->SetPoints(p);
  outputPD->PassData(pd);

  // Allocate
  //
  output->Allocate(numCells);
  outputCD->CopyAllocate(cd,numCells,numCells/2);
  input->BuildCells(); //needed for GetCellPoints()
  
  int progressInterval = numCells/20 + 1;
  for(cellId=0; cellId < numCells; cellId++)
    {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress ((float)cellId/numCells);
      }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhostLevels && cellGhostLevels[cellId] > updateLevel)
      { // Do not create surfaces in outer ghost cells.
      continue;
      }
    
    input->GetCellPoints(cellId,npts,pts);
    visible = 1;
    if ( !allVisible )
      {
      if ( this->CellClipping && cellId < this->CellMinimum ||
      cellId > this->CellMaximum )
        {
        visible = 0;
        }
      else
        {
        for (i=0; i < npts; i++) 
          {
		  ptId = pts[i];
          x = input->GetPoint(ptId);

          if ( (this->PointClipping && (ptId < this->PointMinimum ||
                                        ptId > this->PointMaximum) ) ||
               (this->ExtentClipping && 
                (x[0] < this->Extent[0] || x[0] > this->Extent[1] ||
                 x[1] < this->Extent[2] || x[1] > this->Extent[3] ||
                 x[2] < this->Extent[4] || x[2] > this->Extent[5] )) )
            {
            visible = 0;
            break;
            }
          }
        }
      }
    
    // now if visible extract geometry
    if (allVisible || visible)
      {
      type = input->GetCellType(cellId);
      newCellId = output->InsertNextCell(type,npts,pts);
      outputCD->CopyData(cd,cellId,newCellId);
      } //if visible
    } //for all cells
  
  // Update ourselves and release memory
  //
  output->Squeeze();

  vtkDebugMacro(<<"Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");
}

void vtkGeometryFilter::UnstructuredGridExecute()
{
  vtkUnstructuredGrid *input= (vtkUnstructuredGrid *)this->GetInput();
  vtkCellArray *Connectivity = input->GetCells();
  if (Connectivity == NULL) {return;}
  int i, cellId;
  int allVisible;
  vtkIdType npts;
  vtkIdType *pts;
  vtkPoints *p = input->GetPoints();
  int numCells=input->GetNumberOfCells();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkCellArray *Verts, *Lines, *Polys, *Strips;
  vtkIdList *cellIds, *faceIds;
  char *cellVis;
  int newCellId, faceId, *faceVerts, numFacePts;
  float *x;
  int PixelConvert[4];
  // ghost cell stuff
  unsigned char  updateLevel = (unsigned char)(output->GetUpdateGhostLevel());
  unsigned char  *cellGhostLevels = 0;  
  
  PixelConvert[0] = 0;
  PixelConvert[1] = 1;
  PixelConvert[2] = 3;
  PixelConvert[3] = 2;
  
  vtkDebugMacro(<<"Executing geometry filter for unstructured grid input");

  vtkDataArray* temp = 0;
  if (cd)
    {
    temp = cd->GetArray("vtkGhostLevels");
    }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
    {
    vtkDebugMacro("No appropriate ghost levels field available.");
    }
  else
    {
    cellGhostLevels = ((vtkUnsignedCharArray*)temp)->GetPointer(0);
    }
  
  // Check input
  if ( Connectivity == NULL )
    {
    vtkDebugMacro(<<"Nothing to extract");
    return;
    }

  // Determine nature of what we have to do
  cellIds = vtkIdList::New();
  faceIds = vtkIdList::New();
  if ( (!this->CellClipping) && (!this->PointClipping) &&
       (!this->ExtentClipping) )
    {
    allVisible = 1;
    cellVis = NULL;
    }
  else
    {
    allVisible = 0;
    cellVis = new char[numCells];
    }

  // Just pass points through, never merge
  output->SetPoints(input->GetPoints());
  outputPD->PassData(pd);

  outputCD->CopyAllocate(cd,numCells,numCells/2);

  Verts = vtkCellArray::New();
  Verts->Allocate(numCells/4+1,numCells);
  Lines = vtkCellArray::New();
  Lines->Allocate(numCells/4+1,numCells);
  Polys = vtkCellArray::New();
  Polys->Allocate(numCells/4+1,numCells);
  Strips = vtkCellArray::New();
  Strips->Allocate(numCells/4+1,numCells);
  
  // Loop over the cells determining what's visible
  if (!allVisible)
    {
    for (cellId=0, Connectivity->InitTraversal(); 
         Connectivity->GetNextCell(npts,pts); 
         cellId++)
      {
      cellVis[cellId] = 1;
      if ( this->CellClipping && cellId < this->CellMinimum ||
           cellId > this->CellMaximum )
        {
        cellVis[cellId] = 0;
        }
      else
        {
        for (i=0; i < npts; i++) 
          {
          x = p->GetPoint(pts[i]);
          if ( (this->PointClipping && (pts[i] < this->PointMinimum ||
                                        pts[i] > this->PointMaximum) ) ||
               (this->ExtentClipping && 
                (x[0] < this->Extent[0] || x[0] > this->Extent[1] ||
                 x[1] < this->Extent[2] || x[1] > this->Extent[3] ||
                 x[2] < this->Extent[4] || x[2] > this->Extent[5] )) )
            {
            cellVis[cellId] = 0;
            break;
            }//point/extent clipping
          }//for each point
        }//if point clipping needs checking
      }//for all cells
    }//if not all visible
  
  // Loop over all cells now that visibility is known
  // (Have to compute visibility first for 3D cell boundarys)
  int progressInterval = numCells/20 + 1;
  for (cellId=0, Connectivity->InitTraversal(); 
       Connectivity->GetNextCell(npts,pts); 
       cellId++)
    {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress ((float)cellId/numCells);
      }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhostLevels && cellGhostLevels[cellId] > updateLevel)
      { // Do not create surfaces in outer ghost cells.
      continue;
      }
    
    if (allVisible || cellVis[cellId])  //now if visible extract geometry
      {
      switch (input->GetCellType(cellId))
        {
        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
          newCellId = Verts->InsertNextCell(npts,pts);
          outputCD->CopyData(cd,cellId,newCellId);
          break;
          
        case VTK_LINE: 
        case VTK_POLY_LINE:
          newCellId = Lines->InsertNextCell(npts,pts);
          outputCD->CopyData(cd,cellId,newCellId);
          break;
          
        case VTK_TRIANGLE:
        case VTK_QUAD:
        case VTK_POLYGON:
          newCellId = Polys->InsertNextCell(npts,pts);
          outputCD->CopyData(cd,cellId,newCellId);
          break;

        case VTK_TRIANGLE_STRIP:
          newCellId = Strips->InsertNextCell(npts,pts);
          outputCD->CopyData(cd,cellId,newCellId);
          break;
          
        case VTK_PIXEL:
          newCellId = Polys->InsertNextCell(npts);
          for ( i=0; i < npts; i++)
            {
            Polys->InsertCellPoint(pts[PixelConvert[i]]);
            }
          outputCD->CopyData(cd,cellId,newCellId);
          break;
          
        case VTK_TETRA:
          for (faceId = 0; faceId < 4; faceId++)
            {
            faceIds->Reset();
            faceVerts = vtkTetra::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            numFacePts = 3;
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 || 
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
              {
              newCellId = Polys->InsertNextCell(numFacePts);
              for ( i=0; i < numFacePts; i++)
                {
                Polys->InsertCellPoint(pts[faceVerts[i]]);
                }
              outputCD->CopyData(cd,cellId,newCellId);
              }
            }
          break;
          
        case VTK_VOXEL:
          for (faceId = 0; faceId < 6; faceId++)
            {
            faceIds->Reset();
            faceVerts = vtkVoxel::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 || 
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
              {
              newCellId = Polys->InsertNextCell(numFacePts);
              for ( i=0; i < numFacePts; i++)
                {
                Polys->InsertCellPoint(pts[faceVerts[PixelConvert[i]]]);
                }
              outputCD->CopyData(cd,cellId,newCellId);
              }
            }
          break;

        case VTK_HEXAHEDRON:
          for (faceId = 0; faceId < 6; faceId++)
            {
            faceIds->Reset();
            faceVerts = vtkHexahedron::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 || 
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
              {
              newCellId = Polys->InsertNextCell(numFacePts);
              for ( i=0; i < numFacePts; i++)
                {
                Polys->InsertCellPoint(pts[faceVerts[i]]);
                }
              outputCD->CopyData(cd,cellId,newCellId);
              }
            }
          break;
          
        case VTK_WEDGE:
          for (faceId = 0; faceId < 5; faceId++)
            {
            faceIds->Reset();
            faceVerts = vtkWedge::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            numFacePts = 3;
            if (faceVerts[3] >= 0)
              {
              faceIds->InsertNextId(pts[faceVerts[3]]);
              numFacePts = 4;
              }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 || 
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
              {
              newCellId = Polys->InsertNextCell(numFacePts);
              for ( i=0; i < numFacePts; i++)
                {
                Polys->InsertCellPoint(pts[faceVerts[i]]);
                }
              outputCD->CopyData(cd,cellId,newCellId);
              }
            }
          break;
          
        case VTK_PYRAMID:
          for (faceId = 0; faceId < 5; faceId++)
            {
            faceIds->Reset();
            faceVerts = vtkPyramid::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            numFacePts = 3;
            if (faceVerts[3] >= 0)
              {
              faceIds->InsertNextId(pts[faceVerts[3]]);
              numFacePts = 4;
              }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 || 
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
              {
              newCellId = Polys->InsertNextCell(numFacePts);
              for ( i=0; i < numFacePts; i++)
                {
                Polys->InsertCellPoint(pts[faceVerts[i]]);
                }
              outputCD->CopyData(cd,cellId,newCellId);
              }
            }
          break;
        } //switch
      } //if visible
    } //for all cells
  
  // Update ourselves and release memory
  //
  output->SetVerts(Verts);
  Verts->Delete();
  output->SetLines(Lines);
  Lines->Delete();
  output->SetPolys(Polys);
  Polys->Delete();
  output->SetStrips(Strips);
  Strips->Delete();
  
  output->Squeeze();

  vtkDebugMacro(<<"Extracted " << input->GetNumberOfPoints() << " points,"
  << output->GetNumberOfCells() << " cells.");

  cellIds->Delete();
  faceIds->Delete();
  if ( cellVis )
    {
    delete [] cellVis;
    }
}

void vtkGeometryFilter::StructuredGridExecute()
{
  int cellId, i, newCellId;
  vtkStructuredGrid *input=(vtkStructuredGrid *)this->GetInput();
  int numCells=input->GetNumberOfCells();
  char *cellVis;
  vtkGenericCell *cell;
  float *x;
  vtkIdList *ptIds;
  vtkIdList *cellIds;
  vtkIdList *pts;
  int ptId, *faceVerts, faceId, numFacePts;
  vtkIdType *facePts;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  int allVisible;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkCellArray *cells;
  // ghost cell stuff
  unsigned char  updateLevel = (unsigned char)(output->GetUpdateGhostLevel());
  unsigned char  *cellGhostLevels = 0;
  
  cellIds = vtkIdList::New();
  pts = vtkIdList::New();

  vtkDebugMacro(<<"Executing geometry filter with structured grid input");

  cell = vtkGenericCell::New();

  vtkDataArray* temp = 0;
  if (cd)
    {
    temp = cd->GetArray("vtkGhostLevels");
    }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
    {
    vtkDebugMacro("No appropriate ghost levels field available.");
    }
  else
    {
    cellGhostLevels = ((vtkUnsignedCharArray*)temp)->GetPointer(0);
    }
  
  if ( (!this->CellClipping) && (!this->PointClipping) && 
  (!this->ExtentClipping) )
    {
    allVisible = 1;
    cellVis = NULL;
    }
  else
    {
    allVisible = 0;
    cellVis = new char[numCells];
    }

  // Mark cells as being visible or not
  //
  if ( ! allVisible )
    {
    for(cellId=0; cellId < numCells; cellId++)
      {
      cellVis[cellId] = 1;
      if ( this->CellClipping && cellId < this->CellMinimum ||
      cellId > this->CellMaximum )
        {
        cellVis[cellId] = 0;
        }
      else
        {
        input->GetCell(cellId,cell);
        ptIds = cell->GetPointIds();
        for (i=0; i < ptIds->GetNumberOfIds(); i++) 
          {
          ptId = ptIds->GetId(i);
          x = input->GetPoint(ptId);

          if ( (this->PointClipping && (ptId < this->PointMinimum ||
          ptId > this->PointMaximum) ) ||
          (this->ExtentClipping && 
          (x[0] < this->Extent[0] || x[0] > this->Extent[1] ||
          x[1] < this->Extent[2] || x[1] > this->Extent[3] ||
          x[2] < this->Extent[4] || x[2] > this->Extent[5] )) )
            {
            cellVis[cellId] = 0;
            break;
            }
          }
        }
      }
    }

  // Allocate - points are never merged
  //
  output->SetPoints(input->GetPoints());
  outputPD->PassData(pd);
  outputCD->CopyAllocate(cd,numCells,numCells/2);

  cells = vtkCellArray::New();
  cells->Allocate(numCells,numCells/2);
  
  // Traverse cells to extract geometry
  //
  int progressInterval = numCells/20 + 1;
  for(cellId=0; cellId < numCells; cellId++)
    {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress ((float)cellId/numCells);
      }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhostLevels && cellGhostLevels[cellId] > updateLevel)
      { // Do not create surfaces in outer ghost cells.
      continue;
      }
    
    if ( (allVisible || cellVis[cellId]))
      {
      input->GetCell(cellId,cell);
      switch (cell->GetCellDimension())
        {
        // create new points and then cell
        case 0: case 1: case 2:
          newCellId = cells->InsertNextCell(cell);
          outputCD->CopyData(cd,cellId,newCellId);
          break;

        case 3: //must be hexahedron
          facePts = cell->GetPointIds()->GetPointer(0);
          for (faceId = 0; faceId < 6; faceId++)
            {
            pts->Reset();
            faceVerts = vtkHexahedron::GetFaceArray(faceId);
            pts->InsertNextId(facePts[faceVerts[0]]);
            pts->InsertNextId(facePts[faceVerts[1]]);
            pts->InsertNextId(facePts[faceVerts[2]]);
            pts->InsertNextId(facePts[faceVerts[3]]);
            numFacePts = 4;
            input->GetCellNeighbors(cellId, pts, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 || 
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
              {
              newCellId = cells->InsertNextCell(numFacePts);
              for ( i=0; i < numFacePts; i++)
                {
                cells->InsertCellPoint(facePts[faceVerts[i]]);
                }
              outputCD->CopyData(cd,cellId,newCellId);
              }
            }
          break;

        } //switch
      } //if visible
    } //for all cells

  switch (input->GetDataDimension())
    {
    case 0: 
      output->SetVerts(cells);
      break;
    case 1: 
      output->SetLines(cells);
      break;
    case 2: case 3:
      output->SetPolys(cells);
    }
  
  vtkDebugMacro(<<"Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  cells->Delete();
  cell->Delete();
  output->Squeeze();
  cellIds->Delete();
  pts->Delete();
  if ( cellVis )
    {
    delete [] cellVis;
    }
}


void vtkGeometryFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  int piece, numPieces, ghostLevels;
  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("No Input");
    return;
    }
  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  ghostLevels = output->GetUpdateGhostLevel();
  
  if (numPieces > 1)
    {
    ++ghostLevels;
    }

  this->GetInput()->SetUpdateExtent(piece, numPieces, ghostLevels);

  this->GetInput()->RequestExactExtentOn();
}

void vtkGeometryFilter::ExecuteInformation()
{
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("No Input");
    return;
    }
}

