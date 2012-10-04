/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLCReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSLCReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <ctype.h>

vtkStandardNewMacro(vtkSLCReader);

// Constructor for a vtkSLCReader.
vtkSLCReader::vtkSLCReader()
{
  this->FileName = NULL;
  this->Error = 0;
}

vtkSLCReader::~vtkSLCReader()
{
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
int vtkSLCReader::RequestInformation (
  vtkInformation       * request,
  vtkInformationVector** inputVector,
  vtkInformationVector * outputVector)
{
  FILE *fp;
  int   temp;
  double f[3];
  int   size[3];
  int   magic_num;
  this->Error = 1;

  if (!this->FileName)
    {
    vtkErrorMacro(<<"A FileName must be specified.");
    return 0;
    }

  // Initialize
  if ((fp = fopen(this->FileName, "rb")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
    }
  this->FileDimensionality = 3;
  if (fscanf( fp, "%d", &magic_num ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to read magic number");
    return 1;
    }
  if( magic_num != 11111 )
    {
    vtkErrorMacro(<< "SLC magic number is not correct");
    return 1;
    }

  f[0] = f[1] = f[2] = 0.0;
  this->SetDataOrigin(f);

  if (fscanf( fp, "%d", size ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to read size[0]");
    return 1;
    }
  if (fscanf( fp, "%d", size+1 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to read size[1]");
    return 1;
    }
  if (fscanf( fp, "%d", size+2 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to read size[2]");
    return 1;
    }
  this->SetDataExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);

  // Skip Over bits_per_voxel Field */
  if (fscanf( fp, "%d",   &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over bits per pixel");
    return 1;
    }

  if (fscanf( fp, "%lf", f ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to spacing[0]");
    return 1;
    }
  if (fscanf( fp, "%lf", f+1 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to spacing[1]");
    return 1;
    }
  if (fscanf( fp, "%lf", f+2 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to spacing[2]");
    return 1;
    }
  this->SetDataSpacing(f);

  // Skip Over unit_type, data_origin, and data_modification 
  if (fscanf( fp, "%d", &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over unit type");
    return 1;
    }
  if (fscanf( fp, "%d", &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over data origin");
    return 1;
    }
  if (fscanf( fp, "%d", &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over data modification");
    return 1;
    }

  this->SetDataScalarType(VTK_UNSIGNED_CHAR);
  this->SetNumberOfScalarComponents(1);

  fclose( fp );
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}
          
// Reads an SLC file and creates a vtkStructuredPoints dataset.
void vtkSLCReader::ExecuteData(vtkDataObject* )
{ 
  vtkImageData *output = this->GetOutput();
  
  output->SetExtent(output->GetWholeExtent());
  output->AllocateScalars();

  if (!output->GetPointData()->GetScalars())
    {
    return;
    }
  output->GetPointData()->GetScalars()->SetName("SLCImage");

  FILE *fp;

  int   temp;
  int   data_compression;
  int   plane_size;
  double f[3];
  int   size[3];
  int   magic_num;
  int   z_counter;
  int   icon_width, icon_height;
  int   compressed_size;

  unsigned char *icon_ptr;
  unsigned char *compressed_ptr;
  unsigned char *scan_ptr = NULL;

  this->Error = 1;
  
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

  if (fscanf( fp, "%d", &magic_num ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to read magic number");
    return;
    }
  if( magic_num != 11111 )
    {
    vtkErrorMacro(<< "SLC magic number is not correct");
    return;
    }

  f[0] = f[1] = f[2] = 0.0;
  output->SetOrigin(f);

  if (fscanf( fp, "%d", size ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed read size[0]");
    return;
    }
  if (fscanf( fp, "%d", size+1 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed read size[1]");
    return;
    }
  if (fscanf( fp, "%d", size+2 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed read size[2]");
    return;
    }
  output->SetDimensions(size);

  // Skip Over bits_per_voxel Field */
  if (fscanf( fp, "%d",   &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over bits per voxel");
    return;
    }
  if (fscanf( fp, "%lf", f ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed read spacing[0]");
    return;
    }
  if (fscanf( fp, "%lf", f+1 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed read spacing[1]");
    return;
    }
  if (fscanf( fp, "%lf", f+2 ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed read spacing[2]");
    return;
    }
  output->SetSpacing(f);

  // Skip Over unit_type, data_origin, and data_modification 
  if (fscanf( fp, "%d", &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over unit type");
    return;
    }
  if (fscanf( fp, "%d", &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over data origin");
    return;
    }
  if (fscanf( fp, "%d", &temp ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over data modification");
    return;
    }

  if (fscanf( fp, "%d\n", &data_compression ) != 1)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to read data compression");
    return;
    }

  plane_size = size[0] * size[1];
#ifndef NDEBUG
  int   volume_size = plane_size * size[2];
#endif

  // Skip Over Icon
  if (fscanf( fp, "%d %d X", &icon_width,  &icon_height ) != 2)
    {
    vtkErrorMacro(
      <<"Error reading file: " << this->FileName
      << "Failed to skip over icon");
    return;
    }
  icon_ptr = new unsigned char[(icon_width*icon_height)];

  if (fread( icon_ptr, (icon_width*icon_height), 1, fp ) != 1)
    {
    vtkErrorMacro ("SLCReader error reading file: " << this->FileName
                   << " Premature EOF while reading icon.");
    return;
    }
  if (fread( icon_ptr, (icon_width*icon_height), 1, fp ) != 1)
    {
    vtkErrorMacro ("SLCReader error reading file: " << this->FileName
                   << " Premature EOF while reading icon.");
    return;
    }
  if (fread( icon_ptr, (icon_width*icon_height), 1, fp ) != 1)
    {
    }

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
          return;
          }

        break;

      case 1:

        if( scan_ptr )
          {
          delete [] scan_ptr;
          }

        if (fscanf( fp, "%d X", &compressed_size ) != 1)
          {
          vtkErrorMacro(
            <<"Error reading file: " << this->FileName
            << "Failed to read compressed size");
          return;
          }

        compressed_ptr = new unsigned char[compressed_size];

        if( fread(compressed_ptr, 1, compressed_size, fp) != 
            (unsigned int)compressed_size )
          {
          vtkErrorMacro( << "Unable to read compressed slice " << 
            z_counter << " from SLC File" );
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
    void* outputSlice = output->GetScalarPointer(0, 0, z_counter);
    memcpy(outputSlice, scan_ptr, plane_size);
    }

  delete [] scan_ptr;

  vtkDebugMacro(<< "Read " << volume_size << " points");

  fclose( fp );
  this->Error = 0;
}

int vtkSLCReader::CanReadFile(const char* fname)
{
  FILE* fp;
  int   magic_num = 0;
  if ((fp = fopen(fname, "rb")) == NULL)
    {
    return 0;
    }

  if (fscanf( fp, "%d", &magic_num ) != 1)
    {
    fclose(fp);
    return 0;
    }
  if( magic_num != 11111 )
    {
    fclose(fp);
    return 0;
    }
  fclose(fp);
  return 3;
}


void vtkSLCReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Error: " << this->Error << "\n";
  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
}
