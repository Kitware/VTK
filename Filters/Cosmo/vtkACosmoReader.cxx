/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkACosmoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkACosmoReader.cxx

Copyright (c) 2007, 2009 Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkACosmoReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkConfigure.h"
#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"


// C/C++ includes
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

//#define DEBUG

vtkStandardNewMacro(vtkACosmoReader);

#define FILE_BIG_ENDIAN 0
#define FILE_LITTLE_ENDIAN 1
#define TAG_SIZE_32_BIT 0
#define TAG_SIZE_64_BIT 1
#define DIMENSION 3
#define X 0
#define VX 1
#define Y 2
#define VY 3
#define Z 4
#define VZ 5
#define MASS 6
#define NUMBER_OF_VAR 3
#define BYTES_PER_DATA_MINUS_TAG 7 * sizeof(vtkTypeFloat32)
#define NUMBER_OF_FLOATS 7
#define NUMBER_OF_INTS 1

// Data-structure to store information about individual blocks
struct block_t {
  int Level; // starts from 1
  int IndexWithinLevel; // starts from 0
  size_t FileOffSet;
  double Bounds[6];
};



//----------------------------------------------------------------------------
vtkACosmoReader::vtkACosmoReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->BaseFileName = "";
  this->FileName     = NULL;

  this->ByteSwap  = 0;
  this->BoxSize   = 90.140846;
  this->TagSize   = TAG_SIZE_32_BIT;
  this->Level     = 1;
  this->TotalNumberOfLevels = 0;

  this->MetaData = NULL;
  this->MetadataIsLoaded = false;
}

//----------------------------------------------------------------------------
vtkACosmoReader::~vtkACosmoReader()
{
  if( this->FileName != NULL)
    {
    delete [] this->FileName;
    }

  if( this->MetaData != NULL )
    {
    this->MetaData->Delete();
    }

  this->NBlocks.clear();
  this->ParticleBlocks.clear();
  this->RequestedBlocks.clear();
}

//----------------------------------------------------------------------------
void vtkACosmoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Byte Swap: "
     << (this->ByteSwap ? "ON" : "OFF") << endl;
  os << indent << "BoxSize: " << this->BoxSize << endl;
  os << indent << "TagSize: " << (this->TagSize ? "64-bit" : "32-bit")
     << endl;
}

//----------------------------------------------------------------------------
void vtkACosmoReader::ReadMetaDataFile(
    const int levelIdx, std::string file)
{
  assert("pre: level index out-of-bounds" &&
          (levelIdx > 0) && (levelIdx <= this->TotalNumberOfLevels) );
#ifdef DEBUG
  std::cout << "\t[INFO]: Loading metadata `" << file << "`...";
  std::cout.flush();
#endif

  // STEP 0: Open metadata file
  int blockCount = 0;
  std::ifstream ifs;
  ifs.open(file.c_str());
  if( !ifs.is_open() )
    {
    throw std::runtime_error("Cannot open metadata file!\n");
    return;
    }

  // STEP 2: Parse information
  std::string line = "";
  std::vector<std::string> tokens;
  while(std::getline(ifs,line))
    {
    // Tokenize line
    tokens.clear();
    std::istringstream iss(line);
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(),
              std::back_inserter< std::vector<std::string> >(tokens));

    assert("pre: encountered invalid line" && (tokens.size()==7) );

    // Construct block
    block_t block;
    block.Level = levelIdx;
    block.IndexWithinLevel = blockCount;
    block.FileOffSet = atoi(tokens[0].c_str());
    block.Bounds[0] = atof(tokens[1].c_str()); // x-min
    block.Bounds[1] = atof(tokens[2].c_str()); // x-max
    block.Bounds[2] = atof(tokens[3].c_str()); // y-min
    block.Bounds[3] = atof(tokens[4].c_str()); // y-max
    block.Bounds[4] = atof(tokens[5].c_str()); // z-min
    block.Bounds[5] = atof(tokens[6].c_str()); // z-max

    // Push to list of blocks
    this->ParticleBlocks.push_back( block );

    ++blockCount;
    } // END parse file

  // STEP 3: Close metadata file
  ifs.close();

  // STEP 4: Update NBlocks at this level
  this->NBlocks[ levelIdx ] = blockCount;

#ifdef DEBUG
  std::cout << "[DONE]\n";
  std::cout.flush();
