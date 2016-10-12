/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHAVSVolumeMapper.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/* Copyright 2005, 2006 by University of Utah. */

#include "vtkHAVSVolumeMapper.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellIterator.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <set>
#include <vector>

#include <cmath>

// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkHAVSVolumeMapper)

//----------------------------------------------------------------------------
// A helper class for sorting faces by their centroids
class vtkHAVSSortedFace
{
public:
  vtkHAVSSortedFace() {}
  vtkHAVSSortedFace(unsigned int f, unsigned int d)
  {
    this->Face = f;
    this->Distance = d ^ ((-(static_cast<int>(d) >> 31)) | 0x80000000);
  }

  bool operator<(const vtkHAVSSortedFace &rhs) const
  {
    return this->Distance < rhs.Distance;
  }
  bool operator<=(const vtkHAVSSortedFace &rhs) const
  {
    return this->Distance <= rhs.Distance;
  }
  bool operator>=(const vtkHAVSSortedFace &rhs) const
  {
    return this->Distance >= rhs.Distance;
  }

  unsigned int Face;
  unsigned int Distance;
};

//----------------------------------------------------------------------------
// A helper class to filter unique faces
class vtkHAVSFace
{
public:
  vtkHAVSFace(unsigned int a, unsigned int b, unsigned int c)
  {
    this->Boundary = true;
    this->Idx[0] = a;
    this->Idx[1] = b;
    this->Idx[2] = c;
  }

  vtkHAVSFace()
  {
    this->Boundary = true;
    this->Idx[0] = 0;
    this->Idx[1] = 0;
    this->Idx[2] = 0;
  }

  unsigned int Idx[3];
  mutable bool Boundary;
};

//----------------------------------------------------------------------------
// An STL set to filter unique triangles
class vtkHAVSFaceSetPIMPL
{
public:
  vtkHAVSFaceSetPIMPL()
  {
  }
  ~vtkHAVSFaceSetPIMPL()
  {
  }

  // Compare two triangles
  struct vtkHAVSLTFace
  {
    bool operator() (const vtkHAVSFace &f1, const vtkHAVSFace &f2) const
    {
      unsigned int min1, mid1, max1, min2, mid2, max2;

      min1 = (f1.Idx[0] < f1.Idx[1] && f1.Idx[0] < f1.Idx[2]) ? f1.Idx[0] :
        ((f1.Idx[1] < f1.Idx[2]) ? f1.Idx[1] : f1.Idx[2]);
      max1 = (f1.Idx[0] > f1.Idx[1] && f1.Idx[0] > f1.Idx[2]) ? f1.Idx[0] :
        ((f1.Idx[1] > f1.Idx[2]) ? f1.Idx[1] : f1.Idx[2]);
      mid1 = (f1.Idx[0] != min1 && f1.Idx[0] != max1) ? f1.Idx[0] :
        ((f1.Idx[1] != min1 && f1.Idx[1] != max1) ? f1.Idx[1] : f1.Idx[2]);

      min2 = (f2.Idx[0] < f2.Idx[1] && f2.Idx[0] < f2.Idx[2]) ? f2.Idx[0] :
        ((f2.Idx[1] < f2.Idx[2]) ? f2.Idx[1] : f2.Idx[2]);
      max2 = (f2.Idx[0] > f2.Idx[1] && f2.Idx[0] > f2.Idx[2]) ? f2.Idx[0] :
        ((f2.Idx[1] > f2.Idx[2]) ? f2.Idx[1] : f2.Idx[2]);
      mid2 = (f2.Idx[0] != min2 && f2.Idx[0] != max2) ? f2.Idx[0] :
        ((f2.Idx[1] != min2 && f2.Idx[1] != max2) ? f2.Idx[1] : f2.Idx[2]);

      if (min1 == min2)
      {
        if (mid1 == mid2)
        {
          return max1 < max2;
        }
        else
        {
          return mid1 < mid2;
        }
      }
      return min1 < min2;
    }
  };

