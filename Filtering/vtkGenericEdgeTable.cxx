/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericEdgeTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericEdgeTable.h"
#include "vtkObjectFactory.h"

#include <vtkstd/vector>
#include <assert.h>

typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;   /* unsigned 1-byte quantities */

//                            2  4  8  16  32, 64, 128,  256,  512,  1024,
//                            2048  4096
static int PRIME_NUMBERS[] = {1, 3, 7, 13, 31, 61, 127,  251,  509,  1021,
                              2039, 4093};
//  53,         97,         193,       389,       769,
//  1543,       3079,       6151,      12289,     24593,
//  49157,      98317,      196613,    393241,    786433,
//  1572869,    3145739,    6291469,   12582917,  25165843,


vtkCxxRevisionMacro(vtkGenericEdgeTable, "1.4");
vtkStandardNewMacro(vtkGenericEdgeTable);


// Description:
// Constructor with a scalar field of `size' doubles.
// \pre positive_number_of_components: size>0
vtkGenericEdgeTable::PointEntry::PointEntry(int size)
{
  assert("pre: positive_number_of_components" && size>0);
  this->Reference = -10;
  
  this->Coord[0]  = -100;
  this->Coord[1]  = -100;
  this->Coord[2]  = -100;
  this->Scalar=new double[size];
  this->numberOfComponents=size;
}

class vtkEdgeTablePoints
{
public:
  typedef vtkstd::vector<vtkGenericEdgeTable::PointEntry> VectorPointTableType;
  typedef vtkstd::vector<VectorPointTableType> PointTableType;

  void Resize(vtkIdType size);
  void LoadFactor();
  void DumpPoints();

  PointTableType PointVector;
  vtkIdType Modulo;
};

//-----------------------------------------------------------------------------
void vtkEdgeTablePoints::Resize(vtkIdType newSize)
{
  vtkIdType size = PointVector.size();

  if( size <= newSize )
    {
    PointVector.resize(newSize);
    int index = (int)(log((double)newSize)/log(2.));
    this->Modulo = PRIME_NUMBERS[index];
    cout << "this->Modulo:" << newSize << "," << index << ","
         << this->Modulo << endl;
    }

  assert((unsigned)size < PointVector.size() ); // valid size
  assert( 0 ); // TODO
}
//-----------------------------------------------------------------------------
void vtkEdgeTablePoints::LoadFactor()
{
  vtkIdType numEntries = 0;
  vtkIdType numBins = 0;

  vtkIdType size = PointVector.size();
  cerr << "EdgeTablePoints:\n";
  for(int i=0; i<size; i++)
    {
    numEntries += PointVector[i].size();
    if( PointVector[i].size() ) numBins++;
    cerr << PointVector[i].size() << ",";
    }
  cerr << "\n";
  cout << size << "," << numEntries << "," << numBins << "," << Modulo
            << "\n";
}

//-----------------------------------------------------------------------------
void vtkEdgeTablePoints::DumpPoints()
{
  vtkIdType size = PointVector.size();
  for(int i=0; i<size; i++)
    {
    VectorPointTableType v = PointVector[i];
    for (VectorPointTableType::iterator it = v.begin(); it!=v.end(); ++it) 
      {
      //PointEntry
      cout << "PointEntry: " << it->PointId << " " << it->Reference << ":(" <<
      it->Coord[0] << "," << it->Coord[1] << "," << it->Coord[2] << ")"
           << endl;
      }
    }
}

//-----------------------------------------------------------------------------
class vtkEdgeTableEdge
{
public:
  typedef vtkstd::vector<vtkGenericEdgeTable::EdgeEntry> VectorEdgeTableType;
  typedef vtkstd::vector<VectorEdgeTableType> EdgeTableType;

  void Resize(vtkIdType size);
  void LoadFactor();
  void DumpEdges();

  EdgeTableType Vector;
  vtkIdType Modulo;
};

