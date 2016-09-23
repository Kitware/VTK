/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredImplicitConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredImplicitConnectivity.h"

// VTK includes
#include "vtkDataArray.h"
#include "vtkFieldDataSerializer.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkStructuredGrid.h"

// C/C++ includes
#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>
#include <vector>

//==============================================================================
// INTERNAL DATASTRUCTURES & DEFINITIONS
//==============================================================================

// Some usefull extent macros
#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]

#define I(ijk) ijk[0]
#define J(ijk) ijk[1]
#define K(ijk) ijk[2]

namespace vtk
{
namespace detail
{

// Given two intervals A=[a1,a2] and B[b1,b2] the IntervalsConnect struct
// enumerates the cases where interval A connects to Interval B.
struct IntervalsConnect
{
  // NOTE: This enum is arranged s.t., negating a value in [-4,4] will yield
  // the mirror inverse
  enum connectivity_t
  {
    IMPLICIT_LO = -4, // Interval A implicitly connects with B on A's low end
    SUBSET      = -3, // Interval A is completely inside interval B
    OVERLAP_LO  = -2, // Interval A intersects with B on A's low end
    LO          = -1, // A's low end touches B's high end A.Low() == B.High()
    ONE_TO_ONE  =  0, // Intervals A,B are exactly the same.
    HI          =  1, // A's high end touches B's low end A.High() == B.Low()
    OVERLAP_HI  =  2, // Interval A intersects with B on A's high end
    SUPERSET    =  3, // Interval A *contains* all of interval B
    IMPLICIT_HI =  4, // Interval A implicitly connects with B on its high end.

    DISJOINT    =  5, // Intervals A,B are completely disjoint.
    UNDEFINED   =  6  // Undefined
  };

  static
  std::string OrientationToString( int orient[3] )
  {
    std::ostringstream oss;
    oss << "(";
    for(int i=0; i < 3; ++i)
    {
      if(i==1 || i==2)
      {
        oss << ", ";
      }
      switch( orient[i] )
      {
        case IMPLICIT_LO:
          oss << "IMPLICIT_LO";
          break;
        case SUBSET:
          oss << "SUBSET";
          break;
        case OVERLAP_LO:
          oss << "OVERLAP_LO";
          break;
        case LO:
          oss << "LO";
          break;
        case ONE_TO_ONE:
          oss << "ONE_TO_ONE";
          break;
        case HI:
          oss << "HI";
          break;
        case OVERLAP_HI:
          oss << "OVERLAP_HI";
          break;
        case SUPERSET:
          oss << "SUPERSET";
          break;
        case IMPLICIT_HI:
          oss << "IMPLICIT_HI";
          break;
        case DISJOINT:
          oss << "DISJOINT";
          break;
        case UNDEFINED:
          oss << "UNDEFINED";
          break;
        default:
          oss << "*UNKNOWN*";
      } // END switch
    } // END for
    oss << ")";
    return( oss.str() );
  }

}; // END struct IntervalsConnect

//------------------------------------------------------------------------------
//  Interval class Definition
//------------------------------------------------------------------------------
class Interval
{
public:
  Interval() : lo(0), hi(-1) {};
  Interval(const int l, const int h) : lo(l), hi(h) {};
  ~Interval() {};

  int Low() const { return this->lo; };
  int High() const { return this->hi; };
  int Cardinality() const { return(this->hi-this->lo+1); };
  bool Valid() const { return(this->lo <= this->hi); };
  void Set(const int l, const int h) { this->lo=l; this->hi=h; };
  void Invalidate() {this->Set(0,-1);}
  bool Within(const Interval& B) const
    { return( (this->lo >= B.Low()) && (this->hi <= B.High()) ); };

  bool ImplicitNeighbor(const Interval& B, int& type);
  static bool ImplicitNeighbors(
      const Interval& A, const Interval& B, int& type);

  bool Intersects(const Interval& B, Interval& Overlap, int& type);
  static bool Intersects(const Interval& A, const Interval& B,
                         Interval& Overlap, int& type);
private:
  int lo;
  int hi;
};

//------------------------------------------------------------------------------
bool Interval::ImplicitNeighbors(const Interval& A, const Interval& B, int& t)
{
  assert("pre: interval is not valid!" && A.Valid());
  assert("pre: B interval is not valid!" && B.Valid() );

  bool status = false;
  if( A.High()+1 == B.Low() )
  {
    status = true;
    t = IntervalsConnect::IMPLICIT_HI;
  }
  else if( B.High()+1 == A.Low() )
  {
    status = true;
    t = IntervalsConnect::IMPLICIT_LO;
  }
  return( status );
}

//------------------------------------------------------------------------------
bool Interval::ImplicitNeighbor(const Interval& B, int& type)
{
  return( Interval::ImplicitNeighbors(*this,B,type) );
}

//------------------------------------------------------------------------------
bool Interval::Intersects(const Interval& A, const Interval& B,
                          Interval& Overlap, int& type)
{
  assert("pre: interval is not valid!" && A.Valid());
  assert("pre: B interval is not valid!" && B.Valid() );

  bool status = false;

  // Disjoint cases
  if( A.High() < B.Low() )
  {
    type = IntervalsConnect::DISJOINT;
    Overlap.Invalidate();
    status = false;
  }
  else if( B.High() < A.Low() )
  {
    type = IntervalsConnect::DISJOINT;
    Overlap.Invalidate();
    status = false;
  }
  // ONE_TO_ONE case
  else if( A.Cardinality()==B.Cardinality() &&
           A.Low()==B.Low() &&
           A.High()==B.High() )
  {
    type = IntervalsConnect::ONE_TO_ONE;
    Overlap.Set(A.Low(),A.High());
    status = true;
  }
  // A is a SUBSET of B
  else if( A.Within(B) )
  {
    type = IntervalsConnect::SUBSET;
    Overlap.Set(A.Low(),A.High());
    status = true;
  }
  // A is a superset of B
  else if( B.Within(A) )
  {
    type = IntervalsConnect::SUPERSET;
    Overlap.Set(B.Low(),B.High());
    status = true;
  }
  // A touches B on the high end
  else if( A.High() == B.Low() )
  {
    type = IntervalsConnect::HI;
    Overlap.Set(A.High(),A.High());
    status = true;
  }
  // A touches B on the low end
  else if( A.Low() == B.High() )
  {
    type = IntervalsConnect::LO;
    Overlap.Set(A.Low(),A.Low());
    status = true;
  }
  // A intersects B on its low end
  else if( (A.Low() >= B.Low()) && (A.Low() <= B.High()) )
  {
    type = IntervalsConnect::OVERLAP_LO;
    Overlap.Set(A.Low(),B.High());
    status = true;
  }
  // A intersects B on its high end
  else if( (A.High() >= B.Low() ) && (A.High() <= B.High()) )
  {
    type = IntervalsConnect::OVERLAP_HI;
    Overlap.Set(B.Low(),A.High());
    status = true;
  }
  else
  {
    vtkGenericWarningMacro(
        << "Undefined interval intersection!"
        << "Code should not reach here!!!");
    type = IntervalsConnect::UNDEFINED;
    status = false;
    Overlap.Invalidate();
  }
  return( status );
}

//------------------------------------------------------------------------------
bool Interval::Intersects(const Interval& B, Interval& Overlap, int& type)
{
  return( Interval::Intersects(*this, B, Overlap, type) );
}

//------------------------------------------------------------------------------
struct ImplicitNeighbor
{
  int Rank;           // the rank of the neighbor
  int Extent[6];      // the extent of the neighbor
  int Orientation[3]; // the orientation w.r.t the local extent
  int Overlap[6];     // the overlap extent