  std::set<vtkHAVSFace, vtkHAVSLTFace> FaceSet;
};

//----------------------------------------------------------------------------
// A helper classes to build a scalar histogram
class vtkHAVSScalarInterval
{
public:
  vtkHAVSScalarInterval() {};
  void AddFace(unsigned int f) { this->Faces.push_back(f);};
  unsigned int GetSize() { return static_cast<unsigned int>(this->Faces.size()); }
  unsigned int GetFace(unsigned int f) { return this->Faces[f]; }

private:
  std::vector<unsigned int> Faces;
};

//----------------------------------------------------------------------------
// A helper classes to build a scalar histogram
class vtkHAVSScalarHistogram
{
private:
  vtkHAVSScalarInterval *ScalarTable;
  unsigned int NumberOfBuckets;
  unsigned int NumberOfFaces;

public:
  vtkHAVSScalarHistogram()
  {
    this->ScalarTable = NULL;
    this->NumberOfBuckets = 0;
  }

  vtkHAVSScalarHistogram(unsigned int nBuckets)
  {
    this->NumberOfBuckets = nBuckets;
    this->ScalarTable = new vtkHAVSScalarInterval[nBuckets];
    this->NumberOfFaces = 0;
  }

  ~vtkHAVSScalarHistogram()
  {
    if (this->ScalarTable) { delete [] ScalarTable; }
  }

  void DefineBuckets(unsigned int nBuckets)
  {
    this->NumberOfBuckets = nBuckets;
    this->ScalarTable = new vtkHAVSScalarInterval[nBuckets];
    this->NumberOfFaces = 0;
  }

  void AddFace (float s, unsigned int f)
  {
    unsigned int i = (unsigned int) (s * this->NumberOfBuckets);
    if (i > this->NumberOfBuckets-1) { i = this->NumberOfBuckets-1; }
    this->ScalarTable[i].AddFace(f);
    this->NumberOfFaces++;
  }

  unsigned int GetFace(unsigned int i, unsigned int f)
  {
    return this->ScalarTable[i].GetFace(f);
  }

  unsigned int GetBucketSize(unsigned int i)
  {
    return this->ScalarTable[i].GetSize();
  }

  unsigned int GetNumberOfBuckets() { return this->NumberOfBuckets; }
  unsigned int GetNumberOfFaces() { return this->NumberOfFaces; }
  unsigned int GetMaxBucketSize()
  {
    unsigned int max = 0;
    for (unsigned int i = 0; i < this->NumberOfBuckets; i++)
    {
      if (this->ScalarTable[i].GetSize() > max)
      {
        max = this->ScalarTable[i].GetSize();
      }
    }
    return max;
  }
};


//----------------------------------------------------------------------------
// return the correct type of UnstructuredGridVolumeMapper
vtkHAVSVolumeMapper::vtkHAVSVolumeMapper()
{
  this->Vertices                   = NULL;
  this->Scalars                    = NULL;
  this->ScalarRange[0]             = 0.0;
  this->ScalarRange[1]             = 1.0;
  this->Triangles                  = NULL;
  this->OrderedTriangles           = NULL;
  this->SortedFaces                = NULL;
  this->RadixTemp                  = NULL;
  this->Centers                    = NULL;
  this->NumberOfVertices           = 0;
  this->NumberOfCells              = 0;
  this->NumberOfScalars            = 0;
  this->NumberOfTriangles          = 0;
  this->NumberOfBoundaryTriangles  = 0;
  this->NumberOfInternalTriangles  = 0;
  this->BoundaryTriangles          = NULL;
  this->InternalTriangles          = NULL;
  this->CurrentLevelOfDetail       = 100.0;
  this->LevelOfDetailTriangleCount = 0;
  this->LevelOfDetailTargetTime    = 0.1;
  this->LevelOfDetail              = false;
  this->LevelOfDetailMethod        = VTK_FIELD_LEVEL_OF_DETAIL;
  this->PartiallyRemoveNonConvexities = true;
  this->MaxEdgeLength              = 1.0;
  this->LevelOfDetailMaxEdgeLength = 1.0;
  this->UnitDistance               = 1.0;
  this->GPUDataStructures          = true;
  this->TransferFunction           = NULL;
  this->TransferFunctionSize       = 128;
  this->Initialized                = 0;
  this->KBufferSize                = VTK_KBUFFER_SIZE_6;
  this->KBufferState               = VTK_KBUFFER_SIZE_6;
  this->FrameNumber                = 0;
  this->TotalRenderTime            = 0.0;
  this->LastVolume                 = NULL;
}

