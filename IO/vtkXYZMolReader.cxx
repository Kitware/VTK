/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYZMolReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkXYZMolReader.h"

#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkXYZMolReader, "1.2");
vtkStandardNewMacro(vtkXYZMolReader);

vtkXYZMolReader::vtkXYZMolReader()
{
  this->TimeStep  = 0;
  this->MaxTimeStep  = 0;
}

vtkXYZMolReader::~vtkXYZMolReader()
{
}

void vtkXYZMolReader::ReadSpecificMolecule(FILE *fp)
{
  char linebuf[82], dum1[8];
  float x[3];
  int k, t=0;

  if(this->MaxTimeStep  == 0)
    {
    fgets(linebuf, 80, fp);
    sscanf(linebuf, "%d", &this->NumberOfAtoms);
    k = 1;
    while(NULL != fgets(linebuf, 80, fp))
      {
      k++;
      }

    this->SetMaxTimeStep((k/(this->NumberOfAtoms + 2)) - 1);
    vtkDebugMacro(<< "Setting MaxTimeStep to " << this->MaxTimeStep);
    rewind(fp);
    }

  while(t < this->TimeStep)
    {
    fgets(linebuf, 80, fp);
    sscanf(linebuf, "%d", &this->NumberOfAtoms);
    fgets(linebuf, 80, fp);

    k=0;
    while(k < this->NumberOfAtoms)
      {
      fgets(linebuf, 80, fp);
      k++;
      }
    t++;
    }

  fgets(linebuf, 80, fp);
  sscanf(linebuf, "%d", &this->NumberOfAtoms);

  this->Points->Allocate(this->NumberOfAtoms);
  this->AtomType->Allocate(this->NumberOfAtoms);

  fgets(linebuf, 80, fp);
  t=0;
  k=0;
  while(k < this->NumberOfAtoms)
    {
    fscanf(fp, "%s %f %f %f", dum1, x, x+1, x+2);
    this->Points->InsertNextPoint(x);
    this->AtomType->InsertNextValue(this->MakeAtomType(dum1));
    k++;
    }
}

void vtkXYZMolReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "MaxTimeStep: " << this->MaxTimeStep;
}
