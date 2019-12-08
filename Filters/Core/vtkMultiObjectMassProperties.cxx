/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiObjectMassProperties.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiObjectMassProperties.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"

vtkStandardNewMacro(vtkMultiObjectMassProperties);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

struct ComputeProperties
{
  vtkPolyData* Mesh;
  vtkPoints* Points;
  double Center[3];
  unsigned char* Orient;
  double* Areas;
  double* Volumes;

  vtkSMPThreadLocalObject<vtkPolygon> Polygon;
  vtkSMPThreadLocalObject<vtkIdList> Triangles;

  ComputeProperties(
    vtkPolyData* mesh, double center[3], unsigned char* orient, double* areas, double* volumes)
    : Mesh(mesh)
    , Orient(orient)
    , Areas(areas)
    , Volumes(volumes)
  {
    this->Points = this->Mesh->GetPoints();

    this->Center[0] = center[0];
    this->Center[1] = center[1];
    this->Center[2] = center[2];
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkPolygon*& polygon = this->Polygon.Local();
    polygon->PointIds->Allocate(128);
    polygon->Points->Allocate(128);

    vtkIdList*& tris = this->Triangles.Local();
    tris->Allocate(128); // allocate some memory
  }

  // There is a lot of data shuffling between the dataset and the cells going
  // on. This could be optimized if it ever comes to that.
  void operator()(vtkIdType polyId, vtkIdType endPolyId)
  {
    vtkPoints* inPts = this->Points;
    double x[3], n[3];
    double* areas = this->Areas + polyId;
    double* volumes = this->Volumes + polyId;
    const unsigned char* orient = this->Orient + polyId;
    const double* c = this->Center;
    vtkIdType npts;
    const vtkIdType* pts;
    vtkIdType numTris;
    vtkPolygon*& poly = this->Polygon.Local();
    vtkIdList*& tris = this->Triangles.Local();
    int i;
    double x0[3], x1[3], x2[3], vol;
    double v210, v120, v201, v021, v102, v012;

    for (; polyId < endPolyId; ++polyId)
    {
      this->Mesh->GetCellPoints(polyId, npts, pts);

      // Compute area of polygon.
      *areas++ = poly->ComputeArea(inPts, npts, pts, n);

      // Now need to compute volume contribution of polygon.
      poly->PointIds->SetNumberOfIds(npts);
      poly->Points->SetNumberOfPoints(npts);
      for (i = 0; i < npts; i++)
      {
        poly->PointIds->SetId(i, pts[i]);
        inPts->GetPoint(pts[i], x);
        poly->Points->SetPoint(i, x);
      }

      // The volume computation implemented using signed tetrahedra from
      // generating triangles. Thus polygons may need tessellation.
      poly->Triangulate(tris);
      numTris = tris->GetNumberOfIds() / 3;

      // Loop over each triangle from the tessellation
      for (vol = 0.0, i = 0; i < numTris; i++)
      {
        poly->Points->GetPoint(tris->GetId(3 * i), x0);
        poly->Points->GetPoint(tris->GetId(3 * i + 1), x1);
        poly->Points->GetPoint(tris->GetId(3 * i + 2), x2);

        // Better numerics if the volume is computed with respect to
        // a nearby point... here we use the center point of the data.
        v210 = (x2[0] - c[0]) * (x1[1] - c[1]) * (x0[2] - c[2]);
        v120 = (x1[0] - c[0]) * (x2[1] - c[1]) * (x0[2] - c[2]);
        v201 = (x2[0] - c[0]) * (x0[1] - c[1]) * (x1[2] - c[2]);
        v021 = (x0[0] - c[0]) * (x2[1] - c[1]) * (x1[2] - c[2]);
        v102 = (x1[0] - c[0]) * (x0[1] - c[1]) * (x2[2] - c[2]);
        v012 = (x0[0] - c[0]) * (x1[1] - c[1]) * (x2[2] - c[2]);

        vol += -v210 + v120 + v201 - v021 - v102 + v012;
      } // for each triangle in this polygon

      // Ordering consistency affects sign of volume contribution
      *volumes++ = (1.0 / 6.0) * (*orient++ != 0 ? 1.0 : -1.0) * vol;
    } // for this polygon
  }

  void Reduce() {}