//----------------------------------------------------------------------------
vtkHAVSVolumeMapper::~vtkHAVSVolumeMapper()
{
  delete [] this->Vertices;
  delete [] this->Scalars;
  delete [] this->Triangles;
  delete [] this->BoundaryTriangles;
  delete [] this->InternalTriangles;
  delete [] this->SortedFaces;
  delete [] this->RadixTemp;
  delete [] this->Centers;
  delete [] this->TransferFunction;
}

//----------------------------------------------------------------------------
// Filter unique triangles from tets, create vertex buffer objects or vertex
// arrays, and find the maximum edge length of the triangles to be used as a
// normalization in the lookup tables.
void vtkHAVSVolumeMapper::InitializePrimitives(vtkVolume *vol)
{
  // Check for valid input
  vtkUnstructuredGridBase *ugrid = this->GetInput();
  vtkIdType numCells = ugrid->GetNumberOfCells();
  if (!numCells)
  {
    this->InitializationError = vtkHAVSVolumeMapper::NO_CELLS;
    return;
  }
  bool tetrahedra = true;
  vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(ugrid->NewCellIterator());
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
       cellIter->GoToNextCell())
  {
    if (cellIter->GetNumberOfPoints() != 4 &&
        cellIter->GetNumberOfPoints() != 3)
    {
      tetrahedra = false;
      break;
    }
  }
  if (!tetrahedra)
  {
    this->InitializationError = vtkHAVSVolumeMapper::NON_TETRAHEDRA;
    return;
  }

  delete [] this->Vertices;
  delete [] this->Triangles;
  delete [] this->BoundaryTriangles;
  delete [] this->InternalTriangles;
  delete [] this->SortedFaces;
  delete [] this->RadixTemp;
  delete [] this->Centers;


  // Extract the triangles from the tetrahedra
  this->NumberOfCells = numCells;

  vtkHAVSFaceSetPIMPL *faceSetContainer = new vtkHAVSFaceSetPIMPL();

  std::pair<std::set<vtkHAVSFace, vtkHAVSFaceSetPIMPL::vtkHAVSLTFace>::iterator, bool> result1;
  std::pair<std::set<vtkHAVSFace, vtkHAVSFaceSetPIMPL::vtkHAVSLTFace>::iterator, bool> result2;
  std::pair<std::set<vtkHAVSFace, vtkHAVSFaceSetPIMPL::vtkHAVSLTFace>::iterator, bool> result3;
  std::pair<std::set<vtkHAVSFace, vtkHAVSFaceSetPIMPL::vtkHAVSLTFace>::iterator, bool> result4;

  // Insert faces into an stl set
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
       cellIter->GoToNextCell())
  {
    if (cellIter->GetNumberOfPoints() == 4)
    {
      vtkIdList *ids = cellIter->GetPointIds();

      vtkHAVSFace f1(ids->GetId(0), ids->GetId(1), ids->GetId(2));
      vtkHAVSFace f2(ids->GetId(0), ids->GetId(1), ids->GetId(3));
      vtkHAVSFace f3(ids->GetId(0), ids->GetId(2), ids->GetId(3));
      vtkHAVSFace f4(ids->GetId(1), ids->GetId(2), ids->GetId(3));

      result1 = faceSetContainer->FaceSet.insert(f1);
      result2 = faceSetContainer->FaceSet.insert(f2);
      result3 = faceSetContainer->FaceSet.insert(f3);
      result4 = faceSetContainer->FaceSet.insert(f4);

      if (!result1.second) { (*result1.first).Boundary = false; }
      if (!result2.second) { (*result2.first).Boundary = false; }
      if (!result3.second) { (*result3.first).Boundary = false; }
      if (!result4.second) { (*result4.first).Boundary = false; }
    }
    else if (cellIter->GetNumberOfPoints() == 3)
    {
      vtkIdList *ids = cellIter->GetPointIds();
      vtkHAVSFace f1(ids->GetId(0), ids->GetId(1), ids->GetId(2));
      result1 = faceSetContainer->FaceSet.insert(f1);
      if (!result1.second) { (*result1.first).Boundary = false; }
    }
  }

  int boundaryCount = 0;
  std::set<vtkHAVSFace, vtkHAVSFaceSetPIMPL::vtkHAVSLTFace>::iterator it;
  it = faceSetContainer->FaceSet.begin();
  while(it != faceSetContainer->FaceSet.end())
  {
    vtkHAVSFace f = *it++;
    if (f.Boundary)
    {
      boundaryCount++;
    }
  }

  this->NumberOfVertices = ugrid->GetNumberOfPoints();
  this->NumberOfTriangles = static_cast<unsigned int>(faceSetContainer->FaceSet.size());
  this->LevelOfDetailTriangleCount = this->NumberOfTriangles;
  this->NumberOfBoundaryTriangles = boundaryCount;
  this->NumberOfInternalTriangles =
    this->NumberOfTriangles - this->NumberOfBoundaryTriangles;
  this->Vertices = new float[this->NumberOfVertices*3];
  this->Triangles = new unsigned int[this->NumberOfTriangles*3];
  this->BoundaryTriangles =
    new unsigned int[this->NumberOfBoundaryTriangles];
  this->InternalTriangles =
    new unsigned int[this->NumberOfInternalTriangles];
  this->SortedFaces = new vtkHAVSSortedFace[this->NumberOfTriangles];
  this->RadixTemp = new vtkHAVSSortedFace[this->NumberOfTriangles];
  this->Centers = new float[this->NumberOfTriangles*3];

  // Fill up vertices
  for (unsigned int i = 0; i < this->NumberOfVertices; i++)
  {
    double *p = ugrid->GetPoint(i);
    for (int j = 0; j < 3; j++)
    {
      this->Vertices[i*3+j] = (float)p[j];
    }
  }

  // Fill up triangles with unique tetrahedra faces
  int iFaceCount = 0;
  int bFaceCount = 0;
  int faceCount = 0;
  it = faceSetContainer->FaceSet.begin();
  while(it != faceSetContainer->FaceSet.end())
  {
    vtkHAVSFace f = *it++;
    if (f.Boundary)
    {
      this->BoundaryTriangles[bFaceCount++] = faceCount;
    }
    else
    {
      this->InternalTriangles[iFaceCount++] = faceCount;
    }
    for (int j = 0; j < 3; j++)
    {
      this->Triangles[faceCount*3+j] = f.Idx[j];
    }
    faceCount++;
  }

  delete faceSetContainer;

  // Calculate triangle centers and max edge length
  float max = 0.0;
  for (unsigned int i = 0; i < this->NumberOfTriangles; i++)
  {
    int t1 = this->Triangles[i*3+0];
    int t2 = this->Triangles[i*3+1];
    int t3 = this->Triangles[i*3+2];
    double p1[3], p2[3], p3[3];
    for (int j = 0; j < 3; j++)
    {
      p1[j] = this->Vertices[t1*3+j];
      p2[j] = this->Vertices[t2*3+j];
      p3[j] = this->Vertices[t3*3+j];
    }
    float d1 = (p2[0]-p1[0])*(p2[0]-p1[0])+(p2[1]-p1[1])*(p2[1]-p1[1])+
      (p2[2]-p1[2])*(p2[2]-p1[2]);
    float d2 = (p3[0]-p1[0])*(p3[0]-p1[0])+(p3[1]-p1[1])*(p3[1]-p1[1])+
      (p3[2]-p1[2])*(p3[2]-p1[2]);
    float d3 = (p2[0]-p3[0])*(p2[0]-p3[0])+(p2[1]-p3[1])*(p2[1]-p3[1])+
      (p2[2]-p3[2])*(p2[2]-p3[2]);
    if (d1 > max) { max = d1; }
    if (d2 > max) { max = d2; }
    if (d3 > max) { max = d3; }
    for (int j = 0; j < 3; j++)
    {
      this->Centers[i*3+j] = (p1[j] + p2[j] + p3[j])/3.0;
    }
  }

  this->MaxEdgeLength = sqrt(max);
  this->LevelOfDetailMaxEdgeLength = ugrid->GetLength();
  this->UnitDistance = vol->GetProperty()->GetScalarOpacityUnitDistance();
}

