/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkACosmoReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkACosmoReader.h

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
// .NAME vtkACosmoReader - Adaptively read a binary cosmology data file
//

#ifndef __vtkACosmoReader_h
#define __vtkACosmoReader_h

#include "vtkFiltersCosmoModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

// C/C++ includes
#include <string> // For C++ string
#include <vector> // For STL vector

// Forward declarations
struct block_t; // defined in the implementation

class vtkInformation;
class vtkInformationVector;
class vtkMultiBlockDataSet;
class vtkUnstructuredGrid;

class VTKFILTERSCOSMO_EXPORT vtkACosmoReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkACosmoReader *New();
  vtkTypeMacro(vtkACosmoReader, vtkMultiBlockDataSetAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the cosmology particle binary file to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Set/Get the box size for the simulation (range along x,y,z)
  // Negative x,y,z values are subtracted from this for wraparound
  vtkSetMacro(BoxSize, double);
  vtkGetMacro(BoxSize, double);

  // Description:
  // Set/Get the endian-ness of the binary file
  vtkSetMacro(ByteSwap, int);
  vtkGetMacro(ByteSwap, int);

  // Description:
  // When false (default) 32-bit tags are read from the file.  When
  // on, 64-bit tags are read from the file.
  vtkSetMacro(TagSize, int);
  vtkGetMacro(TagSize, int);

  // Description:
  // Sets the level of resolution
  vtkSetMacro(Level,int);
  vtkGetMacro(Level,int);

protected:
  vtkACosmoReader();
  ~vtkACosmoReader();

  // Standard pipeline methods
  virtual int RequestInformation(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);


  // Description:
  // Loads the metadata
  void LoadMetaData();

  // Description:
  // Processes the user-supplied FileName and extracts the
  // base file name, as well as, the total number of levels.
  void ExtractInfoFromFileName();

  // Description:
  // Reads the metadata file with the given filename at the specified level.
  void ReadMetaDataFile(const int levelIdx, std::string file);

  // Description:
  // Given the level and index of the block within that level, this method
  // returns the block index.
  int GetBlockIndex(const int level, const int idx);

  // Description:
  // Given an output information object, this method will populate
  // the vector of block Ids to read in.
  void SetupBlockRequest(vtkInformation *outInfo);

  // Description:
  // Read in the block corresponding to the given index
  void ReadBlock(const int blockIdx, vtkMultiBlockDataSet *mbds);

  // Description:
  // Given the block level and index within the level, this method returns
  // the block's starting offset within the file.
  int GetBlockStartOffSetInFile(const int level, const int index);

  // Description:
  // Given the file and start/end offsets of a block, this method reads in
  // the particles for a contiguous block.
  void ReadBlockFromFile(
      std::string file,
      const int start, const int end,
      vtkUnstructuredGrid* particles);

  std::string BaseFileName; // Base path and file name
  char* FileName;           // Name of binary particle file
  bool MetadataIsLoaded; // Indicates if the meta data has been loaded

  double BoxSize; // Maximum of x,y,z locations from simulation
  int ByteSwap;   // indicates whether to swap data or not
  int TagSize;    // Size of the tag, 0 = 32-bit or 1 = 64-bit
  int Level;      // level of resolution to load (staring from 1)
  int TotalNumberOfLevels; // The total number of levels

  vtkMultiBlockDataSet *MetaData;

// BTX
  std::vector< int > NBlocks; // Number of blocks at level "i"
  std::vector< block_t > ParticleBlocks; // stores block info for each block
  std::vector< int > RequestedBlocks; // list of blocks to load
// ETX

private:
  vtkACosmoReader(const vtkACosmoReader&);  // Not implemented.
  void operator=(const vtkACosmoReader&);  // Not implemented.
};

#endif
