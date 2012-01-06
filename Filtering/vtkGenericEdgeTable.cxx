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

#include <vector>
#include <assert.h>

vtkStandardNewMacro(vtkGenericEdgeTable);

static int PRIME_NUMBERS[] = {1, 3, 7, 13, 31, 61, 127,  251,  509,  1021,
                              2039, 4093};

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
  this->Scalar = new double[size];
  this->numberOfComponents = size;
}

class vtkEdgeTablePoints
{
public:
  typedef std::vector<vtkGenericEdgeTable::PointEntry> VectorPointTableType;
  typedef std::vector<VectorPointTableType> PointTableType;

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
    int index = static_cast<int>(log(static_cast<double>(newSize))/log(2.));
    this->Modulo = PRIME_NUMBERS[index];
    //cout << "this->Modulo:" << newSize << "," << index << ","
    //     << this->Modulo << endl;
    }

  assert(static_cast<unsigned>(size) < PointVector.size() ); // valid size
  // For now you are not supposed to use this method
  assert( 0 );
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
  typedef std::vector<vtkGenericEdgeTable::EdgeEntry> VectorEdgeTableType;
  typedef std::vector<VectorEdgeTableType> EdgeTableType;

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
    int index = static_cast<int>(log(static_cast<double>(newSize))/log(2.));
    this->Modulo = PRIME_NUMBERS[index];
    cout << "this->Modulo:" << index << ":" << this->Modulo << endl;
    }

  // For now you are not supposed to use this method
  assert(0);
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
static inline void OrderEdge(vtkIdType &e1, vtkIdType &e2)
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

  // Default to only one component
  this->NumberOfComponents = 1;
  
  // The whole problem is here to find the proper size for a descent hash table
  // Since we do not allow check our size as we go the hash table
  // Should be big enough from the begining otherwise we'll loose the
  // constant time access
  // But on the other hand we do not want it to be too big for mem consumption
  // A compromise of 4093 was found fo be working in a lot of case
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

  //Be carefull with reference the equal is not overloaded
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];
  
  //Need to check size again
  //Here we just puch_back at the end, 
  //the vector should not be very long and should not contain any empty spot.
  EdgeEntry newEntry;
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

//  vtkDebugMacro( << "Inserting Edge:" << e1 << "," << e2 << ": ref="  << 
//    newEntry.Reference << ", cellId=" << cellId << ", split=" << toSplit << 
//    ", ptId=" << newEntry.PtId );

  vect.push_back( newEntry );
}

//-----------------------------------------------------------------------------
// Try to remove an edge, in fact decrement the ref count
int vtkGenericEdgeTable::RemoveEdge(vtkIdType e1, vtkIdType e2)
{
  int ref = 0;
  //reorder so that e1 < e2;
  OrderEdge(e1, e2);

  //vtkDebugMacro( << "RemoveEdge:" << e1 << "," << e2 );

  vtkIdType pos = this->HashFunction(e1, e2);
  assert("check: valid range po" &&
         static_cast<unsigned>(pos) < this->EdgeTable->Vector.size() );

  //Need to check size first
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];

  int found = 0;

  vtkEdgeTableEdge::VectorEdgeTableType::iterator it;
  for(it = vect.begin(); it != vect.end(); )
  {
    if( it->E1 == e1 && it->E2 == e2 )
      {
      if( --it->Reference == 0 )
        {
        // Ok this edge is about to be physically removed, remove also the point
        // is contained, if one:
        if( it->ToSplit )
          {
          assert("check: positive id" && it->PtId >= 0 );

          this->RemovePoint( it->PtId );
          }
        }
      found = 1;
      ref = it->Reference;
      }

    if( it->E1 == e1 && it->E2 == e2 && it->Reference == 0 )
      {
      it = vect.erase( it );
      }
    else
      {
      ++it;
      }
  }

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
  
  if( static_cast<unsigned>(pos) >= this->EdgeTable->Vector.size() )
    {
    vtkDebugMacro( << "No entry were found in the hash table" );
    return -1;
    }
  
  assert("check: valid range pos" &&
         static_cast<unsigned>(pos)<this->EdgeTable->Vector.size() );
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
  int vectsize = static_cast<int>(vect.size());
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
int vtkGenericEdgeTable::IncrementEdgeReferenceCount(vtkIdType e1,
                                                     vtkIdType e2, 
                                                     vtkIdType cellId )
{
  int index;
  //reorder so that e1 < e2;
  OrderEdge(e1, e2);

  //vtkDebugMacro( << "IncrementEdgeReferenceCount:" << e1 << "," << e2  );

  vtkIdType pos = this->HashFunction(e1, e2);
  assert("check: valid range pos" &&
         static_cast<unsigned>(pos) < this->EdgeTable->Vector.size());

  //Need to check size first
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];
  
  int vectsize = static_cast<int>(vect.size());
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
  assert("check: valid range pos" &&
         static_cast<unsigned>(pos) < this->EdgeTable->Vector.size());

  //Need to check size first
  vtkEdgeTableEdge::VectorEdgeTableType &vect = this->EdgeTable->Vector[pos];

  int vectsize = static_cast<int>(vect.size());
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

