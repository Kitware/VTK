/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearExtrusionFilter.cxx
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
#include "vtkLinearExtrusionFilter.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkLinearExtrusionFilter* vtkLinearExtrusionFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLinearExtrusionFilter");
  if(ret)
    {
    return (vtkLinearExtrusionFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLinearExtrusionFilter;
}

// Create object with normal extrusion type, capping on, scale factor=1.0,
// vector (0,0,1), and extrusion point (0,0,0).
vtkLinearExtrusionFilter::vtkLinearExtrusionFilter()
{
  this->ExtrusionType = VTK_NORMAL_EXTRUSION;
  this->Capping = 1;
  this->ScaleFactor = 1.0;
  this->Vector[0] = this->Vector[1] = 0.0; this->Vector[2] = 1.0;
  this->ExtrusionPoint[0] = this->ExtrusionPoint[1] = this->ExtrusionPoint[2] = 0.0;
}

float *vtkLinearExtrusionFilter::ViaNormal(float x[3], vtkIdType id,
                                           vtkDataArray *n)
{
  static float xNew[3], *normal;
  int i;

  normal = n->GetTuple(id);
  for (i=0; i<3; i++) 
    {
    xNew[i] = x[i] + this->ScaleFactor*normal[i];
    }

  return xNew;
}

float *vtkLinearExtrusionFilter::ViaVector(float x[3],
                                           vtkIdType vtkNotUsed(id), 
					   vtkDataArray *vtkNotUsed(n))
{
  static float xNew[3];
  int i;

  for (i=0; i<3; i++) 
    {
    xNew[i] = x[i] + this->ScaleFactor*this->Vector[i];
    }

  return xNew;
}

float *vtkLinearExtrusionFilter::ViaPoint(float x[3], vtkIdType vtkNotUsed(id),
					  vtkDataArray *vtkNotUsed(n))
{
  static float xNew[3];
  int i;

  for (i=0; i<3; i++) 
    {
    xNew[i] = x[i] + this->ScaleFactor*(x[i] - this->ExtrusionPoint[i]);
    }

  return xNew;
}

void vtkLinearExtrusionFilter::Execute()
{
  vtkIdType numPts, numCells;
  vtkPolyData *input= this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkDataArray *inNormals=NULL;
  vtkPolyData *mesh;
  vtkPoints *inPts;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  vtkIdType inCellId, outCellId;
  int numEdges, dim;
  vtkIdType *pts, npts, ptId, ncells, p1, p2;
  int i, j;
  float *x;
  vtkPoints *newPts;
  vtkCellArray *newLines=NULL, *newPolys=NULL, *newStrips=NULL;
  vtkCell *edge;
  vtkIdList *cellIds, *cellPts;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  // Here is a big pain about ordering of cells. (Copy CellData)
  vtkIdList *lineIds;
  vtkIdList *polyIds;
  vtkIdList *stripIds;

  // Initialize / check input
  //
  vtkDebugMacro(<<"Linearly extruding data");

  numPts=input->GetNumberOfPoints();
  numCells=input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
    {
    vtkErrorMacro(<<"No data to extrude!");
    return;
    }
  //
  // Decide which vector to use for extrusion
  //
  if ( this->ExtrusionType == VTK_POINT_EXTRUSION )
    {
    this->ExtrudePoint = &vtkLinearExtrusionFilter::ViaPoint;
    }
  else if ( this->ExtrusionType == VTK_NORMAL_EXTRUSION  &&
	    (inNormals = pd->GetActiveNormals()) != NULL )
    {
    this->ExtrudePoint = &vtkLinearExtrusionFilter::ViaNormal;
    inNormals = pd->GetActiveNormals();
    }
  else // VTK_VECTOR_EXTRUSION
    {
    this->ExtrudePoint = &vtkLinearExtrusionFilter::ViaVector;
    }

  // Build cell data structure.
  //
  mesh = vtkPolyData::New();
  inPts = input->GetPoints();
  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
  mesh->SetPoints(inPts);
  mesh->SetVerts(inVerts);
  mesh->SetLines(inLines);
  mesh->SetPolys(inPolys);
  mesh->SetStrips(inStrips);
  if (inPolys->GetNumberOfCells() || inStrips->GetNumberOfCells()) 
    {
    mesh->BuildLinks();
    }
  
  cellIds = vtkIdList::New();
  cellIds->Allocate(VTK_CELL_SIZE);


  // Allocate memory for output. We don't copy normals because surface geometry
  // is modified. Copy all points - this is the usual requirement and it makes
  // creation of skirt much easier.
  //
  output->GetCellData()->CopyNormalsOff();
  output->GetCellData()->CopyAllocate(input->GetCellData(),
                                      3*input->GetNumberOfCells());

  outputPD->CopyNormalsOff();
  outputPD->CopyAllocate(pd,2*numPts);
  newPts = vtkPoints::New(); newPts->SetNumberOfPoints(2*numPts);
  if ( (ncells=inVerts->GetNumberOfCells()) > 0 ) 
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(ncells,2));
    }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells()/10 +
           inStrips->GetNumberOfCells()/10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(ncells,4));

  vtkIdType progressInterval=numPts/10+1;
  int abort=0;

  // copy points
  for (ptId=0; ptId < numPts; ptId++)
    { 
    if ( ! (ptId % progressInterval) ) //manage progress / early abort
      {
      this->UpdateProgress (0.25*ptId/numPts);
      }

    x = inPts->GetPoint(ptId);
    newPts->SetPoint(ptId,x);
    newPts->SetPoint(ptId+numPts,
                     (this->*(this->ExtrudePoint))(x,ptId,inNormals));
    outputPD->CopyData(pd,ptId,ptId);
    outputPD->CopyData(pd,ptId,ptId+numPts);
    }
  
  // We need the cellid to copy cell data. Skip points and lines.
  inCellId = outCellId =0;
  if (input->GetVerts())
    {
    inCellId += input->GetVerts()->GetNumberOfCells();
    }
  if (input->GetLines())
    {
    inCellId += input->GetLines()->GetNumberOfCells();
    }
  // We need to keep track of input cell ids used to generate
  // output cells so that we can copy cell data at the end.
  // We do not know how many lines, polys and strips we will get
  // before hand.
  lineIds = vtkIdList::New();
  polyIds = vtkIdList::New();
  stripIds = vtkIdList::New();


  // If capping is on, copy 2D cells to output (plus create cap)
  //
  if ( this->Capping )
    {
    if ( inPolys->GetNumberOfCells() > 0 )
      {
      newPolys = vtkCellArray::New();
      newPolys->Allocate(inPolys->GetSize());
      for ( inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
        {
        newPolys->InsertNextCell(npts,pts);
        polyIds->InsertNextId(inCellId);
        newPolys->InsertNextCell(npts);
        for (i=0; i < npts; i++)
	  {
          newPolys->InsertCellPoint(pts[i] + numPts);
	  }
        polyIds->InsertNextId(inCellId);
        ++inCellId;
        }
      }
    
    if ( inStrips->GetNumberOfCells() > 0 )
      {
      for ( inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
        {
        newStrips->InsertNextCell(npts,pts);
        stripIds->InsertNextId(inCellId);
        newStrips->InsertNextCell(npts);
        for (i=0; i < npts; i++)
	  {
          newStrips->InsertCellPoint(pts[i] + numPts);
	  }
        stripIds->InsertNextId(inCellId);
        ++inCellId;
        }
      }
    }
  this->UpdateProgress (0.4);

  // Loop over all polygons and triangle strips searching for boundary edges. 
  // If boundary edge found, extrude triangle strip.
  //
  progressInterval=numCells/10+1;
  vtkGenericCell *cell = vtkGenericCell::New();
  for ( inCellId=0; inCellId < numCells && !abort; inCellId++)
    {
    if ( ! (inCellId % progressInterval) ) //manage progress / early abort
      {
      this->UpdateProgress (0.4 + 0.6*inCellId/numCells);
      abort = this->GetAbortExecute();
      }

    mesh->GetCell(inCellId,cell);
    cellPts = cell->GetPointIds();

    if ( (dim=cell->GetCellDimension()) == 0 ) //create lines from points
      {
      for (i=0; i<cellPts->GetNumberOfIds(); i++)
        {
        newLines->InsertNextCell(2);
        ptId = cellPts->GetId(i);
        newLines->InsertCellPoint(ptId);
        newLines->InsertCellPoint(ptId+numPts);
        lineIds->InsertNextId(inCellId);
        }
      }

    else if ( dim == 1 ) // create strips from lines
      {
      for (i=0; i < (cellPts->GetNumberOfIds()-1); i++)
        {
        p1 = cellPts->GetId(i);
        p2 = cellPts->GetId(i+1);
        newStrips->InsertNextCell(4);
        newStrips->InsertCellPoint(p1);
        newStrips->InsertCellPoint(p2);
        newStrips->InsertCellPoint(p1+numPts);
        newStrips->InsertCellPoint(p2+numPts);
        stripIds->InsertNextId(inCellId);
        }
      }

    else if ( dim == 2 ) // create strips from boundary edges
      {
      numEdges = cell->GetNumberOfEdges();
      for (i=0; i<numEdges; i++)
        {
        edge = cell->GetEdge(i);
        for (j=0; j<(edge->GetNumberOfPoints()-1); j++)
          {
          p1 = edge->PointIds->GetId(j);
          p2 = edge->PointIds->GetId(j+1);
          mesh->GetCellEdgeNeighbors(inCellId, p1, p2, cellIds);

          if ( cellIds->GetNumberOfIds() < 1 ) //generate strip
            {
            newStrips->InsertNextCell(4);
            newStrips->InsertCellPoint(p1);
            newStrips->InsertCellPoint(p2);
            newStrips->InsertCellPoint(p1+numPts);
            newStrips->InsertCellPoint(p2+numPts);
            stripIds->InsertNextId(inCellId);
            }
          } //for each sub-edge
        } //for each edge
      } //for each polygon or triangle strip
    } //for each cell
  cell->Delete();

  // Now Copy cell data.
  outCellId = 0;
  j = lineIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
    {
    output->GetCellData()->CopyData(input->GetCellData(),
                                    lineIds->GetId(i),outCellId);
    ++outCellId;
    }
  j = polyIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
    {
    output->GetCellData()->CopyData(input->GetCellData(),
                                    polyIds->GetId(i),outCellId);
    ++outCellId;
    }
  j = stripIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
    {
    output->GetCellData()->CopyData(input->GetCellData(),
                                    stripIds->GetId(i),outCellId);
    ++outCellId;
    }
  lineIds->Delete();
  lineIds = NULL;
  stripIds->Delete();
  stripIds = NULL;
  polyIds->Delete();
  polyIds = NULL;


  // Send data to output and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  cellIds->Delete();
  mesh->Delete();

  if ( newLines ) 
    {
    output->SetLines(newLines);
    newLines->Delete();
    }

  if ( newPolys ) 
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }

  output->SetStrips(newStrips);
  newStrips->Delete();

  output->Squeeze();
}



void vtkLinearExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  if ( this->ExtrusionType == VTK_VECTOR_EXTRUSION )
    {
    os << indent << "Extrusion Type: Extrude along vector\n";
    os << indent << "Vector: (" << this->Vector[0] << ", " 
       << this->Vector[1] << ", " << this->Vector[2] << ")\n";
    }
  else if ( this->ExtrusionType == VTK_NORMAL_EXTRUSION )
    {
    os << indent << "Extrusion Type: Extrude along vertex normals\n";
    }
  else //POINT_EXTRUSION
    {
    os << indent << "Extrusion Type: Extrude towards point\n";
    os << indent << "Extrusion Point: (" << this->ExtrusionPoint[0] << ", " 
       << this->ExtrusionPoint[1] << ", " << this->ExtrusionPoint[2] << ")\n";
    }

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}

