#include <ctype.h>

#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkSLCReader.h"

// Description:
// Constructor for a vtkSLCReader.
vtkSLCReader::vtkSLCReader()
{
  this->Filename = NULL;
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
  if ((fp = fopen(this->Filename, "r")) == NULL)
  {
    vtkErrorMacro(<< "File " << this->Filename << " not found");
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
  output->SetAspectRatio(f);

  // Skip Over unit_type, data_origin, and data_modification 
  fscanf( fp, "%d", &temp );
  fscanf( fp, "%d", &temp );
  fscanf( fp, "%d", &temp );

  fscanf( fp, "%d\n", &data_compression );

  plane_size = size[0] * size[1];
  volume_size = plane_size * size[2];
  newScalars = new vtkUnsignedCharScalars(volume_size);
//  newScalars = new vtkUnsignedShortScalars(volume_size);

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

        if( fread( scan_ptr, 1, plane_size, fp ) != plane_size )
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

        if( fread(compressed_ptr, 1, compressed_size, fp) != compressed_size )
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

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
}
