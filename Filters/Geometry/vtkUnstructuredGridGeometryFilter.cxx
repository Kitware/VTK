/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridGeometryFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
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
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTetra.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include "vtkQuadraticTetra.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticWedge.h"
#include "vtkQuadraticPyramid.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkIncrementalPointLocator.h"


#include <vector>
#include <cassert>

vtkStandardNewMacro(vtkUnstructuredGridGeometryFilter);

#if 0
//-----------------------------------------------------------------------------
// Pool allocator: interface is defined in section 20.1.5
// "Allocator Requirement" in the C++ norm.
template <class G> class vtkPoolAllocator
{
public:
  // It is ugly but it is the norm...
  typedef  G *pointer;
  typedef  const G * const_pointer;
  typedef  G&        reference;
  typedef  G const & const_reference;
  typedef  G         value_type;
  typedef  size_t    size_type;
  typedef  ptrdiff_t difference_type;
  template<H> struct rebind
  {
    typedef vtkPoolAllocator<H> other;
  };

  pointer address(reference r) const
    {
      return &r;
    }
  const_pointer address(const_reference r) const
    {
      return &r;
    }

  // Space for n Gs.
  // Comment from the norm:
  // Memory is allocated for `n' objects of type G but objects are not
  // constructed. allocate may raise an exception. The result is a random
  // access iterator.
  pointer allocate(size_type n,
                   vtkPoolAllocator<void>::const_pointer VTK_NOT_USED(hint)=0)
    {

    }

  // Deallocate n Gs, don't destroy.
  // Comment from the norm:
  // All `n' G objects in the area pointed by `p' must be destroyed prior to
  // this call. `n'  must match the value passed to allocate to obtain this
  // memory.
  // \pre p_exists: p!=0
  void deallocate(pointer p,
                  size_type n)
    {
      // Pre-conditions
      assert("p_exists" && p!=0);


    }

  // Comment from the norm:
  // The largest value that can meaningfully be passed to allocate().
  size_type max_size() const throw()
    {
      // Implementation is the same than in gcc:
      return size_t(-1)/sizeof(G);
    }

  // Default constructor.
  vtkPoolAllocator() throw()
    {
    }

  // Copy constructor.
  template<H> vtkPoolAllocator(const vtkPoolAllocator<H> &other) throw()
    {
      assert("check: NOT USED" & 0);
    }

  // Destructor.
  ~vtkPoolAllocator() throw()
    {
    }

  // Return the size of the chunks.
  // \post positive_result: result>0
  static int GetChunkSize()
    {
      return this.Allocator.GetChunkSize();
    }

  // Set the chunk size.
  // \pre positive_size: size>0.
  // \post is_set: value==GetChunkSize()
  static void SetChunkSize(int size)
    {
      // Pre-conditions.
      assert("pre: positive_size" && size>0);

      this.Allocator.SeChunkSize(size);

      // Post-conditions.
      assert("post: is_set" && value==this.GetChunkSize());
    }

  // Initialize *p by val.
  // Comment from the norm:
  // Effect: new((void *)p) G(val)
  void construct(pointer p,
                 const G &val);

  // Destroy *p but don't deallocate.
  // Comment from the norm:
  // Effect: ((G*)p)->~G()
  void destroy(pointer p);

protected:
  static vtkPoolManager<G> Allocator;
};

// Initialization of the static variable.
template <class G> vtkPoolManager<G> vtkPoolAllocator<G>::Allocator();

// Comment from the norm:
// Return true iff storage allocated from each can be deallocated via the
// other.
template <class G> bool operator==(const allocator<G>&VTK_NOT_USED(a1),
                                   const allocator<G>&VTK_NOT_USED(a2)) throw()
{
  return true;
}

// Comment from the norm:
// Same as !(a1==a2).
template <class G> bool operator!=(const allocator<G>&a1,
                                   const allocator<G>&a2) throw()
{
  return !(a1==a2);
}
#endif

const unsigned int VTK_DEFAULT_CHUNK_SIZE=50;
const int VTK_DEFAULT_NUMBER_OF_CHUNKS=100;