  static void Execute(vtkIdType numPolys, vtkPolyData* output, double center[3],
    unsigned char* orient, double* areas, double* volumes)
  {
    ComputeProperties compute(output, center, orient, areas, volumes);
    vtkSMPTools::For(0, numPolys, compute);
  }
};

} // anonymous namespace

//================= Begin VTK class proper =======================================
//----------------------------------------------------------------------------
// Constructs with initial 0 values.
vtkMultiObjectMassProperties::vtkMultiObjectMassProperties()
{
  this->SkipValidityCheck = 0;
  this->AllValid = 0;
  this->TotalVolume = 0.0;
  this->TotalArea = 0.0;

  // The labeling (e.g., ids) of objects. The ObjectValidity is a 0/1 flag
  // indicating whether the object is manifold (i.e., valid) meaning every
  // edge is used exactly two times.
  this->NumberOfObjects = 0;
  this->ObjectIds = nullptr;
  this->ObjectValidity = nullptr;
  this->ObjectVolumes = nullptr;
  this->ObjectAreas = nullptr;

  // Processing waves for performing connected traversal
  this->CellNeighbors = vtkIdList::New();
  this->Wave = nullptr;
  this->Wave2 = nullptr;
}

//----------------------------------------------------------------------------
// Destroy any allocated memory.
vtkMultiObjectMassProperties::~vtkMultiObjectMassProperties()
{
  this->CellNeighbors->Delete();
}

