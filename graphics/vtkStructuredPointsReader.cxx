/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkStructuredPointsReader.h"

vtkStructuredPointsReader::vtkStructuredPointsReader()
{
}

unsigned long int vtkStructuredPointsReader::GetMTime()
{
  unsigned long dtime = this->vtkSource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vtk structured points data file to read.
void vtkStructuredPointsReader::SetFileName(char *name) 
{
  this->Reader.SetFileName(name);
}
char *vtkStructuredPointsReader::GetFileName() 
{
  return this->Reader.GetFileName();
}

// Description:
// Get the type of file (VTK_ASCII or VTK_BINARY).
int vtkStructuredPointsReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkStructuredPointsReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vtkStructuredPointsReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkStructuredPointsReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vtkStructuredPointsReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkStructuredPointsReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vtkStructuredPointsReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkStructuredPointsReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vtkStructuredPointsReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkStructuredPointsReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vtkStructuredPointsReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkStructuredPointsReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vtkStructuredPointsReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}

void vtkStructuredPointsReader::Execute()
{
  int numPts=0;
  char line[256];
  int npts;
  int dimsRead=0, arRead=0, originRead=0;
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;
  

  vtkDebugMacro(<<"Reading vtk structured points file...");
  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if (!this->Reader.OpenVTKFile() || !this->Reader.ReadHeader())
      return;
//
// Read structured points specific stuff
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

    if ( strncmp(this->Reader.LowerCase(line),"structured_points",17) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader.CloseVTKFile ();
      return;
      }
//
// Read keyword and number of points
//
    numPts = output->GetNumberOfPoints(); // get default
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

      else if ( !strncmp(line,"aspect_ratio",12) || !strncmp(line,"spacing",7) )
        {
        float ar[3];
        if (!(this->Reader.ReadFloat(ar) && 
	      this->Reader.ReadFloat(ar+1) && 
	      this->Reader.ReadFloat(ar+2)))
          {
          vtkErrorMacro(<<"Error reading spacing!");
          this->Reader.CloseVTKFile ();
          return;
          }

        output->SetSpacing(ar);
        arRead = 1;
        }

      else if ( ! strncmp(line,"origin",6) )
        {
        float origin[3];
        if (!(this->Reader.ReadFloat(origin) && 
	      this->Reader.ReadFloat(origin+1) && 
	      this->Reader.ReadFloat(origin+2)))
          {
          vtkErrorMacro(<<"Error reading origin!");
          this->Reader.CloseVTKFile ();
          return;
          }

        output->SetOrigin(origin);
        originRead = 1;
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
          vtkErrorMacro(<<"Number of points don't match data values!");
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
      if ( !arRead ) vtkWarningMacro(<<"No spacing read.");
      if ( !originRead ) vtkWarningMacro(<<"No origin read.");
    }

  else if ( !strncmp(line,"point_data",10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Reader.ReadInt(&npts))
      {
      vtkErrorMacro(<<"Cannot read point data!");
      this->Reader.CloseVTKFile ();
      return;
      }
    this->Reader.ReadPointData(output, numPts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyord: " << line);
    }
  this->Reader.CloseVTKFile ();
}

void vtkStructuredPointsReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
