/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedGraphHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
// .NAME vtkDistributedGraphHelper.cxx - distributed graph helper for vtkGraph
//
// .SECTION Description
// Attach a subclass of this helper to a vtkGraph to turn it into a distributed graph
#include "vtkDistributedGraphHelper.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkStdString.h"
#include "vtkVariant.h"


#include <climits> // CHAR_BIT
#include <cassert> // assert()


vtkInformationKeyMacro(vtkDistributedGraphHelper, DISTRIBUTEDVERTEXIDS, Integer);
vtkInformationKeyMacro(vtkDistributedGraphHelper, DISTRIBUTEDEDGEIDS, Integer);

//----------------------------------------------------------------------------
// class vtkDistributedGraphHelper
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkDistributedGraphHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  int numProcs
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int myRank
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  os << indent << "Processor: " << myRank << " of " << numProcs << endl;
}

//----------------------------------------------------------------------------
vtkDistributedGraphHelper::vtkDistributedGraphHelper()
{
  this->Graph = 0;
  this->VertexDistribution = 0;
}

//----------------------------------------------------------------------------
vtkDistributedGraphHelper::~vtkDistributedGraphHelper()
{
}

//----------------------------------------------------------------------------
vtkIdType vtkDistributedGraphHelper::GetVertexOwner(vtkIdType v) const
{
  vtkIdType owner = v;
  int numProcs
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (numProcs > 1)
    {
    // An alternative to this obfuscated code is to provide
    // an 'unsigned' equivalent to vtkIdType.  Could then safely
    // do a logical right-shift of bits, e.g.:
    //   owner = (vtkIdTypeUnsigned) v >> this->indexBits;
    if (v & this->signBitMask)
      {
      owner ^= this->signBitMask;               // remove sign bit
      vtkIdType tmp = owner >> this->indexBits; // so can right-shift
      owner = tmp | this->highBitShiftMask;     // and append sign bit back
      }
    else
      {
      owner = v >> this->indexBits;
      }
    }
  else  // numProcs = 1
    {
    owner = 0;
    }

  return owner;
}

//----------------------------------------------------------------------------
vtkIdType vtkDistributedGraphHelper::GetVertexIndex(vtkIdType v) const
{
  vtkIdType index = v;
  int numProcs
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (numProcs > 1)
    {
    // Shift off the Owner bits.  (Would a mask be faster?)
    index = (v << this->procBits) >> this->procBits;
    }

  return index;
}

//----------------------------------------------------------------------------
vtkIdType vtkDistributedGraphHelper::GetEdgeOwner(vtkIdType e_id) const
{
  vtkIdType owner = e_id;
  int numProcs =
    this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (numProcs > 1)
    {
    if (e_id & this->signBitMask)
      {
      owner ^= this->signBitMask;               // remove sign bit
      vtkIdType tmp = owner >> this->indexBits; // so can right-shift
      owner = tmp | this->highBitShiftMask;     // and append sign bit back
      }
    else
      {
      owner = e_id >> this->indexBits;
      }
    }
  else  // numProcs = 1
    {
    owner = 0;
    }

  return owner;
}

//----------------------------------------------------------------------------
vtkIdType vtkDistributedGraphHelper::GetEdgeIndex(vtkIdType e_id) const
{
  vtkIdType index = e_id;
  int numProcs
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (numProcs > 1)
    {
    // Shift off the Owner bits.  (Would a mask be faster?)
    index = (e_id << this->procBits) >> this->procBits;
    }

  return index;
}

//----------------------------------------------------------------------------
vtkIdType vtkDistributedGraphHelper::MakeDistributedId(int owner, vtkIdType local)
{
  int numProcs
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  if (numProcs > 1)
    {
    assert(owner >= 0 && owner < numProcs);
    return (static_cast<vtkIdType>(owner) << this->indexBits) | local;
    }

  return local;
}

//----------------------------------------------------------------------------
void vtkDistributedGraphHelper::AttachToGraph(vtkGraph *graph)
{
  this->Graph = graph;

  // Some factors and masks to help speed up encoding/decoding {owner,index}
  int numProcs
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int tmp = numProcs - 1;
  // The following is integer arith equiv of ceil(log2(numProcs)):
  int numProcBits = 0;
  while( tmp != 0 )
    {
    tmp >>= 1;
    numProcBits++;
    }
  if (numProcs == 1)  numProcBits = 1;

  this->signBitMask = VTK_ID_MIN;
  this->highBitShiftMask = static_cast<vtkIdType>(1) << numProcBits;
  this->procBits = numProcBits + 1;
  this->indexBits = (sizeof(vtkIdType) * CHAR_BIT) - (numProcBits + 1);
}

//----------------------------------------------------------------------------
void
vtkDistributedGraphHelper::
SetVertexPedigreeIdDistribution(vtkVertexPedigreeIdDistribution Func,
                                void *userData)
{
  this->VertexDistribution = Func;
  this->VertexDistributionUserData = userData;
}

//----------------------------------------------------------------------------
vtkIdType
vtkDistributedGraphHelper::
GetVertexOwnerByPedigreeId(const vtkVariant& pedigreeId)
{
  vtkIdType numProcs
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  if (this->VertexDistribution)
    {
    return (this->VertexDistribution(pedigreeId,
                                     this->VertexDistributionUserData)
            % numProcs);
    }

  // Hash the variant in a very lame way.
  double numericValue;
  vtkStdString stringValue;
  const unsigned char *charsStart, *charsEnd;
  if (pedigreeId.IsNumeric())
    {
    // Convert every numeric value into a double.
    numericValue = pedigreeId.ToDouble();

    // Hash the characters in the double.
    charsStart = reinterpret_cast<const unsigned char*>(&numericValue);
    charsEnd = charsStart + sizeof(double);
    }
  else if (pedigreeId.GetType() == VTK_STRING)
    {
    stringValue = pedigreeId.ToString();
    charsStart = reinterpret_cast<const unsigned char*>(stringValue.c_str());
    charsEnd = charsStart + stringValue.size();
    }
  else
    {
    vtkErrorMacro("Cannot hash vertex pedigree ID of type "
                  << pedigreeId.GetType());
    return 0;
    }

  unsigned long hash = 5381;
  for (; charsStart != charsEnd; ++charsStart)
    {
    hash = ((hash << 5) + hash) ^ *charsStart;
    }

  return hash % numProcs;
}
