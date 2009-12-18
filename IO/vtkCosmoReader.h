/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoReader.h

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
// .NAME vtkCosmoReader - Read a binary cosmology data file
//
// .SECTION Description
// vtkCosmoReader creates a vtkUnstructuredGrid from a binary cosmology file.
// The file contains fields for:
//     x_position, x_velocity (float)
//     y_position, y_velocity (float)
//     z-position, z_velocity (float)
//     mass (float)
//     identification tag (integer)
//
// If the file contains particle information x,y,z is the location of the
// particle in simulation space with a velocity vector and a mass which
// will be the same for all particles.
//
// If the file contains halo information x,y,z is the location of the
// particle which is the centroid of all particles in the halo and
// the mass is the collective mass of the halo.  In order to find the
// number of particles in a halo, take the mass of a single particle
// and divide it into the mass of a halo.
//
// The stride variable will read every nth particle into the unstructured
// grid to get a subsampling.  It has been noted that this is not the best
// thing to do for subsampling since particle points are generated in
// stripes.  A better thing to do would be to take a random sampling.
//

#ifndef __vtkCosmoReader_h
#define __vtkCosmoReader_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkDataArraySelection;
class vtkStdString;

namespace cosmo
{
  const int FILE_BIG_ENDIAN = 0;
  const int FILE_LITTLE_ENDIAN = 1;
  const int DIMENSION = 3;
  
  const int X          = 0; // Location X coordinate
  const int X_VELOCITY = 1; // Velocity in X direction
  const int Y          = 2; // Location Y coordinate
  const int Y_VELOCITY = 3; // Velocity in Y direction
  const int Z          = 4; // Location Z coordinate
  const int Z_VELOCITY = 5; // Velocity in Z direction
  const int MASS       = 6; // Mass of record item

  const int NUMBER_OF_VAR = 3;
  const size_t BYTES_PER_DATA_MINUS_TAG = 7 * sizeof(vtkTypeFloat32);
  
  const int USE_VELOCITY = 0;
  const int USE_MASS = 1;
  const int USE_TAG = 2;
}

class VTK_IO_EXPORT vtkCosmoReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCosmoReader *New();
  vtkTypeRevisionMacro(vtkCosmoReader, vtkUnstructuredGridAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the cosmology particle binary file to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the number of data variables at the cell centers
  vtkGetMacro(NumberOfVariables, int);

  // Description:
  // Set/Get the endian-ness of the binary file
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);

  // Set/Get the stride for reading a subset of the particles
  vtkSetMacro(Stride, int);
  vtkGetMacro(Stride, int);

  // Set/Get the box size for the simulation (range along x,y,z)
  // Negative x,y,z values are subtracted from this for wraparound
  vtkSetMacro(BoxSize, double);
  vtkGetMacro(BoxSize, double);

  // Description:
  // Set/Get the range of indices of interest
  vtkGetVector2Macro(PositionRange, vtkIdType);

  // Description:
  // When off (the default) only points are produced. When on, a VTK_VERTEX 
  // cell is generated for each point.
  vtkSetMacro(MakeCells, int);
  vtkGetMacro(MakeCells, int);

  // Description:
  // When false (default) 32-bit tags are read from the file.  When
  // on, 64-bit tags are read from the file.
  vtkSetMacro(TagSize, int);
  vtkGetMacro(TagSize, int);

  // Description:
  // Get the reader's output
  virtual vtkUnstructuredGrid *GetOutput();
  virtual vtkUnstructuredGrid *GetOutput(int index);

  // Description:
  // Set/Get the endian-ness of the binary file
  virtual void SetByteOrderToBigEndian();
  virtual void SetByteOrderToLittleEndian();

  // Description:
  // Using the file size determine how many data records exist
  virtual void ComputeDefaultRange();

  // Description:
  // The following methods allow selective reading of solutions fields.
  // By default, ALL data fields on the nodes are read, but this can
  // be modified.
  virtual int GetNumberOfPointArrays();
  virtual const char* GetPointArrayName(int index);
  virtual int GetPointArrayStatus(const char* name);
  virtual void SetPointArrayStatus(const char* name, int status);
  virtual void DisableAllPointArrays();
  virtual void EnableAllPointArrays();

protected:
  vtkCosmoReader();
  ~vtkCosmoReader();
 
  virtual int RequestInformation(vtkInformation *, 
    vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, 
    vtkInformationVector **, vtkInformationVector *);

  virtual void ReadFile(vtkUnstructuredGrid *);

  char* FileName; // Name of binary particle file
  ifstream *FileStream; // Data stream

  vtkIdType NumberOfNodes; // Number of particles
  int NumberOfVariables; // Number of attached data variables

  int ByteOrder; // Endian
  int Stride; // Read in every nth data particle
  vtkIdType PositionRange[2]; // Range of particle indices
  double BoxSize; // Maximum of x,y,z locations from simulation

  // Selected field of interest
  vtkDataArraySelection* PointDataArraySelection;

  vtkStdString *VariableName;
  vtkIdType *ComponentNumber; // Components per variable

  int MakeCells; // Make cells for particles, not just points
  int TagSize; // Size of the tag, 0 = 32-bit or 1 = 64-bit

  // internal variable for parallel striped reads by vtkPCosmoReader
  int parallelStride;

private:
  vtkCosmoReader(const vtkCosmoReader&);  // Not implemented.
  void operator=(const vtkCosmoReader&);  // Not implemented.
};

#endif