//-----------------------------------------------------------------------------
void vtkEdgeTableEdge::Resize(vtkIdType newSize)
{
  vtkIdType size = Vector.size();

  if( size <= newSize )
    {
    Vector.resize(newSize);
    int index = (int)(log((double)newSize)/log(2.));
    this->Modulo = PRIME_NUMBERS[index];
    cout << "this->Modulo:" << index << ":" << this->Modulo << endl;
    }

  assert(0); // TODO
}

//-----------------------------------------------------------------------------
void vtkEdgeTableEdge::LoadFactor()
{
  vtkIdType numEntry = 0;
  vtkIdType numBins = 0;

  vtkIdType size = Vector.size();
  cerr << "EdgeTableEdge:\n";
  for(int i=0; i<size; i++)
    {
    VectorEdgeTableType v = Vector[i];
    numEntry += v.size();
    if(v.size()) numBins++;
    }
  cerr << "\n";
  cerr << size << "," << numEntry << "," << numBins << "," << Modulo
            << "\n";
}

//-----------------------------------------------------------------------------
void vtkEdgeTableEdge::DumpEdges()
{
  vtkIdType size = Vector.size();
  for(int i=0; i<size; i++)
    {
    VectorEdgeTableType v = Vector[i];
    for (VectorEdgeTableType::iterator it = v.begin(); it!=v.end(); ++it) 
      {
      //EdgeEntry
      cout << "EdgeEntry: (" << it->E1 << "," << it->E2 << ") " << 
      it->Reference << "," << it->ToSplit << "," << it->PtId <<  endl;
      }
    }
}

//-----------------------------------------------------------------------------
static void OrderEdge(vtkIdType &e1, vtkIdType &e2)
{
  vtkIdType temp1 = e1;
  vtkIdType temp2 = e2;
  e1 = temp1 < temp2 ? temp1 : temp2;
  e2 = temp1 > temp2 ? temp1 : temp2;
}

//-----------------------------------------------------------------------------
// Instantiate object based on maximum point id.
vtkGenericEdgeTable::vtkGenericEdgeTable()
{
  this->EdgeTable = new vtkEdgeTableEdge;
  this->HashPoints = new vtkEdgeTablePoints;

  this->NumberOfComponents=1;
  
#if 1
  this->EdgeTable->Vector.resize(4093);
  this->EdgeTable->Modulo = 4093;
  this->HashPoints->PointVector.resize(4093);  //127
  this->HashPoints->Modulo = 4093;
#else
  this->EdgeTable->Vector.resize(509);
  this->EdgeTable->Modulo = 509;
  this->HashPoints->PointVector.resize(127);
  this->HashPoints->Modulo = 127;
#endif

  this->LastPointId = 0;
}

//-----------------------------------------------------------------------------
vtkGenericEdgeTable::~vtkGenericEdgeTable()
{
  //this->EdgeTable->LoadFactor();
  delete this->EdgeTable;
  delete this->HashPoints;
}

//-----------------------------------------------------------------------------
// We assume the edge is not being split:
void vtkGenericEdgeTable::InsertEdge(vtkIdType e1, vtkIdType e2,
                                     vtkIdType cellId, int ref )
{
  vtkIdType ptId;
  this->InsertEdge(e1, e2, cellId, ref, 0, ptId);
}

//-----------------------------------------------------------------------------
// the edge is being split and we want the new ptId
void vtkGenericEdgeTable::InsertEdge(vtkIdType e1, vtkIdType e2,
                                     vtkIdType cellId, int ref,
                                     vtkIdType &ptId )
{
  this->InsertEdge(e1, e2, cellId, ref, 1, ptId );
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::InsertEdge(vtkIdType e1, vtkIdType e2,
                                     vtkIdType cellId, int ref, int toSplit,
                                     vtkIdType &ptId )
{
  if( e1 == e2 )
    {
    vtkErrorMacro( << "Not an edge:" << e1 << "," << e2 );
    }
  assert("pre: not degenerated edge" && e1!=e2);
  
  //reorder so that e1 < e2;
  OrderEdge(e1, e2);

  vtkIdType pos = this->HashFunction(e1, e2);

  //Need to check size first
  //this->EdgeTable->Resize( pos );

//  if( !(pos < this->EdgeTable->Vector.size() ) )
//    {
//    int kk = 0;
//    kk++;
//    }
  
  //Be carefull with reference the equal is not overloaded
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];
  
  //Need to check size again
  //Here we just puch_back at the end, 
  //the vector should not be very long and should not contain any empty spot.
  EdgeEntry newEntry; // = vect.back();
  newEntry.E1 = e1;
  newEntry.E2 = e2;
  newEntry.Reference = ref;
  newEntry.ToSplit = toSplit;
  newEntry.CellId = cellId;

  if(newEntry.ToSplit) 
    {
    newEntry.PtId = ptId = this->LastPointId++;
    }
  else
    {
    newEntry.PtId = ptId = -1;
    }

  vtkDebugMacro( << "Inserting Edge:" << e1 << "," << e2 << ": ref="  << 
    newEntry.Reference << ", cellId=" << cellId << ", split=" << toSplit << 
    ", ptId=" << newEntry.PtId );

  vect.push_back( newEntry );
}

