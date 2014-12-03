/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRFlashReaderInternal.hpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRFlashReaderInternal.hpp -- Low-level Flash Reader
//
// .SECTION Description
//  Consists of the low-level Flash Reader used by the vtkAMRFlashReader.
//
// .SECTION See Also
//  vtkAMRFlashReader

#ifndef VTKAMRFLASHREADERINTERNAL_HPP_
#define VTKAMRFLASHREADERINTERNAL_HPP_

#include <cassert>
#include <vector>
#include <map>
#include <cstring>
#include <string>

#include "vtkSetGet.h"
#include "vtkDataSet.h"
#include "vtkObject.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCellData.h"
#include "vtkByteSwap.h"

#define H5_USE_16_API
#include "vtk_hdf5.h"

//==============================================================================
//            I N T E R N A L   F L A S H     R E A D E R
//==============================================================================

#define  FLASH_READER_MAX_DIMS     3
#define  FLASH_READER_LEAF_BLOCK   1
#define  FLASH_READER_FLASH3_FFV8  8
#define  FLASH_READER_FLASH3_FFV9  9

typedef  struct tagFlashReaderIntegerScalar
{
  char   Name[20];                // name  of the integer scalar
  int    Value;                   // value of the integer scalar
} FlashReaderIntegerScalar;

typedef  struct tagFlashReaderDoubleScalar
{
  char   Name[20];                // name  of the real scalar
  double Value;                   // value of the real scalar
} FlashReaderDoubleScalar;

typedef  struct tagFlashReaderSimulationParameters
{
  int    NumberOfBlocks;          // number of all blocks
  int    NumberOfTimeSteps;       // number of time steps
  int    NumberOfXDivisions;      // number of divisions per block along x axis
  int    NumberOfYDivisions;      // number of divisions per block along y axis
  int    NumberOfZDivisions;      // number of divisions per block along z axis
  double Time;                    // the time of this step
  double TimeStep;                // time interval
  double RedShift;
} FlashReaderSimulationParameters;

typedef  struct tagBlock
{
  int    Index;                   // Id of the block
  int    Level;                   // LOD level
  int    Type;                    // a leaf block?
  int    ParentId;                // Id  of the parent block
  int    ChildrenIds[8];          // Ids of the children    blocks
  int    NeighborIds[6];          // Ids of the neighboring blocks
  int    ProcessorId;             // Id  of the processor
  int    MinGlobalDivisionIds[3]; // first (global) division index
  int    MaxGlobalDivisionIds[3]; // last  (global) division index
  double Center[3];               // center of the block
  double MinBounds[3];            // lower left  of the bounding box
  double MaxBounds[3];            // upper right of the bounding box
} Block;

typedef  struct tagFlashReaderSimulationInformation
{
  int    FileFormatVersion;
  char   SetupCall[400];
  char   FileCreationTime[80];
  char   FlashVersion[80];
  char   BuildData[80];
  char   BuildDirectory[80];
  char   build_machine[80];
  char   CFlags[400];
  char   FFlags[400];
  char   SetupTimeStamp[80];
  char   BuildTimeStamp[80];
} FlashReaderSimulationInformation;

static std::string GetSeparatedParticleName( const std::string & variable )
{
  std::string sepaName = variable;

  if ( sepaName.length() > 9 && sepaName.substr(0,9) == "particle_" )
    {
    sepaName = std::string( "Particles/" ) + sepaName.substr( 9 );
    }
  else
    {
    sepaName = std::string( "Particles/" ) + sepaName;
    }

  return sepaName;
}


// ----------------------------------------------------------------------------
//                     Class  vtkFlashReaderInternal (begin)
// ----------------------------------------------------------------------------


class vtkFlashReaderInternal
{
public:
  vtkFlashReaderInternal()  { this->Init(); }
  ~vtkFlashReaderInternal() { this->Init(); }

  int      NumberOfBlocks;            // number of ALL blocks
  int      NumberOfLevels;            // number of levels
  int      FileFormatVersion;         // version of file format
  int      NumberOfParticles;         // number of particles
  int      NumberOfLeafBlocks;        // number of leaf blocks
  int      NumberOfDimensions;        // number of dimensions
  int      NumberOfProcessors;        // number of processors
  int      HaveProcessorsInfo;        // processor Ids available?
  int      BlockGridDimensions[3];    // number of grid points
  int      BlockCellDimensions[3];    // number of divisions
  int      NumberOfChildrenPerBlock;  // number of children  per block
  int      NumberOfNeighborsPerBlock; // number of neighbors per block

  char *   FileName;                  // Flash data file name
  hid_t    FileIndex;                 // file handle
  double   MinBounds[3];              // lower left  of the bounding-box
  double   MaxBounds[3];              // upper right of the bounding box
  FlashReaderSimulationParameters     SimulationParameters;   // CFD simulation
  FlashReaderSimulationInformation    SimulationInformation;  // CFD simulation

  // blocks
  std::vector< Block >             Blocks;
  std::vector<  int  >             LeafBlocks;
  std::vector< std::string >    AttributeNames;

  // particles
  std::string                      ParticleName;
  std::vector< hid_t >             ParticleAttributeTypes;
  std::vector< std::string >    ParticleAttributeNames;
  std::map< std::string, int >  ParticleAttributeNamesToIds;


  int      GetCycle();
  double   GetTime();

  void     Init();
  void     SetFileName( char * fileName ) { this->FileName = fileName; }
  const char* GetParticleName(char* variableName)
   {
   static std::string particleName;
   particleName = GetSeparatedParticleName(std::string(variableName));
   return particleName.c_str();
   }

  void     ReadMetaData();
  void     ReadProcessorIds();
  void     ReadDoubleScalars( hid_t fileIndx );
  void     ReadIntegerScalars( hid_t fileIndx );
  void     ReadVersionInformation( hid_t fileIndx );
  void     ReadSimulationParameters
           ( hid_t fileIndx, bool bTmCycle = false ); // time and cycle only
  void     GetBlockMinMaxGlobalDivisionIds();

  void     ReadBlockTypes();
  void     ReadBlockBounds();
  void     ReadBlockCenters();
  void     ReadBlockStructures();
  void     ReadRefinementLevels();
  void     ReadDataAttributeNames();

  void     ReadParticlesComponent
           ( hid_t dataIndx, const char * compName, double * dataBuff );
  void     ReadParticleAttributes();
  void     ReadParticleAttributesFLASH3();
  void     GetBlockAttribute( const char *atribute, int blockIdx,
                              vtkDataSet *pDataSet );
};




// ----------------------------------------------------------------------------
//                     Class  vtkFlashReaderInternal ( end )
// ----------------------------------------------------------------------------
#endif /* VTKAMRFLASHREADERINTERNAL_HPP_ */
// VTK-HeaderTest-Exclude: vtkAMRFlashReaderInternal.h