  std::string ToString()
  {
    std::ostringstream oss;

    oss << "rank=" << this->Rank << " ";
    oss << "extent=[";
    oss << this->Extent[0] << ", ";
    oss << this->Extent[1] << ", ";
    oss << this->Extent[2] << ", ";
    oss << this->Extent[3] << ", ";
    oss << this->Extent[4] << ", ";
    oss << this->Extent[5] << "] ";
    oss << "overlap=[";
    oss << this->Overlap[0] << ", ";
    oss << this->Overlap[1] << ", ";
    oss << this->Overlap[2] << ", ";
    oss << this->Overlap[3] << ", ";
    oss << this->Overlap[4] << ", ";
    oss << this->Overlap[5] << "] ";
    oss << "orientation=";
    oss << IntervalsConnect::OrientationToString(this->Orientation);

    return( oss.str() );
  }
};

//------------------------------------------------------------------------------
struct DomainMetaData
{
  int WholeExtent[6];     // Extent of the entire domain

  int DataDescription;    // Data-description of the distributed dataset.
  int NDim;               // Number of dimensions according to DataDescription.
  int DimIndex[3];        // Stores the dimensions of the dataset in the
                          // the right order. This essentially allows to
                          // process 2-D (XY,XZ,YZ) and 3-D datasets in a
                          // transparent way.

  int GlobalImplicit[3];  // indicates for each dimension if there is globally
                          // implicit connectivity. Any value > 0 indicates
                          // implicit connectivity in the given direction.

  // Flat list of extents. Extents are organized as follows:
  // [id, imin, imax, jmin, jmax, kmin, kmax]
  std::vector< int > ExtentListInfo;

  /// \brief Checks if a grid with the given extent is within this domain
  /// \param ext the extent of the grid in query
  /// \return status true if the grid is insided, else false.
  bool HasGrid(int ext[6])
    { return( vtkStructuredExtent::Smaller(ext,this->WholeExtent) );  };

  /// \brief Initializes the domain metadata.
  void Initialize(int wholeExt[6])
  {
    memcpy(this->WholeExtent,wholeExt,6*sizeof(int));
    this->DataDescription =
        vtkStructuredData::GetDataDescriptionFromExtent(wholeExt);

    if (this->DataDescription == VTK_EMPTY)
    {
      return;
    }

    // Sanity checks!
    assert( "pre: data description is VTK_EMPTY!" &&
             (this->DataDescription != VTK_EMPTY) );
    assert( "pre: dataset must be 2-D or 3-D" &&
            (this->DataDescription >= VTK_XY_PLANE) );

    this->NDim = -1;
    std::fill(this->DimIndex,this->DimIndex+3,-1);
    std::fill(this->GlobalImplicit,this->GlobalImplicit+3,0);

    switch( this->DataDescription )
    {
      case VTK_XY_PLANE:
        this->NDim        = 2;
        this->DimIndex[0] = 0;
        this->DimIndex[1] = 1;
        break;
      case VTK_XZ_PLANE:
        this->NDim        = 2;
        this->DimIndex[0] = 0;
        this->DimIndex[1] = 2;
        break;
      case VTK_YZ_PLANE:
        this->NDim        = 2;
        this->DimIndex[0] = 1;
        this->DimIndex[1] = 2;
        break;
      case VTK_XYZ_GRID:
        this->NDim        = 3;
        this->DimIndex[0] = 0;
        this->DimIndex[1] = 1;
        this->DimIndex[2] = 2;
        break;
      default:
        vtkGenericWarningMacro(
            << "Cannot handle data description: "
            << this->DataDescription << "\n");
    } // END switch

    assert( "post: NDim==2 || NDim==3" && ( this->NDim==2 || this->NDim==3 ) );
  }

};

//------------------------------------------------------------------------------
struct StructuredGrid
{
  int ID;
  int Extent[6];
  int DataDescription;

  int Grow[3];     // indicates if the grid grows to the right along each dim.
  int Implicit[3]; // indicates implicit connectivity alone each dim.

  vtkPoints* Nodes;
  vtkPointData* PointData;

  // arrays used if the grid is a rectilinear grid
  vtkDataArray* X_Coords;
  vtkDataArray* Y_Coords;
  vtkDataArray* Z_Coords;