#endif
}

//----------------------------------------------------------------------------
void vtkACosmoReader::LoadMetaData()
{
  this->ExtractInfoFromFileName();

  // Note we start numbering levels from 1, level 0 has no blocks
  this->NBlocks.resize( this->TotalNumberOfLevels+1 );
  this->NBlocks[0] = 0;

  std::ostringstream metaFile;
  for(int i=1; i <= this->TotalNumberOfLevels; ++i)
    {
    metaFile.clear();
    metaFile.str("");
    metaFile << this->BaseFileName << ".0." << i << ".cosmo.meta";
    this->ReadMetaDataFile(i,metaFile.str());
    } // END for all levels

#ifdef DEBUG
  for( int i=0; i < this->NBlocks.size(); ++i)
    {
    std::cout << "\t[INFO]: LEVEL=" << i
          << " NBLOCKS=" << this->NBlocks[i] << "\n";
    std::cout.flush();
    }
  std::cout << "\t[INFO]: NBLOCKS: " << this->ParticleBlocks.size() << "\n";
  std::cout.flush();
#endif

  this->MetadataIsLoaded = true;
}
//----------------------------------------------------------------------------
void vtkACosmoReader::ExtractInfoFromFileName()
{
  if( (this->FileName==NULL) || (strcmp(this->FileName,"")==0) )
    {
    // unspecified or empty filename
    return;
    }

  std::vector<std::string> tokens;
  std::string tmpFileName = std::string(this->FileName);
  std::string delimiter = ".";
  size_t pos = tmpFileName.find(delimiter);
  for( ;pos != std::string::npos; pos=tmpFileName.find(delimiter) )
    {
    tokens.push_back( tmpFileName.substr(0,pos) );
    tmpFileName.erase(0,pos+delimiter.length());
    }

  if(tokens.size() != 3 )
    {
    vtkErrorMacro("Cannot process file: " << this->FileName);
    }

  this->BaseFileName        = tokens[0];
  int process = atoi(tokens[1].c_str());
  if( process > 0 )
    {
    vtkErrorMacro(
      "Data was sampled in parallel, this is currently not supported");
    }
  this->TotalNumberOfLevels = atoi(tokens[2].c_str());

#ifdef DEBUG
  std::cout << "\t[INFO]: BASEFILE: " << this->BaseFileName  << std::endl;
  std::cout << "\t[INFO]: NLEVELS: "  << this->TotalNumberOfLevels << std::endl;
  std::cout.flush();
#endif

}

//----------------------------------------------------------------------------
int vtkACosmoReader::GetBlockIndex(const int level, const int idx)
{
//  std::cout << "LEVEL=" << level << " IDX=" << idx << " NBLOCKS="
//            << this->NBlocks[level] << std::endl;
//  std::cout.flush();

  assert("pre: level is out-of-bounds!" &&
          (level >= 0) && (level <= this->TotalNumberOfLevels) );
  assert("pre: idx is out-of-bounds!" &&
          (idx >= 0) && (idx < this->NBlocks[level]) );
  if( level==0 )
    {
    return( idx );
    }
  else
    {
    int sumBlocks = 0;
    for(int i=0; i < level; ++ i)
      {
      sumBlocks += this->NBlocks[i];
      }
    return( sumBlocks+idx );
    }
}

