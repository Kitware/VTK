/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkUnstructuredGridReader.hh"

vtkUnstructuredGridReader::vtkUnstructuredGridReader()
{
}

vtkUnstructuredGridReader::~vtkUnstructuredGridReader()
{
}

unsigned long int vtkUnstructuredGridReader::GetMTime()
{
  unsigned long dtime = this->vtkUnstructuredGridSource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vtk polygonal data file to read.
void vtkUnstructuredGridReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vtkUnstructuredGridReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vtkUnstructuredGridReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkUnstructuredGridReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vtkUnstructuredGridReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkUnstructuredGridReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vtkUnstructuredGridReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkUnstructuredGridReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vtkUnstructuredGridReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkUnstructuredGridReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vtkUnstructuredGridReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkUnstructuredGridReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vtkUnstructuredGridReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkUnstructuredGridReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vtkUnstructuredGridReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}

void vtkUnstructuredGridReader::Execute()
{
  FILE *fp;
  int numPts=0;
  int retStat;
  char line[257];
  int npts, size, ncells;
  vtkCellArray *cells=NULL;
  int *types=NULL;

  vtkDebugMacro(<<"Reading vtk unstructured grid...");
  this->Initialize();
  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if ( !(fp=this->Reader.OpenVTKFile()) || !this->Reader.ReadHeader(fp) )
      return;
//
// Read unstructured grid specific stuff
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      return;
      } 

    if ( strncmp(this->Reader.LowerCase(line),"unstructured_grid",17) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
//
// Might find points, cells, and cell types
//
    while (1)
      {
      if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) break;

      if ( ! strncmp(this->Reader.LowerCase(line),"points",6) )
        {
        if ( (retStat=fscanf(fp,"%d", &numPts)) == EOF || retStat < 1 ) 
          {
          vtkErrorMacro(<<"Cannot read number of points!");
          return;
          }

        if ( ! this->Reader.ReadPoints(fp, (vtkPointSet *)this, numPts) ) return;
        }

      else if ( ! strncmp(line,"cells",5) )
        {
        if ((retStat=fscanf(fp,"%d %d", &ncells, &size)) == EOF || retStat < 2) 
          {
          vtkErrorMacro(<<"Cannot read cells!");
          return;
          }

        cells = new vtkCellArray;
        if ( !this->Reader.ReadCells(fp, size, cells->WritePtr(ncells,size)) ) return;
        cells->WrotePtr();
        if ( cells && types ) this->SetCells(types, cells);
        cells->Delete();
        }

      else if ( ! strncmp(line,"cell_types",5) )
        {
        if ( (retStat=fscanf(fp,"%d", &ncells)) == EOF || retStat < 1 ) 
          {
          vtkErrorMacro(<<"Cannot read cell types!");
          return;
          }

        types = new int[ncells];
        if ( this->Reader.GetFileType() == BINARY )
          {
          if ( (fgets(line,256,fp) == NULL) ||
          (fread(types,sizeof(int),ncells,fp) != ncells) )
            {
            vtkErrorMacro(<<"Error reading binary cell types!");
            return;
            }
          }
        else //ascii
          {
          for (int i=0; i<ncells; i++)
            {
            if ((retStat=fscanf(fp,"%d",types+i)) == EOF || retStat < 1) 
              {
              vtkErrorMacro(<<"Error reading cell types!");
              return;
              }
            }
          }
        if ( cells && types ) this->SetCells(types, cells);
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
          {
          vtkErrorMacro(<<"Cannot read point data!");
          return;
          }
        
        if ( npts != numPts )
          {
          vtkErrorMacro(<<"Number of points don't match!");
          return;
          }

        this->Reader.ReadPointData(fp, (vtkDataSet *)this, npts);
        break; //out of this loop
        }

      else
        {
        vtkErrorMacro(<< "Unrecognized keyord: " << line);
        return;
        }
      }
      if ( ! this->GetPoints() ) vtkWarningMacro(<<"No points read!");
      if ( ! (cells && types) )  vtkWarningMacro(<<"No topology read!");
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if ( (retStat=fscanf(fp,"%d", &numPts)) == EOF || retStat < 1 ) 
      {
      vtkErrorMacro(<<"Cannot read point data!");
      return;
      }

    this->Reader.ReadPointData(fp, (vtkDataSet *)this, numPts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyord: " << line);
    }
//
// Clean-up and get out
//
  if ( types ) delete [] types;

  vtkDebugMacro(<<"Read " <<this->GetNumberOfPoints() <<" points," <<this->GetNumberOfCells() <<" cells.\n");
  return;
}

void vtkUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkUnstructuredGridSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent.GetNextIndent());
}
