/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkUnstructuredGridReader.h"
#include "vtkByteSwap.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkUnstructuredGridReader* vtkUnstructuredGridReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkUnstructuredGridReader");
  if(ret)
    {
    return (vtkUnstructuredGridReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkUnstructuredGridReader;
}




#ifdef read
#undef read
#endif

vtkUnstructuredGridReader::vtkUnstructuredGridReader()
{
  this->Reader = vtkDataReader::New();
  this->Reader->SetSource(this);
  this->Recursing = 0;
}

vtkUnstructuredGridReader::~vtkUnstructuredGridReader()
{
  this->Reader->Delete();
}

unsigned long int vtkUnstructuredGridReader::GetMTime()
{
  unsigned long dtime = this->vtkSource::GetMTime();
  unsigned long rtime = this->Reader->GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Specify file name of vtk polygonal data file to read.
void vtkUnstructuredGridReader::SetFileName(char *name) 
{
  this->Reader->SetFileName(name);
}
char *vtkUnstructuredGridReader::GetFileName() 
{
  return this->Reader->GetFileName();
}

// Get the type of file (ASCII or BINARY)
int vtkUnstructuredGridReader::GetFileType() 
{
  return this->Reader->GetFileType();
}

// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkUnstructuredGridReader::SetScalarsName(char *name) 
{
  this->Reader->SetScalarsName(name);
}
char *vtkUnstructuredGridReader::GetScalarsName() 
{
  return this->Reader->GetScalarsName();
}

// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkUnstructuredGridReader::SetVectorsName(char *name) 
{
  this->Reader->SetVectorsName(name);
}
char *vtkUnstructuredGridReader::GetVectorsName() 
{
  return this->Reader->GetVectorsName();
}

// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkUnstructuredGridReader::SetTensorsName(char *name) 
{
  this->Reader->SetTensorsName(name);
}
char *vtkUnstructuredGridReader::GetTensorsName() 
{
  return this->Reader->GetTensorsName();
}

// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkUnstructuredGridReader::SetNormalsName(char *name) 
{
  this->Reader->SetNormalsName(name);
}
char *vtkUnstructuredGridReader::GetNormalsName() 
{
  return this->Reader->GetNormalsName();
}

// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkUnstructuredGridReader::SetTCoordsName(char *name) 
{
  this->Reader->SetTCoordsName(name);
}
char *vtkUnstructuredGridReader::GetTCoordsName() 
{
  return this->Reader->GetTCoordsName();
}

// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkUnstructuredGridReader::SetLookupTableName(char *name) 
{
  this->Reader->SetLookupTableName(name);
}
char *vtkUnstructuredGridReader::GetLookupTableName() 
{
  return this->Reader->GetLookupTableName();
}

// Set the name of the field data to extract. If not specified, uses 
// first field data encountered in file.
void vtkUnstructuredGridReader::SetFieldDataName(char *name) 
{
  this->Reader->SetFieldDataName(name);
}
char *vtkUnstructuredGridReader::GetFieldDataName() 
{
  return this->Reader->GetFieldDataName();
}

void vtkUnstructuredGridReader::Execute()
{
  int i, numPts=0, numCells=0;
  char line[256];
  int npts, size, ncells;
  int piece, numPieces, skip1, read2, skip3, tmp;
  vtkCellArray *cells=NULL;
  int *types=NULL;
  int done=0;
  vtkUnstructuredGrid *output = this->GetOutput();

  // to test streaming
  //output->SetUpdateExtent(1, 2);

  vtkDebugMacro(<<"Reading vtk unstructured grid...");
  if ( this->Debug )
    {
    this->Reader->DebugOn();
    }
  else
    {
    this->Reader->DebugOff();
    }

  if (!this->Reader->OpenVTKFile() || !this->Reader->ReadHeader())
    {
    return;
    }

  // Read unstructured grid specific stuff
  //
  if (!this->Reader->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->Reader->CloseVTKFile ();
    return;
    }

  if ( !strncmp(this->Reader->LowerCase(line),"dataset",(unsigned long)7) )
    {
    // Make sure we're reading right type of geometry
    //
    if (!this->Reader->ReadString(line))
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->Reader->CloseVTKFile ();
      return;
      } 

    if ( strncmp(this->Reader->LowerCase(line),"unstructured_grid",17) )
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->Reader->CloseVTKFile ();
      return;
      }

    // Might find points, cells, and cell types
    //
    while (!done)
      {
      if (!this->Reader->ReadString(line))
	{
	break;
	}

      if ( ! strncmp(this->Reader->LowerCase(line),"points",6) )
        {
        if (!this->Reader->Read(&numPts))
          {
          vtkErrorMacro(<<"Cannot read number of points!");
          this->Reader->CloseVTKFile ();
          return;
          }

        if (!this->Reader->ReadPoints(output, numPts))
          {
          this->Reader->CloseVTKFile ();
          return;
          }
        }

      else if ( !strncmp(line,"cells",5))
        {
	output->GetUpdateExtent(piece, numPieces);
        if (!(this->Reader->Read(&ncells) && this->Reader->Read(&size)))
          {
          vtkErrorMacro(<<"Cannot read cells!");
          this->Reader->CloseVTKFile ();
          return;
          }

	// the number of ints to read befor we get to the piece.
	skip1 = piece * ncells / numPieces;
	// the number of ints to read as part of piece.
	read2 = ((piece+1) * ncells / numPieces) - skip1;
	// the number of ints after the piece
	skip3 = ncells - skip1 - read2;

        cells = vtkCellArray::New();
        if (!this->Reader->ReadCells(size, cells->WritePointer(read2,size),
				     skip1, read2, skip3) )
          {
          this->Reader->CloseVTKFile ();
          return;
          }
        if (cells && types)
	  {
	  output->SetCells(types, cells);
	  }
        }

      else if (!strncmp(line,"cell_types",10))
        {
	output->GetUpdateExtent(piece, numPieces);
        if (!this->Reader->Read(&ncells))
          {
          vtkErrorMacro(<<"Cannot read cell types!");
          this->Reader->CloseVTKFile ();
          return;
          }
	// the number of ints to read befor we get to the piece.
	skip1 = piece * ncells / numPieces;
	// the number of ints to read as part of piece.
	read2 = ((piece+1) * ncells / numPieces) - skip1;
	// the number of ints after the piece
	skip3 = ncells - skip1 - read2;

	//cerr << skip1 << " --- " << read2 << " --- " << skip3 << endl;
	// allocate array for piece cell types
        types = new int[read2];
        if (this->Reader->GetFileType() == VTK_BINARY)
          {
          // suck up newline
          this->Reader->GetIStream()->getline(line,256);
	  // skip
	  if (skip1 != 0)
	    {
	    this->Reader->GetIStream()
	      ->seekg((long)sizeof(int)*skip1, ios::cur);
	    }
          this->Reader->GetIStream()->read((char *)types,sizeof(int)*read2);
	  // skip
	  if (skip3 != 0)
	    {
	    this->Reader->GetIStream()
	      ->seekg((long)sizeof(int)*skip3, ios::cur);
	    }

          if (this->Reader->GetIStream()->eof())
            {
            vtkErrorMacro(<<"Error reading binary cell types!");
            this->Reader->CloseVTKFile ();
            return;
            }
          vtkByteSwap::Swap4BERange(types,read2);
          }
        else //ascii
          {
	  // skip types before piece
          for (i=0; i<skip1; i++)
            {
            if (!this->Reader->Read(&tmp))
              {
              vtkErrorMacro(<<"Error reading cell types!");
              this->Reader->CloseVTKFile ();
              return;
              }
            }
	  // read types for piece
          for (i=0; i<read2; i++)
            {
            if (!this->Reader->Read(types+i))
              {
              vtkErrorMacro(<<"Error reading cell types!");
              this->Reader->CloseVTKFile ();
              return;
              }
            }
	  // skip types after piece
          for (i=0; i<skip3; i++)
            {
            if (!this->Reader->Read(&tmp))
              {
              vtkErrorMacro(<<"Error reading cell types!");
              this->Reader->CloseVTKFile ();
              return;
              }
            }
          }
        if ( cells && types )
	  {
	  output->SetCells(types, cells);
	  }
        }

      else if ( ! strncmp(line, "cell_data", 9) )
        {
        if (!this->Reader->Read(&numCells))
          {
          vtkErrorMacro(<<"Cannot read cell data!");
          this->Reader->CloseVTKFile ();
          return;
          }
        
        if ( ncells != numCells )
          {
          vtkErrorMacro(<<"Number of cells don't match!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadCellData(output, ncells);
        break; //out of this loop
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if (!this->Reader->Read(&npts))
          {
          vtkErrorMacro(<<"Cannot read point data!");
          this->Reader->CloseVTKFile ();
          return;
          }
        
        if ( npts != numPts )
          {
          vtkErrorMacro(<<"Number of points don't match!");
          this->Reader->CloseVTKFile ();
          return;
          }

        this->Reader->ReadPointData(output, npts);
        break; //out of this loop
        }

      else
        {
        vtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->Reader->CloseVTKFile ();
        return;
        }
      }
      if ( ! output->GetPoints() ) vtkWarningMacro(<<"No points read!");
      if ( ! (cells && types) )  vtkWarningMacro(<<"No topology read!");
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    vtkWarningMacro(<<"No geometry defined in data file!");
    if (!this->Reader->Read(&numPts))
      {
      vtkErrorMacro(<<"Cannot read point data!");
      this->Reader->CloseVTKFile ();
      return;
      }

    this->Reader->ReadPointData(output, numPts);
    }

  else 
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }

  // Clean-up and get out
  //
  if (types)
    {
    delete [] types;
    }
  if (cells)
    {
    cells->Delete();
    }

  vtkDebugMacro(<<"Read " <<output->GetNumberOfPoints() <<" points," 
                <<output->GetNumberOfCells() <<" cells.\n");

  this->Reader->CloseVTKFile ();
  return;
}

void vtkUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  // the reader ivar's source will be this Reader-> we must do this to prevent 
  // infinite printing
  if (!this->Recursing)
    { 
    vtkUnstructuredGridSource::PrintSelf(os,indent);
    this->Recursing = 1;
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  this->Recursing = 0;
}
