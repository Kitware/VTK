/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLCReader.cxx
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
#include <ctype.h>

#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkSLCReader.h"

// Description:
// Constructor for a vtkSLCReader.
vtkSLCReader::vtkSLCReader()
{
  this->FileName = NULL;
}

// Description:
// Decodes an array of eight bit run-length encoded data.
unsigned char* vtkSLCReader::Decode_8bit_data( unsigned char *in_ptr, 
					       int size )
{
  unsigned char           *curr_ptr;
  unsigned char           *decode_ptr;
  unsigned char           *return_ptr;
  unsigned char           current_value;
  unsigned char           remaining;

  curr_ptr = in_ptr;

  decode_ptr = return_ptr = new unsigned char[size];

  while( 1 )
  {
    current_value = *(curr_ptr++);

    if( !(remaining = (current_value & 0x7f)) )
      break;

    if( current_value & 0x80 )
    {
      while( remaining-- )
        *(decode_ptr++) = *(curr_ptr++);
    }
    else
    {
      current_value = *(curr_ptr++);
      while ( remaining-- )
          *(decode_ptr++) = current_value;
    }

  }

  return return_ptr;
}

// Description:
// Reads an SLC file and creates a vtkStructuredPoints dataset.
void vtkSLCReader::Execute()
{
  FILE *fp;

  vtkUnsignedCharScalars *newScalars;

//  vtkUnsignedShortScalars *newScalars;

  int	temp;
  int	data_compression;
  int	plane_size;
  int	volume_size;
  float f[3];
  int	size[3];
  int	magic_num;
  int	z_counter;
  int	icon_width, icon_height;
  int	voxel_count;
  int	compressed_size;
  int	i;

  unsigned char	*icon_ptr = NULL;
  unsigned char	*compressed_ptr = NULL;
  unsigned char	*scan_ptr = NULL;
  unsigned char	*sptr = NULL;

  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;

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
  newScalars = vtkUnsignedCharScalars::New();
  newScalars->SetNumberOfScalars(volume_size);

  // Skip Over Icon
  fscanf( fp, "%d %d X", &icon_width,  &icon_height );
  icon_ptr = new unsigned char[(icon_width*icon_height)];

  fread( icon_ptr, 1, (icon_width*icon_height), fp );
  fread( icon_ptr, 1, (icon_width*icon_height), fp );
  fread( icon_ptr, 1, (icon_width*icon_height), fp );

  delete icon_ptr;

  voxel_count = 0;

  // Read In Data Plane By Plane
  for( z_counter=0; z_counter<size[2]; z_counter++ )
  {
    // Read a single plane into temp memory
    switch( data_compression )
    {
      case 0:

        if( !scan_ptr )
          scan_ptr = new unsigned char[plane_size];

        if( fread( scan_ptr, 1, plane_size, fp ) != (unsigned int)plane_size )
	{
          vtkErrorMacro( << 
	    "Unable to read slice " << z_counter << " from SLC File" );
	  return;
	}

        break;

      case 1:

        if( scan_ptr )
          delete scan_ptr;

        fscanf( fp, "%d X", &compressed_size );

        compressed_ptr = new unsigned char[compressed_size];

        if( fread(compressed_ptr, 1, compressed_size, fp) != 
	    (unsigned int)compressed_size )
	{
          vtkErrorMacro( << "Unable to read compressed slice " << 
	    z_counter << " from SLC File" );
	  return;
	}

	scan_ptr = Decode_8bit_data( compressed_ptr, plane_size );

	delete compressed_ptr;

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
    //      newScalars->SetScalar( (z_counter*plane_size + i), (unsigned short)(*sptr++) );
    }
  }

  delete scan_ptr;

  vtkDebugMacro(<< "Read " << volume_size << " points");

  if( newScalars )
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }

  fclose( fp );
}

void vtkSLCReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
}
