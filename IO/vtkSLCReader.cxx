/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLCReader.cxx
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
#include <ctype.h>

#include "vtkSLCReader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSLCReader* vtkSLCReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSLCReader");
  if(ret)
    {
    return (vtkSLCReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSLCReader;
}




// Constructor for a vtkSLCReader.
vtkSLCReader::vtkSLCReader()
{
  this->FileName = NULL;
  this->Error = 0;
}

vtkSLCReader::~vtkSLCReader()
  {
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  }

// Decodes an array of eight bit run-length encoded data.
unsigned char* vtkSLCReader::Decode8BitData( unsigned char *in_ptr, 
                                               int size )
{
  unsigned char           *curr_ptr;
  unsigned char           *decode_ptr;
  unsigned char           *return_ptr;
  unsigned char           current_value;
  unsigned char           remaining;
  int done=0;
  
  curr_ptr = in_ptr;

  decode_ptr = return_ptr = new unsigned char[size];

  while( !done )
  {
    current_value = *(curr_ptr++);

    if( !(remaining = (current_value & 0x7f)) )
      {
      break;
      }

    if( current_value & 0x80 )
    {
      while( remaining-- )
        {
        *(decode_ptr++) = *(curr_ptr++);
        }
    }
    else
    {
      current_value = *(curr_ptr++);
      while ( remaining-- )
        {
        *(decode_ptr++) = current_value;
        }
    }

  }

  return return_ptr;
}


// This will be needed when we make this an imaging filter.
void vtkSLCReader::ExecuteInformation()
{
  FILE *fp;
  int   temp;
  float f[3];
  int   size[3];
  int   magic_num;

  this->Error = 1;
  vtkStructuredPoints *output = this->GetOutput();

  if (!this->FileName)
    {
    vtkErrorMacro(<<"A FileName must be specified.");
    return;
    }

  // Initialize
  if ((fp = fopen(this->FileName, "rb")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }

  fscanf( fp, "%d", &magic_num );
  if( magic_num != 11111 )
    {
    vtkErrorMacro(<< "SLC magic number is not correct");
    return;
    }

  f[0] = f[1] = f[2] = 0.0;
  output->SetOrigin(f);

  fscanf( fp, "%d", size );
  fscanf( fp, "%d", size+1 );
  fscanf( fp, "%d", size+2 );
  output->SetWholeExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);

  // Skip Over bits_per_voxel Field */
  fscanf( fp, "%d",   &temp );

  fscanf( fp, "%f", f );
  fscanf( fp, "%f", f+1 );
  fscanf( fp, "%f", f+2 );
  output->SetSpacing(f);

  // Skip Over unit_type, data_origin, and data_modification 
  fscanf( fp, "%d", &temp );
  fscanf( fp, "%d", &temp );
  fscanf( fp, "%d", &temp );

  output->SetScalarType(VTK_UNSIGNED_CHAR);
  output->SetNumberOfScalarComponents(1);

  fclose( fp );
}
	  
// Reads an SLC file and creates a vtkStructuredPoints dataset.
void vtkSLCReader::Execute()
{
  FILE *fp;

  vtkScalars *newScalars;

  int   temp;
  int   data_compression;
  int   plane_size;
  int   volume_size;
  float f[3];
  int   size[3];
  int   magic_num;
  int   z_counter;
  int   icon_width, icon_height;
  int   compressed_size;
  int   i;

  unsigned char *icon_ptr = NULL;
  unsigned char *compressed_ptr = NULL;
  unsigned char *scan_ptr = NULL;
  unsigned char *sptr = NULL;

  this->Error = 1;
  vtkStructuredPoints *output = this->GetOutput();
  
  if (!this->FileName)
    {
    vtkErrorMacro(<<"A FileName must be specified.");
    return;
    }

  // Initialize
  if ((fp = fopen(this->FileName, "rb")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }

  fscanf( fp, "%d", &magic_num );
  if( magic_num != 11111 )
    {
    vtkErrorMacro(<< "SLC magic number is not correct");
    return;
    }

  f[0] = f[1] = f[2] = 0.0;
  output->SetOrigin(f);

  fscanf( fp, "%d", size );
  fscanf( fp, "%d", size+1 );
  fscanf( fp, "%d", size+2 );
  output->SetDimensions(size);

  // Skip Over bits_per_voxel Field */
  fscanf( fp, "%d",   &temp );

  fscanf( fp, "%f", f );
  fscanf( fp, "%f", f+1 );
  fscanf( fp, "%f", f+2 );
  output->SetSpacing(f);

  // Skip Over unit_type, data_origin, and data_modification 
  fscanf( fp, "%d", &temp );
  fscanf( fp, "%d", &temp );
  fscanf( fp, "%d", &temp );

  fscanf( fp, "%d\n", &data_compression );

  plane_size = size[0] * size[1];
  volume_size = plane_size * size[2];
  newScalars = vtkScalars::New(VTK_UNSIGNED_CHAR,1);
  newScalars->SetNumberOfScalars(volume_size);

  // Skip Over Icon
  fscanf( fp, "%d %d X", &icon_width,  &icon_height );
  icon_ptr = new unsigned char[(icon_width*icon_height)];

  fread( icon_ptr, 1, (icon_width*icon_height), fp );
  fread( icon_ptr, 1, (icon_width*icon_height), fp );
  fread( icon_ptr, 1, (icon_width*icon_height), fp );

  delete [] icon_ptr;

  // Read In Data Plane By Plane
  for( z_counter=0; z_counter<size[2]; z_counter++ )
    {
    if ( !(z_counter % 10) && !z_counter )
      {
      this->UpdateProgress((float)z_counter/size[2]);
      }

    // Read a single plane into temp memory
    switch( data_compression )
      {
      case 0:

        if( !scan_ptr )
          {
          scan_ptr = new unsigned char[plane_size];
          }

        if( fread( scan_ptr, 1, plane_size, fp ) != (unsigned int)plane_size )
	  {
          vtkErrorMacro( << 
            "Unable to read slice " << z_counter << " from SLC File" );
          newScalars->Delete();
          return;
	  }

        break;

      case 1:

        if( scan_ptr )
	  {
          delete [] scan_ptr;
	  }

        fscanf( fp, "%d X", &compressed_size );

        compressed_ptr = new unsigned char[compressed_size];

        if( fread(compressed_ptr, 1, compressed_size, fp) != 
            (unsigned int)compressed_size )
	  {
          vtkErrorMacro( << "Unable to read compressed slice " << 
            z_counter << " from SLC File" );
          newScalars->Delete();
          return;
	  }

        scan_ptr = this->Decode8BitData( compressed_ptr, plane_size );
	delete [] compressed_ptr;

        break;
      default:
        vtkErrorMacro(<< "Unknown SLC compression type: " << 
          data_compression );
        break;
      }

    sptr = scan_ptr;

    // Copy plane into volume
    for( i=0; i<plane_size; i++ )
      {
      newScalars->SetScalar( (z_counter*plane_size + i), *sptr++ );
      }
    }

  delete [] scan_ptr;

  vtkDebugMacro(<< "Read " << volume_size << " points");

  if( newScalars )
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }

  fclose( fp );
  this->Error = 0;
}

void vtkSLCReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Error: " << this->Error << "\n";
  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
}