//----------------------------------------------------------------------------
// Get current scalars, normalize them, and create GPU structure
void vtkHAVSVolumeMapper::InitializeScalars()
{
  vtkUnstructuredGridBase *ugrid = this->GetInput();

  if (this->Scalars) { delete [] this->Scalars; }
  this->Scalars = NULL;

  // Fill up scalars
  int UsingCellColor;
  vtkDataArray *scalarData = this->GetScalars(ugrid, this->ScalarMode,
                                              this->ArrayAccessMode,
                                              this->ArrayId,
                                              this->ArrayName,
                                              UsingCellColor);
  if (!scalarData)
  {
    this->InitializationError = vtkHAVSVolumeMapper::NO_SCALARS;
    return;
  }
  if (UsingCellColor)
  {
    this->InitializationError = vtkHAVSVolumeMapper::CELL_DATA;
    return;
  }

  this->NumberOfScalars = scalarData->GetNumberOfTuples();
  this->Scalars = new float[this->NumberOfScalars];

  for (unsigned int i = 0; i < this->NumberOfScalars; i++)
  {
    double *s = scalarData->GetTuple(i);
    this->Scalars[i] = (float)s[0];
  }

  // Normalize scalars
  if (this->NumberOfScalars)
  {
    scalarData->GetRange(this->ScalarRange,0);
    double diff = this->ScalarRange[1]-this->ScalarRange[0];
    for (unsigned int i = 0; i < this->NumberOfScalars; i++)
    {
      this->Scalars[i] = (this->Scalars[i] - this->ScalarRange[0])/diff;
    }
  }
}