//-----------------------------------------------------------------------------
// Memory management with a pool of objects to make allocation of chunks of
// objects instead of slow per-object allocation.
// Assumption about class G: has a public default constructor.
template <class G> class vtkPoolManager
{
public:
  // Default constructor.
  vtkPoolManager()
    {
      this->Chunks=0;
      this->ChunkSize=VTK_DEFAULT_CHUNK_SIZE;
    }

  // Initialize the pool with a set of empty chunks.
  void Init()
    {
      if(this->Chunks==0)
        {
        this->Chunks=new std::vector<std::vector<G> *>();
        this->Chunks->reserve(VTK_DEFAULT_NUMBER_OF_CHUNKS);
        }
    }

  // Is the pool initialized?
  int IsInitialized()
    {
      return this->Chunks!=0;
    }

  // Return a new `G' object.
  // \pre is_initialized: IsInitialized()
  G *Allocate()
    {
      assert("pre: is_initialized" && this->IsInitialized());
      G *result=0;
      size_t c=this->Chunks->size();
      if(c==0) // first Allocate()
        {
        this->Chunks->resize(1);
        (*this->Chunks)[0]=new std::vector<G>();
        // Allocate the first chunk
        (*this->Chunks)[0]->reserve(this->ChunkSize);
        (*this->Chunks)[0]->resize(1);
        result=&((*((*this->Chunks)[0]))[0]);
        }
      else
        {
        // At the end of the current chunk?
        if((*this->Chunks)[c-1]->size()==this->ChunkSize)
          {
          // No more chunk?
          if(this->Chunks->size()==this->Chunks->capacity())
            {
            // double the capacity.
            this->Chunks->reserve(this->Chunks->capacity()*2);
            }
          // Allocate the next chunk.
          size_t chunkIdx=this->Chunks->size();
          this->Chunks->resize(chunkIdx+1);
          (*this->Chunks)[chunkIdx]=new std::vector<G>();
          (*this->Chunks)[chunkIdx]->reserve(this->ChunkSize);
          // Return the first element of this new chunk.
          (*this->Chunks)[chunkIdx]->resize(1);
          result=&((*((*this->Chunks)[chunkIdx]))[0]);
          }
        else
          {
          size_t c2=(*this->Chunks)[c-1]->size();
          (*this->Chunks)[c-1]->resize(c2+1);
          result=&((*((*this->Chunks)[c-1]))[c2]);
          }
        }
      return result;
    }

  // Destructor.
  ~vtkPoolManager()
    {
      if(this->Chunks!=0)
        {
        size_t c=this->Chunks->size();
        size_t i=0;
        while(i<c)
          {
          delete (*this->Chunks)[i];
          ++i;
          }
        delete Chunks;
        }
    }

  // Return the size of the chunks.
  // \post positive_result: result>0
  unsigned int GetChunkSize()
    {
      return this->ChunkSize;
    }

  // Set the chunk size.
  // \pre not_yet_initialized: !IsInitialized()
  // \pre positive_size: size>0.
  // \post is_set: size==GetChunkSize()
  void SetChunkSize(unsigned int size)
    {
      // Pre-conditions.
      assert("pre: not_yet_initialized" && !this->IsInitialized());
      assert("pre: positive_size" && size>0);

      this->ChunkSize=size;

      // Post-conditions.
      assert("post: is_set" && size==this->GetChunkSize());
    }
protected:
  std::vector<std::vector<G> *> *Chunks;
  unsigned int ChunkSize;
};

// The 2D cell with the maximum number of points is VTK_BIQUADRATIC_QUAD
const int VTK_MAXIMUM_NUMBER_OF_POINTS=9;

//-----------------------------------------------------------------------------
// Surface element: face of a 3D cell.
//  As this is internal use only, we put variables as public.
class vtkSurfel
{
public:
  // 2D cell type:
  // VTK_TRIANGLE,
  // VTK_POLYGON,
  // VTK_PIXEL,
  // VTK_QUAD,
  // VTK_QUADRATIC_TRIANGLE,
  // VTK_QUADRATIC_QUAD,
  // VTK_BIQUADRATIC_QUAD,
  // VTK_BIQUADRATIC_TRIANGLE
  // VTK_QUADRATIC_LINEAR_QUAD
  vtkIdType Type;

  // Dataset point Ids that form the surfel.
  vtkIdType Points[VTK_MAXIMUM_NUMBER_OF_POINTS];

  // Number of points defining the cell.
  // For cells with a fixed number of points like triangle, it looks redundant.
  // However, it is useful for polygon (pentagonal or hexagonal face).
  vtkIdType NumberOfPoints;

  // Index of the point with the smallest dataset point Id.
  // SmallestIdx>=0 && SmallestIdx<NumberOfPoints.
  // Its dataset point Id is given by Points[SmallestIdx]
  vtkIdType SmallestIdx;

  // Id of the 3D cell this surfel belongs to,
  // -1 if it belongs to more than one (it means the surfel is not on the
  // boundary of the dataset, so it will be not visible.
  vtkIdType Cell3DId;

  // A surfel is also an element of a one-way linked list: in the hashtable,
  // each key entry is a one-way linked list of Surfels.
  vtkSurfel *Next;
};