//----------------------------------------------------------------------------
// Description:
// This method measures volume, surface area, and normalized shape index.
// Currently, the input is a ploydata which consists of triangles.
int vtkMultiObjectMassProperties::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output. Check to make sure data is available.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPolys = input->GetPolys()->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();

  this->AllValid = 1; // assumed valid until proven otherwise
  this->TotalArea = 0.0;
  this->TotalVolume = 0.0;
  if (numPolys < 1 || numPts < 1)
  {
    vtkErrorMacro(<< "No data!");
    return 1;
  }

  // Attribute data
  vtkPointData *pd = input->GetPointData(), *outputPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outputCD = output->GetCellData();

  // Determine if some data is being skipped over and either shallow copy out
  // or copy the cell attribute data and prune the extra cells. Points are
  // always passed through.
  vtkIdType polyId;
  vtkIdType numCells = input->GetNumberOfCells();
  if (numCells == numPolys)
  {
    // Just copy stuff through and we'll add arrays
    output->CopyStructure(input);
    outputPD->PassData(pd);
    outputCD->PassData(cd);
  }
  else
  {
    vtkWarningMacro(<< "Skipping some non-poly cells");

    // Pass points through, can always use vtkCleanPolyData to eliminate
    // unused points.
    output->SetPoints(input->GetPoints());
    outputPD->PassData(pd);

    // Just process polys and copy over associated cell data
    input->BuildCells();
    output->SetPolys(input->GetPolys());
    vtkIdType cellId;
    unsigned char cellType;
    for (polyId = 0, cellId = 0; cellId < numCells; ++cellId)
    {
      cellType = input->GetCellType(cellId);
      if (cellType == VTK_TRIANGLE || cellType == VTK_QUAD || cellType == VTK_POLYGON)
      {
        outputCD->CopyData(cd, cellId, polyId);
        polyId++;
      }
    }
  }

  // Okay to identify objects, perform connected traversal. Requires
  // neighborhood information (traverse via shared edges). All edges in an
  // object must be used exactly twice if the object is considered valid.
  this->NumberOfObjects = 0;
  output->BuildLinks();

  bool performValidityCheck = false;
  int idx;
  vtkIdType* objectIds;
  vtkIdTypeArray* objectIdsArray =
    static_cast<vtkIdTypeArray*>(output->GetCellData()->GetArray("ObjectIds", idx));
  if (!this->SkipValidityCheck || objectIdsArray == nullptr ||
    objectIdsArray->GetDataType() != VTK_ID_TYPE)
  {
    performValidityCheck = true;
    this->ObjectIds = objectIdsArray = vtkIdTypeArray::New();
    this->ObjectIds->SetName("ObjectIds");
    this->ObjectIds->SetNumberOfTuples(numPolys);
    outputCD->AddArray(objectIdsArray);
    objectIds = objectIdsArray->GetPointer(0);
    std::fill_n(objectIds, numPolys, (-1));
  }
  else
  {
    objectIds = objectIdsArray->GetPointer(0);
  }

  this->ObjectValidity = vtkUnsignedCharArray::New();
  this->ObjectValidity->SetName("ObjectValidity");
  output->GetFieldData()->AddArray(this->ObjectValidity);
  unsigned char* valid;

  // All polygons initially assumed oriented properly
  unsigned char* orient = new unsigned char[numPolys];
  std::fill_n(orient, numPolys, 1);

  // This traversal identifies the number of objects in the mesh, and whether
  // they are valid (closed, manifold).
  if (performValidityCheck)
  {
    // Loop over all polys and traverse unmarked, edge connected polygons. Make sure
    // the objects are valid, and label polygons with object ids.
    this->Wave = vtkIdList::New();
    this->Wave->Allocate(numPolys / 4 + 1, numPolys);
    this->Wave2 = vtkIdList::New();
    this->Wave2->Allocate(numPolys / 4 + 1, numPolys);

    for (polyId = 0; polyId < numPolys; ++polyId)
    {
      if (objectIds[polyId] < 0)
      {
        // found another object
        this->Wave->InsertNextId(polyId);
        objectIds[polyId] = this->NumberOfObjects;
        this->ObjectValidity->InsertTuple1(this->NumberOfObjects, 1);
        this->TraverseAndMark(output, objectIds, this->ObjectValidity, orient);
        this->NumberOfObjects++;
        this->Wave->Reset();
        this->Wave2->Reset();
      }
    }

    // Clean up
    this->Wave->Delete();
    this->Wave2->Delete();
    valid = this->ObjectValidity->GetPointer(0);

    // Roll up the valid flag
    for (idx = 0; idx < this->NumberOfObjects; ++idx)
    {
      this->AllValid &= valid[idx];
    }
  } // if validity check required

  // Here we assume that the object ids provided are associated with valid
  // objects. However we need to traverse provided array to determine number
  // of objects.
  else
  {
    this->NumberOfObjects = 0;
    for (polyId = 0; polyId < numPolys; ++polyId)
    {
      if (this->NumberOfObjects < (objectIds[polyId] + 1))
      {
        this->NumberOfObjects = (objectIds[polyId] + 1);
      }
    }
    this->ObjectValidity->SetNumberOfTuples(this->NumberOfObjects);
    valid = this->ObjectValidity->GetPointer(0);
    std::fill_n(valid, this->NumberOfObjects, 1);
    this->AllValid = 1;
  }

  // Now compute the areas and volumes. This can be done in parallel. We
  // compute on a per polygon basis and sum the results later. Note that the
  // polygon volumes are the contribution that the polygon makes to the total
  // object volume (may be negative or positive).
  vtkDoubleArray* polyAreas = vtkDoubleArray::New();
  polyAreas->SetName("Areas");
  polyAreas->SetNumberOfTuples(numPolys);
  outputCD->AddArray(polyAreas);
  double* pAreas = polyAreas->GetPointer(0);

  vtkDoubleArray* polyVolumes = vtkDoubleArray::New();
  polyVolumes->SetName("Volumes");
  polyVolumes->SetNumberOfTuples(numPolys);
  outputCD->AddArray(polyVolumes);
  double* pVolumes = polyVolumes->GetPointer(0);

  // Need reference origin to compute volumes
  double center[3];
  output->GetCenter(center);

  // Compute areas and volumes in parallel
  ComputeProperties::Execute(numPolys, output, center, orient,
                             polyAreas->GetPointer(0),
                             polyVolumes->GetPointer(0));

  // Roll up the results into total results on a per object basis.
  this->ObjectAreas = vtkDoubleArray::New();
  this->ObjectAreas->SetName("ObjectAreas");
  output->GetFieldData()->AddArray(this->ObjectAreas);
  this->ObjectAreas->SetNumberOfTuples(this->NumberOfObjects);
  this->ObjectAreas->FillValue(0.0);
  double* areas = this->ObjectAreas->GetPointer(0);
  std::fill_n(areas, this->NumberOfObjects, 0.0);

  this->ObjectVolumes = vtkDoubleArray::New();
  this->ObjectVolumes->SetName("ObjectVolumes");
  output->GetFieldData()->AddArray(this->ObjectVolumes);
  this->ObjectVolumes->SetNumberOfTuples(this->NumberOfObjects);
  double* volumes = this->ObjectVolumes->GetPointer(0);
  std::fill_n(volumes, this->NumberOfObjects, 0.0);

  // Roll up final numbers
  for (polyId = 0; polyId < numPolys; ++polyId)
  {
    areas[objectIds[polyId]] += pAreas[polyId];
    volumes[objectIds[polyId]] += pVolumes[polyId];
  }

  // Volumes are always positive
  for (idx = 0; idx < this->NumberOfObjects; ++idx)
  {
    this->TotalArea += areas[idx];
    if (valid[idx])
    {
      volumes[idx] = fabs(volumes[idx]);
      this->TotalVolume += volumes[idx];
    }
  }

  // Clean up and get out
  if (performValidityCheck)
  {
    this->ObjectIds->Delete();
  }
  this->ObjectValidity->Delete();
  this->ObjectVolumes->Delete();
  this->ObjectAreas->Delete();
  polyAreas->Delete();
  polyVolumes->Delete();
  delete[] orient;

  return 1;
}