//----------------------------------------------------------------------------
// Setup Level-Of-Detail Strategy
void vtkHAVSVolumeMapper::SetLevelOfDetailMethod(int method)
{
  this->LevelOfDetailMethod = method;
  if (this->Initialized)
  {
    InitializeLevelOfDetail();
  }
}

//----------------------------------------------------------------------------
// Initialize data structures for Level-of-Detail heuristics
void vtkHAVSVolumeMapper::InitializeLevelOfDetail()
{
  if (this->LevelOfDetailMethod == VTK_FIELD_LEVEL_OF_DETAIL)
  {
    if (!this->Scalars) { return; }

    const int nBuckets = 128;
    vtkHAVSScalarHistogram levelOfDetailScalarHistogram(nBuckets);

    for (unsigned int i = 0; i < this->NumberOfInternalTriangles; i++)
    {
      unsigned int f = this->InternalTriangles[i];
      float s1 = this->Scalars[this->Triangles[f*3+0]];
      float s2 = this->Scalars[this->Triangles[f+3+1]];
      float s3 = this->Scalars[this->Triangles[f+3+2]];
      levelOfDetailScalarHistogram.AddFace((s1+s2+s3)/3.0, f);
    }

    unsigned int vertCount = 0;
    for (unsigned int i = 0; i < levelOfDetailScalarHistogram.GetMaxBucketSize(); i++)
    {
      for (unsigned int j = 0; j < levelOfDetailScalarHistogram.GetNumberOfBuckets(); j++)
      {
        if (i < levelOfDetailScalarHistogram.GetBucketSize(j))
        {
          this->InternalTriangles[vertCount++] = levelOfDetailScalarHistogram.GetFace(j,i);
        }
      }
    }
  }
  else if (this->LevelOfDetailMethod == VTK_AREA_LEVEL_OF_DETAIL)
  {
    vtkHAVSSortedFace *areas = new vtkHAVSSortedFace[this->NumberOfInternalTriangles];
    vtkHAVSSortedFace *tmp = new vtkHAVSSortedFace[this->NumberOfInternalTriangles];
    for (unsigned int i = 0; i < this->NumberOfInternalTriangles; i++)
    {
      unsigned int f = this->InternalTriangles[i];
      int t1 = this->Triangles[f*3+0];
      int t2 = this->Triangles[f*3+1];
      int t3 = this->Triangles[f*3+2];
      double p1[3], p2[3], p3[3];
      for (int j = 0; j < 3; j++)
      {
        p1[j] = this->Vertices[t1*3+j];
        p2[j] = this->Vertices[t2*3+j];
        p3[j] = this->Vertices[t3*3+j];
      }

      // Calculate edge lengths
      float d1 = (p2[0]-p1[0])*(p2[0]-p1[0])+(p2[1]-p1[1])*(p2[1]-p1[1])+
        (p2[2]-p1[2])*(p2[2]-p1[2]);
      float d2 = (p3[0]-p1[0])*(p3[0]-p1[0])+(p3[1]-p1[1])*(p3[1]-p1[1])+
        (p3[2]-p1[2])*(p3[2]-p1[2]);
      float d3 = (p2[0]-p3[0])*(p2[0]-p3[0])+(p2[1]-p3[1])*(p2[1]-p3[1])+
        (p2[2]-p3[2])*(p2[2]-p3[2]);

      // Randomize area
      union float_to_unsigned_int
      {
        float f;
        unsigned int ui;
      };

      float_to_unsigned_int total;

      total.f = (d1+d2+d3) *
                     (static_cast<float>(rand())/static_cast<float>(RAND_MAX));

      vtkHAVSSortedFace a(f, total.ui);
      areas[i] = a;
    }

    this->FRadixSort(areas, tmp, 0, this->NumberOfInternalTriangles);

    // Put ranked triangles back into array
    for (unsigned int i = 0; i < this->NumberOfInternalTriangles; i++)
    {
      this->InternalTriangles[i] = areas[this->NumberOfInternalTriangles-1-i].Face;
    }
    delete [] areas;
    delete [] tmp;
  }
}

