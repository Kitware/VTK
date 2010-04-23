/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrderedTriangulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOrderedTriangulator.h"

#include "vtkCellArray.h"
#include "vtkEdgeTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"
#include "vtkHeap.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/list>
#include <vtkstd/vector>
#include <vtkstd/stack>
#include <vtkstd/map>
#include <assert.h>

vtkStandardNewMacro(vtkOrderedTriangulator);

#ifdef _WIN32_WCE
# ifndef __PLACEMENT_NEW_INLINE
#  define __PLACEMENT_NEW_INLINE
   inline void *__cdecl operator new(size_t, void *_P) { return (_P); }
#  if _MSC_VER >= 1200
    inline void __cdecl operator delete(void *, void *) { return; }
#  endif
# endif
#else
# ifdef VTK_USE_ANSI_STDLIB
#  include <new>
# else
#  include <new.h>
# endif
#endif

// Old HP compiler does not support operator delete that is called
// when a constructor called by operator new throws.
#if defined(__HP_aCC) && (__HP_aCC < 061200)
# define VTK_NO_PLACEMENT_DELETE
#endif
// SGI compiler does not support placement delete that is called when
// a constructor called by placement new throws.
#if defined(__sgi) && !defined(__GNUC__)
# define VTK_NO_PLACEMENT_DELETE
#endif

// Classes are used to represent points, faces, and tetras-------------------
// This data structure consists of points and tetras, with the face used
// temporarily as a place holder during triangulation.
struct OTPoint;
struct OTFace;
struct OTTetra;

//---Class represents a point (and related typedefs)--------------------------
// Note that the points has two sets of coordinates: the first the actual
// position X[3] and the second the coordinate used for performing
// triangulation (usually a parametric coordinate P[3]).
struct OTPoint
{
  OTPoint() : Type(Inside), Id(0), SortId(0), SortId2(0), OriginalId(0),
              InsertionId(0)
    {
      this->X[0] = this->X[1] = this->X[2] = 0.0;
      this->P[0] = this->P[1] = this->P[2] = 0.0;
    }

  enum PointClassification
    {Inside=0,Outside=1,Boundary=2,Added=3,NoInsert=4};
  PointClassification Type;
  double X[3]; //Actual position of point
  double P[3]; //Triangulation coordinate (typically parametric coordinate)

  //Id of originating point
  vtkIdType Id;

  //Id used to sort points prior to triangulation
  vtkIdType SortId;

  //Second id used to sort in triangulation
  //This can be used in situations where one id is not enough
  //(for example, when the id is related to an edge which
  // is described by two points)
  vtkIdType SortId2;

  //Id based on order seen in InsertPoint()
  vtkIdType OriginalId;
  //Id after sorting the points (i.e. order inserted into mesh)
  vtkIdType InsertionId;
};
struct PointListType : public vtkstd::vector<OTPoint>
{
  PointListType() : vtkstd::vector<OTPoint>() {}
  OTPoint* GetPointer(int ptId)
    {return &( *(this->begin()+ptId) ); }
};
typedef PointListType::iterator PointListIterator;

//---Class represents a face (and related typedefs)--------------------------
struct OTFace //used during tetra construction
{
  void *operator new(size_t size, vtkHeap *heap)
    {return heap->AllocateMemory(size);}
#if !defined(VTK_NO_PLACEMENT_DELETE)
  void operator delete(void*,vtkHeap*) {}
#endif

  OTPoint *Points[3]; //the three points of the face
  OTTetra *Neighbor;
  double   Normal[3];
  double   N2;

  void ComputePseudoNormal()
    {
      double v20[3], v10[3];
      v20[0] = this->Points[2]->P[0] - this->Points[0]->P[0];
      v20[1] = this->Points[2]->P[1] - this->Points[0]->P[1];
      v20[2] = this->Points[2]->P[2] - this->Points[0]->P[2];
      v10[0] = this->Points[1]->P[0] - this->Points[0]->P[0];
      v10[1] = this->Points[1]->P[1] - this->Points[0]->P[1];
      v10[2] = this->Points[1]->P[2] - this->Points[0]->P[2];
      vtkMath::Cross(v10,v20,this->Normal);
      this->N2 = vtkMath::Dot(this->Normal,this->Normal);
    }
  int IsValidCavityFace(double X[3],double tol2)
    {
      double vp[3], d;
      vp[0] = X[0] - this->Points[0]->P[0];
      vp[1] = X[1] - this->Points[0]->P[1];
      vp[2] = X[2] - this->Points[0]->P[2];
      d = vtkMath::Dot(vp,this->Normal);
      return ( (d > 0.0L && (d*d) > (tol2*this->N2)) ? 1 : 0 );
    }
};
typedef vtkstd::vector<OTFace*>            FaceListType;
typedef vtkstd::vector<OTFace*>::iterator  FaceListIterator;

//---Class represents a tetrahedron (and related typedefs)--------------------
typedef vtkstd::list<OTTetra*>             TetraListType;
typedef vtkstd::list<OTTetra*>::iterator   TetraListIterator;
struct TetraStackType : public vtkstd::stack<OTTetra*>
{
  TetraStackType() : vtkstd::stack<OTTetra*>() {}
  void clear() {while (!this->empty()) this->pop();}
};
typedef vtkstd::vector<OTTetra*>           TetraQueueType;
typedef vtkstd::vector<OTTetra*>::iterator TetraQueueIterator;

struct OTTetra
{
  void *operator new(size_t size, vtkHeap *heap)
    {return heap->AllocateMemory(size);}
#if !defined(VTK_NO_PLACEMENT_DELETE)
  void operator delete(void*,vtkHeap*) {}
#endif

  OTTetra() : Radius2(0.0L), CurrentPointId(-1), Type(OutsideCavity)
    {
    this->Center[0] = this->Center[1] = this->Center[2] = 0.0L;
    this->Points[0] = this->Points[1] = this->Points[2] = this->Points[3] = 0;
    this->Neighbors[0] = this->Neighbors[1] =
      this->Neighbors[2] = this->Neighbors[3] = 0;
    this->DeleteMe = 0;
    }

  // Center and radius squared of circumsphere of this tetra
  double Radius2;
  double Center[3];

  // Note: there is a direct correlation between the points and the faces
  // i.e., the ordering of the points and face neighbors.
  OTTetra *Neighbors[4]; //the four face neighbors
  OTPoint *Points[4]; //the four points

  // The following are used during point insertion
  int CurrentPointId; //indicated current point being inserted
  enum TetraClassification
    {Inside=0,Outside=1,All=2,InCavity=3,OutsideCavity=4,Exterior=5};
  TetraClassification Type;

  // Supporting triangulation operators
  void GetFacePoints(int i, OTFace *face);
  int InCircumSphere(double x[3]);
  TetraClassification DetermineType(); //inside, outside
  int DeleteMe;
};

//---Class represents the Delaunay triangulation using points and tetras.
// Additional support for the Delaunay triangulation process.
struct vtkOTMesh
{
  vtkOTMesh(vtkHeap *heap) :
    NumberOfTetrasClassifiedInside(0), NumberOfTemplates(0)
    {
      this->EdgeTable = vtkEdgeTable::New();
      this->Heap = heap;
    }
  ~vtkOTMesh()
    {
      this->EdgeTable->Delete();
    }