//----------------------------------------------------------------------------
// This method not only identified connected objects, it ensures that they
// are manifold (i.e., valid) and polygons are oriented in a consistent manner.
// Consistent normal orientation is necessary to correctly compute volumes.
void vtkMultiObjectMassProperties::TraverseAndMark(
  vtkPolyData* output, vtkIdType* objectIds, vtkDataArray* valid, unsigned char* orient)
{
  vtkIdType i, j, k, numIds, polyId, npts, p0, p1, numNei, neiId;
  const vtkIdType* pts;
  vtkIdType numNeiPts;
  const vtkIdType* neiPts;
  vtkIdList* wave = this->Wave;
  vtkIdList* wave2 = this->Wave2;
  vtkIdList* tmpWave;

  // Process all cells in this connected wave
  while ((numIds = wave->GetNumberOfIds()) > 0)
  {
    for (i = 0; i < numIds; i++)
    {
      polyId = wave->GetId(i);
      output->GetCellPoints(polyId, npts, pts);

      for (j = 0; j < npts; ++j)
      {
        p0 = pts[j];
        p1 = pts[(j + 1) % npts];

        output->GetCellEdgeNeighbors(polyId, p0, p1, this->CellNeighbors);

        // Manifold requires exactly one edge neighbor. Don't worry about
        // consistency check with invalid objects.
        numNei = this->CellNeighbors->GetNumberOfIds();
        if (numNei != 1)
        { // mark invalid
          valid->InsertTuple1(this->NumberOfObjects, 0);
        }

        // Determine whether neighbor is consistent with the current
        // cell. The neighbor ordering of the edge (p0,p1) should be reversed
        // (p1,p0). Otherwise it is inconsistent and is marked as such. For
        // non-valid objects we don't care about consistency.
        else // have one neighbor.
        {
          neiId = this->CellNeighbors->GetId(0);
          output->GetCellPoints(neiId, numNeiPts, neiPts);
          for (k = 0; k < numNeiPts; ++k)
          {
            if (neiPts[k] == p1)
            {
              break;
            }
          }
          if (neiPts[(k + 1) % numNeiPts] != p0)
          {
            orient[neiId] = (orient[polyId] == 1 ? 0 : 1);
          }
        }

        for (k = 0; k < numNei; ++k)
        {
          neiId = this->CellNeighbors->GetId(k);
          if (objectIds[neiId] < 0)
          {
            objectIds[neiId] = this->NumberOfObjects;
            wave2->InsertNextId(neiId);
          }
        } // for all edge neighbors
      }   // for all edges
    }     // for all cells in this wave

    tmpWave = wave;
    wave = wave2;
    wave2 = tmpWave;
    tmpWave->Reset();
  } // while wave is not empty
}

//----------------------------------------------------------------------------
void vtkMultiObjectMassProperties::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Skip Validity Check: " << this->SkipValidityCheck << "\n";
  os << indent << "Number of Objects: " << this->NumberOfObjects << "\n";
  os << indent << "All Valid: " << this->AllValid << "\n";
  os << indent << "Total Volume: " << this->TotalVolume << "\n";
  os << indent << "Total Area: " << this->TotalArea << "\n";
}
