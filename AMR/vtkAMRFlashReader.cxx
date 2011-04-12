/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRFlashReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRFlashReader.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkAMRUtilities.h"

vtkStandardNewMacro(vtkAMRFlashReader);

//------------------------------------------------------------------------------
vtkAMRFlashReader::vtkAMRFlashReader()
{
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkAMRFlashReader::~vtkAMRFlashReader()
{

}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::SetFileName( const char* fileName )
{
  // TODO: implement this
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::ReadMetaData()
{
  // TODO: implement this
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::GenerateBlockMap()
{
  // TODO: implement this
}

//-----------------------------------------------------------------------------
int vtkAMRFlashReader::GetBlockLevel( const int blockIdx )
{
  // TODO: implement this
  return 0;
}

//-----------------------------------------------------------------------------
int vtkAMRFlashReader::GetNumberOfBlocks()
{
  // TODO: implement this
  return 0;
}

//-----------------------------------------------------------------------------
int vtkAMRFlashReader::GetNumberOfLevels()
{
  // TODO: implement this
  return 0;
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::GetBlock(
    int index, vtkHierarchicalBoxDataSet *hbds,
    vtkstd::vector< int > &idxcounter)
{
  // TODO: implement this
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::SetUpDataArraySelections()
{
  // TODO: implement this
}