//-----------------------------------------------------------------------------
vtkIdType vtkGenericEdgeTable::HashFunction(vtkIdType e1, vtkIdType e2)
{
  return (e1 + e2) % this->EdgeTable->Modulo;
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkGenericEdgeTable::Initialize(vtkIdType start)
{
  if(this->LastPointId)
    {
    //if different from zero then raise problem:
    vtkDebugMacro( << "You are not supposed to initialize during algorithm" );
    return;
    }

  this->LastPointId = start;
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
  return ptId % this->HashPoints->Modulo;
}
//-----------------------------------------------------------------------------
// Check if point ptId exist in the hash table
int vtkGenericEdgeTable::CheckPoint(vtkIdType ptId)
{
  int index;
  vtkIdType pos = this->HashFunction(ptId);

  if( static_cast<unsigned>(pos) >= this->HashPoints->PointVector.size() )
    {
    return 0;
    }
 
  assert("check: valid range pos" &&
         static_cast<unsigned>(pos)<this->HashPoints->PointVector.size() );
 
  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = 
    this->HashPoints->PointVector[pos];

  int vectsize = static_cast<int>(vect.size());
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
         static_cast<unsigned>(pos) < this->HashPoints->PointVector.size() );

  // Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = 
    this->HashPoints->PointVector[pos];

  //Need to check size again

  int vectsize = static_cast<int>(vect.size());
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
         static_cast<unsigned>(pos) < this->HashPoints->PointVector.size() );

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
  int found = 0;
  vtkIdType pos = this->HashFunction(ptId);

  //Need to check size first
  assert("check: valid range pos" && 
         static_cast<unsigned>(pos) < this->HashPoints->PointVector.size() );

  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = 
    this->HashPoints->PointVector[pos];

  //vtkDebugMacro( << "Remove Point:" << ptId << ":" << vect.size() );

  vtkEdgeTablePoints::VectorPointTableType::iterator it;
  for (it= vect.begin(); it!= vect.end(); ) 
    {
    PointEntry &ent = *it;

    if( ent.PointId == ptId )
      {
      --ent.Reference;
      found = 1;
      }

    if( ent.PointId == ptId && ent.Reference == 0 )
      {
      it = vect.erase(it);
      }
    else
      {
      ++it;
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
  if( !(static_cast<unsigned>(pos) < this->HashPoints->PointVector.size() ))
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

//  vtkDebugMacro( << "InsertPointAndScalar:" << ptId << ":" 
//    << pt[0] << "," << pt[1] << "," << pt[2] << "->" 
//    << s[0] << "," << s[1] << "," << s[2] );

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
  int found = 0;
  vtkIdType pos = this->HashFunction(ptId);

  //Need to check size first
  assert("check: valid range pos" &&
         static_cast<unsigned>(pos) < this->HashPoints->PointVector.size() );

  //Be carefull with reference the equal is not overloaded
  vtkEdgeTablePoints::VectorPointTableType &vect = this->HashPoints->PointVector[pos];

  //vtkDebugMacro(<< "IncrementPointReferenceCount:" << ptId << ":" << vect.size() );

  for (index=0; index<vect.size(); index++) 
    {
    PointEntry &ent = vect[index];
    if( ent.PointId == ptId )
      {
      ent.Reference++;
      found = 1;
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
  this->HashPoints->LoadFactor();
}
