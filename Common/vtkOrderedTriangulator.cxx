/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrderedTriangulator.cxx
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
#include "vtkOrderedTriangulator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkTetra.h"
#include "vtkEdgeTable.h"
#include "vtkObjectFactory.h"

// TO DO:
// + In place new to avoid new/delete
// + Clean up interface to classes
// + Avoid copying face into face list
// + AssignNeighbors() needs to use mask to quickly determine face
//   and reduce code size

// Begin be defining a class for managing a linked list ---------------------
//

// A vector of type T to support operations.
template <class T>
class vtkOTVector
{
private:
  T* Array;
  int MaxId;
  int Size;
  T* Front; //to support Pop()
  
public:
  typedef T* Iterator;
  Iterator Begin() {return this->Array;}
  Iterator End() {return this->Array + this->MaxId + 1;}

  vtkOTVector(unsigned long size=100) 
    {
      this->Array = this->Front = new T [size];
      this->Size = (size == 0 ? 100 : size);
      this->MaxId = -1;
    }
  ~vtkOTVector() {if ( this->Array ) {delete [] this->Array;}}
  void Allocate(int size)
    {
      this->MaxId = -1;
      if ( size <= this->Size ) return;
      if ( this->Array ) delete [] this->Array;
      this->Front = this->Array = new T [size];
      this->Size = size;
    }
  void SetNumberOfValues(int num)
    {
      this->Allocate(num);
      this->MaxId = num-1;
    }
  Iterator InsertNextValue(const T& item)
    {
      if ( (++this->MaxId) >= this->Size )
        {
        T* array = new T [2*this->Size];
        memcpy(array,this->Array,this->Size*sizeof(T));
        this->Size *= 2;
        delete [] this->Array;
        this->Front = this->Array = array;
        }
      this->Array[this->MaxId] = item;
      return this->Array + MaxId;
    }
  T& operator[](int i)
    {
      return *(this->Array + i);
    }
  Iterator GetPointer(int i)
    {
      return this->Array + i;
    }
  unsigned int GetNumberOfValues()
    {
      return this->MaxId + 1;
    }
  void Reset()
    {
      this->MaxId = -1;
      this->Front = this->Array;
    }
  Iterator Pop()
    {
      return (this->Front == this->End() ? this->End() : this->Front++);
    }
  int GetMaxId() {return this->MaxId;}
};


// A linked list of type T to support operations.
template <class T>
class vtkOTLinkedList
{
public:
  class ListContainer //the container for the data
  {
  public:
    ListContainer(T* x):Next(0),Previous(0),Data(x) {}
    T* Data;
    ListContainer* Next;
    ListContainer* Previous;
  };//end class ListContainer

public:
  class Iterator //use it to loop over the list
  {
  public:
    Iterator() : Container(0) {}
    Iterator(ListContainer* c) : Container(c) {}
    Iterator(const ListContainer& c) {this->Container = c.Container;}
    Iterator& operator=(const Iterator& i) 
      {this->Container=i.Container; return *this;}
    Iterator& operator=(ListContainer *c) 
      {this->Container=c; return *this;}
    T& operator*()
      {return *(this->Container->Data);}
    T* GetPointer() 
      {return this->Container->Data;}
    Iterator& operator++() 
      {this->Container = this->Container->Next; return *this;}
    int operator!=(const Iterator& it) const
      {return this->Container != it.Container;}
    int operator==(const Iterator& i) const
      {return this->Container == i.Container;}
    int operator==(const ListContainer* c) const
      {return this->Container == c;}
      