//----------------------------------------------------------------------------
// Prioritize triangles for the current Level-Of-Detail hueristic
void vtkHAVSVolumeMapper::UpdateLevelOfDetail(float renderTime)
{
  if (this->LevelOfDetail)
  {
    float adjust = this->LevelOfDetailTargetTime/renderTime;
    if (adjust <= 0.9 || adjust >= 1.1)
    {
      this->CurrentLevelOfDetail *= adjust;
      if (this->CurrentLevelOfDetail > 100.0) { this->CurrentLevelOfDetail = 100.0; }
    }

    this->LevelOfDetailTriangleCount =
      (unsigned int)(this->NumberOfBoundaryTriangles +
                     (this->CurrentLevelOfDetail/100.0)*(float)this->NumberOfInternalTriangles);
  }
  else
  {
    this->LevelOfDetailTriangleCount = this->NumberOfTriangles;
  }
}

//----------------------------------------------------------------------------
// Build the lookup tables used for partial pre-integration
void vtkHAVSVolumeMapper::InitializeLookupTables(vtkVolume *vol)
{
  // Build transfer function
  if (this->TransferFunction) { delete [] this->TransferFunction; }
  this->TransferFunction = new float[this->TransferFunctionSize*4];

  vtkVolumeProperty *property = vol->GetProperty();
  double x = this->ScalarRange[0];
  double dx = 1.0/((float)this->TransferFunctionSize-1.0)*(this->ScalarRange[1]-this->ScalarRange[0]);
  this->UnitDistance = property->GetScalarOpacityUnitDistance();

  if (property->GetColorChannels() == 1)
  {
    vtkPiecewiseFunction *gray = property->GetGrayTransferFunction();
    vtkPiecewiseFunction *alpha = property->GetScalarOpacity();
    double g, a;
    for (int i = 0; i < this->TransferFunctionSize; i++)
    {
      g = gray->GetValue(x);
      a = alpha->GetValue(x);

      this->TransferFunction[i*4+0] = g;
      this->TransferFunction[i*4+1] = g;
      this->TransferFunction[i*4+2] = g;
      this->TransferFunction[i*4+3] = a / this->UnitDistance;

      x+=dx;
    }
  }
  else
  {
    vtkColorTransferFunction *colors = property->GetRGBTransferFunction();
    vtkPiecewiseFunction *alpha = property->GetScalarOpacity();
    double c[3], a;
    for (int i = 0; i < this->TransferFunctionSize; i++)
    {
      colors->GetColor(x,c);
      a = alpha->GetValue(x);

      this->TransferFunction[i*4+0] = c[0];
      this->TransferFunction[i*4+1] = c[1];
      this->TransferFunction[i*4+2] = c[2];
      this->TransferFunction[i*4+3] = a / this->UnitDistance;

      x+=dx;
    }
  }
}