#define USE_CONST_ITERATOR 1

//-----------------------------------------------------------------------------
// Try to remove an edge, in fact decrement the ref count
int vtkGenericEdgeTable::RemoveEdge(vtkIdType e1, vtkIdType e2)
{
  int ref = 0;
  //reorder so that e1 < e2;
  OrderEdge(e1, e2);

  vtkDebugMacro( << "RemoveEdge:" << e1 << "," << e2 );

  vtkIdType pos = this->HashFunction(e1, e2);
  assert("check: valid range po" && (unsigned)pos < this->EdgeTable->Vector.size() );

  //Need to check size first
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];
  
  //Need to check size again
  //v.erase(v.begin() + index);
  bool found = false;
  
#if !USE_CONST_ITERATOR
  vtkEdgeTableEdge::VectorEdgeTableType::iterator it;
  for(it = vect.begin(); it != vect.end(); ++it)
  {
    //EdgeEntry &ent = *it;
    if( it->E1 == e1 && it->E2 == e2 )
      {
      if( --it->Reference == 0 )
        {
        // Ok this edge is about to be physically remove, remove also the point
        // is contained, if one:
        if( it->ToSplit )
          {
          assert("check: positive id" && it->PtId >= 0 );

          this->RemovePoint( it->PtId );
          }
        //vect.erase(vect.begin() + index);
        vect.erase( it );
        }
      found = true;
      ref = it->Reference;
      }
  }
#else
  int vectsize = vect.size();
  for (int index=0; index<vectsize; index++) 
    {
    EdgeEntry &ent = vect[index];
    if( (ent.E1 == e1) && (ent.E2 == e2))
      {
      if(--ent.Reference == 0) 
        {
        // Ok this edge is about to be physically remove, remove also the point
        // is contained, if one:
        if(ent.ToSplit)
          {
          assert("check: positive id" && ent.PtId >= 0 );

          this->RemovePoint(ent.PtId);
          }
        vect.erase(vect.begin() + index);
        }
      found = true;
      ref = ent.Reference;
      }
    }
#endif

  if( !found )
    {
    //We did not find any corresponding entry to erase, warn user
    vtkErrorMacro( << "No entry were found in the hash table for edge:" << e1 
      << "," << e2  );
    assert("check: not found" &&  0 );
    }

  return ref;
}

//-----------------------------------------------------------------------------
int vtkGenericEdgeTable::CheckEdge(vtkIdType e1, vtkIdType e2, vtkIdType &ptId)
{
  //int index;
  EdgeEntry ent;
  //reorder so that e1 < e2;
  OrderEdge(e1, e2);

  //vtkDebugMacro( << "Checking edge:" << e1 << "," << e2  );

  vtkIdType pos = this->HashFunction(e1, e2);
  
  if( (unsigned)pos >= this->EdgeTable->Vector.size() )
    {
    vtkDebugMacro( << "No entry were found in the hash table" );
    return -1;
    }
  
  assert("check: valid range pos" && (unsigned)pos<this->EdgeTable->Vector.size() );
  //Need to check size first
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];
  
