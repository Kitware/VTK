/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeometryFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPyramid.h"
#include "vtkPentagonalPrism.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTetra.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkIncrementalPointLocator.h"


vtkStandardNewMacro(vtkGeometryFilter);
vtkCxxSetObjectMacro(vtkGeometryFilter, Locator, vtkIncrementalPointLocator)

//----------------------------------------------------------------------------
// Construct with all types of clipping turned off.
vtkGeometryFilter::vtkGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = VTK_ID_MAX;

  this->CellMinimum = 0;
  this->CellMaximum = VTK_ID_MAX;

  this->Extent[0] = -VTK_DOUBLE_MAX;
  this->Extent[1] = VTK_DOUBLE_MAX;
  this->Extent[2] = -VTK_DOUBLE_MAX;
  this->Extent[3] = VTK_DOUBLE_MAX;
  this->Extent[4] = -VTK_DOUBLE_MAX;
  this->Extent[5] = VTK_DOUBLE_MAX;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;

  this->Merging = 1;
  this->Locator = NULL;
}

//----------------------------------------------------------------------------
vtkGeometryFilter::~vtkGeometryFilter()
{
  this->SetLocator(NULL);
}

//----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(double xMin, double xMax, double yMin,
                                     double yMax, double zMin, double zMax)
{
  double extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

//----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(double extent[6])
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

//----------------------------------------------------------------------------
int vtkGeometryFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, newCellId;
  int i, j;
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType numCells=input->GetNumberOfCells();
  char *cellVis;
  vtkGenericCell *cell;
  vtkCell *face;
  double x[3];
  vtkIdList *ptIds;
  vtkIdList *cellIds;
  vtkIdList *pts;
  vtkPoints *newPts;
  vtkIdType ptId;
  int npts;
  vtkIdType pt=0;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  int allVisible;
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  unsigned char  *cellGhosts = NULL;

  if (numCells == 0)
  {
    return 1;
  }

  switch (input->GetDataObjectType())
  {
    case  VTK_POLY_DATA:
      this->PolyDataExecute(input, output);
      return 1;
    case  VTK_UNSTRUCTURED_GRID:
      this->UnstructuredGridExecute(input, output);
      return 1;
    case VTK_STRUCTURED_GRID:
      this->StructuredGridExecute(input, output, outInfo);
      return 1;
  }

  vtkDataArray* temp = 0;
  if (cd)
  {
    temp = cd->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
      cellGhosts=static_cast<vtkUnsignedCharArray *>(temp)->GetPointer(0);
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
        if ( this->CellClipping && (cellId < this->CellMinimum ||
                                    cellId > this->CellMaximum) )
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
          input->GetPoint(ptId, x);

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
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd,numPts,numPts/2);
  outputPD->CopyGlobalIdsOn();
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
  vtkIdType progressInterval = numCells/20 + 1;
  for(cellId=0; cellId < numCells && !abort; cellId++)
  {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
    {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId)/numCells);
      abort = this->GetAbortExecute();
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts &&
        cellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    if ( allVisible || cellVis[cellId] )
    {
      input->GetCell(cellId,cell);
      if (cell->GetCellType() != VTK_EMPTY_CELL)
      {
        switch (cell->GetCellDimension())
        {
          // create new points and then cell
          case 0: case 1: case 2:

            npts = cell->GetNumberOfPoints();
            pts->Reset();
            for ( i=0; i < npts; i++)
            {
              ptId = cell->GetPointId(i);
              input->GetPoint(ptId, x);

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
            int numFaces = cell->GetNumberOfFaces();
            for (j=0; j < numFaces; j++)
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
                  input->GetPoint(ptId, x);
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
      }
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
  delete [] cellVis;

  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkGeometryFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkGeometryFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

//----------------------------------------------------------------------------
vtkMTimeType vtkGeometryFilter::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  if ( this->Locator != NULL )
  {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }
  return mTime;
}

//----------------------------------------------------------------------------
void vtkGeometryFilter::PolyDataExecute(vtkDataSet *dataSetInput,
                                        vtkPolyData *output)
{
  vtkPolyData *input=static_cast<vtkPolyData *>(dataSetInput);
  vtkIdType cellId;
  int i;
  int allVisible;
  vtkIdType npts;
  vtkIdType *pts;
  vtkPoints *p = input->GetPoints();
  vtkIdType numCells=input->GetNumberOfCells();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkIdType newCellId, ptId;
  int visible, type;
  double x[3];
  unsigned char *cellGhosts = 0;

  vtkDebugMacro(<<"Executing geometry filter for poly data input");

  vtkDataArray* temp = 0;
  if (cd)
  {
    temp = cd->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
      cellGhosts=static_cast<vtkUnsignedCharArray *>(temp)->GetPointer(0);
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

  vtkIdType progressInterval = numCells/20 + 1;
  for(cellId=0; cellId < numCells; cellId++)
  {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
    {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId)/numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts &&
        cellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    input->GetCellPoints(cellId,npts,pts);
    visible = 1;
    if ( !allVisible )
    {
        if ( this->CellClipping && (cellId < this->CellMinimum ||
                                    cellId > this->CellMaximum) )
        {
        visible = 0;
        }
      else
      {
        for (i=0; i < npts; i++)
        {
          ptId = pts[i];
          input->GetPoint(ptId, x);

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

//----------------------------------------------------------------------------
void vtkGeometryFilter::UnstructuredGridExecute(vtkDataSet *dataSetInput,
                                                vtkPolyData *output)
{
  vtkUnstructuredGrid *input=static_cast<vtkUnstructuredGrid *>(dataSetInput);
  vtkCellArray *connectivity = input->GetCells();
  if (connectivity == NULL)
  {
    return;
  }
  vtkIdType cellId;
  int allVisible;
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  vtkPoints *p = input->GetPoints();
  vtkIdType numCells=input->GetNumberOfCells();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkCellArray *verts, *lines, *polys, *strips;
  vtkIdList *cellIds, *faceIds;
  char *cellVis;
  int faceId, *faceVerts, numFacePts;
  double x[3];
  int pixelConvert[4];
  unsigned char  *cellGhosts = 0;

  pixelConvert[0] = 0;
  pixelConvert[1] = 1;
  pixelConvert[2] = 3;
  pixelConvert[3] = 2;

  vtkDebugMacro(<<"Executing geometry filter for unstructured grid input");

  vtkDataArray* temp = 0;
  if (cd)
  {
    temp = cd->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts=static_cast<vtkUnsignedCharArray *>(temp)->GetPointer(0);
  }

  // Check input
  if ( connectivity == NULL )
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

  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd,numCells,numCells/2);

  verts = vtkCellArray::New();
  verts->Allocate(numCells/4+1,numCells);
  lines = vtkCellArray::New();
  lines->Allocate(numCells/4+1,numCells);
  polys = vtkCellArray::New();
  polys->Allocate(numCells/4+1,numCells);
  strips = vtkCellArray::New();
  strips->Allocate(numCells/4+1,numCells);

  // Loop over the cells determining what's visible
  if (!allVisible)
  {
    for (cellId=0, connectivity->InitTraversal();
         connectivity->GetNextCell(npts,pts);
         cellId++)
    {
      cellVis[cellId] = 1;
      if ( this->CellClipping && (cellId < this->CellMinimum ||
                                  cellId > this->CellMaximum) )
      {
        cellVis[cellId] = 0;
      }
      else
      {
        for (int i=0; i < npts; i++)
        {
          p->GetPoint(pts[i], x);
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

  // These store the cell ids of the input that map to the
  // new vert/line/poly/strip cells, for copying cell data
  // in appropriate order.
  std::vector< vtkIdType > vertCellIds;
  std::vector< vtkIdType > lineCellIds;
  std::vector< vtkIdType > polyCellIds;
  std::vector< vtkIdType > stripCellIds;
  vertCellIds.reserve( numCells );
  lineCellIds.reserve( numCells );
  polyCellIds.reserve( numCells );
  stripCellIds.reserve( numCells );

  // Loop over all cells now that visibility is known
  // (Have to compute visibility first for 3D cell boundarys)
  int progressInterval = numCells/20 + 1;
  for (cellId=0, connectivity->InitTraversal();
       connectivity->GetNextCell(npts,pts);
       cellId++)
  {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
    {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId)/numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts &&
        cellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    if (allVisible || cellVis[cellId])  //now if visible extract geometry
    {
      //special code for nonlinear cells - rarely occurs, so right now it
      //is slow.
      switch (input->GetCellType(cellId))
      {
        case VTK_EMPTY_CELL:
          break;

        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
          verts->InsertNextCell(npts,pts);
          vertCellIds.push_back(cellId);
          break;

        case VTK_LINE:
        case VTK_POLY_LINE:
          lines->InsertNextCell(npts,pts);
          lineCellIds.push_back(cellId);
          break;

        case VTK_TRIANGLE:
        case VTK_QUAD:
        case VTK_POLYGON:
          polys->InsertNextCell(npts,pts);
          polyCellIds.push_back(cellId);
          break;

        case VTK_TRIANGLE_STRIP:
          strips->InsertNextCell(npts,pts);
          stripCellIds.push_back(cellId);
          break;

        case VTK_PIXEL:
          polys->InsertNextCell(npts);
          // pixelConvert (in the following loop) is an int[4]. GCC 5.1.1
          // warns about pixelConvert[4] being uninitialized due to loop
          // unrolling -- forcibly restricting npts <= 4 prevents this warning.
          if (npts > 4)
          {
            npts = 4;
          }
          for ( int i=0; i < npts; i++)
          {
            polys->InsertCellPoint(pts[pixelConvert[i]]);
          }
          polyCellIds.push_back(cellId);
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
              polys->InsertNextCell(numFacePts);
              for ( int i=0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
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
              polys->InsertNextCell(numFacePts);
              for ( int i=0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[pixelConvert[i]]]);
              }
              polyCellIds.push_back(cellId);
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
              polys->InsertNextCell(numFacePts);
              for ( int i=0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
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
              polys->InsertNextCell(numFacePts);
              for ( int i=0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
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
              polys->InsertNextCell(numFacePts);
              for ( int i=0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case VTK_PENTAGONAL_PRISM:
          for (faceId = 0; faceId < 7; faceId++)
          {
            faceIds->Reset();
            faceVerts = vtkPentagonalPrism::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            if (faceVerts[4] >= 0)
            {
              faceIds->InsertNextId(pts[faceVerts[4]]);
              numFacePts = 5;
            }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 ||
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
            {
              polys->InsertNextCell(numFacePts);
              for ( int i=0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case VTK_HEXAGONAL_PRISM:
          for (faceId = 0; faceId < 8; faceId++)
          {
            faceIds->Reset();
            faceVerts = vtkHexagonalPrism::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            if (faceVerts[4] >= 0)
            {
              faceIds->InsertNextId(pts[faceVerts[4]]);
              faceIds->InsertNextId(pts[faceVerts[5]]);
              numFacePts = 6;
            }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0 ||
                 (!allVisible && !cellVis[cellIds->GetId(0)]) )
            {
              polys->InsertNextCell(numFacePts);
              for ( int i=0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        //Quadratic cells
        case VTK_QUADRATIC_EDGE:
        case VTK_CUBIC_LINE:
        case VTK_QUADRATIC_TRIANGLE:
        case VTK_QUADRATIC_QUAD:
        case VTK_QUADRATIC_POLYGON:
        case VTK_QUADRATIC_TETRA:
        case VTK_QUADRATIC_HEXAHEDRON:
        case VTK_QUADRATIC_WEDGE:
        case VTK_QUADRATIC_PYRAMID:
        case VTK_QUADRATIC_LINEAR_QUAD:
        case VTK_BIQUADRATIC_TRIANGLE:
        case VTK_BIQUADRATIC_QUAD:
        case VTK_TRIQUADRATIC_HEXAHEDRON:
        case VTK_QUADRATIC_LINEAR_WEDGE:
        case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
        case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        {
          vtkGenericCell *cell = vtkGenericCell::New();
          input->GetCell(cellId,cell);
          vtkIdList *ipts = vtkIdList::New();
          vtkPoints *coords = vtkPoints::New();
          vtkIdList *icellIds = vtkIdList::New();

          if ( cell->GetCellDimension() == 1 )
          {
            cell->Triangulate(0,ipts,coords);
            for ( int i=0; i < ipts->GetNumberOfIds(); i+=2)
            {
              lines->InsertNextCell(2);
              lines->InsertCellPoint(ipts->GetId(i));
              lines->InsertCellPoint(ipts->GetId(i+1));
              lineCellIds.push_back(cellId);
            }
          }
          else if ( cell->GetCellDimension() == 2 )
          {
            cell->Triangulate(0,ipts,coords);
            for ( int i=0; i < ipts->GetNumberOfIds(); i+=3)
            {
              polys->InsertNextCell(3);
              polys->InsertCellPoint(ipts->GetId(i));
              polys->InsertCellPoint(ipts->GetId(i+1));
              polys->InsertCellPoint(ipts->GetId(i+2));
              polyCellIds.push_back(cellId);
            }
          }
          else //3D nonlinear cell
          {
            vtkCell *face;
            for (int j=0; j < cell->GetNumberOfFaces(); j++)
            {
              face = cell->GetFace(j);
              input->GetCellNeighbors(cellId, face->PointIds, icellIds);
              if ( icellIds->GetNumberOfIds() <= 0)
              {
                face->Triangulate(0,ipts,coords);
                for ( int i=0; i < ipts->GetNumberOfIds(); i+=3)
                {
                  polys->InsertNextCell(3);
                  polys->InsertCellPoint(ipts->GetId(i));
                  polys->InsertCellPoint(ipts->GetId(i+1));
                  polys->InsertCellPoint(ipts->GetId(i+2));
                  polyCellIds.push_back(cellId);
                }
              }
            }
          } //3d cell
          icellIds->Delete();
          coords->Delete();
          ipts->Delete();
          cell->Delete();
        }
          break; //done with quadratic cells
      } //switch
    } //if visible
  } //for all cells

  // Update ourselves and release memory
  //
  output->SetVerts(verts);
  verts->Delete();
  output->SetLines(lines);
  lines->Delete();
  output->SetPolys(polys);
  polys->Delete();
  output->SetStrips(strips);
  strips->Delete();

  // Copy the cell data in appropriate order : verts / lines / polys / strips
  size_t offset = 0;
  size_t size = vertCellIds.size();
  for ( size_t i = 0; i < size; ++i )
  {
    outputCD->CopyData(cd, vertCellIds[i], static_cast<vtkIdType>(i) );
  }
  offset += size;
  size = lineCellIds.size();
  for ( size_t i = 0; i < size; ++i )
  {
    outputCD->CopyData(cd, lineCellIds[i], static_cast<vtkIdType>(i+offset) );
  }
  offset += size;
  size = polyCellIds.size();
  for ( size_t i = 0; i < size; ++i )
  {
    outputCD->CopyData(cd, polyCellIds[i], static_cast<vtkIdType>(i+offset) );
  }
  offset += size;
  size = stripCellIds.size();
  for ( size_t i = 0; i < size; ++i )
  {
    outputCD->CopyData(cd, stripCellIds[i], static_cast<vtkIdType>(i+offset) );
  }

  output->Squeeze();

  vtkDebugMacro(<<"Extracted " << input->GetNumberOfPoints() << " points,"
    << output->GetNumberOfCells() << " cells.");

  cellIds->Delete();
  faceIds->Delete();
  delete [] cellVis;
}

//----------------------------------------------------------------------------
void vtkGeometryFilter::StructuredGridExecute(vtkDataSet *dataSetInput,
                                              vtkPolyData *output,
                                              vtkInformation *)
{
  vtkIdType cellId, newCellId;
  int i;
  vtkStructuredGrid *input=static_cast<vtkStructuredGrid *>(dataSetInput);
  vtkIdType numCells=input->GetNumberOfCells();
  std::vector<char> cellVis;
  vtkGenericCell *cell;
  double x[3];
  vtkIdList *ptIds;
  vtkIdList *cellIds;
  vtkIdList *pts;
  vtkIdType ptId;
  int *faceVerts, faceId, numFacePts;
  vtkIdType *facePts;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  int allVisible;
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkCellArray *cells;
  unsigned char  *cellGhosts = 0;

  cellIds = vtkIdList::New();
  pts = vtkIdList::New();

  vtkDebugMacro(<<"Executing geometry filter with structured grid input");

  cell = vtkGenericCell::New();

  vtkDataArray* temp = 0;
  if (cd)
  {
    temp = cd->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts =static_cast<vtkUnsignedCharArray *>(temp)->GetPointer(0);
  }

  if ( (!this->CellClipping) && (!this->PointClipping) &&
       (!this->ExtentClipping) )
  {
    allVisible = 1;
  }
  else
  {
    allVisible = 0;
    cellVis.resize(numCells);
  }

  // Mark cells as being visible or not
  //
  if ( ! allVisible )
  {
    for(cellId=0; cellId < numCells; cellId++)
    {
      cellVis[cellId] = 1;
      if ( this->CellClipping && (cellId < this->CellMinimum ||
                                  cellId > this->CellMaximum) )
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
          input->GetPoint(ptId, x);

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
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd,numCells,numCells/2);

  cells = vtkCellArray::New();
  cells->Allocate(numCells,numCells/2);

  // Traverse cells to extract geometry
  //
  vtkIdType progressInterval = numCells/20 + 1;
  for(cellId=0; cellId < numCells; cellId++)
  {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
    {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId)/numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts &&
        cellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
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
}

//----------------------------------------------------------------------------
int vtkGeometryFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    ++ghostLevels;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}