//----------------------------------------------------------------------------
int vtkACosmoReader::RequestInformation(
  vtkInformation* rqst,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
#ifdef DEBUG
    std::cout << "[INFO]: REQUEST_INFORMATION...\n";
    std::cout.flush();
#endif

  // STEP 0: Return immediately if the metadata has already been loaded
  if( this->MetadataIsLoaded )
    {
#ifdef DEBUG
    std::cout << "\t[INFO]: metadata has been already loaded...\n";
    std::cout.flush();
#endif
    return( 1 );
    }


#ifdef DEBUG
    std::cout << "\t[INFO]: Loading metadata...\n";
    std::cout.flush();
#endif

  // STEP 1: Propagate request to super-class
  this->Superclass::RequestInformation(rqst,inputVector,outputVector);

  // STEP 2: Load the raw metadata from the file
  this->LoadMetaData();

  // STEP 3: Construct VTK metadata object to put on the pipeline
  // NOTE: level numbering starts from 1, hence, level 0, has no blocks!
  this->MetaData = vtkMultiBlockDataSet::New();
  this->MetaData->SetNumberOfBlocks( this->TotalNumberOfLevels+1 );
  for(int i=0; i <= this->TotalNumberOfLevels; ++i)
    {
    vtkMultiBlockDataSet *levelBlocks = vtkMultiBlockDataSet::New();
    levelBlocks->SetNumberOfBlocks( this->NBlocks[i] );
    for(int j=0; j < this->NBlocks[i]; ++j)
      {
      int blockIdx = this->GetBlockIndex(i,j);

      // NOTE: we just store metadata in the information object of each block
      levelBlocks->SetBlock(j,NULL);

      // Set the bounds on the block metadata
      vtkInformation *blockMetadata = levelBlocks->GetMetaData( j );
      assert("pre: block metadata is NULL!" && (blockMetadata != NULL) );
      blockMetadata->Set(
             vtkStreamingDemandDrivenPipeline::BOUNDS(),
             this->ParticleBlocks[blockIdx].Bounds,6
             );

      } // END for all blocks within level `i`

    this->MetaData->SetBlock( i, levelBlocks );
    levelBlocks->Delete();
    } // END for all levels

  // STEP 4: Push the metadata on the pipeline
  vtkInformation* info = outputVector->GetInformationObject(0);
  assert( "pre: output information object is NULL" && (info != NULL) );
  info->Set( vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(),
             this->MetaData );

  return 1;
}