//-----------------------------------------------------------------------------
// Hashtable of surfels.
const int VTK_HASH_PRIME=31;
class vtkHashTableOfSurfels
{
public:
  // Constructor for the number of points in the dataset and an initialized
  // pool.
  // \pre positive_number: numberOfPoints>0
  // \pre pool_exists: pool!=0
  // \pre initialized_pool: pool->IsInitialized()
  vtkHashTableOfSurfels(int numberOfPoints,
                        vtkPoolManager<vtkSurfel> *pool)
    :HashTable(numberOfPoints)
    {
      assert("pre: positive_number" && numberOfPoints>0);
      assert("pre: pool_exists" && pool!=0);
      assert("pre: initialized_pool" && pool->IsInitialized());

      this->Pool=pool;
      int i=0;
      int c=numberOfPoints;
      while(i<c)
        {
        this->HashTable[i]=0;
        ++i;
        }
    }
  std::vector<vtkSurfel *> HashTable;

  // Add a face defined by its cell type `faceType', its number of points,
  // its list of points and the cellId of the 3D cell it belongs to.
  // \pre valid_range: numberOfPoints>=0 && numberOfPoints<=VTK_MAXIMUM_NUMBER_OF_POINTS
  void InsertFace(vtkIdType cellId,
                  vtkIdType faceType,
                  int numberOfPoints,
                  vtkIdType points[VTK_MAXIMUM_NUMBER_OF_POINTS])
    {
      assert("pre: valid_range" && numberOfPoints>=0 && numberOfPoints<=VTK_MAXIMUM_NUMBER_OF_POINTS);

      // Compute the smallest id among the corner points.
      int smallestIdx;
      vtkIdType smallestId;
      int i;

      int numberOfCornerPoints;

      switch(faceType)
        {
        case VTK_QUADRATIC_TRIANGLE:
        case VTK_BIQUADRATIC_TRIANGLE:
          numberOfCornerPoints=3;
          break;
        case VTK_QUADRATIC_QUAD:
        case VTK_QUADRATIC_LINEAR_QUAD:
        case VTK_BIQUADRATIC_QUAD:
          numberOfCornerPoints=4;
          break;
        default:
          numberOfCornerPoints=numberOfPoints;
          break;
        }

      smallestIdx=0;
      smallestId=points[smallestIdx];
      i=1;
      while(i<numberOfCornerPoints)
        {
        if(points[i]<smallestId)
          {
          smallestIdx=i;
          smallestId=points[i];
          }
        ++i;
        }

      // Compute the hashkey/code
      size_t key=(faceType*VTK_HASH_PRIME+smallestId)%(this->HashTable.size());

      // Get the list at this key (several not equal faces can share the
      // same hashcode). This is the first element in the list.
      vtkSurfel *first=this->HashTable[key];
      vtkSurfel *surfel;
      if(first==0)
        {
        // empty list.
        surfel=this->Pool->Allocate();

        //Just add this new face.
        this->HashTable[key]=surfel;
        }
      else
        {
        int found=0;
        vtkSurfel *current=first;
        vtkSurfel *previous=current;
        while(!found && current!=0)
          {
          found=current->Type==faceType;
          if(found)
            {
            if(faceType==VTK_QUADRATIC_LINEAR_QUAD)
              {
              // weird case
              // the following for combinations are equivalent
              // 01 23, 45, smallestIdx=0, go->
              // 10 32, 45, smallestIdx=1, go<-
              // 23 01, 54, smallestIdx=2, go->
              // 32 10, 54, smallestIdx=3, go<-

              // if current=0 or 2, other face has to be 1 or 3
              // if current=1 or 3, other face has to be 0 or 2

              if(points[0]==current->Points[1])
                {
                found=(points[1]==current->Points[0] &&
                       points[2]==current->Points[3] &&
                       points[3]==current->Points[2] &&
                       points[4]==current->Points[4] &&
                       points[5]==current->Points[5]);
                }
              else
                {
                if(points[0]==current->Points[3])
                  {
                  found=(points[1]==current->Points[2] &&
                         points[2]==current->Points[1] &&
                         points[3]==current->Points[0] &&
                         points[4]==current->Points[5] &&
                         points[5]==current->Points[4]);
                  }
                else
                  {
                  found=0;
                  }
                }
              }
            else
              {
              // If the face is already from another cell. The the first
              // corner point with smallest id will match.

              // The other corner points
              // will be given in reverse order (opposite orientation)
              i=0;
              while(found && i<numberOfCornerPoints)
                {
                // we add numberOfPoints before modulo. Modulo does not work
                // with negative values.
                found=current->Points[(current->SmallestIdx-i+numberOfCornerPoints)%numberOfCornerPoints]
                  ==points[(smallestIdx+i)%numberOfCornerPoints];
                ++i;
                }

              // Check for other kind of points for none linear faces.
              switch(faceType)
                {
                case VTK_QUADRATIC_TRIANGLE:
                  // the mid-edge points
                  i=0;
                  while(found && i<3)
                    {
                    // we add numberOfPoints before modulo. Modulo does not work
                    // with negative values.
                    // -1: start at the end in reverse order.
                    found=current->Points[numberOfCornerPoints+((current->SmallestIdx-i+3-1)%3)]
                      ==points[numberOfCornerPoints+((smallestIdx+i)%3)];
                    ++i;
                    }
                  break;
                case VTK_BIQUADRATIC_TRIANGLE:
                  // the center point
                  found=current->Points[6]==points[6];

                  // the mid-edge points
                  i=0;
                  while(found && i<3)
                    {
                    // we add numberOfPoints before modulo. Modulo does not work
                    // with negative values.
                    // -1: start at the end in reverse order.
                    found=current->Points[numberOfCornerPoints+((current->SmallestIdx-i+3-1)%3)]
                      ==points[numberOfCornerPoints+((smallestIdx+i)%3)];
                    ++i;
                    }
                  break;
                case VTK_QUADRATIC_QUAD:
                  // the mid-edge points
                  i=0;
                  while(found && i<4)
                    {
                    // we add numberOfPoints before modulo. Modulo does not work
                    // with negative values.
                    found=current->Points[numberOfCornerPoints+((current->SmallestIdx-i+4-1)%4)]
                      ==points[numberOfCornerPoints+((smallestIdx+i)%4)];
                    ++i;
                    }
                  break;
                case VTK_BIQUADRATIC_QUAD:
                  // the center point
                  found=current->Points[8]==points[8];

                  // the mid-edge points
                  i=0;
                  while(found && i<4)
                    {
                    // we add numberOfPoints before modulo. Modulo does not work
                    // with negative values.
                    found=current->Points[numberOfCornerPoints+((current->SmallestIdx-i+4-1)%4)]
                      ==points[numberOfCornerPoints+((smallestIdx+i)%4)];
                    ++i;
                    }
                  break;
                default: // other faces are linear: we are done.
                  break;
                }
              }
            }
          previous=current;
          current=current->Next;
          }
        if(found)
          {
          previous->Cell3DId=-1;
          surfel=0;
          }
        else
          {
          surfel=this->Pool->Allocate();
          previous->Next=surfel;
          }
        }
      if(surfel!=0)
        {
        surfel->Next=0;
        surfel->Type=faceType;
        surfel->NumberOfPoints=numberOfPoints;
        surfel->SmallestIdx=smallestIdx;
        surfel->Cell3DId=cellId;
        i=0;
        while(i<numberOfPoints)
          {
          surfel->Points[i]=points[i];
          ++i;
          }
        }
    }
protected:
  vtkPoolManager<vtkSurfel> *Pool;
};

