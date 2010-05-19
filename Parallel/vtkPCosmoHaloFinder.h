/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCosmoHaloFinder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPCosmoHaloFinder.h

Copyright (c) 2007, 2009, Los Alamos National Security, LLC

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
// .NAME vtkPCosmoHaloFinder - find halos within a cosmology data file
// .SECTION Description
// vtkPCosmoHaloFinder is a filter object that operates on the unstructured 
// grid of all particles and assigns each particle a halo id.
//

#ifndef __vtkPCosmoHaloFinder_h
#define __vtkPCosmoHaloFinder_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPCosmoHaloFinder : public vtkUnstructuredGridAlgorithm
{
 public:
  static vtkPCosmoHaloFinder *New();

  vtkTypeMacro(vtkPCosmoHaloFinder,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the communicator object for interprocess communication
  virtual vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController*);

  // Description:
  // Specify the number of seeded particles in one dimension (total = np^3)
  vtkSetMacro(NP, int);
  vtkGetMacro(NP, int);

  // Description:
  // Specify the physical box dimensions size (rL) (default 91)
  vtkSetMacro(RL, float);
  vtkGetMacro(RL, float);

  // Description:
  // Specify the ghost cell spacing (edge boundary of box) (default 5)
  vtkSetMacro(Overlap, float);
  vtkGetMacro(Overlap, float);

  // Description:
  // Specify the minimum number of particles for a halo (pmin)
  vtkSetMacro(PMin, int);
  vtkGetMacro(PMin, int);

  // Description:
  // Specify the linking length (bb)
  vtkSetMacro(BB, float);
  vtkGetMacro(BB, float);

  // Description:
  // Specify the particle mass
  vtkSetMacro(ParticleMass, float);
  vtkGetMacro(ParticleMass, float);

  // Description:
  // Copy the halo information to the original particles (Default on)
  vtkSetMacro(CopyHaloDataToParticles, int);
  vtkGetMacro(CopyHaloDataToParticles, int);

 protected:
  vtkPCosmoHaloFinder();
  ~vtkPCosmoHaloFinder();

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);

  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);

  vtkMultiProcessController* Controller;

  int NP; // number of particles in the original simulation
  float RL; // The physical box dimensions (rL)
  float Overlap; // The ghost cell boundary space
  int PMin; // The minimum particles for a halo
  float BB; // The linking length
  float ParticleMass;
  int CopyHaloDataToParticles;

 private:
  vtkPCosmoHaloFinder(const vtkPCosmoHaloFinder&);  // Not implemented.
  void operator=(const vtkPCosmoHaloFinder&);  // Not implemented.

};

#endif //  __vtkPCosmoHaloFinder_h
