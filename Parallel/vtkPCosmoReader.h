/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCosmoReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPCosmoReader.h

Copyright (c) 2009 Los Alamos National Security, LLC

All rights reserved.

Copyright 2009. Los Alamos National Security, LLC. 
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
// .NAME vtkPCosmoReader - Read a binary cosmology data file
//
// .SECTION Description
// vtkPCosmoReader creates a vtkUnstructuredGrid from a binary cosmology file.
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

#ifndef __vtkPCosmoReader_h
#define __vtkPCosmoReader_h

#include "vtkCosmoReader.h"

class vtkDataArraySelection;
class vtkStdString;
class vtkMultiProcessController;
class vtkDistributedDataFilter;

class VTK_PARALLEL_EXPORT vtkPCosmoReader : public vtkCosmoReader
{
public:
  static vtkPCosmoReader *New();
  vtkTypeRevisionMacro(vtkPCosmoReader, vtkCosmoReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Using the file size determine how many data records exist
  virtual void ComputeDefaultRange();

  // Description:
  // Set the communicator object for interprocess communication
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);

  // Description:
  // If set, it will attempt to read in a paralle
  // striped (strided) manner per
  // processor, otherwise, it reads in a block wise manner.
  // Default is blockwise.
  vtkGetMacro(ReadStriped, int);
  vtkSetMacro(ReadStriped, int);

protected:
  vtkPCosmoReader();
  ~vtkPCosmoReader();
 
  virtual int RequestInformation
    (vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData
    (vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkMultiProcessController* Controller; // Interprocess communication

  int ReadStriped;

private:
  vtkPCosmoReader(const vtkPCosmoReader&);  // Not implemented.
  void operator=(const vtkPCosmoReader&);  // Not implemented.
};

#endif