//-----------------------------------------------------------------------------
// Object used to traverse an hashtable of surfels.
class vtkHashTableOfSurfelsCursor
{
public:
  // Initialize the cursor with the table to traverse.
  // \pre table_exists: table!=0
  void Init(vtkHashTableOfSurfels *table)
    {
      assert("pre: table_exists" && table!=0);
      this->Table=table;
      this->AtEnd=1;
    }

  // Move the cursor to the first surfel.
  // If the table is empty, the cursor is at the end of the table.
  void Start()
    {
      this->CurrentKey=0;
      this->CurrentSurfel=0;

      size_t c=Table->HashTable.size();
      int done=this->CurrentKey>=c;
      if(!done)
        {
        this->CurrentSurfel=this->Table->HashTable[this->CurrentKey];
        done=this->CurrentSurfel!=0;
        }
      while(!done)
        {
        ++this->CurrentKey;
        done=this->CurrentKey>=c;
        if(!done)
          {
          this->CurrentSurfel=this->Table->HashTable[this->CurrentKey];
          done=this->CurrentSurfel!=0;
          }
        }
      this->AtEnd=this->CurrentSurfel==0;
    }

  // Is the cursor at the end of the table? (ie. no more surfel?)
  int IsAtEnd()
    {
      return this->AtEnd;
    }

  // Return the surfel the cursor is pointing to.
  vtkSurfel *GetCurrentSurfel()
    {
      assert("pre: not_at_end"&& !IsAtEnd());
      return this->CurrentSurfel;
    }

  // Move the cursor to the next available surfel.
  // If there is no more surfel, the cursor is at the end of the table.
  void Next()
    {
      assert("pre: not_at_end"&& !IsAtEnd());
      CurrentSurfel=CurrentSurfel->Next;
      size_t c=Table->HashTable.size();
      if(this->CurrentSurfel==0)
        {
        ++this->CurrentKey;
        int done=this->CurrentKey>=c;
        if(!done)
          {
          this->CurrentSurfel=this->Table->HashTable[this->CurrentKey];
          done=this->CurrentSurfel!=0;
          }
        while(!done)
          {
          ++this->CurrentKey;
          done=this->CurrentKey>=c;
          if(!done)
            {
            this->CurrentSurfel=this->Table->HashTable[this->CurrentKey];
            done=this->CurrentSurfel!=0;
            }
          }
        this->AtEnd=this->CurrentSurfel==0;
        }
    }

protected:
  vtkHashTableOfSurfels *Table;
  size_t CurrentKey;
  vtkSurfel *CurrentSurfel;
  int AtEnd;
};