    ListContainer *Container;
  };//end class Iterator
  
private:
  // Data members for linked list (pointers to start and end of the list)
  Iterator Head;
  Iterator Tail;
  
public:
  //Methods for linked list
  vtkOTLinkedList()
    {
      ListContainer *tail = new ListContainer(0);//create dummy container
      this->Head = this->Tail = tail;            //marks end of list
    }
  ~vtkOTLinkedList()
    {
      this->Reset();
    }
  Iterator& Begin() {return this->Head;}
  Iterator& End() {return this->Tail;}
  void Insert(const T& item) //constructs T and adds to top of list
    {
      T *x = new T(item);
      ListContainer *container = new ListContainer(x);
      if ( this->Head.Container->Next )
        {
        container->Next = this->Head.Container;
        container->Next->Previous = container;
        }
      else //first insertion
        {
        container->Next = this->Tail.Container;
        this->Tail.Container->Previous = container;
        }
      this->Head.Container = container;
    }
  Iterator& Delete(Iterator& i) //Deletes data in ith position; returns next
    {                         
      if ( i == this->Tail ) {return this->Tail;}
      ListContainer *next = i.Container->Next;
      if ( i == this->Head )
        {
        this->Head = next;
        next->Previous = 0;
        }
      else
        {
        next->Previous = i.Container->Previous;
        i.Container->Previous->Next = next;
        }
      delete i.Container;
      return (i=next);
    }
  void Reset()
    {
      for (Iterator i = this->Begin(); i != this->End(); i = this->Delete(i))
        ;
    }
};

// Classes are used to represent points, faces, and tetras-------------------
class vtkOTPoint
{
public:
  vtkOTPoint() : Id(0), InternalId(0), Type(Inside) 
    {this->X[0] = this->X[1] = this->X[2] = 0.0;}
  enum PointClassification {Inside=0,Outside=1,Boundary=2,Added=3};
  unsigned long Id; //Id to data outside this class
  unsigned long InternalId; //Id (order) of point insertion
  double X[3];
  PointClassification Type; //inside, outside
};

struct vtkOTTetra;
struct vtkOTFace //used during tetra construction
{
  vtkOTFace() : Neighbor(0)
    {
      Points[0]=Points[1]=Points[2]=0;
    }
  vtkOTPoint *Points[3]; //the three points of the face
  vtkOTTetra *Neighbor;
};

//---Class represents a tetrahedron-----------------------------------------
struct vtkOTTetra
{
  vtkOTTetra() : Radius2(0.0)
    {
      Center[0]=Center[1]=Center[2]=0.0L;
      Neighbors[0]=Neighbors[1]=Neighbors[2]=Neighbors[3]=0;
      Points[0]=Points[1]=Points[2]=Points[3]=0;
    }

  double Radius2;
  double Center[3];
  // Note: there is a direct correlation between the points and the faces
  // i.e., the ordering of the points and face neighbors.
  vtkOTTetra *Neighbors[4]; //the four face neighbors
  vtkOTPoint *Points[4]; //the four points
  // These are used during point insertion
  unsigned long CurrentPointId;
  enum TetraClassification 
    {Inside=0,Outside=1,All=2,InCavity=3,OutsideCavity=4};
  TetraClassification Type;

  void GetFacePoints(int i, vtkOTFace& face);
  int InSphere(double x[3]);
  TetraClassification GetType(); //inside, outside
};

// Class is used to hold lists of points, faces, and tetras-------------------
class vtkOTMesh
{
public:
  vtkOTMesh() 
    {
      this->EdgeTable = vtkEdgeTable::New();
    }
  ~vtkOTMesh() 
    {
      this->EdgeTable->Delete();
    }
  
  vtkOTVector<vtkOTPoint> Points;
  vtkOTLinkedList<vtkOTTetra*> Tetras;
  vtkOTVector<vtkOTFace> CavityFaces;
  vtkOTVector<vtkOTTetra*> TetraQueue;
  vtkEdgeTable *EdgeTable;
  
  void Reset()
    {
      this->Points.Reset();
      this->Tetras.Reset();
      this->CavityFaces.Reset();
      this->TetraQueue.Reset();
      this->EdgeTable->Reset();
    }
};


//------------------------------------------------------------------------
vtkOrderedTriangulator* vtkOrderedTriangulator::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOrderedTriangulator");
  if(ret)
    {
    return (vtkOrderedTriangulator *)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOrderedTriangulator;
}

//------------------------------------------------------------------------
vtkOrderedTriangulator::vtkOrderedTriangulator()
{
  this->Mesh = new vtkOTMesh;
  this->NumberOfPoints = 0;
  this->PreSorted = 0;
}

//------------------------------------------------------------------------
vtkOrderedTriangulator::~vtkOrderedTriangulator()
{
  delete this->Mesh;
}