  PointListType   Points;          //Points in the mesh
  TetraListType   Tetras;          //Tetrahedra in the mesh
  FaceListType    CavityFaces;     //Faces forming an insertion cavity
  TetraQueueType  VisitedTetras;   //Those tetra already visited during insertion
  TetraStackType  TetraStack;      //Stack of tetra visited during point insertion
  TetraQueueType  DegenerateQueue; //Tetra involved in degenerate triangulation
  vtkEdgeTable   *EdgeTable;       //Edges used to create triangulation of cavity
  double          Tolerance2;      //Used to control error
  vtkHeap        *Heap;            //Many allocations occur in efficent heap

  int             NumberOfTetrasClassifiedInside;
  int             NumberOfTemplates;
  TetraListIterator CurrentTetra;

  void Reset()
    {
      this->Points.clear();
      this->Tetras.clear();
      this->CavityFaces.clear();
      this->VisitedTetras.clear();
      this->TetraStack.clear();
      this->DegenerateQueue.clear();
      this->EdgeTable->Reset();
    }

  OTTetra *CreateTetra(OTPoint *p, OTFace *face);
  OTTetra *WalkToTetra(OTTetra *t,double x[3],int depth,double bc[4]);
  int CreateInsertionCavity(OTPoint* p, OTTetra *tetra, double bc[4]);
  int ClassifyTetras();
  void DumpInsertionCavity(double x[3]);
};

//---Classes and typedefs used to support triangulation templates.
// Triangulation templates are used instead of Delaunay triangulation
// because they are so much faster. Because there are so many possible
// triangulations/templates possible, triangulation templates are
// computed on the fly and then cached.
//
// Two lists are kept. The first is a list of lists of templates for
// each cell type. The second is a list of templates for each cell.
//
// A specific template. The number of tetras and the tetra connectivity.
struct OTTemplate
{
  vtkIdType  NumberOfTetras;
  vtkIdType *Tetras;

  OTTemplate(vtkIdType numberOfTetras, vtkHeap *heap)
    {
      this->NumberOfTetras = numberOfTetras;
      this->Tetras = static_cast<vtkIdType*>(
        heap->AllocateMemory(sizeof(vtkIdType)*numberOfTetras*4) );
    }
  void *operator new(size_t size, vtkHeap *heap)
    {return heap->AllocateMemory(size);}
#if !defined(VTK_NO_PLACEMENT_DELETE)
  void operator delete(void*,vtkHeap*) {}
#endif
};


// Typedefs for a list of templates for a particular cell. Key is the
// template index.
typedef vtkstd::map<TemplateIDType,OTTemplate*>           TemplateList;
typedef vtkstd::map<TemplateIDType,OTTemplate*>::iterator TemplateListIterator;

//
// Typedefs for a list of lists of templates keyed on cell type
struct vtkOTTemplates : public vtkstd::map<int,TemplateList*> {};
typedef vtkstd::map<int,TemplateList*>::iterator TemplatesIterator;


//------------------------------------------------------------------------
vtkOrderedTriangulator::vtkOrderedTriangulator()
{
  // In place news (using allocators) are done here
  this->Heap = vtkHeap::New();
  this->Heap->SetBlockSize(500000);

  this->Mesh = new vtkOTMesh(this->Heap);
  this->NumberOfPoints = 0;
  this->PreSorted = 0;
  this->UseTwoSortIds = 0;

  this->UseTemplates = 0;
  this->NumberOfCellPoints = 0;
  this->NumberOfCellEdges = 0;
  this->Templates = new vtkOTTemplates;
  this->TemplateHeap = vtkHeap::New();
  this->TemplateHeap->SetBlockSize(250000);
}

//------------------------------------------------------------------------
vtkOrderedTriangulator::~vtkOrderedTriangulator()
{
  delete this->Mesh;
  this->Heap->Delete();

  TemplatesIterator titer;
  for (titer=this->Templates->begin(); titer != this->Templates->end(); ++titer)
    {
    delete (*titer).second;
    }
  delete this->Templates;
  this->TemplateHeap->Delete();
}

//------------------------------------------------------------------------
void vtkOrderedTriangulator::InitTriangulation(double xmin, double xmax,
                                               double ymin, double ymax,
                                               double zmin, double zmax,
                                               int numPts)
{
  double bounds[6];
  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->InitTriangulation(bounds,numPts);

  // The templates remain valid and are reused.
}

//------------------------------------------------------------------------
void vtkOrderedTriangulator::InitTriangulation(double bounds[6], int numPts)
{
  this->Heap->Reset();
  this->Mesh->Reset();
  this->NumberOfPoints = 0;
  this->MaximumNumberOfPoints = numPts;
  this->Mesh->Points.resize(numPts+6);

  for (int i=0; i<6; i++)
    {
    this->Bounds[i] = bounds[i];
    }
}