//-----------------------------------------------------------------------------
// Construct with all types of clipping turned off.
vtkUnstructuredGridGeometryFilter::vtkUnstructuredGridGeometryFilter()
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

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;
  this->OriginalCellIdsName = NULL;
  this->OriginalPointIdsName = NULL;

  this->Merging = 1;
  this->Locator = NULL;

  this->HashTable=0;
}

//-----------------------------------------------------------------------------
vtkUnstructuredGridGeometryFilter::~vtkUnstructuredGridGeometryFilter()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }

  this->SetOriginalCellIdsName(NULL);
  this->SetOriginalPointIdsName(NULL);
}

//-----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkUnstructuredGridGeometryFilter::SetExtent(double xMin,
                                                  double xMax,
                                                  double yMin,
                                                  double yMax,
                                                  double zMin,
                                                  double zMax)
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

//-----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkUnstructuredGridGeometryFilter::SetExtent(double extent[6])
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

//-----------------------------------------------------------------------------
int vtkUnstructuredGridGeometryFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Input may just have the UnstructuredGridBase
  // interface, but output should be an unstructured grid.
  vtkUnstructuredGridBase *input = vtkUnstructuredGridBase::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

//  this->DebugOn();

  // Input
  vtkIdType numCells=input->GetNumberOfCells();
  if(numCells==0)
    {
    vtkDebugMacro(<<"Nothing to extract");
    return 1;
    }
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkFieldData *fd=input->GetFieldData();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkPoints *inPts=input->GetPoints();
  vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());

  // Output
  vtkPointData *outputPD=output->GetPointData();
  vtkCellData *outputCD=output->GetCellData();
  vtkFieldData *outputFD=output->GetFieldData();