#if USE_CONST_ITERATOR
  vtkEdgeTableEdge::VectorEdgeTableType::const_iterator it;
  for(it = vect.begin(); it != vect.end(); ++it)
    {
    if( (it->E1 == e1) && (it->E2 == e2))
      {
      ptId = it->PtId;
      break;
      }
    }

  if( it == vect.end())
    {
    //We did not find any corresponding entry, warn user
    vtkDebugMacro( << "No entry were found in the hash table" );
    return -1;
    }

  return it->ToSplit;
#else
  int vectsize = vect.size();
  int index;
  for (index=0; index<vectsize; index++) 
    {
    ent = vect[index];
    if( ent.E1 == e1 && ent.E2 == e2 )
      {
      ptId = ent.PtId;
      break;
      }
    }

  if( index == vectsize )
    {
    //We did not find any corresponding entry, warn user
    vtkDebugMacro( << "No entry were found in the hash table" );
    return -1;
    }

  return ent.ToSplit;
#endif

}

//-----------------------------------------------------------------------------
int vtkGenericEdgeTable::IncrementEdgeReferenceCount(vtkIdType e1, vtkIdType e2, 
                                               vtkIdType cellId )
{
  int index;
  //reorder so that e1 < e2;
  OrderEdge(e1, e2);

  //vtkDebugMacro( << "IncrementEdgeReferenceCount:" << e1 << "," << e2  );

  vtkIdType pos = this->HashFunction(e1, e2);
  assert("check: valid range pos" && (unsigned)pos < this->EdgeTable->Vector.size());

  //Need to check size first
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];
  
  int vectsize = vect.size();
  for (index=0; index<vectsize; index++) 
    {
    EdgeEntry &ent = vect[index];
    if( (ent.E1 == e1) && (ent.E2 == e2))
      {
      if( ent.CellId == cellId )
        {
        ent.Reference++;
        //vtkDebugMacro( << "Incrementing: (" << e1 << "," << e2 << ") " << ent.Reference );
        }
      else
        {
        // If cellIds are different it means we pass from one cell to another
        // therefore the first time we should not increment the ref count
        // as it has already been taken into account.
        ent.CellId = cellId;
        }
      return -1;
      }
    }

    //We did not find any corresponding entry, warn user
    vtkErrorMacro( << "No entry were found in the hash table" );
    return -1;
}

//-----------------------------------------------------------------------------
int vtkGenericEdgeTable::CheckEdgeReferenceCount(vtkIdType e1, vtkIdType e2)
{
  int index;
  OrderEdge(e1, e2);   //reorder so that e1 < e2;

  //vtkDebugMacro( << "CheckEdgeReferenceCount:" << e1 << "," << e2  );

  vtkIdType pos = this->HashFunction(e1, e2);
  assert("check: valid range pos" && (unsigned)pos < this->EdgeTable->Vector.size());

  //Need to check size first
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];
  
  int vectsize = vect.size();
  for (index=0; index<vectsize; index++) 
    {
    EdgeEntry &ent = vect[index];
    if( (ent.E1 == e1) && (ent.E2 == e2))
      {
      assert("check: positive reference" &&  ent.Reference >= 0 );
      return ent.Reference;
      }
    }

  //We did not find any corresponding entry, warn user
  vtkErrorMacro( << "No entry were found in the hash table" );
  return -1;
}