//----------------------------------------------------------------------------
void vtkACosmoReader::SetupBlockRequest(vtkInformation *outInfo)
{
  assert("pre: output information should not be NULL!" && (outInfo != NULL));

#ifdef DEBUG
    std::cout << "\t[INFO]: Setting up block request...\n";
    std::cout.flush();
#endif

  this->RequestedBlocks.clear();

  if(outInfo->Has(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES()))
    {
#ifdef DEBUG
    std::cout << "\t[INFO]: Getting request from downstream!!!\n";
    std::cout.flush();
#endif

    int size = outInfo->Length(
        vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    int *ids = outInfo->Get(
        vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    this->RequestedBlocks.resize( size );
    std::copy(ids,ids+size,this->RequestedBlocks.begin());
    } // END if
  else
    {
    int loadLevel = (this->Level > this->TotalNumberOfLevels)?
          this->TotalNumberOfLevels : this->Level;

#ifdef DEBUG
    std::cout << "\t[INFO]: LOAD LEVEL=" << loadLevel << std::endl;
    std::cout.flush();
#endif

    this->RequestedBlocks.resize(this->NBlocks[ loadLevel ]);
    for(int i=0; i < this->NBlocks[ loadLevel ]; ++i )
      {
      this->RequestedBlocks[ i ] = this->GetBlockIndex(loadLevel,i);
      } // END for all blocks at the load level

    } // END else

#ifdef DEBUG
    std::cout << "\t[INFO]: Setting up block request...[DONE]\n";
    std::cout.flush();
#endif
}

//----------------------------------------------------------------------------
int vtkACosmoReader::GetBlockStartOffSetInFile(
    const int level,const int index)
{
  if(index == 0)
    {
    return 0;
    }
  else
    {
    int prevBlockIdx = this->GetBlockIndex(level,index-1);
    return static_cast<int>(this->ParticleBlocks[ prevBlockIdx ].FileOffSet);
    }
}

//----------------------------------------------------------------------------
void vtkACosmoReader::ReadBlock(
    const int blockIdx, vtkMultiBlockDataSet *mbds)
{
  assert("pre: blockIdx is out-of-bounds!" &&
         (blockIdx >= 0) && (blockIdx < this->ParticleBlocks.size()));
  assert("pre: multiblock output dataset is NULL!" && (mbds != NULL) );

  // STEP 0: Get block and level/index information
  block_t* blockPtr = &this->ParticleBlocks[blockIdx];
  assert("pre: blockPtr is NULL!" && (blockPtr != NULL) );
  int level = blockPtr->Level;
  int index = blockPtr->IndexWithinLevel;

  // STEP 1: Get block offsets within file
  int startOffSet = this->GetBlockStartOffSetInFile(level,index);
  int endOffSet = static_cast<int>(blockPtr->FileOffSet);
#ifdef DEBUG
  std::cout << "\t[INFO]: BLOCK: "        << blockIdx    << std::endl;
  std::cout << "\t[INFO]: LEVEL: "        << level       << std::endl;
  std::cout << "\t[INFO]: INDEX: "        << index       << std::endl;
  std::cout << "\t[INFO]: START OFFSET: " << startOffSet << std::endl;
  std::cout << "\t[INFO]: END OFFSET: "   << endOffSet   << std::endl;
  std::cout.flush();
#endif
  assert("pre: bogus start/end offset" && (startOffSet < endOffSet) );

  // STEP 2: Construct cosmo filename
  std::ostringstream cosmoFile;
  cosmoFile << this->BaseFileName << ".0." << level << ".cosmo";

  // STEP 3: Read block from the file
  vtkUnstructuredGrid *particles = vtkUnstructuredGrid::New();
  this->ReadBlockFromFile(cosmoFile.str(),startOffSet,endOffSet,particles);

  // STEP 4: Store the block in the output multi-block data-structure
  vtkMultiBlockDataSet *levelDS =
      vtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(level));
  assert("pre: level data-structure is NULL!" && (levelDS!=NULL) );
  levelDS->SetBlock(index,particles);
  particles->Delete();
}

//----------------------------------------------------------------------------
int vtkACosmoReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
#ifdef DEBUG
    std::cout << "[INFO]: REQUEST_DATA" << std::endl;
    std::cout.flush();
#endif

  // STEP 0: Get output and output info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output =
      vtkMultiBlockDataSet::SafeDownCast(
          outInfo->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: output information object is NULL!" && (outInfo != NULL));
  assert("pre: output object is NULL!" && (output != NULL) );

  // STEP 1: Setup the block request
  this->SetupBlockRequest(outInfo);

  // STEP 2: Initialize output, NOTE: level 0 is empty
  output->SetNumberOfBlocks( this->TotalNumberOfLevels+1 );
  for(int i=0; i <= this->TotalNumberOfLevels; ++i)
    {
    vtkMultiBlockDataSet *levelDS = vtkMultiBlockDataSet::New();
    levelDS->SetNumberOfBlocks( this->NBlocks[i] );
    output->SetBlock( i, levelDS );
    levelDS->Delete();
    } // END for all levels

  // STEP 3: Load requested blocks
  for(unsigned int block=0; block < this->RequestedBlocks.size(); ++block)
    {
    int blockIdx = this->RequestedBlocks[ block ];
    this->ReadBlock(blockIdx, output);
    } // END for all requested blocks

  outInfo = NULL;
  output  = NULL;
  return 1;
}

//----------------------------------------------------------------------------
void vtkACosmoReader::ReadBlockFromFile(
    std::string file, const int start, const int end,
    vtkUnstructuredGrid *particles)
{
  assert("pre: input particles grid is NULL!" && (particles != NULL));

#ifdef DEBUG
  std::cout << "\t[INFO]: Reading Block from \"" << file << "\"\n";
  std::cout << "\t[INFO]: start=" << start << " end=" << end << std::endl;
  std::cout.flush();
#endif

  // STEP 0: Open file
  std::ifstream ifs;
  ifs.open(file.c_str(), std::ios::in | std::ios::binary);
  if( !ifs.is_open() )
    {
    throw std::runtime_error("Cannot open cosmo file!\n");
    }

  // STEP 1: Get file length
  ifs.seekg(0L,ios::end);
  size_t fileLength = ifs.tellg();
  (void)fileLength;
#ifdef DEBUG
  std::cout << "\t[INFO]: FileSize=" << fileLength << std::endl;
  std::cout.flush();
#endif

  assert("pre: end offset out of file bounds!" && (end <= fileLength) );

  // STEP 2: Compute number of particles to be read
  size_t tagSize = (this->TagSize==TAG_SIZE_32_BIT)?
      sizeof(int) : sizeof(vtkTypeInt64);
  vtkIdType numParticles = ( (end-start)/(BYTES_PER_DATA_MINUS_TAG+tagSize) );
#ifdef DEBUG
  std::cout << "\t[INFO]: NumParticles=" << numParticles << std::endl;
  std::cout.flush();
#endif

  // STEP 3: Allocate VTK data-structures
  vtkPoints *points = vtkPoints::New();
  points->SetDataTypeToFloat();
  points->SetNumberOfPoints( numParticles );

  vtkCellArray *cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(numParticles,1));

  vtkFloatArray *velocity = vtkFloatArray::New();
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples( numParticles );
  velocity->SetName("velocity");
  float* velPtr = static_cast<float*>(velocity->GetVoidPointer(0));

  vtkFloatArray *mass = vtkFloatArray::New();
  mass->SetNumberOfComponents(1);
  mass->SetNumberOfTuples( numParticles );
  mass->SetName("mass");
  float* massPtr = static_cast<float*>(mass->GetVoidPointer(0));

  vtkIdTypeArray *tag = vtkIdTypeArray::New();
  tag->SetNumberOfComponents(1);
  tag->SetNumberOfTuples( numParticles );
  tag->SetName("tag");
  vtkIdType* tagPtr = static_cast<vtkIdType*>(tag->GetVoidPointer(0));


  // STEP 4: Read the data

  // record elements
  vtkTypeFloat32 fBlock[NUMBER_OF_FLOATS]; // x,xvel,y,yvel,z,zvel,mass
  char *iBlock = new char[NUMBER_OF_INTS*tagSize];
  size_t numBytesRead = 0;
  for(vtkIdType idx=0; idx < numParticles; ++idx)
    {
    cells->InsertNextCell(1,&idx);

    size_t position = start + idx*(BYTES_PER_DATA_MINUS_TAG+tagSize);
    assert("pre: file position out of bounds!" &&
            (position >= start) && (position < end) );

    ifs.seekg(position, ios::beg);

    // Read the floating point part of the data
    ifs.read((char*)fBlock, NUMBER_OF_FLOATS*sizeof(vtkTypeFloat32));
    numBytesRead = ifs.gcount();
    if( numBytesRead != NUMBER_OF_FLOATS*sizeof(vtkTypeFloat32))
      {
      std::cout << "ERROR: cannor read fBlock!\n";
      std::cout.flush();
      continue;
      }

    // Read the tag
    ifs.read(iBlock,NUMBER_OF_INTS*tagSize);
    numBytesRead = ifs.gcount();
    if( numBytesRead != NUMBER_OF_INTS*tagSize)
      {
      std::cout << "ERROR: cannot read iBlock!\n";
      std::cout.flush();
      continue;
      }

    if(this->ByteSwap)
      {
      vtkByteSwap::SwapVoidRange(
          fBlock,NUMBER_OF_FLOATS,(int)sizeof(vtkTypeFloat32));
      vtkByteSwap::SwapVoidRange(iBlock, NUMBER_OF_INTS, (int)tagSize);
      }

    // Handle wrapping of particles across periodic boundaries
    fBlock[X] = fBlock[X] < 0.0 ? this->BoxSize + fBlock[X] :
      (fBlock[X] > this->BoxSize ? fBlock[X] - this->BoxSize : fBlock[X]);
    fBlock[Y] = fBlock[Y] < 0.0 ? this->BoxSize + fBlock[Y] :
      (fBlock[Y] > this->BoxSize ? fBlock[Y] - this->BoxSize : fBlock[Y]);
    fBlock[Z] = fBlock[Z] < 0.0 ? this->BoxSize + fBlock[Z] :
      (fBlock[Z] > this->BoxSize ? fBlock[Z] - this->BoxSize : fBlock[Z]);

    points->SetPoint(idx,fBlock[X],fBlock[Y],fBlock[Z]);

    velPtr[idx*3]   = fBlock[VX];
    velPtr[idx*3+1] = fBlock[VY];
    velPtr[idx*3+2] = fBlock[VZ];

    massPtr[idx] = fBlock[MASS];

    tagPtr[idx] = *((vtkIdType*)iBlock);
    } // END for all particles

  delete [] iBlock;


  // STEP 5: set output particles
  particles->SetPoints( points );
  points->Delete();

  particles->SetCells(VTK_VERTEX,cells);
  cells->Delete();

  particles->GetPointData()->AddArray(velocity);
  velocity->Delete();

  particles->GetPointData()->AddArray(mass);
  mass->Delete();

  particles->GetPointData()->AddArray(tag);
  tag->Delete();

  particles->Squeeze();

  // STEP 6: cleanup and close the file
  velPtr  = NULL;
  massPtr = NULL;
  tagPtr  = NULL;

  ifs.close();
}