//  vtkUnsignedCharArray *types=vtkUnsignedCharArray::New();
//  types->Allocate(numCells);
//  vtkIdTypeArray *locs=vtkIdTypeArray::New();
//  locs->Allocate(numCells);
//  vtkCellArray *conn=vtkCellArray::New();
//  conn->Allocate(numCells);

  // Ghost cells
  unsigned char updateLevel=(unsigned char)(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));


  unsigned char *cellGhostLevels=0;
  vtkDataArray *temp=0;
  if(cd!=0)
    {
    temp=cd->GetArray("vtkGhostLevels");
    }
  if(temp!=0&&temp->GetDataType()==VTK_UNSIGNED_CHAR&&temp->GetNumberOfComponents()==1)
    {
    cellGhostLevels=((vtkUnsignedCharArray*)temp)->GetPointer(0);
    }
  else
    {
    vtkDebugMacro("No appropriate ghost levels field available.");
    }


  // Visibility of cells.
  char *cellVis;
  int allVisible=(!this->CellClipping) && (!this->PointClipping) &&
    (!this->ExtentClipping) &&(cellGhostLevels==0);
  if(allVisible)
    {
    cellVis=NULL;
    }
  else
    {
    cellVis=new char[numCells];
    }

  vtkIdType cellId;
  vtkIdType npts=0;
  vtkIdType *pts=0;
  int i;
  double x[3];

  // Loop over the cells determining what's visible
  if(!allVisible)
    {
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
         cellIter->GoToNextCell())
      {
      cellId = cellIter->GetCellId();
      npts = cellIter->GetNumberOfPoints();
      pts = cellIter->GetPointIds()->GetPointer(0);
      if((cellGhostLevels!=0 && cellGhostLevels[cellId] > updateLevel)||
         (this->CellClipping && (cellId < this->CellMinimum ||
                                 cellId > this->CellMaximum)) )
        {
        // the cell is a ghost cell or is clipped.
        cellVis[cellId] = 0;
        }
      else
        {
        i=0;
        cellVis[cellId] = 1;
        while(i<npts && cellVis[cellId])
          {
          inPts->GetPoint(pts[i], x);
          cellVis[cellId]=!((this->PointClipping &&
                             (pts[i] < this->PointMinimum ||
                              pts[i] > this->PointMaximum) ) ||
                             (this->ExtentClipping &&
                              (x[0] < this->Extent[0] ||
                               x[0] > this->Extent[1] ||
                               x[1] < this->Extent[2] ||
                               x[1] > this->Extent[3] ||
                               x[2] < this->Extent[4] ||
                               x[2] > this->Extent[5] )) );

          ++i;
          }//for each point
        }//if point clipping needs checking
      }//for all cells
    }//if not all visible

  vtkIdList *cellIds=vtkIdList::New();


  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  output->Allocate(numCells);
  outputPD->CopyAllocate(pd,numPts,numPts/2);
  vtkSmartPointer<vtkIdTypeArray> originalPointIds;
  if (this->PassThroughPointIds)
    {
    originalPointIds = vtkSmartPointer<vtkIdTypeArray>::New();
    originalPointIds->SetName(this->GetOriginalPointIdsName());
    originalPointIds->SetNumberOfComponents(1);
    originalPointIds->Allocate(numPts, numPts/2);
    }

  outputCD->CopyAllocate(cd,numCells,numCells/2);
  vtkSmartPointer<vtkIdTypeArray> originalCellIds;
  if (this->PassThroughCellIds)
    {
    originalCellIds = vtkSmartPointer<vtkIdTypeArray>::New();
    originalCellIds->SetName(this->GetOriginalCellIdsName());
    originalCellIds->SetNumberOfComponents(1);
    originalCellIds->Allocate(numCells, numCells/2);
    }

  // Shallow copy field data not associated with points or cells
  outputFD->ShallowCopy(fd);

  vtkIdType *pointMap=0;

  if(this->Merging)
    {
    if(this->Locator==0)
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion(newPts,input->GetBounds());
    }
  else
    {
    pointMap=new vtkIdType[numPts];
    for (i=0; i<numPts; i++)
      {
      pointMap[i]=-1; //initialize as unused
      }
    }

  // Traverse cells to extract geometry
  int progressCount = 0;
  int abort=0;
  vtkIdType progressInterval = numCells/20 + 1;

  vtkPoolManager<vtkSurfel> *pool=new vtkPoolManager<vtkSurfel>;
  pool->Init();
  this->HashTable=new vtkHashTableOfSurfels(numPts,pool);

  vtkIdType ptId;
  vtkIdType newPtId;
  vtkIdType newCellId;

  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal() && !abort;
       cellIter->GoToNextCell())
    {
    cellId = cellIter->GetCellId();
    //Progress and abort method support
    if ( progressCount >= progressInterval )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress ((double)cellId/numCells);
      abort = this->GetAbortExecute();
      progressCount = 0;
      }
    progressCount++;

    vtkIdType points[VTK_MAXIMUM_NUMBER_OF_POINTS];

    npts = cellIter->GetNumberOfPoints();
    pts = cellIter->GetPointIds()->GetPointer(0);
    if ( allVisible || cellVis[cellId] )
      {
      int cellType = cellIter->GetCellType();
      if((cellType>=VTK_EMPTY_CELL && cellType<=VTK_QUAD)
         ||(cellType>=VTK_QUADRATIC_EDGE && cellType<=VTK_QUADRATIC_QUAD)
         ||(cellType==VTK_BIQUADRATIC_QUAD)
         ||(cellType==VTK_QUADRATIC_LINEAR_QUAD)
         ||(cellType==VTK_BIQUADRATIC_TRIANGLE)
         ||(cellType==VTK_CUBIC_LINE)
        )
        {
        vtkDebugMacro(<<"not 3D cell. type="<<cellType);
        // not 3D: just copy it
        cellIds->Reset();
        if ( this->Merging )
          {
          for (i=0; i < npts; i++)
            {
            ptId =pts[i]; // cell->PointIds->GetId(i);
            input->GetPoint(ptId, x);
            if ( this->Locator->InsertUniquePoint(x, newPtId) )
              {
              outputPD->CopyData(pd,ptId,newPtId);
              if (this->PassThroughPointIds)
                {
                originalPointIds->InsertValue(newPtId, ptId);
                }
              }
            cellIds->InsertNextId(newPtId);
            }
          }//merging coincident points
        else
          {
          for (i=0; i < npts; i++)
            {
            ptId = pts[i]; // cell->PointIds->GetId(i);
            if ( pointMap[ptId] < 0 )
              {
              newPtId=newPts->InsertNextPoint(inPts->GetPoint(ptId));
              pointMap[ptId]=newPtId;
              outputPD->CopyData(pd, ptId, newPtId);
              if (this->PassThroughPointIds)
                {
                originalPointIds->InsertValue(newPtId, ptId);
                }
              }
            cellIds->InsertNextId(pointMap[ptId]);
            }
          }//keeping original point list

        newCellId = output->InsertNextCell(cellType,cellIds);
        outputCD->CopyData(cd, cellId, newCellId);
        if (this->PassThroughCellIds)
          {
          originalCellIds->InsertValue(newCellId, cellId);
          }
        }
      else // added the faces to the hashtable
        {
        vtkDebugMacro(<<"3D cell. type="<<cellType);
        int *faceIndices;
        int face;
        int pt;
        switch(cellType)
          {
          case VTK_TETRA:
            face=0;
            while(face<4)
              {
              faceIndices=vtkTetra::GetFaceArray(face);
              pt=0;
              while(pt<3)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_TRIANGLE,3,points);
              ++face;
              }
            break;
          case VTK_VOXEL:
            // note, faces are PIXEL not QUAD. We don't need to convert
            //  to QUAD because PIXEL exist in an UnstructuredGrid.
            face=0;
            while(face<6)
              {
              faceIndices=vtkVoxel::GetFaceArray(face);
              pt=0;
              while(pt<4)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_PIXEL,4,points);
              ++face;
              }
            break;
          case VTK_HEXAHEDRON:
            face=0;
            while(face<6)
              {
              faceIndices=vtkHexahedron::GetFaceArray(face);
              pt=0;
              while(pt<4)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUAD,4,points);
              ++face;
              }
            break;
          case VTK_WEDGE:
            face=0;
            while(face<2)
              {
              faceIndices=vtkWedge::GetFaceArray(face);
              pt=0;
              while(pt<3)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_TRIANGLE,3,points);
              ++face;
              }
            while(face<5)
              {
              faceIndices=vtkWedge::GetFaceArray(face);
              pt=0;
              while(pt<4)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUAD,4,points);
              ++face;
              }
            break;
          case VTK_PYRAMID:
            faceIndices=vtkPyramid::GetFaceArray(0);
            pt=0;
            while(pt<4)
              {
              points[pt]=pts[faceIndices[pt]];
              ++pt;
              }
            this->HashTable->InsertFace(cellId,VTK_QUAD,4,points);
            face=1;
            while(face<5)
              {
              faceIndices=vtkPyramid::GetFaceArray(face);
              pt=0;
              while(pt<3)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_TRIANGLE,3,points);
              ++face;
              }
            break;
          case VTK_PENTAGONAL_PRISM:
            face=0;
            while(face<2)
              {
              faceIndices=vtkPentagonalPrism::GetFaceArray(face);
              pt=0;
              while(pt<5)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_POLYGON,5,points);
              ++face;
              }
            while(face<7)
              {
              faceIndices=vtkPentagonalPrism::GetFaceArray(face);
              pt=0;
              while(pt<4)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUAD,4,points);
              ++face;
              }
            break;
          case VTK_HEXAGONAL_PRISM:
            face=0;
            while(face<2)
              {
              faceIndices=vtkHexagonalPrism::GetFaceArray(face);
              pt=0;
              while(pt<6)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_POLYGON,6,points);
              ++face;
              }
            while(face<8)
              {
              faceIndices=vtkHexagonalPrism::GetFaceArray(face);
              pt=0;
              while(pt<4)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUAD,4,points);
              ++face;
              }
            break;
          case VTK_QUADRATIC_TETRA:
            face=0;
            while(face<4)
              {
              faceIndices=vtkQuadraticTetra::GetFaceArray(face);
              pt=0;
              while(pt<6)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_TRIANGLE,6,
                                          points);
              ++face;
              }
            break;
          case VTK_QUADRATIC_HEXAHEDRON:
            face=0;
            while(face<6)
              {
              faceIndices=vtkQuadraticHexahedron::GetFaceArray(face);
              pt=0;
              while(pt<8)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_QUAD,8,
                                          points);
              ++face;
              }
            break;
          case VTK_QUADRATIC_WEDGE:
            face=0;
            while(face<2)
              {
              faceIndices=vtkQuadraticWedge::GetFaceArray(face);
              pt=0;
              while(pt<6)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_TRIANGLE,6,
                                          points);
              ++face;
              }
            while(face<5)
              {
              faceIndices=vtkQuadraticWedge::GetFaceArray(face);
              pt=0;
              while(pt<8)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_QUAD,8,
                                          points);
              ++face;
              }
            break;
          case VTK_QUADRATIC_PYRAMID:
            faceIndices=vtkQuadraticPyramid::GetFaceArray(0);
            pt=0;
            while(pt<8)
              {
              points[pt]=pts[faceIndices[pt]];
              ++pt;
              }
            this->HashTable->InsertFace(cellId,VTK_QUADRATIC_QUAD,8,points);
            face=1;
            while(face<5)
              {
              faceIndices=vtkQuadraticPyramid::GetFaceArray(face);
              pt=0;
              while(pt<6)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_TRIANGLE,6,points);
              ++face;
              }
            break;
          case VTK_TRIQUADRATIC_HEXAHEDRON:
            face=0;
            while(face<6)
              {
              faceIndices=vtkTriQuadraticHexahedron::GetFaceArray(face);
              pt=0;
              while(pt<9)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_BIQUADRATIC_QUAD,9,
                                          points);
              ++face;
              }
            break;
          case VTK_QUADRATIC_LINEAR_WEDGE:
            face=0;
            while(face<2)
              {
              faceIndices=vtkQuadraticLinearWedge::GetFaceArray(face);
              pt=0;
              while(pt<6)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_TRIANGLE,6,
                                          points);
              ++face;
              }
            while(face<5)
              {
              faceIndices=vtkQuadraticLinearWedge::GetFaceArray(face);
              pt=0;
              while(pt<6)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_LINEAR_QUAD,6,
                                          points);
              ++face;
              }
            break;
          case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
            face=0;
            while(face<2)
              {
              faceIndices=vtkBiQuadraticQuadraticWedge::GetFaceArray(face);
              pt=0;
              while(pt<6)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_TRIANGLE,6,
                                          points);
              ++face;
              }
            while(face<5)
              {
              faceIndices=vtkBiQuadraticQuadraticWedge::GetFaceArray(face);
              pt=0;
              while(pt<9)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_BIQUADRATIC_QUAD,9,
                                          points);
              ++face;
              }
            break;
          case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
            face=0;
            while(face<4)
              {
              faceIndices=vtkBiQuadraticQuadraticHexahedron::GetFaceArray(face);
              pt=0;
              while(pt<9)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_BIQUADRATIC_QUAD,9,
                                          points);
              ++face;
              }
            while(face<6)
              {
              faceIndices=vtkBiQuadraticQuadraticHexahedron::GetFaceArray(face);
              pt=0;
              while(pt<8)
                {
                points[pt]=pts[faceIndices[pt]];
                ++pt;
                }
              this->HashTable->InsertFace(cellId,VTK_QUADRATIC_QUAD,8,
                                          points);
              ++face;
              }
            break;
          default:
            vtkErrorMacro(<< "Cell type "
                          << vtkCellTypes::GetClassNameFromTypeId(cellType)
                          << "(" << cellType << ")"
                          << " is not a 3D cell.")
          }
        }
      } //if cell is visible
    } //for all cells

  // Loop over visible surfel (coming from a unique cell) in the hashtable:

  vtkHashTableOfSurfelsCursor cursor;
  cursor.Init(this->HashTable);
  cursor.Start();
  while(!cursor.IsAtEnd() && !abort)
    {
    vtkSurfel *surfel=cursor.GetCurrentSurfel();
    cellId=surfel->Cell3DId;
    if(cellId>=0) // on dataset boundary
      {
      vtkIdType cellType=surfel->Type;
      npts=surfel->NumberOfPoints;
      // Dataset point Ids that form the surfel.
      pts=surfel->Points;

      cellIds->Reset();
      if ( this->Merging )
        {
        for (i=0; i < npts; i++)
          {
          ptId = pts[i];
          input->GetPoint(ptId, x);
          if ( this->Locator->InsertUniquePoint(x, newPtId) )
            {
            outputPD->CopyData(pd,ptId,newPtId);
            if (this->PassThroughPointIds)
              {
              originalPointIds->InsertValue(newPtId, ptId);
              }
            }
          cellIds->InsertNextId(newPtId);
          }
        }//merging coincident points
      else
        {
        for (i=0; i < npts; i++)
          {
          ptId = pts[i];
          if ( pointMap[ptId] < 0 )
            {
            newPtId=newPts->InsertNextPoint(inPts->GetPoint(ptId));
            pointMap[ptId]=newPtId;
            outputPD->CopyData(pd, ptId, newPtId);
            if (this->PassThroughPointIds)
              {
              originalPointIds->InsertValue(newPtId, ptId);
              }
            }
          cellIds->InsertNextId(pointMap[ptId]);
          }
        }//keeping original point list

      newCellId = output->InsertNextCell(cellType,cellIds);
      outputCD->CopyData(cd, cellId, newCellId);
      if (this->PassThroughCellIds)
        {
        originalCellIds->InsertValue(newCellId, cellId);
        }
      }
    cursor.Next();
    }
  if(!this->Merging)
    {
    delete[] pointMap;
    }
  cellIds->Delete();
  delete this->HashTable;
  delete pool;


  // Set the output.
  output->SetPoints(newPts);
  newPts->Delete();