ub4 hash( ub1 *k, ub4 length, ub4 initval);
//-----------------------------------------------------------------------------
vtkIdType vtkGenericEdgeTable::HashFunction(vtkIdType e1, vtkIdType e2)
{
  return (e1 + e2) % this->EdgeTable->Modulo;//~6.3s in release

  //return ((unsigned)(e1 << 16 | e1 >> 16) ^ e2) % this->EdgeTable->Modulo; //~6.4s in release
  //return ((unsigned)e1 ^ (unsigned)e2) % this->EdgeTable->Modulo;//~7.2s in release
  
  //vtkIdType temp = (e1 ^ e2);
  //return ((unsigned)(temp << 16  | temp >> 16)) % this->EdgeTable->Modulo;//~7.2s in release
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

//  os << indent << "NumberOfEdges: " << this->GetNumberOfEdges() << "\n";
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::Initialize(vtkIdType start)
{
  if(this->LastPointId)
    {
    //if different from zero then raise problem:
    //vtkErrorMacro( << "You are not supposed to initialize during algorithm" );
    return;
    }

  this->LastPointId = start;
}

//-----------------------------------------------------------------------------
// Description:
// Return the last point id inserted.
vtkIdType vtkGenericEdgeTable::GetLastPointId()
{
  return this->LastPointId;
}
  
//-----------------------------------------------------------------------------
// Description:
// Increment the last point id.
void vtkGenericEdgeTable::IncrementLastPointId()
{
  ++this->LastPointId;
}

//-----------------------------------------------------------------------------
// Description:
// Return the total number of components for the point-centered attributes.
// \post positive_result: result>0
int vtkGenericEdgeTable::GetNumberOfComponents()
{
  return this->NumberOfComponents;
}

//-----------------------------------------------------------------------------
// Description:
// Set the total number of components for the point-centered attributes.
void vtkGenericEdgeTable::SetNumberOfComponents(int count)
{
  assert("pre: positive_count" && count>0);
  this->NumberOfComponents=count;
}

//-----------------------------------------------------------------------------
vtkIdType vtkGenericEdgeTable::HashFunction(vtkIdType ptId)
{
  //cout << ptId << "," << this->HashPoints->Modulo << "\n";
  return ptId % this->HashPoints->Modulo;
  //return ( ptId << 16  | ptId >> 16 ) % this->HashPoints->Modulo;
}
//-----------------------------------------------------------------------------
// Check if point ptId exist in the hash table
int vtkGenericEdgeTable::CheckPoint(vtkIdType ptId)
{
  int index;
  vtkIdType pos = this->HashFunction(ptId);

  //Need to check size first
  //this->HashPoints->Resize( pos );
  
  if( (unsigned)pos >= this->HashPoints->PointVector.size() )
    {
    return 0;
    }
  
  assert("check: valid range pos" &&
           (unsigned)pos<this->HashPoints->PointVector.size() );
  
  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = 
    this->HashPoints->PointVector[pos];

  int vectsize = vect.size();
  for (index=0; index<vectsize; index++) 
    {
    if( vect[index].PointId == ptId )
      {
      return 1;
      }
    }

  if( index == vectsize )
    {
    //We did not find any corresponding entry
    return 0;
    }
  
  vtkErrorMacro( << "Error, impossible case" );
  return -1;
}

//-----------------------------------------------------------------------------
// Find point coordinate and scalar associated of point ptId
//
int vtkGenericEdgeTable::CheckPoint(vtkIdType ptId, double point[3],
                                    double *scalar)
{
  // pre: sizeof(scalar)==GetNumberOfComponents()
  int index;
  vtkIdType pos = this->HashFunction(ptId);
  assert("check: valid range pos" &&
           (unsigned)pos < this->HashPoints->PointVector.size() );

  // Need to check size first
  // this->HashPoints->Resize( pos );
//  if( pos > this->HashPoints->PointVector.size() )
//    {
//    vtkErrorMacro( << "No entry were found in the hash table" );
//    return 0;
//    }

  // Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = 
    this->HashPoints->PointVector[pos];

  //Need to check size again

  int vectsize = vect.size();
  for (index=0; index<vectsize; index++) 
    {
    PointEntry &ent = vect[index];
    if( ent.PointId == ptId )
      {
      memcpy(point,ent.Coord,sizeof(double)*3);
      memcpy(scalar,ent.Scalar,sizeof(double)*this->NumberOfComponents);
      return 1;
      }
    }

  if( index == vectsize )
    {
    //We did not find any corresponding entry, warn user
    vtkErrorMacro( << "No entry were found in the hash table:" << ptId );
    return 0;
    }

  assert("check: TODO" && 0 );
  return 1;
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::InsertPoint(vtkIdType ptId, double point[3])
{
  vtkIdType pos = this->HashFunction(ptId);

  //Need to check size first
  //this->HashPoints->Resize( pos );
  assert("check: valid range pos" && 
           (unsigned)pos < this->HashPoints->PointVector.size() );

  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = 
    this->HashPoints->PointVector[pos];

  //Need to check size again
  //Here we just puch_back at the end, 
  //the vector should not be very long and should not contain any empty spot.
  PointEntry newEntry(this->NumberOfComponents); // = vect.back();
  newEntry.PointId = ptId;
  memcpy(newEntry.Coord,point,sizeof(double)*3);
  newEntry.Reference = 1;

//  vtkDebugMacro( << "Inserting Point:" << ptId << ":" 
//    << point[0] << "," << point[1] << "," << point[2] );
  vect.push_back( newEntry );
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::RemovePoint(vtkIdType ptId)
{
  unsigned int index;
  bool found = false;
  vtkIdType pos = this->HashFunction(ptId);

  //Need to check size first
  //this->HashPoints->Resize( pos );
  assert("check: valid range pos" && 
           (unsigned)pos < this->HashPoints->PointVector.size() );

  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = 
    this->HashPoints->PointVector[pos];

  //vtkDebugMacro( << "Remove Point:" << ptId << ":" << vect.size() );

  for (index=0; index<vect.size(); index++) 
    {
    PointEntry &ent = vect[index];
    if( ent.PointId == ptId )
      {
      if(--ent.Reference == 0 )
        {
        vect.erase(vect.begin() + index);
        }
      found = true;
      }
    }

  if( !found )
    {
    //We did not find any corresponding entry, warn user
    vtkErrorMacro( << "No entry were found in the hash table" );
    }
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::InsertPointAndScalar(vtkIdType ptId, double pt[3],
                                               double *s)
{
  // sizeof(s)=this->NumberOfComponents
  vtkIdType pos = this->HashFunction(ptId);

  //Need to check size first
  //this->HashPoints->Resize( pos );
  if( !((unsigned)pos < this->HashPoints->PointVector.size() ))
    {
    int kk = 2;
    kk++;
    }

  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = this->HashPoints->PointVector[pos];

  //Please keep the following:
#if 0
  //Need to check size again
  //Here we just puch_back at the end, 
  //the vector should not be very long and should not contain any empty spot.
  for (index=0; index<vect.size(); index++) 
    {
    PointEntry ent = vect[index];
    if( ent.PointId == ptId )
      {
      //The point was already in the table:
      assert("check: same id" &&  ent.PointId == ptId );
      assert("check: same x" && ent.Coord[0] == pt[0]);
      assert("check: same y" && ent.Coord[1] == pt[1]);
      assert("check: same z" && ent.Coord[2] == pt[2]);
      assert("check: same x scalar" && ent.Scalar[0] == s[0]);
      assert("check: same y scalar" && ent.Scalar[1] == s[1]);
      assert("check: same z scalar" && ent.Scalar[2] == s[2]);
      
      vtkErrorMacro( << "The point was already in the table" ); //FIXME
      
      return;
      }
    }
#endif

  //We did not find any matching point thus insert it
  
  PointEntry newEntry(this->NumberOfComponents); // = vect.back();
  newEntry.PointId = ptId;
  memcpy(newEntry.Coord,pt,sizeof(double)*3);
  memcpy(newEntry.Scalar,s,sizeof(double)*this->NumberOfComponents);
  newEntry.Reference = 1;

  vtkDebugMacro( << "InsertPointAndScalar:" << ptId << ":" 
    << pt[0] << "," << pt[1] << "," << pt[2] << "->" 
    << s[0] << "," << s[1] << "," << s[2] );

  vect.push_back( newEntry );
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::DumpTable()
{
  this->EdgeTable->DumpEdges();
  this->HashPoints->DumpPoints();
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::IncrementPointReferenceCount(vtkIdType ptId )
{
  unsigned int index;
  bool found = false;
  vtkIdType pos = this->HashFunction(ptId);

  //Need to check size first
  //this->HashPoints->Resize( pos );
  //vtkDebugMacro( << "IncrementPointReferenceCount:" << pos << ":" << this->HashPoints->PointVector.size() );

  assert("check: valid range pos" &&
         (unsigned)pos < this->HashPoints->PointVector.size() );

  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = this->HashPoints->PointVector[pos];

  //vtkDebugMacro(<< "IncrementPointReferenceCount:" << ptId << ":" << vect.size() );

  for (index=0; index<vect.size(); index++) 
    {
    PointEntry &ent = vect[index];
    if( ent.PointId == ptId )
      {
      ent.Reference++;
      found = true;
      }
    }

  if( !found )
    {
    //We did not find any corresponding entry, warn user
    vtkErrorMacro( << "No entry were found in the hash table" );
    }
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::LoadFactor()
{
  vtkDebugMacro( << "------ Begin LoadFactor ------- " );
  this->EdgeTable->LoadFactor();
  //this->HashPoints->LoadFactor();
}


/*
http://www.utm.edu/research/primes/lists/small/1000.txt

2      3      5      7     11     13     17     19     23     29 
     31     37     41     43     47     53     59     61     67     71 
     73     79     83     89     97    101    103    107    109    113 
    127    131    137    139    149    151    157    163    167    173 
    179    181    191    193    197    199    211    223    227    229 
    233    239    241    251    257    263    269    271    277    281 
    283    293    307    311    313    317    331    337    347    349 
    353    359    367    373    379    383    389    397    401    409 
    419    421    431    433    439    443    449    457    461    463 
    467    479    487    491    499    503    509    521    523    541 
    547    557    563    569    571    577    587    593    599    601 
    607    613    617    619    631    641    643    647    653    659 
    661    673    677    683    691    701    709    719    727    733 
    739    743    751    757    761    769    773    787    797    809 
    811    821    823    827    829    839    853    857    859    863 
    877    881    883    887    907    911    919    929    937    941 
    947    953    967    971    977    983    991    997   1009   1013 
   1019   1021
*/




#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

/*
--------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.
For every delta with one or two bits set, and the deltas of all three
  high bits or all three low bits, whether the original value of a,b,c
  is almost all zero or is uniformly distributed,
* If mix() is run forward or backward, at least 32 bits in a,b,c
  have at least 1/4 probability of changing.
* If mix() is run forward, every bit of c will change between 1/3 and
  2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
mix() was built out of 36 single-cycle latency instructions in a 
  structure that could supported 2x parallelism, like so:
      a -= b; 
      a -= c; x = (c>>13);
      b -= c; a ^= x;
      b -= a; x = (a<<8);
      c -= a; b ^= x;
      c -= b; x = (b>>13);
      ...
  Unfortunately, superscalar Pentiums and Sparcs can't take advantage 
  of that parallelism.  They've also turned some of those single-cycle
  latency instructions into multi-cycle latency instructions.  Still,
  this is the fastest good hash I could find.  There were about 2^^68
  to choose from.  I only looked at a billion or so.
--------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/*
--------------------------------------------------------------------
hash() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  len     : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 6*len+35 instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------
*/

ub4 hash( register ub1 *k, register ub4 length, register ub4 initval)
//register ub1 *k;        /* the key */
//register ub4  length;   /* the length of the key */
//register ub4  initval;  /* the previous hash, or an arbitrary value */
{
   register ub4 a,b,c,len;

   /* Set up the internal state */
   len = length;
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   c = initval;         /* the previous hash value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((ub4)k[1]<<8) +((ub4)k[2]<<16) +((ub4)k[3]<<24));
      b += (k[4] +((ub4)k[5]<<8) +((ub4)k[6]<<16) +((ub4)k[7]<<24));
      c += (k[8] +((ub4)k[9]<<8) +((ub4)k[10]<<16)+((ub4)k[11]<<24));
      mix(a,b,c);
      k += 12; len -= 12;
   }

   /*------------------------------------- handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((ub4)k[10]<<24);
   case 10: c+=((ub4)k[9]<<16);
   case 9 : c+=((ub4)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((ub4)k[7]<<24);
   case 7 : b+=((ub4)k[6]<<16);
   case 6 : b+=((ub4)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((ub4)k[3]<<24);
   case 3 : a+=((ub4)k[2]<<16);
   case 2 : a+=((ub4)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}