  std::vector< ImplicitNeighbor > Neighbors;

//------------------------------------------------------------------------------
  bool IsRectilinearGrid()
  {
    if( (this->X_Coords != NULL) &&
        (this->Y_Coords != NULL) &&
        (this->Z_Coords != NULL) )
    {
      return true;
    }
    return false;
  }

//------------------------------------------------------------------------------
  void Clear()
  {
    if( this->Nodes != NULL )
    {
      this->Nodes->Delete();
      this->Nodes = NULL;
    }
    if( this->PointData != NULL )
    {
      this->PointData->Delete();
      this->PointData = NULL;
    }
    if( this->X_Coords != NULL )
    {
      this->X_Coords->Delete();
      this->X_Coords = NULL;
    }
    if( this->Y_Coords != NULL )
    {
      this->Y_Coords->Delete();
      this->Y_Coords = NULL;
    }
    if( this->Z_Coords != NULL )
    {
      this->Z_Coords->Delete();
      this->Z_Coords = NULL;
    }
    this->Neighbors.clear();
  }

//------------------------------------------------------------------------------
  void Initialize(StructuredGrid* grid)
  {
    assert("pre: input grid is NULL!" && (grid != NULL) );

    this->Initialize(grid->ID,grid->Extent,NULL,NULL);

    // Grow the extent in each dimension as needed
    for(int i=0; i < 3; ++i)
    {
      if( grid->Grow[i]==1 )
      {
        this->Extent[i*2+1] += 1;
      } // END if
    } // END for all dimensions

    // the number of nodes in the grown extent
    vtkIdType nnodes = vtkStructuredData::GetNumberOfPoints(
        this->Extent,grid->DataDescription);

   // Allocate coordinates, if needed
   if( grid->Nodes != NULL )
   {
     this->Nodes = vtkPoints::New();
     this->Nodes->SetDataType( grid->Nodes->GetDataType() );
     this->Nodes->SetNumberOfPoints( nnodes );
   } // END if has points
   else
   {
     this->Nodes = NULL;
   }

   // Allocate rectilinear grid coordinates, if needed
   if( (grid->X_Coords != NULL) &&
       (grid->Y_Coords != NULL) &&
       (grid->Z_Coords != NULL) )
   {
     int dims[3];
     vtkStructuredData::GetDimensionsFromExtent(
         this->Extent,dims,this->DataDescription);

     this->X_Coords = vtkDataArray::CreateDataArray(
         grid->X_Coords->GetDataType());
     this->X_Coords->SetNumberOfTuples( dims[0] );
     for(vtkIdType idx=0; idx < grid->X_Coords->GetNumberOfTuples(); ++idx)
     {
       this->X_Coords->SetTuple(idx,idx,grid->X_Coords);
     }

     this->Y_Coords = vtkDataArray::CreateDataArray(
         grid->Y_Coords->GetDataType());
     this->Y_Coords->SetNumberOfTuples( dims[1] );
     for(vtkIdType idx=0; idx < grid->Y_Coords->GetNumberOfTuples(); ++idx)
     {
        this->Y_Coords->SetTuple(idx,idx,grid->Y_Coords);
     }

     this->Z_Coords = vtkDataArray::CreateDataArray(
         grid->Z_Coords->GetDataType());
     this->Z_Coords->SetNumberOfTuples( dims[2] );
     for(vtkIdType idx=0; idx < grid->Z_Coords->GetNumberOfTuples(); ++idx)
     {
        this->Z_Coords->SetTuple(idx,idx,grid->Z_Coords);
     }
   } // END if rectilinear grid
   else
   {
     grid->X_Coords = NULL;
     grid->Y_Coords = NULL;
     grid->Z_Coords = NULL;
   }

   // Allocate fields, if needed
   if( grid->PointData != NULL )
   {
     this->PointData = vtkPointData::New();
     this->PointData->CopyAllocate(grid->PointData,nnodes);

     // NOTE: CopyAllocate, allocates the buffers internally, but, does not
     // set the number of tuples of each array to nnodes.
     for(int array=0; array < this->PointData->GetNumberOfArrays(); ++array)
     {
       vtkDataArray* a = this->PointData->GetArray( array );
       a->SetNumberOfTuples(nnodes);
     } // END for all arrays

   }
   else
   {
     this->PointData = NULL;
   }

   // copy everything from the given grid
   int desc   = grid->DataDescription;
   int ijk[3] = {0,0,0};

   for( I(ijk)=IMIN(grid->Extent); I(ijk) <= IMAX(grid->Extent); ++I(ijk) )
   {
     for( J(ijk)=JMIN(grid->Extent); J(ijk) <= JMAX(grid->Extent); ++J(ijk) )
     {
       for( K(ijk)=KMIN(grid->Extent); K(ijk) <= KMAX(grid->Extent); ++K(ijk) )
       {
         // Compute the source index
         vtkIdType srcIdx =
             vtkStructuredData::ComputePointIdForExtent(grid->Extent,ijk,desc);

         // Compute the target index
         vtkIdType targetIdx =
             vtkStructuredData::ComputePointIdForExtent(this->Extent,ijk,desc);

         // Copy nodes
         if( this->Nodes != NULL )
         {
           this->Nodes->SetPoint(targetIdx,grid->Nodes->GetPoint(srcIdx));
         }

         // Copy node-centered fields
         if( this->PointData != NULL )
         {
           this->PointData->CopyData(grid->PointData,srcIdx,targetIdx);
         }

       } // END for all k
     } // END for all j
   } // END for all i

  }

//------------------------------------------------------------------------------
  void Initialize(int id, int ext[6], vtkDataArray* x_coords,
      vtkDataArray* y_coords, vtkDataArray* z_coords, vtkPointData* fields)
  {
    assert("pre: NULL x_coords!" && (x_coords != NULL) );
    assert("pre: NULL y_coords!" && (y_coords != NULL) );
    assert("pre: NULL z_coords!" && (z_coords != NULL) );

    this->ID = id;
    memcpy(this->Extent,ext,6*sizeof(int));
    this->DataDescription = vtkStructuredData::GetDataDescriptionFromExtent(ext);
    std::fill(this->Grow,this->Grow+3,0);
    std::fill(this->Implicit,this->Implicit+3,0);

    this->Nodes = NULL;

    // Effectively, shallow copy the coordinate arrays and maintain ownership
    // of these arrays in the caller.
    this->X_Coords = vtkDataArray::CreateDataArray(x_coords->GetDataType());
    this->X_Coords->SetVoidArray(
        x_coords->GetVoidPointer(0),x_coords->GetNumberOfTuples(),1);

    this->Y_Coords = vtkDataArray::CreateDataArray(y_coords->GetDataType());
    this->Y_Coords->SetVoidArray(
        y_coords->GetVoidPointer(0),y_coords->GetNumberOfTuples(),1);

    this->Z_Coords = vtkDataArray::CreateDataArray(z_coords->GetDataType());
    this->Z_Coords->SetVoidArray(
        z_coords->GetVoidPointer(0),z_coords->GetNumberOfTuples(),1);

    if(fields != NULL)
    {
      this->PointData = vtkPointData::New();
      this->PointData->ShallowCopy(fields);
    }
    else
    {
      this->PointData = NULL;
    }
  }

//------------------------------------------------------------------------------
  void Initialize(int id, int ext[6], vtkPoints* nodes, vtkPointData* fields)
  {
  this->ID = id;
  memcpy(this->Extent,ext,6*sizeof(int));
  this->DataDescription = vtkStructuredData::GetDataDescriptionFromExtent(ext);
  std::fill(this->Grow,this->Grow+3,0);
  std::fill(this->Implicit,this->Implicit+3,0);

  this->X_Coords = NULL;
  this->Y_Coords = NULL;
  this->Z_Coords = NULL;

  if(nodes != NULL)
  {
    this->Nodes = vtkPoints::New();
    this->Nodes->ShallowCopy(nodes);
  }
  else
  {
    this->Nodes = NULL;
  }

  if(fields != NULL)
  {
    this->PointData = vtkPointData::New();
    this->PointData->ShallowCopy(fields);
  }
  else
  {
    this->PointData = NULL;
  }
  }

};

//------------------------------------------------------------------------------
//  CommManager class Definition
//------------------------------------------------------------------------------

class CommunicationManager
{
public:
  CommunicationManager() {};
  ~CommunicationManager() { this->Clear(); };

