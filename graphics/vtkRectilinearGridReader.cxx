/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkRectilinearGridReader.h"

vtkRectilinearGridReader::vtkRectilinearGridReader()
{
}

unsigned long int vtkRectilinearGridReader::GetMTime()
{
  unsigned long dtime = this->vtkSource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vtk polygonal data file to read.
void vtkRectilinearGridReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vtkRectilinearGridReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (VTK_ASCII or VTK_BINARY)
int vtkRectilinearGridReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkRectilinearGridReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vtkRectilinearGridReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkRectilinearGridReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vtkRectilinearGridReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkRectilinearGridReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vtkRectilinearGridReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkRectilinearGridReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vtkRectilinearGridReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkRectilinearGridReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vtkRectilinearGridReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkRectilinearGridReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vtkRectilinearGridReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}

void vtkRectilinearGridReader::Execute()
{
  int numPts=0, npts, ncoords;
  char line[256];
  int dimsRead=0;
  vtkRectilinearGrid *output=(vtkRectilinearGrid *)this->Output;
  
  vtkDebugMacro(<<"Reading vtk rectilinear grid file...");
  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if (!this->Reader.OpenVTKFile() || !this->Reader.ReadHeader())
      return;
//
// Read rectilinear grid specific stuff
//
  if (!this->Reader.ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->Reader.CloseVTKFile ();
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if (!this->Reader.ReadString(line))
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->Reader.CloseVTKFile ();
      return;
      } 

    if ( strncmp(this->Reader.LowerCase(line),"rectilinear_grid",17) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader.CloseVTKFile ();
      return;
      }
//
// Read keyword and number of points
//
    while (1)
      {
      if (!this->Reader.ReadString(line)) break;

      if ( ! strncmp(this->Reader.LowerCase(line),"dimensions",10) )
        {
        int dim[3];
        if (!(this->Reader.ReadInt(dim) && 
	      this->Reader.ReadInt(dim+1) && 
	      this->Reader.ReadInt(dim+2)))
          {
          vtkErrorMacro(<<"Error reading dimensions!");
          this->Reader.CloseVTKFile ();
          return;
          }

        numPts = dim[0] * dim[1] * dim[2];
        output->SetDimensions(dim);
        dimsRead = 1;
        }

      else if ( ! strncmp(line,"x_coordinate",12) )
        {
        if (!this->Reader.ReadInt(&ncoords))
          {
          vtkErrorMacro(<<"Error reading x coordinates!");
          this->Reader.CloseVTKFile ();
          return;
          }

        this->Reader.ReadCoordinates(output, 0, ncoords);
        }

      else if ( ! strncmp(line,"y_coordinate",12) )
        {
        if (!this->Reader.ReadInt(&ncoords))
          {
          vtkErrorMacro(<<"Error reading y coordinates!");
          this->Reader.CloseVTKFile ();
          return;
          }

        this->Reader.ReadCoordinates(output, 1, ncoords);
        }

      else if ( ! strncmp(line,"z_coordinate",12) )
        {
        if (!this->Reader.ReadInt(&ncoords))
          {
          vtkErrorMacro(<<"Error reading z coordinates!");
          this->Reader.CloseVTKFile ();
          return;
          }

        this->Reader.ReadCoordinates(output, 2, ncoords);
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if (!this->Reader.ReadInt(&npts))
          {
          vtkErrorMacro(<<"Cannot read point data!");
          this->Reader.CloseVTKFile ();
          return;
          }
        
        if ( npts != numPts )
          {
          vtkErrorMacro(<<"Number of points don't match!");
          this->Reader.CloseVTKFile ();
          return;
          }

        this->Reader.ReadPointData(output, npts);
        break; //out of this loop
        }

      else
        {
        vtkErrorMacro(<< "Unrecognized keyord: " << line);
        this->Reader.CloseVTKFile ();
        return;
        }
      }

      if ( !dimsRead ) vtkWarningMacro(<<"No dimensions read.");
      if ( !output->GetXCoordinates() || 
      output->GetXCoordinates()->GetNumberOfScalars() < 1 )
        {
        vtkWarningMacro(<<"No x coordinatess read.");
        }
      if ( !output->GetYCoordinates() || 
      output->GetYCoordinates()->GetNumberOfScalars() < 1 )
        {
        vtkWarningMacro(<<"No y coordinates read.");
        }
      if ( !output->GetZCoordinates() || 
      output->GetZCoordinates()->GetNumberOfScalars() < 1 )
        {
        vtkWarningMacro(<<"No z coordinates read.");
        }
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Reader.ReadInt(&npts))
      {
      vtkErrorMacro(<<"Cannot read point data!");
      this->Reader.CloseVTKFile ();
      return;
      }
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyord: " << line);
    }

  this->Reader.CloseVTKFile ();
}

void vtkRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRectilinearGridSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
