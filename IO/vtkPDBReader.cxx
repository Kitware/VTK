/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDBReader.cxx
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

#include "vtkPDBReader.h"

#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"

vtkCxxRevisionMacro(vtkPDBReader, "1.4");
vtkStandardNewMacro(vtkPDBReader);

vtkPDBReader::vtkPDBReader()
{
}

vtkPDBReader::~vtkPDBReader()
{
}

void vtkPDBReader::ReadSpecificMolecule(FILE* fp)
{
  char linebuf[82], dum1[8], dum2[8];
  char    atype[4+1];
  int hydr = 0;
  int i, j;
  float x[3];

  this->NumberOfAtoms = 0;
  this->Points->Allocate(500);
  this->AtomType->Allocate(500);

  vtkDebugMacro( << "PDB File (" << this->HBScale 
    << ", " << this->BScale << ")");
  while(fgets(linebuf, sizeof linebuf, fp) != NULL &&
    strncmp("END", linebuf, 3)) 
    {
    if((0==strncmp("ATOM",linebuf,4) || 0==strncmp("atom",linebuf,4)) ||
      (0==strncmp("HETATM",linebuf,6) || 0==strncmp("hetatm",linebuf,6))) 
      {
      sscanf(&linebuf[12],"%4s", dum1);
      sscanf(&linebuf[17],"%3s", dum2);
      sscanf(&linebuf[30],"%8f%8f%8f", x, x+1, x+2);
      if(hydr == 0) 
        {
        this->Points->InsertNextPoint(x);

        for(j=0, i=static_cast<int>(strspn(dum1, " ")); i < 5; i++)
          {
          atype[j++] = dum1[i];
          }

        this->NumberOfAtoms++;
        } 
      else if( !(dum1[0]=='H' || dum1[0]=='h') ) 
        { /* skip hydrogen */
        this->Points->InsertNextPoint(x);
        for(j=0, i=static_cast<int>(strspn(dum1, " ")); i < 5; i++)
          {
          atype[j++] = dum1[i];
          }

        //sprintf(aamin[NumberOfAtoms],"%3s", dum2);
        this->NumberOfAtoms++;
        }
      this->AtomType->InsertNextValue(this->MakeAtomType(atype));
      }
    }
  this->Points->Squeeze();
}

void vtkPDBReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
