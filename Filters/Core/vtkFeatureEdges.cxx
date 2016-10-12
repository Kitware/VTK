/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFeatureEdges.h"

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"

vtkStandardNewMacro(vtkFeatureEdges);

// Construct object with feature angle = 30; all types of edges, except
// manifold edges, are extracted and colored.
vtkFeatureEdges::vtkFeatureEdges()
{
  this->FeatureAngle = 30.0;
  this->BoundaryEdges = 1;
  this->FeatureEdges = 1;
  this->NonManifoldEdges = 1;
  this->ManifoldEdges = 0;
  this->Coloring = 1;
  this->Locator = NULL;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

vtkFeatureEdges::~vtkFeatureEdges()
{
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }
}

// Generate feature edges for mesh
int vtkFeatureEdges::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkFloatArray *newScalars = NULL;
  vtkCellArray *newLines;
  vtkPolyData *Mesh;
  int i;
  vtkIdType j, numNei, cellId;
  vtkIdType numBEdges, numNonManifoldEdges, numFedges, numManifoldEdges;
  double scalar, n[3], x1[3], x2[3];
  double cosAngle = 0;
  vtkIdType lineIds[2];
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  vtkCellArray *inPolys, *inStrips, *newPolys;
  vtkFloatArray *polyNormals = NULL;
  vtkIdType numPts, numCells, numPolys, numStrips, nei;
  vtkIdList *neighbors;
  vtkIdType p1, p2, newId;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  unsigned char* ghosts=0;
  vtkDebugMacro(<<"Executing feature edges");

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
    ghosts = static_cast<vtkUnsignedCharArray *>(temp)->GetPointer(0);
  }

  //  Check input
  //
  inPts=input->GetPoints();
  numCells = input->GetNumberOfCells();
  numPolys = input->GetNumberOfPolys();
  numStrips = input->GetNumberOfStrips();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || !inPts ||
       (numPolys < 1 && numStrips < 1) )
  {
    vtkDebugMacro(<<"No input data!");
    return 1;
  }

  if ( !this->BoundaryEdges && !this->NonManifoldEdges &&
       !this->FeatureEdges && !this->ManifoldEdges )
  {
    vtkDebugMacro(<<"All edge types turned off!");
  }

  // Build cell structure.  Might have to triangulate the strips.
  Mesh = vtkPolyData::New();
  Mesh->SetPoints(inPts);
  inPolys=input->GetPolys();
  if ( numStrips > 0 )
  {
    newPolys = vtkCellArray::New();
    if ( numPolys > 0 )
    {
      newPolys->DeepCopy(inPolys);
    }
    else
    {
      newPolys->Allocate(newPolys->EstimateSize(numStrips,5));
    }
    inStrips = input->GetStrips();
    for ( inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
      vtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
    }
    Mesh->SetPolys(newPolys);
    newPolys->Delete();
  }
  else
  {
    newPolys = inPolys;
    Mesh->SetPolys(newPolys);
  }
  Mesh->BuildLinks();

  // Allocate storage for lines/points (arbitrary allocation sizes)
  //
  newPts = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  newPts->Allocate(numPts/10,numPts);
  newLines = vtkCellArray::New();
  newLines->Allocate(numPts/10);
  if ( this->Coloring )
  {
    newScalars = vtkFloatArray::New();
    newScalars->SetName("Edge Types");
    newScalars->Allocate(numCells/10,numCells);
  }

  outPD->CopyAllocate(pd, numPts);
  outCD->CopyAllocate(cd, numCells);

  // Get our locator for merging points
  //
  if ( this->Locator == NULL )
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion (newPts, input->GetBounds());

  // Loop over all polygons generating boundary, non-manifold,
  // and feature edges
  //
  if ( this->FeatureEdges )
  {
    polyNormals = vtkFloatArray::New();
    polyNormals->SetNumberOfComponents(3);
    polyNormals->Allocate(3*newPolys->GetNumberOfCells());

    for (cellId=0, newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts);
    cellId++)
    {
      vtkPolygon::ComputeNormal(inPts,npts,pts,n);
      polyNormals->InsertTuple(cellId,n);
    }

    cosAngle = cos( vtkMath::RadiansFromDegrees( this->FeatureAngle ) );
  }

  neighbors = vtkIdList::New();
  neighbors->Allocate(VTK_CELL_SIZE);

  int abort=0;
  vtkIdType progressInterval=numCells/20+1;

  numBEdges = numNonManifoldEdges = numFedges = numManifoldEdges = 0;
  for (cellId=0, newPolys->InitTraversal();
       newPolys->GetNextCell(npts,pts) && !abort; cellId++)
  {
    if ( ! (cellId % progressInterval) ) //manage progress / early abort
    {
      this->UpdateProgress (static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    for (i=0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i+1)%npts];

      Mesh->GetCellEdgeNeighbors(cellId,p1,p2, neighbors);
      numNei = neighbors->GetNumberOfIds();

      if ( this->BoundaryEdges && numNei < 1 )
      {
        if (ghosts &&
            ghosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
        {
          continue;
        }
        else
        {
          numBEdges++;
          scalar = 0.0;
        }
      }

      else if ( this->NonManifoldEdges && numNei > 1 )
      {
        // check to make sure that this edge hasn't been created before
        for (j=0; j < numNei; j++)
        {
          if ( neighbors->GetId(j) < cellId )
          {
            break;
          }
        }
        if ( j >= numNei )
        {
          if (ghosts &&
              ghosts[cellId]  & vtkDataSetAttributes::DUPLICATECELL)
          {
            continue;
          }
          else
          {
            numNonManifoldEdges++;
            scalar = 0.222222;
          }
        }
        else
        {
          continue;
        }
      }
      else if ( this->FeatureEdges &&
                numNei == 1 && (nei=neighbors->GetId(0)) > cellId )
      {
        double neiTuple[3];
        double cellTuple[3];
        polyNormals->GetTuple(nei, neiTuple);
        polyNormals->GetTuple(cellId, cellTuple);
        if ( vtkMath::Dot(neiTuple, cellTuple) <= cosAngle )
        {
          if (ghosts &&
              ghosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
          {
            continue;
          }
          else
          {
            numFedges++;
            scalar = 0.444444;
          }
        }
        else
        {
          continue;
        }
      }
      else if ( this->ManifoldEdges &&
                numNei == 1 && neighbors->GetId(0) > cellId )
      {
        if (ghosts &&
            ghosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
        {
          continue;
        }
        else
        {
          numManifoldEdges++;
          scalar = 0.666667;
        }
      }
      else
      {
        continue;
      }

      // Add edge to output
      Mesh->GetPoint(p1, x1);
      Mesh->GetPoint(p2, x2);

      if ( this->Locator->InsertUniquePoint(x1, lineIds[0]) )
      {
        outPD->CopyData (pd,p1,lineIds[0]);
      }

      if ( this->Locator->InsertUniquePoint(x2, lineIds[1]) )
      {
        outPD->CopyData (pd,p2,lineIds[1]);
      }

      newId = newLines->InsertNextCell(2,lineIds);
      outCD->CopyData (cd,cellId,newId);
      if ( this->Coloring )
      {
        newScalars->InsertTuple(newId, &scalar);
      }
    }
  }

  vtkDebugMacro(<<"Created " << numBEdges << " boundary edges, "
                << numNonManifoldEdges << " non-manifold edges, "
                << numFedges << " feature edges, "
                << numManifoldEdges << " manifold edges");

  //  Update ourselves.
  //
  if ( this->FeatureEdges )
  {
    polyNormals->Delete();
  }

  Mesh->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  neighbors->Delete();

  output->SetLines(newLines);
  newLines->Delete();
  this->Locator->Initialize();//release any extra memory
  if ( this->Coloring )
  {
    int idx = outCD->AddArray(newScalars);
    outCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  return 1;
}

void vtkFeatureEdges::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
  {
    this->Locator = vtkMergePoints::New();
  }
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkFeatureEdges::SetLocator(vtkIncrementalPointLocator *locator)
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

vtkMTimeType vtkFeatureEdges::GetMTime()
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

int vtkFeatureEdges::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int numPieces, ghostLevel;

  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                ghostLevel + 1);
  }

  return 1;
}

void vtkFeatureEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Boundary Edges: " << (this->BoundaryEdges ? "On\n" : "Off\n");
  os << indent << "Feature Edges: " << (this->FeatureEdges ? "On\n" : "Off\n");
  os << indent << "Non-Manifold Edges: " << (this->NonManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Manifold Edges: " << (this->ManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Coloring: " << (this->Coloring ? "On\n" : "Off\n");

  if ( this->Locator )
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