//------------------------------------------------------------------------
void vtkOrderedTriangulator::InitTriangulation(float bounds[6], int numPts)
{
  double length;
  double center[3];
  double radius2;

  // Set up the internal data structures. Space for six extra points
  // is allocated for the bounding triangulation.
  this->NumberOfPoints = 0;
  this->MaximumNumberOfPoints = numPts;
  this->Mesh->Reset();
  this->Mesh->Points.SetNumberOfValues(numPts+6);
  
  // Create the initial bounding triangulation which is a
  // bounding octahedron: 6 points & 4 tetra.
  center[0] = (double) (bounds[0]+bounds[1])/2.0;
  center[1] = (double) (bounds[2]+bounds[3])/2.0;
  center[2] = (double) (bounds[4]+bounds[5])/2.0;
  length = 2.5 * sqrt( (radius2 = (bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                 (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                 (bounds[5]-bounds[4])*(bounds[5]-bounds[4])) );
  radius2 /= 2.0;
  
  //Define the points (-x,+x,-y,+y,-z,+z)
  this->Mesh->Points[numPts].X[0] = center[0] - length;
  this->Mesh->Points[numPts].X[1] = center[1];
  this->Mesh->Points[numPts].X[2] = center[2];
  this->Mesh->Points[numPts].Id = numPts;
  this->Mesh->Points[numPts].InternalId = numPts;
  this->Mesh->Points[numPts].Type = vtkOTPoint::Added;

  this->Mesh->Points[numPts+1].X[0] = center[0] + length;
  this->Mesh->Points[numPts+1].X[1] = center[1];
  this->Mesh->Points[numPts+1].X[2] = center[2];
  this->Mesh->Points[numPts+1].Id = numPts + 1;
  this->Mesh->Points[numPts+1].InternalId = numPts + 1;
  this->Mesh->Points[numPts+1].Type = vtkOTPoint::Added;

  this->Mesh->Points[numPts+2].X[0] = center[0];              
  this->Mesh->Points[numPts+2].X[1] = center[1] - length;
  this->Mesh->Points[numPts+2].X[2] = center[2];
  this->Mesh->Points[numPts+2].Id = numPts + 2;
  this->Mesh->Points[numPts+2].InternalId = numPts + 2;
  this->Mesh->Points[numPts+2].Type = vtkOTPoint::Added;

  this->Mesh->Points[numPts+3].X[0] = center[0];              
  this->Mesh->Points[numPts+3].X[1] = center[1] + length;
  this->Mesh->Points[numPts+3].X[2] = center[2];
  this->Mesh->Points[numPts+3].Id = numPts + 3;
  this->Mesh->Points[numPts+3].InternalId = numPts + 3;
  this->Mesh->Points[numPts+3].Type = vtkOTPoint::Added;

  this->Mesh->Points[numPts+4].X[0] = center[0];              
  this->Mesh->Points[numPts+4].X[1] = center[1];
  this->Mesh->Points[numPts+4].X[2] = center[2] - length;
  this->Mesh->Points[numPts+4].Id = numPts + 4;
  this->Mesh->Points[numPts+4].InternalId = numPts + 4;
  this->Mesh->Points[numPts+4].Type = vtkOTPoint::Added;

  this->Mesh->Points[numPts+5].X[0] = center[0];              
  this->Mesh->Points[numPts+5].X[1] = center[1];
  this->Mesh->Points[numPts+5].X[2] = center[2] + length;
  this->Mesh->Points[numPts+5].Id = numPts + 5;
  this->Mesh->Points[numPts+5].InternalId = numPts + 5;
  this->Mesh->Points[numPts+5].Type = vtkOTPoint::Added;

  // Create bounding tetras (there are four) as well as the associated faces
  // They all share the same center and radius
  vtkOTTetra *tetras[4];
  for (int i=0; i<4; i++)
    {
    tetras[i] = new vtkOTTetra;
    this->Mesh->Tetras.Insert(tetras[i]);
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
int vtkOrderedTriangulator::InsertPoint(unsigned long id, float x[3], 
                                         int type)
{
  int idx = this->NumberOfPoints++;
  if ( idx > this->MaximumNumberOfPoints )
    {
    vtkErrorMacro(<< "Trying to insert more points than specified");
    return idx;
    }
  
  this->Mesh->Points[idx].Id = id;
  this->Mesh->Points[idx].X[0] = (double) x[0];
  this->Mesh->Points[idx].X[1] = (double) x[1];
  this->Mesh->Points[idx].X[2] = (double) x[2];
  this->Mesh->Points[idx].Type = (vtkOTPoint::PointClassification) type;
  // InternalId is assigned later during point insertion
  
  return idx;
}

//------------------------------------------------------------------------
void vtkOrderedTriangulator::UpdatePointType(int internalId, int type)
{
  this->Mesh->Points[internalId].Type = (vtkOTPoint::PointClassification) type;
}

//------------------------------------------------------------------------
void vtkOTTetra::GetFacePoints(int i, vtkOTFace& face)
  {
    switch (i)
      {
      case 0:
        face.Points[0] = this->Points[0];
        face.Points[1] = this->Points[1];
        face.Points[2] = this->Points[3];
        break;
      case 1:
        face.Points[0] = this->Points[1];
        face.Points[1] = this->Points[2];
        face.Points[2] = this->Points[3];
        break;
      case 2:
        face.Points[0] = this->Points[2];
        face.Points[1] = this->Points[0];
        face.Points[2] = this->Points[3];
        break;
      case 3:
        face.Points[0] = this->Points[0];
        face.Points[1] = this->Points[1];
        face.Points[2] = this->Points[2];
        break;
      }
  }

//------------------------------------------------------------------------
 
extern "C" {
static int SortOnPointIds(const void *val1, const void *val2)
{
  if (((vtkOTPoint *)val1)->Id < ((vtkOTPoint *)val2)->Id)
    {
    return (-1);
    }
  else if (((vtkOTPoint *)val1)->Id > ((vtkOTPoint *)val2)->Id)
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
// See whether point is in sphere of tetrahedron
int vtkOTTetra::InSphere(double x[3])
{
  double dist2;
  
  // check if inside/outside circumcircle
  dist2 = (x[0] - this->Center[0]) * (x[0] - this->Center[0]) + 
          (x[1] - this->Center[1]) * (x[1] - this->Center[1]) +
          (x[2] - this->Center[2]) * (x[2] - this->Center[2]);

  return (dist2 < (0.9999999999L * this->Radius2) ? 1 : 0);
}

//------------------------------------------------------------------------
// Get type based on point types
inline vtkOTTetra::TetraClassification vtkOTTetra::GetType()
{
  if ( (this->Points[0]->Type == vtkOTPoint::Inside || 
        this->Points[0]->Type == vtkOTPoint::Boundary ) &&
       (this->Points[1]->Type == vtkOTPoint::Inside || 
        this->Points[1]->Type == vtkOTPoint::Boundary ) &&
       (this->Points[2]->Type == vtkOTPoint::Inside || 
        this->Points[2]->Type == vtkOTPoint::Boundary ) &&
       (this->Points[3]->Type == vtkOTPoint::Inside || 
        this->Points[3]->Type == vtkOTPoint::Boundary ) )
    {
    return vtkOTTetra::Inside;
    }
  else
    {
    return vtkOTTetra::Outside;
    }
}

inline static int IsAPoint(vtkOTTetra *t, unsigned long id)
{
  if ( id == t->Points[0]->InternalId || id == t->Points[1]->InternalId ||
       id == t->Points[2]->InternalId || id == t->Points[3]->InternalId )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

static void AssignNeighbors(vtkOTTetra* t1, vtkOTTetra* t2)
{
  // assign the face neighbor to t1
  if ( IsAPoint(t2,t1->Points[0]->InternalId) && 
       IsAPoint(t2,t1->Points[1]->InternalId) &&
       IsAPoint(t2,t1->Points[3]->InternalId) )
    {
    t1->Neighbors[0] = t2;
    }
  else if ( IsAPoint(t2,t1->Points[1]->InternalId) && 
            IsAPoint(t2,t1->Points[2]->InternalId) &&
            IsAPoint(t2,t1->Points[3]->InternalId) )
    {
    t1->Neighbors[1] = t2;
    }
  else if ( IsAPoint(t2,t1->Points[2]->InternalId) && 
            IsAPoint(t2,t1->Points[0]->InternalId) &&
            IsAPoint(t2,t1->Points[3]->InternalId) )
    {
    t1->Neighbors[2] = t2;
    }
  else if ( IsAPoint(t2,t1->Points[0]->InternalId) && 
            IsAPoint(t2,t1->Points[1]->InternalId) &&
            IsAPoint(t2,t1->Points[2]->InternalId) )
    {
    t1->Neighbors[3] = t2;
    }
  else
    {
    vtkGenericWarningMacro(<<"Really bad");
    }

  // assign the face neighbor to t2
  if ( IsAPoint(t1,t2->Points[0]->InternalId) && 
       IsAPoint(t1,t2->Points[1]->InternalId) &&
       IsAPoint(t1,t2->Points[3]->InternalId) )
    {
    t2->Neighbors[0] = t1;
    }
  else if ( IsAPoint(t1,t2->Points[1]->InternalId) && 
            IsAPoint(t1,t2->Points[2]->InternalId) &&
            IsAPoint(t1,t2->Points[3]->InternalId) )
    {
    t2->Neighbors[1] = t1;
    }
  else if ( IsAPoint(t1,t2->Points[2]->InternalId) && 
            IsAPoint(t1,t2->Points[0]->InternalId) &&
            IsAPoint(t1,t2->Points[3]->InternalId) )
    {
    t2->Neighbors[2] = t1;
    }
  else if ( IsAPoint(t1,t2->Points[0]->InternalId) && 
            IsAPoint(t1,t2->Points[1]->InternalId) &&
            IsAPoint(t1,t2->Points[2]->InternalId) )
    {
    t2->Neighbors[3] = t1;
    }
  else
    {
    vtkGenericWarningMacro(<<"Really bad");
    }
}

static vtkOTTetra *CreateTetra(vtkOTPoint& p, vtkOTFace& face)
{
  vtkOTTetra *tetra = new vtkOTTetra;
  tetra->Radius2 = vtkTetra::Circumsphere(p.X,
                                          face.Points[0]->X,
                                          face.Points[1]->X,
                                          face.Points[2]->X,
                                          tetra->Center);
  tetra->Points[0] = &p;
  tetra->Points[1] = face.Points[0];
  tetra->Points[2] = face.Points[1];
  tetra->Points[3] = face.Points[2];
  
  if ( face.Neighbor )
    {
    AssignNeighbors(tetra,face.Neighbor);
    }

  return tetra;
}

//------------------------------------------------------------------------
// We start with a point that is inside a tetrahedron. We find face
// neighbors of the tetrahedron that also contain the point. The
// process continues recursively until no more tetrahedron are found.
// Faces that lie between a tetrahedron that is in the cavity and one
// that is not form the cavity boundary, these are kept track of in
// a list.
void CreateInsertionCavity(vtkOTPoint* p, 
                           vtkOTLinkedList<vtkOTTetra*>::Iterator& tptr,
                           vtkOTMesh *Mesh)
{
  // Prepare to insert deleted tetras and cavity faces
  //
  Mesh->CavityFaces.Reset(); //cavity face boundary
  Mesh->TetraQueue.Reset(); //queue of tetras being processed
  Mesh->TetraQueue.InsertNextValue(*tptr);
  (*tptr)->Type = vtkOTTetra::InCavity; //the seed of the cavity
  (*tptr)->CurrentPointId = p->InternalId; //mark visited
  
  // Process queue of tetras until exhausted
  //
  int i;
  vtkOTFace face;
  vtkOTTetra *tetra, *nei;
  vtkOTVector<vtkOTTetra*>::Iterator titer;
  while ( (titer=Mesh->TetraQueue.Pop()) != Mesh->TetraQueue.End() )
    {
    tetra = *titer;
    
    //for each face, see whether the neighbors are in the cavity
    for (i=0; i<4; ++i)
      {
      // If a boundary, the face is added to the list of faces
      if ( (nei=tetra->Neighbors[i]) == 0 )
        {
        tetra->GetFacePoints(i,face);
        face.Neighbor = 0;
        Mesh->CavityFaces.InsertNextValue(face);
        }
      // Not yet visited, check the face as possible boundary
      else if ( nei->CurrentPointId != p->InternalId )
        {
        if ( nei->InSphere(p->X) )
          {
          nei->Type = vtkOTTetra::InCavity;
          Mesh->TetraQueue.InsertNextValue(nei);
          }
        else //a cavity boundary
          {
          nei->Type = vtkOTTetra::OutsideCavity;
          tetra->GetFacePoints(i,face);
          face.Neighbor = nei;
          Mesh->CavityFaces.InsertNextValue(face);
          }
        nei->CurrentPointId = p->InternalId; //mark visited
        }//if a not-visited face neighbor
      // Visited before, check face for cavity boundary
      else if ( nei->Type == vtkOTTetra::OutsideCavity )
        {
        tetra->GetFacePoints(i,face);
        face.Neighbor = nei;
        Mesh->CavityFaces.InsertNextValue(face);
        }
      }//for each of the four faces
    }//while queue not empty
  
  // Make final pass and delete tetras in the cavity
  // //TO DO: add pointers from tetra into linked list to avoid
  // //making a complete pass over all tetras.
  vtkOTLinkedList<vtkOTTetra*>::Iterator t;
  for (t = Mesh->Tetras.Begin(); t != Mesh->Tetras.End(); )
    {
    if ( (*t)->Type == vtkOTTetra::InCavity )
      {
      t = Mesh->Tetras.Delete(t);
      }
    else
      {
      ++t;
      }
    }
}


//------------------------------------------------------------------------
// Use an ordered insertion process in combination with a consistent
// degenerate resolution process to generate a unique Delaunay triangulation.
void vtkOrderedTriangulator::Triangulate()
{
  vtkOTLinkedList<vtkOTTetra*>::Iterator tptr;
  vtkOTPoint *p;
  int ptId, i;

  // Sort the points according to id. The last six points are left
  // where they are (at the end of the list).
  if ( ! this->PreSorted )
    {
    qsort((void *)this->Mesh->Points.GetPointer(0), this->NumberOfPoints, 
          sizeof(vtkOTPoint), SortOnPointIds);
    }

  // Insert each point into the triangulation. Assign internal ids 
  // as we progress.
  for (ptId=0, p=this->Mesh->Points.GetPointer(0); 
       ptId < this->NumberOfPoints; ++p, ++ptId)
    {
    p->InternalId = ptId;

    // Find a tetrahedron containing the point
    for (tptr = this->Mesh->Tetras.Begin(); 
         tptr != this->Mesh->Tetras.End(); ++tptr)
      {
      if ( (*tptr)->InSphere(p->X) )
        {
        break;
        }
      }//for all tetras
    
    if ( tptr == this->Mesh->Tetras.End() )
      {
      vtkDebugMacro(<<"Point not in tetrahedron");
      continue;
      }

    // Delete this tetrahedron and all neighboring tetrahedron that
    // contain the point. This creates a star-convex cavity that is
    // connected to the insertion point to create new tetrahedron.
    CreateInsertionCavity(p, tptr, this->Mesh);
    
    // For each face on the boundary of the cavity, create a new 
    // tetrahedron with the face and point. We've also got to set
    // up tetrahedron face neighbors, so we'll use an edge table
    // to keep track of the tetrahedron that generated the face as
    // a result of sweeping an edge.
    unsigned long v1, v2;
    int tetraId, id;

    this->Mesh->EdgeTable->InitEdgeInsertion(this->MaximumNumberOfPoints+6,1);
    this->Mesh->TetraQueue.Reset();
    vtkOTTetra *tetra;
    vtkOTVector<vtkOTFace>::Iterator fptr;
    
    for (fptr=this->Mesh->CavityFaces.Begin(); 
         fptr != this->Mesh->CavityFaces.End(); ++fptr)
      {
      //create a tetra
      tetra = CreateTetra(*p,*fptr);
      this->Mesh->TetraQueue.InsertNextValue(tetra);
      tetraId = this->Mesh->TetraQueue.GetMaxId();

      for (i=0; i<3; ++i)
        {
        v1 = fptr->Points[i%3]->InternalId;
        v2 = fptr->Points[(i+1)%3]->InternalId;
        if ( (id=this->Mesh->EdgeTable->IsEdge(v1,v2)) == -1 )
          {
          this->Mesh->EdgeTable->InsertEdge(v1,v2,tetraId);
          }
        else
          {
          AssignNeighbors(tetra, this->Mesh->TetraQueue[id]);
          }
        }//for three edges
      }//for each face on the insertion cavity

    //Add new tetras to the list of tetras
    vtkOTVector<vtkOTTetra*>::Iterator t;
    for (t=this->Mesh->TetraQueue.Begin(); 
         t != this->Mesh->TetraQueue.End(); ++t)
      {
      this->Mesh->Tetras.Insert(*t);
      }
    
    }//for all points to be inserted
}


//------------------------------------------------------------------------
int vtkOrderedTriangulator::GetTetras(int classification, 
                                       vtkCellArray *outConnectivity)
{
  vtkOTLinkedList<vtkOTTetra*>::Iterator tptr;
  vtkOTTetra::TetraClassification type; //inside, outside
  int numTetras=0;

  // loop over all tetras getting the ones with the classification requested
  for (tptr=this->Mesh->Tetras.Begin(); 
       tptr != this->Mesh->Tetras.End(); ++tptr)
    {
    type = (*tptr)->GetType();

    if ( type == classification || classification == vtkOTTetra::All)
      {
      numTetras++;
      outConnectivity->InsertNextCell(4);
      outConnectivity->InsertCellPoint((*tptr)->Points[0]->Id);
      outConnectivity->InsertCellPoint((*tptr)->Points[1]->Id);
      outConnectivity->InsertCellPoint((*tptr)->Points[2]->Id);
      outConnectivity->InsertCellPoint((*tptr)->Points[3]->Id);
      }
    }//for all tetras

  return numTetras;
}

int vtkOrderedTriangulator::GetTetras(int classification, 
                                       vtkUnstructuredGrid *ugrid)
{
  // Create the points
  //
  int numTetras=0;
  vtkOTVector<vtkOTPoint>::Iterator p;
  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(this->MaximumNumberOfPoints);
  for ( p=this->Mesh->Points.Begin();
        p != this->Mesh->Points.End(); ++p)
    {
    points->SetPoint(p->InternalId,p->X);
    }
  ugrid->SetPoints(points);
  
  ugrid->Allocate(1000);
  vtkOTLinkedList<vtkOTTetra*>::Iterator tptr;
  vtkOTTetra::TetraClassification type; //inside, outside

  // loop over all tetras getting the ones with the classification requested
  int pts[4];
  for (tptr=this->Mesh->Tetras.Begin(); 
       tptr != this->Mesh->Tetras.End(); ++tptr)
    {
    type = (*tptr)->GetType();

    if ( type == classification || classification == vtkOTTetra::All)
      {
      numTetras++;
      pts[0] = (*tptr)->Points[0]->InternalId;
      pts[1] = (*tptr)->Points[1]->InternalId;
      pts[2] = (*tptr)->Points[2]->InternalId;
      pts[3] = (*tptr)->Points[3]->InternalId;
      ugrid->InsertNextCell(VTK_TETRA,4,pts);
      }
    }//for all tetras
  
  return numTetras;
}

int vtkOrderedTriangulator::AddTetras(int classification, 
                                      vtkUnstructuredGrid *ugrid)
{
  int numTetras=0;
  vtkOTLinkedList<vtkOTTetra*>::Iterator tptr;
  vtkOTTetra::TetraClassification type; //inside, outside

  // loop over all tetras getting the ones with the classification requested
  int pts[4];
  for (tptr=this->Mesh->Tetras.Begin(); 
       tptr != this->Mesh->Tetras.End(); ++tptr)
    {
    type = (*tptr)->GetType();
    
    if ( type == classification || classification == vtkOTTetra::All)
      {
      numTetras++;
      pts[0] = (*tptr)->Points[0]->Id;
      pts[1] = (*tptr)->Points[1]->Id;
      pts[2] = (*tptr)->Points[2]->Id;
      pts[3] = (*tptr)->Points[3]->Id;
      ugrid->InsertNextCell(VTK_TETRA,4,pts);
      }
    }//for all tetras
  
  return numTetras;
}


//------------------------------------------------------------------------
void vtkOrderedTriangulator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "PreSorted: " << (this->PreSorted ? "On\n" : "Off\n");

}