  unsigned char* GetRcvBuffer(const int fromRank);
  unsigned int GetRcvBufferSize(const int fromRank);

  void EnqueueRcv(const int fromRank);
  void EnqueueSend(const int toRank, unsigned char* data, unsigned int nbytes);
  void Exchange(vtkMPIController* comm);
  int NumMsgs();
  void Clear();

private:
  // map send/rcv buffers based on rank.
  std::map< int, unsigned char* > Send;
  std::map< int, int> SendByteSize;
  std::map< int, unsigned char* > Rcv;
  std::map< int, int> RcvByteSize;
  std::vector< vtkMPICommunicator::Request > Requests;

  // exchanges buffer-sizes
  void AllocateRcvBuffers(vtkMPIController* comm);
};

//------------------------------------------------------------------------------
void CommunicationManager::Clear()
{
  this->Requests.clear();
  this->SendByteSize.clear();
  this->RcvByteSize.clear();

  std::map<int,unsigned char*>::iterator it;
  for(it=this->Send.begin(); it != this->Send.end(); ++it)
  {
    delete [] it->second;
  }
  this->Send.clear();

  for(it=this->Rcv.begin(); it != this->Rcv.end(); ++it)
  {
    delete [] it->second;
  }
  this->Rcv.clear();
}

//------------------------------------------------------------------------------
unsigned char* CommunicationManager::GetRcvBuffer(const int fromRank)
{
  assert( "pre: cannot find buffer for requested rank!" &&
          (this->Rcv.find( fromRank ) != this->Rcv.end()) );
  return( this->Rcv[ fromRank ] );
}

//------------------------------------------------------------------------------
unsigned int CommunicationManager::GetRcvBufferSize(const int fromRank)
{
  assert( "pre: cannot find bytesize size of requested rank!" &&
          (this->RcvByteSize.find( fromRank ) != this->RcvByteSize.end()) );
  return( this->RcvByteSize[ fromRank ]  );
}

//------------------------------------------------------------------------------
int CommunicationManager::NumMsgs()
{
  return static_cast<int>( this->Send.size()+this->Rcv.size() );
}

//------------------------------------------------------------------------------
void CommunicationManager::EnqueueRcv(const int fromRank)
{
  assert("pre: rcv from rank has already been enqueued!" &&
          (this->Rcv.find(fromRank)==this->Rcv.end()) );

  this->Rcv[ fromRank ] = NULL;
  this->RcvByteSize[ fromRank ] = 0;
}

//------------------------------------------------------------------------------
void CommunicationManager::EnqueueSend(
      const int toRank, unsigned char* data, unsigned int nbytes)
{
  assert("pre: send to rank has already been enqueued!" &&
          (this->Send.find(toRank)==this->Send.end()));

  this->Send[ toRank ] = data;
  this->SendByteSize[ toRank ] = nbytes;
}

//------------------------------------------------------------------------------
void CommunicationManager::AllocateRcvBuffers(vtkMPIController* comm)
{
  std::map<int,int>::iterator it;

  // STEP 0: Allocate vector to store request objects for non-blocking comm.
  int rqstIdx = 0;
  this->Requests.resize(this->NumMsgs());

  // STEP 1: Post receives
  for(it=this->RcvByteSize.begin(); it != this->RcvByteSize.end(); ++it)
  {
    int fromRank = it->first;
    int* dataPtr = &(it->second);
    comm->NoBlockReceive(dataPtr,1,fromRank,0,this->Requests[rqstIdx]);
    ++rqstIdx;
  }

  // STEP 2: Post Sends
  for(it=this->SendByteSize.begin(); it != this->SendByteSize.end(); ++it)
  {
    int toRank   = it->first;
    int* dataPtr = &(it->second);
    comm->NoBlockSend(dataPtr,1,toRank,0,this->Requests[rqstIdx]);
    ++rqstIdx;
  }

  // STEP 3: WaitAll
  if (!this->Requests.empty())
  {
    comm->WaitAll(this->NumMsgs(),&this->Requests[0]);
  }
  this->Requests.clear();

  // STEP 4: Allocate rcv buffers
  std::map<int,unsigned char*>::iterator bufferIter = this->Rcv.begin();
  for( ;bufferIter != this->Rcv.end(); ++bufferIter)
  {
    int fromRank = bufferIter->first;
    assert("pre: rcv buffer should be NULL!" && (this->Rcv[fromRank]==NULL) );
    this->Rcv[ fromRank ] = new unsigned char[ this->RcvByteSize[fromRank] ];
  }
}

//------------------------------------------------------------------------------
void CommunicationManager::Exchange(vtkMPIController* comm)
{
  std::map<int,unsigned char*>::iterator it;

  // STEP 0: exchange & allocate buffer sizes
  this->AllocateRcvBuffers(comm);

  // STEP 1: Allocate vector to store request objects for non-blocking comm.
  int rqstIdx = 0;
  this->Requests.resize(this->NumMsgs());

  // STEP 2: Post Rcvs
  for(it=this->Rcv.begin(); it != this->Rcv.end(); ++it)
  {
    int fromRank          = it->first;
    unsigned char* buffer = it->second;
    assert("pre: rcv buffer size not found!" &&
            this->RcvByteSize.find(fromRank) != this->RcvByteSize.end() );
    unsigned int bytesize = this->RcvByteSize[ fromRank ];

    comm->NoBlockReceive(buffer,bytesize,fromRank,0,this->Requests[rqstIdx]);
    ++rqstIdx;
  }

  // STEP 3: Post Sends
  for(it=this->Send.begin(); it != this->Send.end(); ++it)
  {
    int toRank            = it->first;
    unsigned char* buffer = it->second;
    assert("pre: rcv buffer size not found!" &&
            this->SendByteSize.find(toRank) != this->SendByteSize.end() );
    unsigned int bytesize = this->SendByteSize[ toRank ];

    comm->NoBlockSend(buffer,bytesize,toRank,0,this->Requests[rqstIdx]);
    ++rqstIdx;
  }

  // STEP 4: WaitAll
  if (!this->Requests.empty())
  {
    comm->WaitAll(this->NumMsgs(),&this->Requests[0]);
  }
  this->Requests.clear();
}

} // END namespace detail
} // END namespace vtk
//==============================================================================
// END INTERNAL DATASTRUCTURE DEFINITIONS
//==============================================================================