//------------------------------------------------------------------------
// Create an initial bounding Delaunay triangulation consisting of four
// tetras arranged in an octahedron.
void vtkOrderedTriangulator::Initialize()
{
  double length;
  double center[3];
  double radius2;

  // Set up the internal data structures. Space for six extra points
  // is allocated for the bounding triangulation.
  int numPts = this->MaximumNumberOfPoints;
  double *bounds = this->Bounds;

  // Create the initial Delaunay triangulation which is a
  // bounding octahedron: 6 points & 4 tetra.
  center[0] = (bounds[0]+bounds[1])/2.0;
  center[1] = (bounds[2]+bounds[3])/2.0;
  center[2] = (bounds[4]+bounds[5])/2.0;
  length = 2.0 * sqrt( (radius2 = (bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                 (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                 (bounds[5]-bounds[4])*(bounds[5]-bounds[4])) );
  radius2 /= 2.0;
  this->Mesh->Tolerance2 = length*length*1.0e-10;

  // Define the points (-x,+x,-y,+y,-z,+z). Theses added points are
  // used to create a bounding octahedron.
  this->Mesh->Points[numPts].P[0] = center[0] - length;
  this->Mesh->Points[numPts].P[1] = center[1];
  this->Mesh->Points[numPts].P[2] = center[2];
  this->Mesh->Points[numPts].Id = numPts;
  this->Mesh->Points[numPts].InsertionId = numPts;
  this->Mesh->Points[numPts].Type = OTPoint::Added;

  this->Mesh->Points[numPts+1].P[0] = center[0] + length;
  this->Mesh->Points[numPts+1].P[1] = center[1];
  this->Mesh->Points[numPts+1].P[2] = center[2];
  this->Mesh->Points[numPts+1].Id = numPts + 1;
  this->Mesh->Points[numPts+1].InsertionId = numPts + 1;
  this->Mesh->Points[numPts+1].Type = OTPoint::Added;

  this->Mesh->Points[numPts+2].P[0] = center[0];
  this->Mesh->Points[numPts+2].P[1] = center[1] - length;
  this->Mesh->Points[numPts+2].P[2] = center[2];
  this->Mesh->Points[numPts+2].Id = numPts + 2;
  this->Mesh->Points[numPts+2].InsertionId = numPts + 2;
  this->Mesh->Points[numPts+2].Type = OTPoint::Added;

  this->Mesh->Points[numPts+3].P[0] = center[0];
  this->Mesh->Points[numPts+3].P[1] = center[1] + length;
  this->Mesh->Points[numPts+3].P[2] = center[2];
  this->Mesh->Points[numPts+3].Id = numPts + 3;
  this->Mesh->Points[numPts+3].InsertionId = numPts + 3;
  this->Mesh->Points[numPts+3].Type = OTPoint::Added;

  this->Mesh->Points[numPts+4].P[0] = center[0];
  this->Mesh->Points[numPts+4].P[1] = center[1];
  this->Mesh->Points[numPts+4].P[2] = center[2] - length;
  this->Mesh->Points[numPts+4].Id = numPts + 4;
  this->Mesh->Points[numPts+4].InsertionId = numPts + 4;
  this->Mesh->Points[numPts+4].Type = OTPoint::Added;

  this->Mesh->Points[numPts+5].P[0] = center[0];
  this->Mesh->Points[numPts+5].P[1] = center[1];
  this->Mesh->Points[numPts+5].P[2] = center[2] + length;
  this->Mesh->Points[numPts+5].Id = numPts + 5;
  this->Mesh->Points[numPts+5].InsertionId = numPts + 5;
  this->Mesh->Points[numPts+5].Type = OTPoint::Added;

  // Create bounding tetras (there are four) as well as the associated faces
  // They all share the same center and radius
  OTTetra *tetras[4];
  for (int i=0; i<4; ++i)
    {
    tetras[i] = new(this->Heap) OTTetra();
    this->Mesh->Tetras.push_front(tetras[i]);
    tetras[i]->Center[0] = center[0];
    tetras[i]->Center[1] = center[1];
    tetras[i]->Center[2] = center[2];
    tetras[i]->Radius2 = radius2;
    }

  //Okay now set up the points and neighbors in the tetras
  tetras[0]->Points[0] = this->Mesh->Points.GetPointer(numPts + 0);
  tetras[0]->Points[1] = this->Mesh->Points.GetPointer(numPts + 2);
  tetras[0]->Points[2] = this->Mesh->Points.GetPointer(numPts + 4);
  tetras[0]->Points[3] = this->Mesh->Points.GetPointer(numPts + 5);
  tetras[0]->Neighbors[0] = 0; //outside
  tetras[0]->Neighbors[1] = tetras[1];
  tetras[0]->Neighbors[2] = tetras[3];
  tetras[0]->Neighbors[3] = 0;

  tetras[1]->Points[0] = this->Mesh->Points.GetPointer(numPts + 2);
  tetras[1]->Points[1] = this->Mesh->Points.GetPointer(numPts + 1);
  tetras[1]->Points[2] = this->Mesh->Points.GetPointer(numPts + 4);
  tetras[1]->Points[3] = this->Mesh->Points.GetPointer(numPts + 5);
  tetras[1]->Neighbors[0] = 0;
  tetras[1]->Neighbors[1] = tetras[2];
  tetras[1]->Neighbors[2] = tetras[0];
  tetras[1]->Neighbors[3] = 0;

  tetras[2]->Points[0] = this->Mesh->Points.GetPointer(numPts + 1);
  tetras[2]->Points[1] = this->Mesh->Points.GetPointer(numPts + 3);
  tetras[2]->Points[2] = this->Mesh->Points.GetPointer(numPts + 4);
  tetras[2]->Points[3] = this->Mesh->Points.GetPointer(numPts + 5);
  tetras[2]->Neighbors[0] = 0;
  tetras[2]->Neighbors[1] = tetras[3];
  tetras[2]->Neighbors[2] = tetras[1];
  tetras[2]->Neighbors[3] = 0;

  tetras[3]->Points[0] = this->Mesh->Points.GetPointer(numPts + 3);
  tetras[3]->Points[1] = this->Mesh->Points.GetPointer(numPts + 0);
  tetras[3]->Points[2] = this->Mesh->Points.GetPointer(numPts + 4);
  tetras[3]->Points[3] = this->Mesh->Points.GetPointer(numPts + 5);
  tetras[3]->Neighbors[0] = 0;
  tetras[3]->Neighbors[1] = tetras[0];
  tetras[3]->Neighbors[2] = tetras[2];
  tetras[3]->Neighbors[3] = 0;
}


//------------------------------------------------------------------------
// Add a point to the list of points to be triangulated.
vtkIdType vtkOrderedTriangulator::InsertPoint(vtkIdType id, double x[3],
                                              double p[3], int type)
{
  vtkIdType idx = this->NumberOfPoints++;
  if ( idx >= this->MaximumNumberOfPoints )
    {
    vtkErrorMacro(<< "Trying to insert more points than specified max="
      << this->MaximumNumberOfPoints << " idx=" << idx);
    return idx;
    }

  this->Mesh->Points[idx].Id = id;
  this->Mesh->Points[idx].SortId = id;
  this->Mesh->Points[idx].SortId2 = -1;
  this->Mesh->Points[idx].OriginalId = idx;
  this->Mesh->Points[idx].InsertionId = -1; //dummy value until inserted
  this->Mesh->Points[idx].X[0] = x[0];
  this->Mesh->Points[idx].X[1] = x[1];
  this->Mesh->Points[idx].X[2] = x[2];
  this->Mesh->Points[idx].P[0] = p[0];
  this->Mesh->Points[idx].P[1] = p[1];
  this->Mesh->Points[idx].P[2] = p[2];
  this->Mesh->Points[idx].Type =
    static_cast<OTPoint::PointClassification>(type);

  return idx;
}

//------------------------------------------------------------------------
// Add a point to the list of points to be triangulated.
vtkIdType vtkOrderedTriangulator::InsertPoint(vtkIdType id, vtkIdType sortid,
                                              double x[3], double p[3],
                                              int type)
{
  vtkIdType idx = this->NumberOfPoints++;
  if ( idx >= this->MaximumNumberOfPoints )
    {
    vtkErrorMacro(<< "Trying to insert more points than specified");
    return idx;
    }

  this->Mesh->Points[idx].Id = id;
  this->Mesh->Points[idx].SortId = sortid;
  this->Mesh->Points[idx].SortId2 = -1;
  this->Mesh->Points[idx].OriginalId = idx;
  this->Mesh->Points[idx].InsertionId = -1; //dummy value until inserted
  this->Mesh->Points[idx].X[0] = x[0];
  this->Mesh->Points[idx].X[1] = x[1];
  this->Mesh->Points[idx].X[2] = x[2];
  this->Mesh->Points[idx].P[0] = p[0];
  this->Mesh->Points[idx].P[1] = p[1];
  this->Mesh->Points[idx].P[2] = p[2];
  this->Mesh->Points[idx].Type =
    static_cast<OTPoint::PointClassification>(type);

  return idx;
}

//------------------------------------------------------------------------
// Add a point to the list of points to be triangulated.
vtkIdType vtkOrderedTriangulator::InsertPoint(vtkIdType id, vtkIdType sortid,
                                              vtkIdType sortid2,
                                              double x[3], double p[3],
                                              int type)
{
  vtkIdType idx = this->NumberOfPoints++;
  if ( idx >= this->MaximumNumberOfPoints )
    {
    vtkErrorMacro(<< "Trying to insert more points than specified");
    return idx;
    }

  this->Mesh->Points[idx].Id = id;
  this->Mesh->Points[idx].SortId = sortid;
  this->Mesh->Points[idx].SortId2 = sortid2;
  this->Mesh->Points[idx].OriginalId = idx;
  this->Mesh->Points[idx].InsertionId = -1; //dummy value until inserted
  this->Mesh->Points[idx].X[0] = x[0];
  this->Mesh->Points[idx].X[1] = x[1];
  this->Mesh->Points[idx].X[2] = x[2];
  this->Mesh->Points[idx].P[0] = p[0];
  this->Mesh->Points[idx].P[1] = p[1];
  this->Mesh->Points[idx].P[2] = p[2];
  this->Mesh->Points[idx].Type =
    static_cast<OTPoint::PointClassification>(type);

  return idx;
}

//------------------------------------------------------------------------
// Used when an already inserted point must have its classification changed
// (e.g., an intersection point is very near another point).
void vtkOrderedTriangulator::UpdatePointType(vtkIdType internalId, int type)
{
  assert("pre: valid_range" && internalId>=0 &&
         internalId<this->NumberOfPoints);
  this->Mesh->Points[internalId].Type =
    static_cast<OTPoint::PointClassification>(type);
}

//------------------------------------------------------------------------
double *vtkOrderedTriangulator::GetPointPosition(vtkIdType internalId)
{
  assert("pre: valid_range" && internalId>=0 &&
         internalId<this->NumberOfPoints);
  return this->Mesh->Points[internalId].P;
}

//------------------------------------------------------------------------
double *vtkOrderedTriangulator::GetPointLocation(vtkIdType internalId)
{
  assert("pre: valid_range" && internalId>=0 &&
         internalId<this->NumberOfPoints);
  return this->Mesh->Points[internalId].X;
}

//------------------------------------------------------------------------
vtkIdType vtkOrderedTriangulator::GetPointId(vtkIdType internalId)
{
  assert("pre: valid_range" && internalId>=0 &&
         internalId<this->NumberOfPoints);
  return this->Mesh->Points[internalId].Id;
}
//------------------------------------------------------------------------
// For a particular tetra and given a face id, return the three points
// defining the face.
void OTTetra::GetFacePoints(int i, OTFace *face)
{
  // The order is carefully choosen to produce a tetrahedron
  // that is not inside out; i.e., the ordering produces a positive
  // Jacobian (computed from first three points points to fourth).
  switch (i)
    {
    case 0:
      face->Points[0] = this->Points[0];
      face->Points[1] = this->Points[3];
      face->Points[2] = this->Points[1];
      break;
    case 1:
      face->Points[0] = this->Points[1];
      face->Points[1] = this->Points[3];
      face->Points[2] = this->Points[2];
      break;
    case 2:
      face->Points[0] = this->Points[0];
      face->Points[1] = this->Points[2];
      face->Points[2] = this->Points[3];
      break;
    case 3:
      face->Points[0] = this->Points[0];
      face->Points[1] = this->Points[1];
      face->Points[2] = this->Points[2];
      break;
    }
  face->ComputePseudoNormal();
}

//------------------------------------------------------------------------
// Routines used to sort the points based on id.
extern "C" {
#ifdef _WIN32_WCE
  int __cdecl vtkSortOnIds(const void *val1, const void *val2)
#else
  int vtkSortOnIds(const void *val1, const void *val2)
#endif
  {
    if (((OTPoint *)val1)->SortId < ((OTPoint *)val2)->SortId)
      {
      return (-1);
      }
    else if (((OTPoint *)val1)->SortId > ((OTPoint *)val2)->SortId)
      {
      return (1);
      }
    else
      {
      return (0);
      }
  }
}

extern "C" {
#ifdef _WIN32_WCE
  int __cdecl vtkSortOnTwoIds(const void *val1, const void *val2)
#else
  int vtkSortOnTwoIds(const void *val1, const void *val2)
#endif
  {
    if (((OTPoint *)val1)->SortId2 < ((OTPoint *)val2)->SortId2)
      {
      return (-1);
      }
    else if (((OTPoint *)val1)->SortId2 > ((OTPoint *)val2)->SortId2)
      {
      return (1);
      }

    if (((OTPoint *)val1)->SortId < ((OTPoint *)val2)->SortId)
      {
      return (-1);
      }
    else if (((OTPoint *)val1)->SortId > ((OTPoint *)val2)->SortId)
      {
      return (1);
      }
    else
      {
      return (0);
      }
  }
}

//------------------------------------------------------------------------
// See whether point is in sphere of tetrahedron.
int OTTetra::InCircumSphere(double x[3])
{
  double dist2;

  // check if inside/outside circumsphere
  dist2 = (x[0] - this->Center[0]) * (x[0] - this->Center[0]) +
          (x[1] - this->Center[1]) * (x[1] - this->Center[1]) +
          (x[2] - this->Center[2]) * (x[2] - this->Center[2]);

  return (dist2 < (0.999999L * this->Radius2) ? 1 : 0);
}

//------------------------------------------------------------------------
// Determine the classification of a tetra based on point types.
inline OTTetra::TetraClassification OTTetra::DetermineType()
{
  if ( (this->Points[0]->Type == OTPoint::Inside ||
        this->Points[0]->Type == OTPoint::Boundary ) &&
       (this->Points[1]->Type == OTPoint::Inside ||
        this->Points[1]->Type == OTPoint::Boundary ) &&
       (this->Points[2]->Type == OTPoint::Inside ||
        this->Points[2]->Type == OTPoint::Boundary ) &&
       (this->Points[3]->Type == OTPoint::Inside ||
        this->Points[3]->Type == OTPoint::Boundary ) )
    {
    this->Type = OTTetra::Inside;
    return OTTetra::Inside;
    }
  else if ( (this->Points[0]->Type == OTPoint::Outside ||
             this->Points[0]->Type == OTPoint::Boundary ) &&
            (this->Points[1]->Type == OTPoint::Outside ||
             this->Points[1]->Type == OTPoint::Boundary ) &&
            (this->Points[2]->Type == OTPoint::Outside ||
             this->Points[2]->Type == OTPoint::Boundary ) &&
            (this->Points[3]->Type == OTPoint::Outside ||
             this->Points[3]->Type == OTPoint::Boundary ) )
    {
    this->Type = OTTetra::Outside;
    return OTTetra::Outside;
    }
  else
    {
    this->Type = OTTetra::Exterior;
    return OTTetra::Exterior;
    }
}

//------------------------------------------------------------------------
// Determine whether the point is used by a specified tetra.
inline static int IsAPoint(OTTetra *t, vtkIdType id)
{
  if ( id == t->Points[0]->InsertionId || id == t->Points[1]->InsertionId ||
       id == t->Points[2]->InsertionId || id == t->Points[3]->InsertionId )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//------------------------------------------------------------------------
// Given two tetra face neighbors, assign the neighbor pointers to each tetra.
static void AssignNeighbors(OTTetra* t1, OTTetra* t2)
{
  static int CASE_MASK[4] = {1,2,4,8};
  int i, index;

  for (i=0, index=0; i<4; ++i)
    {
    if (IsAPoint(t2,t1->Points[i]->InsertionId) )
      {
      index |= CASE_MASK[i];
      }
    }
  switch (index)
    {
    case 11:
      t1->Neighbors[0] = t2;
      break;
    case 14:
      t1->Neighbors[1] = t2;
      break;
    case 13:
      t1->Neighbors[2] = t2;
      break;
    case 7:
      t1->Neighbors[3] = t2;
      break;
    default:
      vtkGenericWarningMacro(<<"Really bad");
    }

  for (i=0, index=0; i<4; ++i)
    {
    if (IsAPoint(t1,t2->Points[i]->InsertionId) )
      {
      index |= CASE_MASK[i];
      }
    }
  switch (index)
    {
    case 11:
      t2->Neighbors[0] = t1;
      break;
    case 14:
      t2->Neighbors[1] = t1;
      break;
    case 13:
      t2->Neighbors[2] = t1;
      break;
    case 7:
      t2->Neighbors[3] = t1;
      break;
    default:
      vtkGenericWarningMacro(<<"Really bad");
    }
}

//------------------------------------------------------------------------
// Instantiate and initialize a tetra.
OTTetra *vtkOTMesh::CreateTetra(OTPoint *p, OTFace *face)
{
  OTTetra *tetra = new(this->Heap) OTTetra;
  this->Tetras.push_front(tetra);
  tetra->Radius2 = vtkTetra::Circumsphere(p->P,
                                          face->Points[0]->P,
                                          face->Points[1]->P,
                                          face->Points[2]->P,
                                          tetra->Center);

  // the order is carefully choosen to produce a tetrahedron
  // that is not inside out; i.e., the ordering produces a positive
  // jacobian (normal computed from first three points points to fourth).
  tetra->Points[0] = face->Points[0];
  tetra->Points[1] = face->Points[1];
  tetra->Points[2] = face->Points[2];
  tetra->Points[3] = p;

  if ( face->Neighbor )
    {
    AssignNeighbors(tetra,face->Neighbor);
    }

  return tetra;
}

//------------------------------------------------------------------------
// We start with a point that is inside a tetrahedron. We find face
// neighbors of the tetrahedron that also contain the point. The
// process continues recursively until no more tetrahedron are found.
// Faces that lie between a tetrahedron that is in the cavity and one
// that is not form the cavity boundary, these are kept track of in
// a list. Eventually the point and boundary faces form new tetrahedra.
int vtkOTMesh::CreateInsertionCavity(OTPoint* p, OTTetra *initialTet,
                                     double [4])
{
  // Prepare to insert deleted tetras and cavity faces
  //
  this->CavityFaces.clear(); //cavity face boundary
  this->VisitedTetras.clear(); //tetras involved in creating cavity
  this->TetraStack.clear(); //queue of tetras being processed
  this->DegenerateQueue.clear(); //queue of tetras that have degenerate faces
  this->TetraStack.push(initialTet);
  initialTet->Type = OTTetra::InCavity; //the seed of the cavity
  initialTet->CurrentPointId = p->InsertionId; //mark visited
  this->VisitedTetras.push_back(initialTet);

  // Process queue of tetras until exhausted
  //
  int i, valid;
  int somethingNotValid=0;
  OTTetra *nei, *tetra;
  TetraQueueIterator t;
  for ( int numCycles=0; !this->TetraStack.empty(); numCycles++)
    {
    tetra = this->TetraStack.top();
    this->TetraStack.pop();

    //for each face, see whether the neighbors are in the cavity
    for (valid=1, i=0; i<4 && valid; ++i)
      {
      nei = tetra->Neighbors[i];
      // If a mesh boundary face, the face is added to the
      // list of insertion cavity faces
      if ( nei == 0 )
        {
        OTFace *face = new(this->Heap) OTFace;
        tetra->GetFacePoints(i,face);
        face->Neighbor = 0;
        this->CavityFaces.push_back(face);
        valid = face->IsValidCavityFace(p->P,this->Tolerance2);
        }
      // Neighbor tetra has not been visited, check for possible face boundary
      else if ( nei->CurrentPointId != p->InsertionId )
        {
        this->VisitedTetras.push_back(nei);
        nei->CurrentPointId = p->InsertionId; //mark visited
        if ( nei->InCircumSphere(p->P) )
          {
          nei->Type = OTTetra::InCavity;
          this->TetraStack.push(nei);
          }
        else //a cavity boundary
          {
          nei->Type = OTTetra::OutsideCavity;
          OTFace *face = new(this->Heap) OTFace;
          tetra->GetFacePoints(i,face);
          face->Neighbor = nei;
          this->CavityFaces.push_back(face);
          valid = face->IsValidCavityFace(p->P,this->Tolerance2);
          }
        }//if a not-visited face neighbor
      // Visited before, add this face as a boundary
      else if ( nei->Type == OTTetra::OutsideCavity )
        {
        OTFace *face = new(this->Heap) OTFace;
        tetra->GetFacePoints(i,face);
        face->Neighbor = nei;
        this->CavityFaces.push_back(face);
        valid = face->IsValidCavityFace(p->P,this->Tolerance2);
        }
      }//for each of the four tetra faces

    //check for validity
    if ( !valid ) //broke out due to invalid face
      {
      somethingNotValid++;
      //add this tetra to queue
      this->DegenerateQueue.push_back(tetra);

      //mark all current tetras unvisited
      for (t = this->VisitedTetras.begin();
           t != this->VisitedTetras.end(); ++t)
        {
        (*t)->CurrentPointId = -1;
        }

      //mark degenerate tetras visited and outside cavity
      TetraQueueIterator titer;
      for ( titer=this->DegenerateQueue.begin();
            titer != this->DegenerateQueue.end(); ++titer)
        {
        (*titer)->CurrentPointId = p->InsertionId;
        (*titer)->Type = OTTetra::OutsideCavity;
        }

      //reinitialize queue
      this->CavityFaces.clear();  //cavity face boundary
      this->VisitedTetras.clear(); //tetras visited during cavity creation
      this->TetraStack.clear();   //reprocess
      this->TetraStack.push(initialTet);
      initialTet->CurrentPointId = p->InsertionId;
      initialTet->Type = OTTetra::InCavity;
      this->VisitedTetras.push_back(initialTet);
      }
    if ( numCycles > 1000 ) return 0;
    }//while queue not empty

  // Make final pass and delete tetras inside the cavity
  for (t = this->VisitedTetras.begin(); t != this->VisitedTetras.end(); ++t)
    {
    tetra = *t;
    if ( tetra->CurrentPointId == p->InsertionId &&
         tetra->Type == OTTetra::InCavity )
      {
      tetra->DeleteMe = 1;
      }
    }

  TetraListIterator it;
  for (it = this->Tetras.begin(); it != this->Tetras.end(); )
    {
    if ((*it)->DeleteMe)
      {
      it = this->Tetras.erase(it);
      }
    else
      {
      ++it;
      }     
    }

#if 0
  //please leave this for debugging purposes
  if ( somethingNotValid )
    {
    this->DumpInsertionCavity(p->P);
//    exit(1);
    }
#endif

  return 1;
}

//------------------------------------------------------------------------
// Returns the number of tetras classified inside; a side effect is that
// all tetra are classified.
int vtkOTMesh::ClassifyTetras()
{
  TetraListIterator t;
  vtkIdType numInsideTetras=0;

  // loop over all tetras getting the ones with the classification requested
  for (t=this->Tetras.begin(); t != this->Tetras.end(); ++t)
    {
    if ( (*t)->DetermineType() == OTTetra::Inside )
      {
      numInsideTetras++;
      }
    }//for all tetras

  return numInsideTetras;
}

//------------------------------------------------------------------------
// Used to debug (writes a VTK file representing the current insertion cavity).
void vtkOTMesh::DumpInsertionCavity(double x[3])
{
  OTFace *face;
  FaceListIterator fptr;

  cout << "# vtk DataFile Version 3.0\n";
  cout << "ordered triangulator output\n";
  cout << "ASCII\n";
  cout << "DATASET POLYDATA\n";

  //write out points
  int numFaces = static_cast<int>(this->CavityFaces.size());
  cout << "POINTS " << 3*numFaces+1 << " double\n";

  for (fptr=this->CavityFaces.begin();
       fptr != this->CavityFaces.end(); ++fptr)
    {
    face = *fptr;
    cout << face->Points[0]->P[0] << " "
         << face->Points[0]->P[1] << " "
         << face->Points[0]->P[2] << " "
         << face->Points[1]->P[0] << " "
         << face->Points[1]->P[1] << " "
         << face->Points[1]->P[2] << " "
         << face->Points[2]->P[0] << " "
         << face->Points[2]->P[1] << " "
         << face->Points[2]->P[2] << "\n";
    }

  //write out point insertion vertex
  cout << x[0] << " " << x[1] << " " << x[2] << "\n\n";
  cout << "VERTICES 1 2 \n";
  cout << "1 " << 3*numFaces << "\n\n";

  //write out triangles
  cout << "POLYGONS " << numFaces << " " <<4*numFaces << "\n";

  int idx=0;
  for (fptr=this->CavityFaces.begin();
       fptr != this->CavityFaces.end(); ++fptr, idx+=3)
    {
    cout << 3 << " " << idx << " " << idx+1 << " " << idx+2 << "\n";
    }
}

//------------------------------------------------------------------------
// Walk to the tetra tha contains this point. Walking is done by moving
// in the direction of the most negative barycentric coordinate (i.e.,
// into the face neighbor).
OTTetra*
vtkOTMesh::WalkToTetra(OTTetra *tetra, double x[3], int depth, double bc[4])
{
  int neg = 0;
  int j, numNeg;
  double negValue;

  // prevent aimless wandering and death by recursion
  if ( depth > 200 )
    {
    return 0;
    }

  vtkTetra::BarycentricCoords(x, tetra->Points[0]->P, tetra->Points[1]->P,
                              tetra->Points[2]->P, tetra->Points[3]->P, bc);

  // find the most negative face
  for ( negValue=VTK_DOUBLE_MAX, numNeg=j=0; j<4; j++ )
    {
    if ( bc[j] < -0.000001 ) //if close enough that's okay
      {
      numNeg++;
      if ( bc[j] < negValue )
        {
        negValue = bc[j];
        neg = j;
        }
      }
    }

  // if no negatives, then inside this tetra
  if ( numNeg <= 0 )
    {
    return tetra;
    }

  // okay, march towards the most negative direction
  switch (neg)
    {
    case 0:
      tetra = tetra->Neighbors[1];
      break;
    case 1:
      tetra = tetra->Neighbors[2];
      break;
    case 2:
      tetra = tetra->Neighbors[0];
      break;
    case 3:
      tetra = tetra->Neighbors[3];
      break;
    }

  if ( tetra )
    {
    return this->WalkToTetra(tetra, x, ++depth, bc);
    }
  else
    {
    return 0;
    }
}

//------------------------------------------------------------------------
// Use an ordered insertion process in combination with a consistent
// degenerate resolution process to generate a unique Delaunay triangulation.
void vtkOrderedTriangulator::Triangulate()
{
  OTPoint *p;
  int i;
  vtkIdType ptId;

  // Sort the points according to id. The last six points are left
  // where they are (at the end of the list).
  if ( ! this->PreSorted )
    {
    if (this->UseTwoSortIds)
      {
      qsort(this->Mesh->Points.GetPointer(0), this->NumberOfPoints,
            sizeof(OTPoint), vtkSortOnTwoIds);
      }
    else
      {
      qsort(this->Mesh->Points.GetPointer(0), this->NumberOfPoints,
            sizeof(OTPoint), vtkSortOnIds);
      }
    }

  // Prepare the data structures (e.g., mesh) for an ordererd triangulation.
  this->Initialize();

  // Insert each point into the triangulation. Assign internal ids
  // as we progress.
  for (ptId=0, p=this->Mesh->Points.GetPointer(0);
       ptId < this->NumberOfPoints; ++p, ++ptId)
    {
    if ( p->Type == OTPoint::NoInsert )
      {
      continue; //skip this point
      }

    p->InsertionId = ptId;

    // Walk to a tetrahedron (start with first one on list)
    double bc[4];
    OTTetra *tetra =
      this->Mesh->WalkToTetra(*(this->Mesh->Tetras.begin()),p->P,0,bc);

    if ( tetra == 0 || !this->Mesh->CreateInsertionCavity(p, tetra, bc) )
      {
      vtkDebugMacro(<<"Point not in tetrahedron");
      continue;
      }

    // For each face on the boundary of the cavity, create a new
    // tetrahedron with the face and point. We've also got to set
    // up tetrahedron face neighbors, so we'll use an edge table
    // to keep track of the tetrahedron that generated the face as
    // a result of sweeping an edge.
    vtkIdType v1, v2;

    this->Mesh->EdgeTable->InitEdgeInsertion(this->MaximumNumberOfPoints+6,2);
    this->Mesh->TetraStack.clear();
    FaceListIterator fptr;
    void *tptr;
    OTTetra *neiTetra;
    OTFace *face;

    for (fptr=this->Mesh->CavityFaces.begin();
         fptr != this->Mesh->CavityFaces.end(); ++fptr)
      {
      face = *fptr;
      //create a tetra (it's added to the list of tetras as a side effect)
      tetra = this->Mesh->CreateTetra(p,face);

      for (i=0; i<3; ++i)
        {
        v1 = face->Points[i%3]->InsertionId;
        v2 = face->Points[(i+1)%3]->InsertionId;
        this->Mesh->EdgeTable->IsEdge(v1,v2,tptr);
        if ( ! tptr )
          {
          this->Mesh->EdgeTable->InsertEdge(v1,v2,tetra);
          }
        else
          {
          neiTetra = static_cast<OTTetra*>(tptr);
          AssignNeighbors(tetra, neiTetra);
          }
        }//for three edges
      }//for each face on the insertion cavity
    }//for all points to be inserted

  // Final classification
  this->Mesh->NumberOfTetrasClassifiedInside = this->Mesh->ClassifyTetras();
}


//------------------------------------------------------------------------
// Perform triangulation using templates (when possible).
void vtkOrderedTriangulator::TemplateTriangulate(int cellType,
                                                 int numPts, int numEdges)
{
  this->CellType = cellType;
  if ( ! this->UseTemplates )
    {
    this->Triangulate();
    return;
    }

  this->NumberOfCellPoints = numPts;
  this->NumberOfCellEdges = numEdges;

  // Sort the points according to id.
  if ( ! this->PreSorted )
    {
    if (this->UseTwoSortIds)
      {
      qsort(this->Mesh->Points.GetPointer(0), this->NumberOfPoints,
            sizeof(OTPoint), vtkSortOnTwoIds);
      }
    else
      {
      qsort(this->Mesh->Points.GetPointer(0), this->NumberOfPoints,
            sizeof(OTPoint), vtkSortOnIds);
      }
    }

  if ( ! this->TemplateTriangulation() )
    {//template triangulation didn't work, triangulate it and add to template cache
    int preSorted = this->PreSorted; //prevents resorting
    this->PreSorted = 1;
    this->Triangulate();
    this->AddTemplate();
    this->PreSorted = preSorted;
    }
}

//------------------------------------------------------------------------
// Add the tetras classified as specified to an unstructured grid.
vtkIdType vtkOrderedTriangulator::GetTetras(int classification,
                                            vtkUnstructuredGrid *ugrid)
{
  // Create the points
  //
  int i;
  vtkIdType numTetras=0;
  PointListIterator p;
  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(this->NumberOfPoints);
  for ( i=0, p=this->Mesh->Points.begin(); i<this->NumberOfPoints; ++i, ++p)
    {
    points->SetPoint(p->InsertionId,p->X);
    }
  ugrid->SetPoints(points);
  points->Delete();

  ugrid->Allocate(1000);
  TetraListIterator t;
  OTTetra *tetra;

  // loop over all tetras getting the ones with the classification requested
  vtkIdType pts[4];
  for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
    {
    tetra = *t;

    if ( tetra->Type == classification || classification == OTTetra::All)
      {
      numTetras++;
      pts[0] = tetra->Points[0]->Id;
      pts[1] = tetra->Points[1]->Id;
      pts[2] = tetra->Points[2]->Id;
      pts[3] = tetra->Points[3]->Id;
      ugrid->InsertNextCell(VTK_TETRA,4,pts);
      }
    }//for all tetras

  return numTetras;
}

//------------------------------------------------------------------------
// Add the tetras classified as specified to an unstructured grid
vtkIdType vtkOrderedTriangulator::AddTetras(int classification,
                                            vtkCellArray *outConnectivity)
{
  TetraListIterator t;
  OTTetra *tetra;
  vtkIdType numTetras=0;

  // loop over all tetras getting the ones with the classification requested
  for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
    {
    tetra = *t;

    if ( tetra->Type == classification || classification == OTTetra::All)
      {
      numTetras++;
      outConnectivity->InsertNextCell(4);
      outConnectivity->InsertCellPoint(tetra->Points[0]->Id);
      outConnectivity->InsertCellPoint(tetra->Points[1]->Id);
      outConnectivity->InsertCellPoint(tetra->Points[2]->Id);
      outConnectivity->InsertCellPoint(tetra->Points[3]->Id);
      }
    }//for all tetras

  return numTetras;
}

//-----------------------------------------------------------------------------
// Assuming that all the inserted points come from a cell `cellId' to
// triangulate, get the tetrahedra in outConnectivity, the points in locator
// and copy point data and cell data. Return the number of added tetras.
// \pre locator_exists: locator!=0
// \pre outConnectivity: outConnectivity!=0
// \pre inPD_exists: inPD!=0
// \pre outPD_exists:  outPD!=0
// \pre inCD_exists: inCD!=0
// \pre outCD_exists: outCD!=0
vtkIdType vtkOrderedTriangulator::AddTetras(int classification,
                                            vtkIncrementalPointLocator *locator,
                                            vtkCellArray *outConnectivity,
                                            vtkPointData *inPD,
                                            vtkPointData *outPD,
                                            vtkCellData *inCD,
                                            vtkIdType cellId,
                                            vtkCellData *outCD)
{
  assert("pre: locator_exists" && locator!=0);
  assert("pre: outConnectivity" && outConnectivity!=0);
  assert("inPD_exists" && inPD!=0);
  assert("pre: outPD_exists" && outPD!=0);
  assert("inCD_exists" && inCD!=0);
  assert("pre: outCD_exists" && outCD!=0);

  TetraListIterator t;
  OTTetra *tetra;
  vtkIdType result=0;

  // loop over all tetras getting the ones with the classification requested
  for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
    {
    tetra = *t;

    if ( tetra->Type == classification || classification == OTTetra::All)
      {
      // Insert the points
      vtkIdType pts[4];

      int i=0;
      while(i<4)
        {
        if(locator->InsertUniquePoint(tetra->Points[i]->X,pts[i]))
          {
          outPD->CopyData(inPD,tetra->Points[i]->Id,pts[i]);
          }
        ++i;
        }

      // Insert the connectivity
      result++;
      vtkIdType newCellId = outConnectivity->InsertNextCell(4,pts);
      outCD->CopyData(inCD,cellId,newCellId);
      }
    }//for all tetras

  return result;
}
//------------------------------------------------------------------------
// Initialize tetra traversal. Used in conjunction with GetNextTetra().
void vtkOrderedTriangulator::InitTetraTraversal()
{
  this->Mesh->CurrentTetra = this->Mesh->Tetras.begin();
}

//------------------------------------------------------------------------
// Retrieve a single tetra. Used in conjunction with InitTetraTraversal().
// Returns 0 when the list is exhausted.
int vtkOrderedTriangulator::GetNextTetra(int classification, vtkTetra *tet,
                                         vtkDataArray *cellScalars,
                                         vtkDoubleArray *tetScalars)
{
  OTTetra *tetra;
  int i;

  // Find the next tetra with the right classification
  while ( this->Mesh->CurrentTetra != this->Mesh->Tetras.end() &&
          (*this->Mesh->CurrentTetra)->Type != classification &&
          (*this->Mesh->CurrentTetra)->Type != OTTetra::All )
    {
    tetra = *(this->Mesh->CurrentTetra);
    ++this->Mesh->CurrentTetra;
    }

  if ( this->Mesh->CurrentTetra != this->Mesh->Tetras.end() )
    {
    tetra = *(this->Mesh->CurrentTetra);
    for (i=0; i<4; i++)
      {
      tet->PointIds->SetId(i,tetra->Points[i]->Id);
      tet->Points->SetPoint(i,tetra->Points[i]->X);
      tetScalars->SetTuple(i,
                           cellScalars->GetTuple(
                             tetra->Points[i]->OriginalId));
      }
    ++this->Mesh->CurrentTetra;
    return 1;
    }
  else
    {
    return 0;
    }
}

//------------------------------------------------------------------------
// Add the tetras classified as specified to a list of point ids and
// point coordinates.
vtkIdType vtkOrderedTriangulator::AddTetras(int classification,
                                            vtkIdList *ptIds,
                                            vtkPoints *pts)
{
  TetraListIterator t;
  OTTetra *tetra;
  vtkIdType numTetras=0;
  int i;

  // loop over all tetras getting the ones with the classification requested
  for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
    {
    tetra = *t;

    if ( tetra->Type == classification || classification == OTTetra::All)
      {
      numTetras++;
      for (i=0; i<4; i++)
        {
        ptIds->InsertNextId(tetra->Points[i]->Id);
        pts->InsertNextPoint(tetra->Points[i]->X);
        }
      }
    }//for all tetras

  return numTetras;
}


//------------------------------------------------------------------------
// Add the tetras classified as specified to an unstructured grid
vtkIdType vtkOrderedTriangulator::AddTetras(int classification,
                                            vtkUnstructuredGrid *ugrid)

{
  vtkIdType numTetras=0;
  TetraListIterator t;
  OTTetra *tetra;

  // loop over all tetras getting the ones with the classification requested
  vtkIdType pts[4];
  for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
    {
    tetra = *t;

    if ( tetra->Type == classification || classification == OTTetra::All)
      {
      numTetras++;
      pts[0] = tetra->Points[0]->Id;
      pts[1] = tetra->Points[1]->Id;
      pts[2] = tetra->Points[2]->Id;
      pts[3] = tetra->Points[3]->Id;
      ugrid->InsertNextCell(VTK_TETRA,4,pts);
      }
    }//for all tetras

  return numTetras;
}

//------------------------------------------------------------------------
// Add the tetras classified as specified to a call array (connectivity list)
vtkIdType vtkOrderedTriangulator::AddTriangles(vtkCellArray *tris)
{
  vtkIdType numTris=0;
  int i;

  // Loop over all tetras examining each unvisited face. Faces whose
  // points are all classified "boundary" are added to the list of
  // faces.
  TetraListIterator t;
  OTTetra *tetra;
  OTFace *face = new(this->Heap) OTFace;

  // loop over all tetras getting the faces classified on the boundary
  for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
    {
    tetra = *t;
    tetra->CurrentPointId = VTK_LARGE_INTEGER; //mark visited
    for (i=0; i<4; i++)
      {
      if ( tetra->Neighbors[i] &&
           tetra->Neighbors[i]->CurrentPointId != VTK_LARGE_INTEGER &&
           tetra->Type != tetra->Neighbors[i]->Type )
        {//face not yet visited
        tetra->GetFacePoints(i,face);
        numTris++;
        tris->InsertNextCell(3);
        tris->InsertCellPoint(face->Points[0]->Id);
        tris->InsertCellPoint(face->Points[1]->Id);
        tris->InsertCellPoint(face->Points[2]->Id);
        }
      }
    }//for all tetras

  return numTris;
}

//------------------------------------------------------------------------
// Add faces classified on the boundary to a cell array (connectivity list)
vtkIdType vtkOrderedTriangulator::AddTriangles(vtkIdType id, vtkCellArray *tris)
{
  vtkIdType numTris=0;
  int i;

  // Loop over all tetras examining each unvisited face. Faces whose
  // points are all classified "boundary" are added to the list of
  // faces.
  TetraListIterator t;
  OTTetra *tetra;
  OTFace *face = new(this->Heap) OTFace;

  // loop over all tetras getting the faces classified on the boundary
  for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
    {
    tetra = *t;
    tetra->CurrentPointId = VTK_LARGE_INTEGER; //mark visited
    for (i=0; i<4; i++)
      {
      if ( tetra->Neighbors[i] &&
           tetra->Neighbors[i]->CurrentPointId != VTK_LARGE_INTEGER &&
           tetra->Type != tetra->Neighbors[i]->Type )
        {//face not yet visited
        tetra->GetFacePoints(i,face);
        if ( face->Points[0]->Id == id || face->Points[1]->Id == id ||
             face->Points[2]->Id == id )
          {
          numTris++;
          tris->InsertNextCell(3);
          tris->InsertCellPoint(face->Points[0]->Id);
          tris->InsertCellPoint(face->Points[1]->Id);
          tris->InsertCellPoint(face->Points[2]->Id);
          }
        }
      }
    }//for all tetras

  return numTris;
}

//---The following code supports templates----------------------------------
// Rather than predefining templates for the many possible triangulations, the
// ordered triangulator is used to generate the template which is then cached
// for later use. The key is that templates are uniquely characterized by a
// template id---a number representing a permutation of the sort of the
// original points.

//---Define template id type. Note: type must be 32 bits--------------------
// Currently the templates are set up for a maximum of eight point ids per
// cell maximum (this is due to the use of the vtkHexahedron). Any point can
// be exchanged with any of the other ids during the sort operation, so each
// exchange is represented with four bits as follows:
//
// +----+----+----+----+----+----+----+----+
// | p0 | p1 | p2 | p3 | p4 | p5 | p6 | p7 |
// +----+----+----+----+----+----+----+----+
//

//------------------------------------------------------------------------
// Given the results of the sorting, compute an index used to specify
// a template id.
inline TemplateIDType vtkOrderedTriangulator::ComputeTemplateIndex()
{
  static TemplateIDType mask[8]={0xF0000000,0x0F000000,0x00F00000,0x000F0000,
                                 0x0000F000,0x00000F00,0x000000F0,0x0000000F};

  int i;
  PointListIterator p;
  TemplateIDType templateID=0;

  for (p=this->Mesh->Points.begin(), i=0; i<this->NumberOfCellPoints; ++i, ++p)
    {
    templateID |= ((templateID & mask[i]) | (p->OriginalId << (32-4*(i+1))));
    }

  return templateID;
}


//------------------------------------------------------------------------
// If a template is missing, add it to the list of templates.
void vtkOrderedTriangulator::AddTemplate()
{
  // Find the template list for the given cell type
  int templateMayBeAvailable;
  TemplateList *tlist;
  TemplatesIterator titer = this->Templates->find(this->CellType);
  if ( titer != this->Templates->end() ) //something found
    {
    templateMayBeAvailable = 1;
    tlist = (*titer).second;
    }
  else //nothing found, have to create an entry for this cell type
    {
    templateMayBeAvailable = 0;
    tlist = new TemplateList;
    (*this->Templates)[this->CellType] = tlist;
    }

  // Create the template: its index and connectivity list
  TemplateIDType index = this->ComputeTemplateIndex();

  // Make sure template has not been created before
  TemplateListIterator tplate = tlist->find(index);
  if ( templateMayBeAvailable && tplate != tlist->end() )
    {
    vtkGenericWarningMacro(<<"Template found when it should not have been");
    }
  else
    {
    this->Mesh->NumberOfTemplates++;

    // The tetras have been classified previously. So allocate space
    // and add it as a template list.
    OTTemplate *otplate = new(this->TemplateHeap)
      OTTemplate(this->Mesh->NumberOfTetrasClassifiedInside,this->TemplateHeap);
    (*tlist)[index] = otplate;
 
    // Now fill in the connectivity list
    int i;
    TetraListIterator t;
    OTTetra *tetra;
    vtkIdType *clist=otplate->Tetras;
    for (t=this->Mesh->Tetras.begin(); t != this->Mesh->Tetras.end(); ++t)
      {
      if ( (*t)->Type == OTTetra::Inside )
        {
        tetra = *t;
        for (i=0; i<4; i++)
          {
          *clist++ = tetra->Points[i]->InsertionId;
          }
        }
      }//for all tetras
    }
}


//------------------------------------------------------------------------
// Use a template to create the triangulation. Return 0 if a template
// could not be used.
int vtkOrderedTriangulator::TemplateTriangulation()
{
  TemplatesIterator titer = this->Templates->find(this->CellType);
  if ( titer != this->Templates->end() ) //something found
    {
    TemplateIDType index = this->ComputeTemplateIndex();
    TemplateList *tlist = (*titer).second;
    TemplateListIterator tlistIter=tlist->find(index);
    if (  tlistIter != tlist->end() ) //something found
      {
      int i, j;
      OTTemplate *tets = (*tlistIter).second;
      vtkIdType numTets = tets->NumberOfTetras;
      vtkIdType *clist = tets->Tetras;
      OTTetra *tetra;
      for (i=0; i<numTets; i++)
        {
        tetra = new(this->Heap) OTTetra();
        this->Mesh->Tetras.push_front(tetra);
        tetra->Type = OTTetra::Inside;
        for (j=0; j<4; j++)
          {
          tetra->Points[j] = this->Mesh->Points.GetPointer(*clist++);
          }
        }//for all tetras in template
      return 1;
      }//if a template found
    }//if a template list for this cell type found

  return 0;
}


//------------------------------------------------------------------------
void vtkOrderedTriangulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PreSorted: " << (this->PreSorted ? "On\n" : "Off\n");
  os << indent << "UseTwoSortIds: " << (this->UseTwoSortIds ? "On\n" : "Off\n");
  os << indent << "UseTemplates: " << (this->UseTemplates ? "On\n" : "Off\n");
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;

}
