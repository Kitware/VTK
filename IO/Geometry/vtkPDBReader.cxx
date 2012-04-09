/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDBReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDBReader.h"

#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"

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