vtkStandardNewMacro(vtkStructuredImplicitConnectivity);

//------------------------------------------------------------------------------
vtkStructuredImplicitConnectivity::vtkStructuredImplicitConnectivity()
{
  this->DomainInfo  = NULL;
  this->InputGrid   = NULL;
  this->OutputGrid  = NULL;
  this->CommManager = NULL;
  this->Controller  = vtkMPIController::SafeDownCast(
            vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkStructuredImplicitConnectivity::~vtkStructuredImplicitConnectivity()
{
  delete this->DomainInfo;
  this->DomainInfo = NULL;

  if( this->InputGrid != NULL )
  {
    this->InputGrid->Clear();
    delete this->InputGrid;
    this->InputGrid = NULL;
  }

  if( this->OutputGrid != NULL )
  {
    this->OutputGrid->Clear();
    delete this->OutputGrid;
    this->OutputGrid = NULL;
  }

  if( this->CommManager != NULL )
  {
    this->CommManager->Clear();
    delete this->CommManager;
    this->CommManager = NULL;
  }

  this->Controller = NULL;
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::PrintSelf(ostream& os,vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "Controller: "      << this->Controller << std::endl;
  if( this->Controller != NULL )
  {
    os << "Number of Ranks: " << this->Controller->GetNumberOfProcesses();
    os << std::endl;
  } // END if Controller != NULL

  os << "Input Grid: " << this->InputGrid  << std::endl;
  if( this->InputGrid != NULL )
  {
    os << "Extent: [" << this->InputGrid->Extent[0];
    os << ", " << this->InputGrid->Extent[1];
    os << ", " << this->InputGrid->Extent[2];
    os << ", " << this->InputGrid->Extent[3];
    os << ", " << this->InputGrid->Extent[4];
    os << ", " << this->InputGrid->Extent[5];
    os << "] " << std::endl;

    os << "Grow: [" << this->InputGrid->Grow[0];
    os << ", " << this->InputGrid->Grow[1];
    os << ", " << this->InputGrid->Grow[2];
    os << "] " << std::endl;

    os << "Number of Neighbors: " << this->InputGrid->Neighbors.size();
    os << std::endl;
    size_t N = this->InputGrid->Neighbors.size();
    for(size_t nei=0; nei < N; ++nei)
    {
      os << "\t" << this->InputGrid->Neighbors[ nei ].ToString();
      os << std::endl;
    } // END for all neighbors
  } // END if InputGrid != NULL
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::SetWholeExtent(int wholeExt[6])
{
  delete this->DomainInfo;

  this->DomainInfo = new vtk::detail::DomainMetaData();
  this->DomainInfo->Initialize(wholeExt);

  assert("post: Domain description does not match across ranks!" &&
          this->GlobalDataDescriptionMatch() );
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::RegisterGrid(
      const int gridID,
      int extent[6],
      vtkPoints* gridNodes,
      vtkPointData* pointData)
{
  // Sanity Checks!
  assert("pre: NULL Domain, whole extent is not set!" &&
          (this->DomainInfo != NULL) );
  assert("pre: input not NULL in this process!" &&
          (this->InputGrid == NULL) );
  assert("pre: input grid ID should be >= 0" && (gridID >= 0) );

  delete this->InputGrid;
  this->InputGrid = NULL;

  // Only add if the grid falls within the output extent. Processes that do
  // not contain the VOI will fail this test.
  if (this->DomainInfo->HasGrid(extent))
  {
    this->InputGrid = new vtk::detail::StructuredGrid();
    this->InputGrid->Initialize(gridID,extent,gridNodes,pointData);
  }
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::RegisterRectilinearGrid(
            const int gridID,
            int extent[6],
            vtkDataArray* xcoords,
            vtkDataArray* ycoords,
            vtkDataArray* zcoords,
            vtkPointData* pointData)
{
  // Sanity Checks!
  assert("pre: NULL Domain, whole extent is not set!" &&
          (this->DomainInfo != NULL) );
  assert("pre: input not NULL in this process!" &&
          (this->InputGrid == NULL) );
  assert("pre: input grid ID should be >= 0" && (gridID >= 0) );

  delete this->InputGrid;
  this->InputGrid = NULL;

  // Only add if the grid falls within the output extent. Processes that do
  // not contain the VOI will fail this test.
  if (this->DomainInfo->HasGrid(extent))
  {
    this->InputGrid = new vtk::detail::StructuredGrid();
    this->InputGrid->Initialize(gridID, extent, xcoords, ycoords, zcoords,
                                pointData);
  }
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::ExchangeExtents()
{
  // Sanity checks!
  assert("pre: null controller!" && (this->Controller != NULL) );
  assert("pre: null domain!" && (this->DomainInfo != NULL) );

  // STEP 0: Construct the extent buffer that will be sent from each process.
  // Each process sends 7 ints: [gridId imin imax jmin jmax kmin kmax]
  int extbuffer[7];
  if( this->InputGrid == NULL )
  {
    // pad the buffer with -1, indicating that this process has no grid
    std::fill(extbuffer,extbuffer+7,-1);
  }
  else
  {
    extbuffer[0] = this->InputGrid->ID;
    memcpy(&extbuffer[1],this->InputGrid->Extent,6*sizeof(int));
  }

  // STEP 1: Allocate receive buffer, we receive 7 ints for each rank
  int nranks = this->Controller->GetNumberOfProcesses();
  this->DomainInfo->ExtentListInfo.resize(7*nranks,0);

  // STEP 2: AllGather
  int* rcvbuffer = &(this->DomainInfo->ExtentListInfo)[0];
  this->Controller->AllGather(extbuffer,rcvbuffer,7);
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::ComputeNeighbors()
{
  if (!this->InputGrid)
  {
    return;
  }

  int type;
  vtk::detail::Interval A; // used to store the local interval at each dim
  vtk::detail::Interval B; // used to store the remote interval at each dim
  vtk::detail::Interval Overlap; // used to store the computed overlap
  vtk::detail::ImplicitNeighbor Neighbor; // used to store neighbor information

  int nranks = this->Controller->GetNumberOfProcesses();
  for(int rank=0; rank < nranks; ++rank)
  {
    int rmtID = this->DomainInfo->ExtentListInfo[rank*7];
    if( (rmtID == this->InputGrid->ID) || (rmtID == -1) )
    {
      // skip self or empty remote grid
      continue;
    }

    int* rmtExtent = &(this->DomainInfo->ExtentListInfo)[rank*7+1];

    // Initialize neighbor data-structure
    Neighbor.Rank = rank;
    memcpy(Neighbor.Extent,rmtExtent,6*sizeof(int));
    memcpy(Neighbor.Overlap,Neighbor.Extent,6*sizeof(int));
    std::fill(Neighbor.Orientation,Neighbor.Orientation+3,
        vtk::detail::IntervalsConnect::UNDEFINED);

    bool disregard = false;
    int nimplicit  = 0;

    for(int dim=0; dim < this->DomainInfo->NDim; ++dim)
    {
      int d = this->DomainInfo->DimIndex[dim];
      assert("pre: invalid dimension!" && (d >= 0) && (d <= 2) );

      A.Set( this->InputGrid->Extent[d*2], this->InputGrid->Extent[d*2+1] );
      B.Set( rmtExtent[d*2], rmtExtent[d*2+1] );

      if( A.ImplicitNeighbor(B,type) )
      {
        this->InputGrid->Implicit[ d ]= 1;
        Neighbor.Orientation[ d ] = type;

        // Compute overlap based on the fact that we are communicating
        // data to the left <=> grow to the right.
        if( type == vtk::detail::IntervalsConnect::IMPLICIT_HI )
        {
          ++nimplicit;
          Neighbor.Overlap[d*2]    =
          Neighbor.Overlap[d*2+1]  = Neighbor.Extent[d*2];
          this->InputGrid->Grow[d] = 1; /* increment by 1 in this dimension */
        } // END if IMPLICIT_HI
        else if( type == vtk::detail::IntervalsConnect::IMPLICIT_LO )
        {
          ++nimplicit;
          Neighbor.Overlap[d*2]   =
          Neighbor.Overlap[d*2+1] = this->InputGrid->Extent[d*2];
        } // END else if IMPLICIT_LO
        else
        {
          vtkGenericWarningMacro(
              << "Invalid implicit connectivity type! "
              << "Code should not reach here!\n"
              );
        } // END else
      } // END if implicit
      else if( A.Intersects(B,Overlap,type) )
      {
        Neighbor.Orientation[ d ] = type;
        Neighbor.Overlap[d*2]     = Overlap.Low();
        Neighbor.Overlap[d*2+1]   = Overlap.High();
      } // END if intersect
      else
      {
        disregard = true;
        Neighbor.Orientation[ d ] = type;
      } // END else
    } // END for all dimensions

    // Determine whether to include the neighbor to the list of neighbors in
    // this rank.

    if( !(nimplicit > 1 || disregard) )
    {
      this->InputGrid->Neighbors.push_back( Neighbor );
    }

  } // END for all ranks
}

//------------------------------------------------------------------------------
bool vtkStructuredImplicitConnectivity::GlobalDataDescriptionMatch()
{
  int sum = -1;
  this->Controller->AllReduce(
      &this->DomainInfo->DataDescription,&sum,1,vtkCommunicator::SUM_OP);
  if( (sum/this->Controller->GetNumberOfProcesses()) ==
      this->DomainInfo->DataDescription)
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkStructuredImplicitConnectivity::HasImplicitConnectivity()
{
  if( this->DomainInfo == NULL )
  {
    vtkGenericWarningMacro(<< "NULL domain, WholeExtent not set!");
    return false;
  }

  if( (this->DomainInfo->GlobalImplicit[0] > 0) ||
      (this->DomainInfo->GlobalImplicit[1] > 0) ||
      (this->DomainInfo->GlobalImplicit[2] > 0) )
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::GetGlobalImplicitConnectivityState()
{
  // Sanity checks!
  assert("pre: null controller!" && (this->Controller != NULL) );

  int sndbuffer[3];
  if( this->InputGrid == NULL)
  {
    std::fill(sndbuffer,sndbuffer+3,0);
  }
  else
  {
    memcpy(sndbuffer,this->InputGrid->Implicit,3*sizeof(int));
  }

  this->Controller->AllReduce(
      sndbuffer,this->DomainInfo->GlobalImplicit,3,vtkCommunicator::SUM_OP);
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::EstablishConnectivity()
{
  // Sanity checks!
  assert("pre: null controller!" && (this->Controller != NULL) );
  assert("pre: NULL domain, WholeExtent not set!" &&
          (this->DomainInfo != NULL) );

  // STEP 0: Exchange extents
  this->ExchangeExtents();

  // STEP 1: Compute Neighbors
  this->ComputeNeighbors();

  // STEP 2: Get Global Implicit connectivity state
  this->GetGlobalImplicitConnectivityState();

  // STEP 3: Barrier synchronization
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::GetOutputStructuredGrid(
        const int gridID, vtkStructuredGrid* grid)
{
  assert("pre: NULL output grid!" && (grid != NULL) );
  assert("pre: output grid is NULL!" && (this->OutputGrid != NULL));
  assert("pre: mismatch gridID" && (this->OutputGrid->ID == gridID));
  assert("pre: output grid has no points!" &&
          (this->OutputGrid->Nodes != NULL) );

  // silence warnings, the intent for the gridID here is for extending the
  // implementation in the future to allow multiple grids per process.
  static_cast<void>(gridID);

  grid->Initialize();
  grid->SetExtent(this->OutputGrid->Extent);
  grid->SetPoints(this->OutputGrid->Nodes);
  grid->GetPointData()->ShallowCopy(this->OutputGrid->PointData);
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::GetOutputImageData(
        const int gridID, vtkImageData* grid)
{
  assert("pre: NULL output grid!" && (grid != NULL) );
  assert("pre: output grid is NULL!" && (this->OutputGrid != NULL));
  assert("pre: mismatch gridID" && (this->OutputGrid->ID == gridID));

  // silence warnings, the intent for the gridID here is for extending the
  // implementation in the future to allow multiple grids per process.
  static_cast<void>(gridID);

  grid->SetExtent(this->OutputGrid->Extent);
  grid->GetPointData()->ShallowCopy(this->OutputGrid->PointData);
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::GetOutputRectilinearGrid(
      const int gridID, vtkRectilinearGrid* grid)
{
  assert("pre: NULL output grid!" && (grid != NULL) );
  assert("pre: output grid is NULL!" && (this->OutputGrid != NULL));
  assert("pre: mismatch gridID" && (this->OutputGrid->ID == gridID));

  // silence warnings, the intent for the gridID here is for extending the
  // implementation in the future to allow multiple grids per process.
  static_cast<void>(gridID);

  grid->SetExtent(this->OutputGrid->Extent);
  grid->GetPointData()->ShallowCopy(this->OutputGrid->PointData);
  grid->SetXCoordinates(this->OutputGrid->X_Coords);
  grid->SetYCoordinates(this->OutputGrid->Y_Coords);
  grid->SetZCoordinates(this->OutputGrid->Z_Coords);
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::ConstructOutput()
{
  if( this->OutputGrid != NULL )
  {
    this->OutputGrid->Clear();
    delete this->OutputGrid;
    this->OutputGrid = NULL;
  }

   this->OutputGrid = new vtk::detail::StructuredGrid();
   this->OutputGrid->Initialize( this->InputGrid );
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::UpdateNeighborList(const int dim)
{
  assert("pre: dimension index out-of-bounds!" && (dim >= 0) && (dim <= 2) );
  assert("pre: input grid is NULL!" && this->InputGrid != NULL );
  assert("pre: domain info is NULL!" && this->DomainInfo != NULL);

  vtk::detail::ImplicitNeighbor* neiPtr = NULL;
  size_t nNeis = this->InputGrid->Neighbors.size();
  for( size_t nei=0; nei < nNeis; ++nei )
  {
    neiPtr     = &(this->InputGrid->Neighbors)[nei];
    int orient = neiPtr->Orientation[ dim ];

    if( orient==vtk::detail::IntervalsConnect::IMPLICIT_HI ||
        orient==vtk::detail::IntervalsConnect::IMPLICIT_LO ||
        orient==vtk::detail::IntervalsConnect::UNDEFINED)
    {
      continue;
    }// END if implicit connectivity

    // Update neighbor extent
    if( neiPtr->Extent[dim*2+1] < this->DomainInfo->WholeExtent[dim*2+1] )
    {
      neiPtr->Extent[dim*2+1]++;
    } // END if update neighbor extent

    // Update overlap extent
    if( neiPtr->Overlap[dim*2+1] < this->DomainInfo->WholeExtent[dim*2+1] &&
        neiPtr->Overlap[dim*2+1]+1 <= neiPtr->Extent[dim*2+1])
    {
      neiPtr->Overlap[dim*2+1]++;
    } // END if update overlap


    assert("post: overlap extent out-of-bounds of output grid extent!" &&
      vtkStructuredExtent::Smaller(neiPtr->Overlap,this->OutputGrid->Extent));
  } // END for all neighbors
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::PackData(
      int ext[6], vtkMultiProcessStream& bytestream)
{
  // Sanity checks
  assert("pre: input grid is NULL!" && (this->InputGrid != NULL) );
  assert("pre: output grid is NULL!" && (this->OutputGrid != NULL) );
  assert("pre: extent is out-of-bounds the output grid!" &&
          vtkStructuredExtent::Smaller(ext,this->OutputGrid->Extent));

  bytestream.Push( ext, 6);

  if( this->OutputGrid->Nodes != NULL )
  {
    bytestream << VTK_STRUCTURED_GRID;
    vtkIdType nnodes = vtkStructuredData::GetNumberOfPoints(ext);
    bytestream << nnodes;

    int ijk[3] = {0,0,0};
    for( I(ijk)=IMIN(ext); I(ijk) <= IMAX(ext); ++I(ijk) )
    {
      for( J(ijk)=JMIN(ext); J(ijk) <= JMAX(ext); ++J(ijk) )
      {
        for( K(ijk)=KMIN(ext); K(ijk) <= KMAX(ext); ++K(ijk) )
        {
          vtkIdType idx = vtkStructuredData::ComputePointIdForExtent(
              this->OutputGrid->Extent,ijk,this->OutputGrid->DataDescription);
          bytestream.Push(this->OutputGrid->Nodes->GetPoint(idx),3);
        } // END for all k
      } // END for all j
    } // END for all i
  } // END if structured grid
  else if( this->OutputGrid->IsRectilinearGrid() )
  {
    bytestream << VTK_RECTILINEAR_GRID;
    vtkDataArray* coords[3];
    coords[0] = this->OutputGrid->X_Coords;
    coords[1] = this->OutputGrid->Y_Coords;
    coords[2] = this->OutputGrid->Z_Coords;
    for(int dim=0; dim < 3; ++dim)
    {
      assert("pre: NULL coordinates" && coords[dim] != NULL);
      int flag = -1;
      if( ext[dim*2] == ext[dim*2+1] )
      {
        flag = 1;
        bytestream << flag;
        bytestream << coords[ dim ]->GetTuple1(0);
      }
      else
      {
        bytestream << flag;
      }
    } // END for all dimensions
  } // END if rectilinear grid
  else
  {
    bytestream << VTK_UNIFORM_GRID;
  }

  // serialize the node-centered fields
  if(this->OutputGrid->PointData != NULL)
  {
    vtkFieldDataSerializer::SerializeSubExtent(
        ext,this->OutputGrid->Extent,this->OutputGrid->PointData,bytestream);
  }
  else
  {
    bytestream << 0;
  }
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::UnPackData(
      unsigned char* buffer, unsigned int size)
{
  assert("pre: output grid is NULL!" && (this->OutputGrid != NULL) );

  if( size == 0 )
  {
    return;
  }

  assert("pre: NULL buffer encountered!" && (buffer != NULL) );

  vtkMultiProcessStream bytestream;
  bytestream.SetRawData(buffer,size);

  int* ext = NULL;
  unsigned int sz = 0;
  bytestream.Pop(ext,sz);
  assert("post: ext size should be 6" && (sz==6) );
  assert("post: ext is out-of-bounds the output grid!" &&
          vtkStructuredExtent::Smaller(ext,this->OutputGrid->Extent));

  int datatype = -1;
  bytestream >> datatype;


  if( datatype == VTK_STRUCTURED_GRID )
  {
    int nnodes   = 0;
    bytestream >> nnodes;
    assert("pre: nnodes must be greater than 0!" && (nnodes > 0) );
    assert("post: output grid must have nodes!" &&
           (this->OutputGrid->Nodes != NULL) );

    int ijk[3]         = {0,0,0};
    double* pnt        = new double[3];
    unsigned int pntsz = 3;

    for( I(ijk)=IMIN(ext); I(ijk) <= IMAX(ext); ++I(ijk) )
    {
      for( J(ijk)=JMIN(ext); J(ijk) <= JMAX(ext); ++J(ijk) )
      {
        for( K(ijk)=KMIN(ext); K(ijk) <= KMAX(ext); ++K(ijk) )
        {
          vtkIdType idx = vtkStructuredData::ComputePointIdForExtent(
              this->OutputGrid->Extent,ijk,this->OutputGrid->DataDescription);
          assert("post: idx is out-of-bounds!" && (idx >= 0) &&
                  (idx < this->OutputGrid->Nodes->GetNumberOfPoints()) );

          bytestream.Pop(pnt,pntsz);
          assert("post: pntsz!=3" && (pntsz==3) );

          this->OutputGrid->Nodes->SetPoint(idx,pnt);
        } // END for all k
      } // END for all j
    } // END for all i

    delete [] pnt;
  } // END if structured
  else if( datatype == VTK_RECTILINEAR_GRID )
  {
    vtkDataArray* coords[3];
    coords[0] = this->OutputGrid->X_Coords;
    coords[1] = this->OutputGrid->Y_Coords;
    coords[2] = this->OutputGrid->Z_Coords;
    for(int dim=0; dim < 3; ++dim)
    {
      assert("pre: NULL coordinates" && coords[dim] != NULL);
      int flag = 0;
      bytestream >> flag;
      if( flag == 1 )
      {
        double coordinate;
        vtkIdType lastIdx = coords[ dim ]->GetNumberOfTuples()-1;
        bytestream >> coordinate;
        coords[ dim ]->SetTuple1(lastIdx, coordinate);
      }
    } // END for all dimensions
  } // END if rectilinear

  // de-serialize the node-centered fields
  if( this->OutputGrid->PointData != NULL )
  {
    vtkFieldDataSerializer::DeSerializeToSubExtent(
      ext,this->OutputGrid->Extent,this->OutputGrid->PointData,bytestream);
  }

  delete [] ext;
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::AllocateBuffers(const int dim)
{
  assert("pre: dimension index out-of-bounds!" && (dim >= 0) && (dim <= 2) );

  // Allocate CommBuffer data-structure
  if(this->CommManager == NULL)
  {
    this->CommManager = new vtk::detail::CommunicationManager();
  }

  // Clear previously calculated buffers, since we call this iteratively as
  // we carry out the communication along each dimension independently.
  this->CommManager->Clear();

  size_t nNeis = this->InputGrid->Neighbors.size();
  for(size_t nei=0; nei < nNeis; ++nei)
  {
    vtk::detail::ImplicitNeighbor* neiPtr= &(this->InputGrid->Neighbors)[nei];
    int orient = neiPtr->Orientation[ dim ];

    if( orient == vtk::detail::IntervalsConnect::IMPLICIT_HI )
    {
      // enqueue rcv from the rank of this neighbor
      this->CommManager->EnqueueRcv(neiPtr->Rank);
    } // END if
    else if( orient == vtk::detail::IntervalsConnect::IMPLICIT_LO)
    {
      // enqueue send to the rank of this neighbor
      vtkMultiProcessStream bytestream;
      this->PackData(neiPtr->Overlap,bytestream);

      unsigned char* buffer = NULL;
      unsigned int bytesize = 0;
      bytestream.GetRawData(buffer,bytesize);

      this->CommManager->EnqueueSend(neiPtr->Rank,buffer,bytesize);
    } // END else if
  } // END for all neighbors
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::GrowGrid(const int dim)
{
  assert("pre: dimension index out-of-bounds!" && (dim >= 0) && (dim <= 2) );
  assert("pre: input grid is NULL!" && this->InputGrid != NULL );

  // STEP 0: Allocate buffers & associated data-structures
  this->AllocateBuffers( dim );
  assert("pre: CommManager is NULL!" && (this->CommManager != NULL) );

  // STEP 1: Exchange data
  this->CommManager->Exchange(this->Controller);

  // STEP 4: Unpack data to output grid
  size_t nNeis = this->InputGrid->Neighbors.size();
  for( size_t nei=0; nei < nNeis; ++nei)
  {
    vtk::detail::ImplicitNeighbor* neiPtr= &(this->InputGrid->Neighbors)[nei];
    int orient  = neiPtr->Orientation[ dim ];
    int neiRank = neiPtr->Rank;

    if( orient == vtk::detail::IntervalsConnect::IMPLICIT_HI)
    {
      unsigned char* buffer = this->CommManager->GetRcvBuffer(neiRank);
      unsigned int   size   = this->CommManager->GetRcvBufferSize(neiRank);
      this->UnPackData(buffer,size);
    } // END if rcv'ed data

  } // END for all neighbors
}

//------------------------------------------------------------------------------
void vtkStructuredImplicitConnectivity::ExchangeData()
{
  // Sanity checks!
  assert( "pre: null controller!" && (this->Controller != NULL) );

  if(this->InputGrid != NULL)
  {
    // STEP 0: construct output grid data-structure
    this->ConstructOutput();

    // STEP 1: Process each dimension
    for(int d=0; d < this->DomainInfo->NDim; ++d)
    {
      int dim = this->DomainInfo->DimIndex[ d ];
      this->GrowGrid( dim );

      // STEP 2: Update neighbor list, w/ the grown grid information
      this->UpdateNeighborList( dim );
    } // END for all dimensions
  } // END if
  else
  {
    this->OutputGrid = NULL;
  } // END else

  // Barrier synchronization
  this->Controller->Barrier();
}