//--------------------------------------------------------------------------
// Sort a portion of the bits
void
vtkHAVSVolumeMapper::FRadix(int byte, int len, vtkHAVSSortedFace *source, vtkHAVSSortedFace *dest, int *count)
{
  unsigned int i, j;
  vtkHAVSSortedFace *k;

  static int index[256];
  index[0] = 0;
  for (i=1; i<256; i++)
    index[i]=index[i-1]+count[i-1];

  for (i=0; i<(unsigned int)len; i++ )
  {
    k = &source[i];
    j = *(unsigned int *)&k->Distance;
    dest[index[(j >> (byte*8))&0xff]++] = *k;
  }
}

//--------------------------------------------------------------------------
// Floating-point radix sort (AKA Huy Sort)
// Works only on 32 bit floating point numbers
void
vtkHAVSVolumeMapper::FRadixSort(vtkHAVSSortedFace *array, vtkHAVSSortedFace *temp, int lo, int up)
{
  int len = up-lo;
  unsigned int i;
  unsigned int u;

  vtkHAVSSortedFace * uints = array + lo;

  int count[4][256] = {{0}};

  // Generate count arrays
  for (i=0; i<(unsigned int)len; i++)
  {
    u = uints[i].Distance;
    count[0][u & 0xff]++;
    count[1][(u >> 8) & 0xff]++;
    count[2][(u >> 16) & 0xff]++;
    count[3][(u >> 24) & 0xff]++;
  }

  // Start sorting
  this->FRadix(0, len, uints, temp, count[0]);
  this->FRadix(1, len, temp, uints, count[1]);
  this->FRadix(2, len, uints, temp, count[2]);
  this->FRadix(3, len, temp, uints, count[3]);
}

