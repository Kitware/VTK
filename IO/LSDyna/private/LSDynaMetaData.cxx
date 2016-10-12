/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LSDynaMetaData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "LSDynaMetaData.h"

//-----------------------------------------------------------------------------
LSDynaMetaData::LSDynaMetaData()
{
  this->FileIsValid = 0;
  this->Dimensionality=0;
  this->NumberOfNodes=0;
  this->FileSizeFactor = 7;
  this->MaxFileLength = this->FileSizeFactor*512*512*8;

  this->Title[0] = '\0';
  this->ReleaseNumber[0] = '\0';
  this->CodeVersion = 0.0;
  this->PreStateSize = 0;
  this->StateSize = 0;
  this->CurrentState = 0;
  this->ElementDeletionOffset = 0;
  this->SPHStateOffset = 0;

  std::vector<std::string> blankNames;
  std::vector<int> blankNumbers;
  for ( int cellType = 0; cellType < LSDynaMetaData::NUM_CELL_TYPES; ++cellType )
  {
    this->NumberOfCells[cellType] = 0;
    this->CellArrayNames[cellType] = blankNames;
    this->CellArrayComponents[cellType] = blankNumbers;
    this->CellArrayStatus[cellType] = blankNumbers;
  }
}

//-----------------------------------------------------------------------------
bool LSDynaMetaData::AddPointArray( std::string name, int numComponents, int status )
{
  for ( unsigned i = 0; i < this->PointArrayNames.size(); ++i )
  {
    if ( this->PointArrayNames[i] == name )
    {
      return false;
    }
  }
  this->PointArrayNames.push_back( name );
  this->PointArrayComponents.push_back( numComponents );
  this->PointArrayStatus.push_back( status );

  return true;
}

//-----------------------------------------------------------------------------
  bool LSDynaMetaData::AddCellArray( int cellType, std::string name, int numComponents, int status )
  {
  for ( unsigned i = 0; i < this->CellArrayNames[cellType].size(); ++i )
  {
    if ( this->CellArrayNames[cellType][i] == name )
    {
      return false;
    }
  }
  this->CellArrayNames[cellType].push_back( name );
  this->CellArrayComponents[cellType].push_back( numComponents );
  this->CellArrayStatus[cellType].push_back( status );

  return true;
  }

//-----------------------------------------------------------------------------
int LSDynaMetaData::GetTotalMaterialCount()
{
  return
    this->Dict["NUMMAT8"] + this->Dict["NUMMATT"] + this->Dict["NUMMAT4"] +
    this->Dict["NUMMAT2"] + this->Dict["NGPSPH"] + this->Dict["NSURF"];
  // Dict["NUMMAT"] is the subset of Dict["NUMMAT4"] materials that are rigid body materials
  // FIXME: Should NSURF be in here at all? I don't have any datasets w/ NSURF > 0, so I can't test.
}

//-----------------------------------------------------------------------------
void LSDynaMetaData::Reset()
{
  this->FileIsValid = 0;
  this->FileSizeFactor = 7;
  this->MaxFileLength = this->FileSizeFactor*512*512*8;

  this->Title[0] = '\0';
  this->ReleaseNumber[0] = '\0';
  this->CodeVersion = 0.0;
  this->PreStateSize = 0;
  this->StateSize = 0;
  this->CurrentState = 0;

  this->Dict.clear();
  this->Fam.Reset();

  this->PointArrayNames.clear();
  this->PointArrayComponents.clear();
  this->PointArrayStatus.clear();

  for ( int cellType = 0; cellType < LSDynaMetaData::NUM_CELL_TYPES; ++cellType )
  {
    this->CellArrayNames[cellType].clear();
    this->CellArrayComponents[cellType].clear();
    this->CellArrayStatus[cellType].clear();
  }

  this->PartNames.clear();
  this->PartIds.clear();
  this->PartMaterials.clear();
  this->PartStatus.clear();

  this->MaterialsOrdered.clear();
  this->MaterialsUnordered.clear();
  this->MaterialsLookup.clear();

  this->RigidSurfaceSegmentSizes.clear();
  this->TimeValues.clear();
}