//  output->SetCells(types,locs,conn);

  if (this->PassThroughPointIds)
    {
    outputPD->AddArray(originalPointIds);
    }
  if (this->PassThroughCellIds)
    {
    outputCD->AddArray(originalCellIds);
    }

  if(!this->Merging && this->Locator)
    {
    this->Locator->Initialize();
    }

//  types->Delete();
//  locs->Delete();
//  conn->Delete();

  output->Squeeze();
  delete [] cellVis;
  return 1;
}

//-----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkUnstructuredGridGeometryFilter::SetLocator(vtkIncrementalPointLocator *locator)
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

//-----------------------------------------------------------------------------
void vtkUnstructuredGridGeometryFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

//-----------------------------------------------------------------------------
int vtkUnstructuredGridGeometryFilter::FillInputPortInformation(
  int,
  vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
            "vtkUnstructuredGridBase");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridGeometryFilter::PrintSelf(ostream& os,
                                                  vtkIndent indent)
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

  os << indent << "PassThroughCellIds: " << this->PassThroughCellIds << endl;
  os << indent << "PassThroughPointIds: " << this->PassThroughPointIds << endl;

  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;

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

//-----------------------------------------------------------------------------
unsigned long int vtkUnstructuredGridGeometryFilter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}

//-----------------------------------------------------------------------------
int vtkUnstructuredGridGeometryFilter::RequestUpdateExtent(
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
