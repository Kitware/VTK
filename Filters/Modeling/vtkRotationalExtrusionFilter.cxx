// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRotationalExtrusionFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRotationalExtrusionFilter);

//------------------------------------------------------------------------------
// Create object with capping on, angle of 360 degrees, resolution = 12, and
// no translation along z-axis.
// vector (0,0,1), and point (0,0,0).
vtkRotationalExtrusionFilter::vtkRotationalExtrusionFilter()
{
  this->Capping = 1;
  this->Angle = 360.0;
  this->DeltaRadius = 0.0;
  this->Translation = 0.0;
  this->Resolution = 12; // 30 degree increments
}

//------------------------------------------------------------------------------
int vtkRotationalExtrusionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts, numCells;
  vtkPointData* pd = input->GetPointData();
  vtkCellData* cd = input->GetCellData();
  vtkPolyData* mesh;
  vtkPoints* inPts;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  int numEdges;
  const vtkIdType* pts = nullptr;
  vtkIdType npts = 0;
  vtkIdType cellId, ptId, ncells;
  double x[3], newX[3], angleIncr, radIncr, transIncr;
  vtkPoints* newPts;
  vtkCellArray *newLines = nullptr, *newPolys = nullptr, *newStrips;
  vtkCell* edge;
  vtkIdList* cellIds;
  int i, j, k;
  vtkIdType p1, p2;
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();
  bool abort = false;

  // Initialize / check input
  //
  vtkDebugMacro(<< "Rotationally extruding data");

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
  {
    vtkErrorMacro(<< "No data to extrude!");
    return 1;
  }

  double normalizedRotationAxis[3] = { this->RotationAxis[0], this->RotationAxis[1],
    this->RotationAxis[2] };
  double norm = vtkMath::Normalize(normalizedRotationAxis);

  // if norm is equal to zero, the extrusion cannot be done
  if (norm == 0.0)
  {
    vtkErrorMacro(<< "Cannot perform extrusion around an axis with a norm of 0.");
    return 0;
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
  if (inPolys || inStrips)
  {
    mesh->BuildLinks();
  }

  // Allocate memory for output. We don't copy normals because surface geometry
  // is modified.
  //
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd, (this->Resolution + 1) * numPts);
  newPts = vtkPoints::New();
  newPts->Allocate((this->Resolution + 1) * numPts);
  if ((ncells = inVerts->GetNumberOfCells()) > 0)
  {
    newLines = vtkCellArray::New();
    newLines->AllocateEstimate(ncells, this->Resolution + 1);
  }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells() / 10 +
    inStrips->GetNumberOfCells() / 10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips = vtkCellArray::New();
  newStrips->AllocateEstimate(ncells, 2 * (this->Resolution + 1));
  outCD->CopyNormalsOff();
  outCD->CopyAllocate(cd, ncells);

  // copy points
  for (ptId = 0; ptId < numPts; ptId++) // base level
  {
    newPts->InsertPoint(ptId, inPts->GetPoint(ptId));
    outPD->CopyData(pd, ptId, ptId);
  }
  this->UpdateProgress(0.1);

  radIncr = this->DeltaRadius / this->Resolution;
  transIncr = this->Translation / this->Resolution;
  angleIncr = vtkMath::RadiansFromDegrees(this->Angle) / this->Resolution;

  double rotationAngleAndAxis[4] = { 0, this->RotationAxis[0], this->RotationAxis[1],
    this->RotationAxis[2] };

  for (i = 1; i <= this->Resolution; i++)
  {
    this->UpdateProgress(0.1 + 0.5 * (i - 1) / this->Resolution);
    for (ptId = 0; ptId < numPts; ptId++)
    {
      inPts->GetPoint(ptId, x);

      rotationAngleAndAxis[0] = i * angleIncr;
      vtkMath::RotateVectorByWXYZ(x, rotationAngleAndAxis, newX);

      newX[0] += normalizedRotationAxis[0] * i * transIncr;
      newX[1] += normalizedRotationAxis[1] * i * transIncr;
      newX[2] += normalizedRotationAxis[2] * i * transIncr;

      double projection[3];
      double radialVector[3];
      vtkMath::ProjectVector(newX, normalizedRotationAxis, projection);
      vtkMath::Subtract(newX, projection, radialVector);

      newX[0] += radialVector[0] * i * radIncr;
      newX[1] += radialVector[1] * i * radIncr;
      newX[2] += radialVector[2] * i * radIncr;

      newPts->InsertPoint(ptId + i * numPts, newX);
      outPD->CopyData(pd, ptId, ptId + i * numPts);
    }
  }

  // To ensure that cell attributes are in consistent order with the
  // cellId's, we process the verts, lines, polys and strips in order.
  vtkIdType newCellId = 0;
  int type;
  if (newLines) // there are verts which produce lines
  {
    for (cellId = 0; cellId < numCells && !abort; cellId++)
    {
      type = mesh->GetCellType(cellId);
      if (type == VTK_VERTEX || type == VTK_POLY_VERTEX)
      {
        mesh->GetCellPoints(cellId, npts, pts);
        for (i = 0; i < npts; i++)
        {
          ptId = pts[i];
          newLines->InsertNextCell(this->Resolution + 1);
          for (j = 0; j <= this->Resolution; j++)
          {
            newLines->InsertCellPoint(ptId + j * numPts);
          }
          outCD->CopyData(cd, cellId, newCellId++);
        }
      } // if a vertex or polyVertex
    }   // for all cells
  }     // if there are verts generating lines
  this->UpdateProgress(0.25);
  abort = this->CheckAbort();

  // If capping is on, copy 2D cells to output (plus create cap). Notice
  // that polygons are done first, then strips.
  //
  if (this->Capping &&
    (this->Angle != 360.0 || this->DeltaRadius != 0.0 || this->Translation != 0.0))
  {
    if (inPolys->GetNumberOfCells() > 0)
    {
      newPolys = vtkCellArray::New();
      newPolys->AllocateCopy(inPolys);

      for (cellId = 0; cellId < numCells && !abort; cellId++)
      {
        type = mesh->GetCellType(cellId);
        if (type == VTK_TRIANGLE || type == VTK_QUAD || type == VTK_POLYGON)
        {
          mesh->GetCellPoints(cellId, npts, pts);
          newPolys->InsertNextCell(npts, pts);
          outCD->CopyData(cd, cellId, newCellId++);
          newPolys->InsertNextCell(npts);
          for (i = 0; i < npts; i++)
          {
            newPolys->InsertCellPoint(pts[i] + this->Resolution * numPts);
          }
          outCD->CopyData(cd, cellId, newCellId++);
        }
      }
    }

    for (cellId = 0; cellId < numCells && !abort; cellId++)
    {
      type = mesh->GetCellType(cellId);
      if (type == VTK_TRIANGLE_STRIP)
      {
        mesh->GetCellPoints(cellId, npts, pts);
        newStrips->InsertNextCell(npts, pts);
        outCD->CopyData(cd, cellId, newCellId++);
        newStrips->InsertNextCell(npts);
        for (i = 0; i < npts; i++)
        {
          newStrips->InsertCellPoint(pts[i] + this->Resolution * numPts);
        }
        outCD->CopyData(cd, cellId, newCellId++);
      }
    }
  } // if capping
  this->UpdateProgress(0.5);
  abort = this->CheckAbort();

  // Now process lines, polys and/or strips to produce strips
  //
  if (inLines->GetNumberOfCells() || inPolys->GetNumberOfCells() || inStrips->GetNumberOfCells())
  {
    cellIds = vtkIdList::New();
    cellIds->Allocate(VTK_CELL_SIZE);
    vtkGenericCell* cell = vtkGenericCell::New();

    for (cellId = 0; cellId < numCells && !abort; cellId++)
    {
      type = mesh->GetCellType(cellId);
      if (type == VTK_LINE || type == VTK_POLY_LINE)
      {
        mesh->GetCellPoints(cellId, npts, pts);
        for (i = 0; i < (npts - 1); i++)
        {
          p1 = pts[i];
          p2 = pts[i + 1];
          newStrips->InsertNextCell(2 * (this->Resolution + 1));
          for (j = 0; j <= this->Resolution; j++)
          {
            newStrips->InsertCellPoint(p2 + j * numPts);
            newStrips->InsertCellPoint(p1 + j * numPts);
          }
          outCD->CopyData(cd, cellId, newCellId++);
        }
      } // if a line

      else if (type == VTK_TRIANGLE || type == VTK_QUAD || type == VTK_POLYGON ||
        type == VTK_TRIANGLE_STRIP)
      { // create strips from boundary edges
        mesh->GetCell(cellId, cell);
        numEdges = cell->GetNumberOfEdges();
        for (i = 0; i < numEdges; i++)
        {
          edge = cell->GetEdge(i);
          for (j = 0; j < (edge->GetNumberOfPoints() - 1); j++)
          {
            p1 = edge->PointIds->GetId(j);
            p2 = edge->PointIds->GetId(j + 1);
            mesh->GetCellEdgeNeighbors(cellId, p1, p2, cellIds);

            if (cellIds->GetNumberOfIds() < 1) // generate strip
            {
              newStrips->InsertNextCell(2 * (this->Resolution + 1));
              for (k = 0; k <= this->Resolution; k++)
              {
                newStrips->InsertCellPoint(p2 + k * numPts);
                newStrips->InsertCellPoint(p1 + k * numPts);
              }
              outCD->CopyData(cd, cellId, newCellId++);
            } // if boundary edge
          }   // for each sub-edge
        }     // for each edge
      }       // for each polygon or triangle strip
    }         // for all cells

    cellIds->Delete();
    cell->Delete();
  } // if strips are being generated
  this->UpdateProgress(1.00);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  mesh->Delete();

  if (newLines)
  {
    output->SetLines(newLines);
    newLines->Delete();
  }

  if (newPolys)
  {
    output->SetPolys(newPolys);
    newPolys->Delete();
  }

  output->SetStrips(newStrips);
  newStrips->Delete();

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
void vtkRotationalExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "Translation: " << this->Translation << "\n";
  os << indent << "Delta Radius: " << this->DeltaRadius << "\n";
  os << indent << "Rotation axis: (" << this->RotationAxis[0] << ", " << this->RotationAxis[1]
     << ", " << this->RotationAxis[2] << ")\n";
}
VTK_ABI_NAMESPACE_END
