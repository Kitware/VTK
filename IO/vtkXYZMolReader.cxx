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

#include <sys/stat.h>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkXYZMolReader, "1.4");
vtkStandardNewMacro(vtkXYZMolReader);

//----------------------------------------------------------------------------
vtkXYZMolReader::vtkXYZMolReader()
{
  this->TimeStep  = 0;
  this->MaxTimeStep  = 0;
}

//----------------------------------------------------------------------------
vtkXYZMolReader::~vtkXYZMolReader()
{
}

//----------------------------------------------------------------------------
char* vtkXYZMolReader::GetNextLine(FILE* fp, char* line, int maxlen)
{
  int cc;
  int len;
  int comment = 0;
  char* ptr = 0;
  do 
    {
    comment = 0;
    if ( !fgets(line, maxlen, fp) )
      {
      //cout << "Problem when reading. EOF?" << endl;
      return 0;
      }
    len = strlen(line);
    for ( cc = 0; cc < len; cc ++ )
      {
      int ch = line[cc];
      if ( ch == '#' )
        {
        comment = 1;
        break;
        }
      else if ( ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' )
        {
        break;
        }
      }
    if ( cc == len )
      {
      comment = 1;
      }
    } 
  while ( comment );
  //cout << "Have line that is not a comment: [" << line << "]" << endl;
  len = strlen(line);
  int ft = 0;
  ptr = line;
  for ( cc = 0; cc < len; cc ++ )
    {
    int ch = line[cc];
    if ( !ft && ( ch == ' ' || ch == '\t' ) )
      {
      ptr++;
      }
    else if ( ch == '#' || ch == '\n' || ch == '\r' )
      {
      line[cc] = 0;
      break;
      }
    else
      {
      ft = 1;
      }
    }
  if ( strlen(ptr) == 0 )
    {
    return 0;
    }
  return ptr;
}

//----------------------------------------------------------------------------
int vtkXYZMolReader::GetLine1(const char* line, int *cnt)
{
  char dummy[1024] = "";
  if ( !line || sscanf(line, "%d%s", cnt, dummy) < 1)
    {
    return 0;
    }
  int cc;
  for ( cc = 0; cc < static_cast<int>(strlen(dummy)); ++cc )
    {
    if ( dummy[cc] != ' ' && dummy[cc] != '\t' && dummy[cc] != '\n' &&
        dummy[cc] != '\r' )
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXYZMolReader::GetLine2(const char* line, char *name)
{
  char dummy[1024] = "";
  if ( !line || sscanf(line, "%s%s", name, dummy) < 1)
    {
    return 0;
    }
  int cc;
  for ( cc = 0; cc < static_cast<int>(strlen(dummy)); ++cc )
    {
    if ( dummy[cc] != ' ' && dummy[cc] != '\t' && dummy[cc] != '\n' &&
        dummy[cc] != '\r' )
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXYZMolReader::GetAtom(const char* line, char* atom, float *x)
{
  char dummy[1024] = "";
  if ( !line || sscanf(line, "%s %f %f %f%s", atom, x, x+1, x+2, dummy) < 4)
    {
    return 0;
    }
  int cc;
  for ( cc = 0; cc < static_cast<int>(strlen(dummy)); ++cc )
    {
    if ( dummy[cc] != ' ' && dummy[cc] != '\t' && dummy[cc] != '\n' &&
        dummy[cc] != '\r' )
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXYZMolReader::InsertAtom(const char* atom, float *pos)
{
  this->Points->InsertNextPoint(pos);
  this->AtomType->InsertNextValue(this->MakeAtomType(atom));
}


//----------------------------------------------------------------------------
int vtkXYZMolReader::CanReadFile(const char* name)
{
  if ( !name )
    {
    return 0;
    }

  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  struct stat fs;
  if(stat(name, &fs) != 0) 
    { 
    return 0; 
    }

  FILE* fp = fopen(name, "r");
  if ( !fp )
    {
    return 0;
    }

  int valid = 0;

  const int maxlen = 1024;
  char buffer[maxlen];
  char comment[maxlen];
  char atom[maxlen];
  char* lptr = 0;
  int num = 0;
  float pos[3];

  lptr = this->GetNextLine(fp, buffer, maxlen);
  if ( this->GetLine1(lptr, &num) )
    {
    // Have header
    lptr = this->GetNextLine(fp, buffer, maxlen);
    if ( this->GetLine2(lptr, comment) )
      {
      lptr = this->GetNextLine(fp, buffer, maxlen);
      if ( this->GetAtom(lptr, atom, pos) )
        {
        valid = 3;
        }
      }
    else if ( this->GetAtom(lptr, atom, pos) )
      {
      valid = 3;
      }
    }
  else
    {
    // No header
    lptr = this->GetNextLine(fp, buffer, maxlen);
    if ( this->GetAtom(lptr, atom, pos) )
      {
      valid = 3;
      }
    }
  
  fclose(fp);
  return valid;
}

//----------------------------------------------------------------------------
void vtkXYZMolReader::ReadSpecificMolecule(FILE *fp)
{
  const int maxlen = 1024;
  char buffer[maxlen];
  char comment[maxlen];
  char* lptr = 0;

  int have_header = 0;
  int num = 0;
  int cnt = 0;
  int ccnt = 0;
  int rcnt = 0;
  int timestep = 1;

  int selectstep = this->TimeStep;

  float pos[3];
  char atom[maxlen];

  this->AtomType->Allocate(1024);
  this->Points->Allocate(1024);
  
  while ( (lptr = this->GetNextLine(fp, buffer, maxlen)) )
    {
    if ( ( cnt == 0 || ccnt == num ) && this->GetLine1(lptr, &num) )
      {
      have_header = 1;
      vtkDebugMacro("Have header. Number of atoms is: " << num);
      ccnt = 0;
      if ( cnt > 0 )
        {
        timestep ++;
        }
      }
    else if ( have_header )
      {
      if ( ccnt == 0 && this->GetLine2(lptr, comment) )
        {
        vtkDebugMacro("Have comment");
        }
      else if ( this->GetAtom(lptr, atom, pos) )
        {
        if ( ccnt >= num )
          {
          vtkErrorMacro("Expecting " << num << " atoms, found: " << ccnt);
          return;
          }
        else
          {
          if ( selectstep == timestep -1 )
            {
            // Got atom with full signature
            this->InsertAtom(atom, pos);
            rcnt ++;
            }
          ccnt ++;
          }
        }
      else
        {
        vtkErrorMacro("Expecting atom, got: " << lptr);
        return;
        }
      }
    else
      {
      if ( this->GetAtom(lptr, atom, pos) )
        {
        // Got atom with simple signature
        this->InsertAtom(atom, pos);
        rcnt ++;
        }
      else
        {
        vtkErrorMacro("Expecting atom, got: " << lptr);
        return;
        }
      }
    ++ cnt;
    }

  // Just some more checking and cleanups
  if ( num == 0 )
    {
    num = rcnt;
    }

  this->AtomType->Squeeze();
  this->Points->Squeeze();

  if ( selectstep >= timestep )
    {
    this->NumberOfAtoms = 0;
    vtkErrorMacro("Only have " << timestep << " time step(s)");
    return;
    }

  vtkDebugMacro("Number of atoms: " << num << " (" << rcnt << ")");
  if ( num != rcnt )
    {
    this->NumberOfAtoms = 0;
    vtkErrorMacro("Expecting " << num << " atoms, got " << rcnt);
    return;
    }

  this->SetMaxTimeStep(timestep);
  this->NumberOfAtoms = num;
}

//----------------------------------------------------------------------------
void vtkXYZMolReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "MaxTimeStep: " << this->MaxTimeStep;
}