//----------------------------------------------------------------------------
void vtkHAVSVolumeMapper::PartialVisibilitySort(float *eye)
{
  float dist2;
  vtkHAVSSortedFace sFace;
  unsigned int sFaceCount = 0;
  unsigned int i;

  for (i = 0; i < this->NumberOfBoundaryTriangles; i++)
  {
    unsigned int f = this->BoundaryTriangles[i];
    float *fc = &this->Centers[f*3];
    dist2 = (eye[0]-fc[0])*(eye[0]-fc[0]) +
      (eye[1]-fc[1])*(eye[1]-fc[1]) +
      (eye[2]-fc[2])*(eye[2]-fc[2]);
    union fori
    {
      float f;
      unsigned int i;
    } floatToInt;
    floatToInt.f = dist2;
    sFace = vtkHAVSSortedFace(f, floatToInt.i);
    this->SortedFaces[sFaceCount++] = sFace;
  }

  unsigned int internalCount =
    this->LevelOfDetailTriangleCount - this->NumberOfBoundaryTriangles;
  for (i = 0; i < internalCount; i++)
  {
    unsigned int f = this->InternalTriangles[i];
    float *fc = &this->Centers[f*3];
    dist2 = (eye[0]-fc[0])*(eye[0]-fc[0]) +
      (eye[1]-fc[1])*(eye[1]-fc[1]) +
      (eye[2]-fc[2])*(eye[2]-fc[2]);
    union fori
    {
      float f;
      unsigned int i;
    } floatToInt;
    floatToInt.f = dist2;
    sFace = vtkHAVSSortedFace(f, floatToInt.i);
    this->SortedFaces[sFaceCount++] = sFace;
  }

  // Sort indices
  this->FRadixSort(this->SortedFaces, this->RadixTemp, 0, this->LevelOfDetailTriangleCount);

  // Reorder triangles for rendering
  for(i = 0; i < this->LevelOfDetailTriangleCount; i++)
  {
    for(unsigned int j = 0; j < 3; j++)
    {
      this->OrderedTriangles[i*3+j] =
        (unsigned int)this->Triangles[this->SortedFaces[i].Face*3+j];
    }
  }
}

//----------------------------------------------------------------------------
bool vtkHAVSVolumeMapper::CheckInitializationError()
{
  if (this->InitializationError ==
      vtkHAVSVolumeMapper::NO_INIT_ERROR)
  {
    return false;
  }

  if (this->InitializationError ==
      vtkHAVSVolumeMapper::NON_TETRAHEDRA)
  {
    vtkErrorMacro(<< "Non-tetrahedral cells not supported!");
  }
  else if (this->InitializationError ==
           vtkHAVSVolumeMapper::UNSUPPORTED_EXTENSIONS)
  {
    vtkErrorMacro(<< "Required OpenGL extensions not supported!" );
  }
  else if (this->InitializationError ==
           vtkHAVSVolumeMapper::NO_SCALARS)
  {
    vtkErrorMacro(<< "Can't use HAVS without scalars!");
  }
  else if (this->InitializationError ==
           vtkHAVSVolumeMapper::CELL_DATA)
  {
    vtkErrorMacro(<< "Can't use HAVS with cell data!");
  }
  else if (this->InitializationError ==
           vtkHAVSVolumeMapper::NO_CELLS)
  {
    vtkErrorMacro(<< "No Cells!");
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkHAVSVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Initialized " << this->Initialized << endl;
  os << indent << "K-Buffer size: " << this->KBufferSize << endl;
  os << indent << "Level Of Detail: " << this->LevelOfDetail << endl;
  os << indent << "Level Of Detail Target Time: " << this->LevelOfDetailTargetTime << endl;
  os << indent << "Level Of Detail Method: " << this->LevelOfDetailMethod << endl;
  os << indent << "Current Level Of Detail: " << this->CurrentLevelOfDetail << endl;
  os << indent << "Number of Boundary Triangles: " << this->NumberOfBoundaryTriangles << endl;
  os << indent << "Number of Internal Triangles: " << this->NumberOfInternalTriangles << endl;
  os << indent << "Remove non-convexities: " << this->PartiallyRemoveNonConvexities << endl;
  os << indent << "Level Of Detail Max Edge Length: " << this->LevelOfDetailMaxEdgeLength << endl;
  os << indent << "Max Edge Length: " << this->MaxEdgeLength << endl;
  os << indent << "Unit Distance: " << this->UnitDistance << endl;
  os << indent << "TransferFunction Size: " << this->TransferFunctionSize << endl;
  os << indent << "GPU Data Structures: " << this->GPUDataStructures << endl;


  this->Superclass::PrintSelf(os,indent);
}

