#include <math.h>

#include "vtkNormalEncoder.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkShortScalars.h"
#include "vtkIntScalars.h"
#include "vtkFloatScalars.h"

// Description:
// This is the templated function that actually computes the EncodedNormal
// and the GradientMagnitude
template <class T>
static void ComputeGradients( vtkNormalEncoder *encoder, T *data_ptr,
			      int thread_id, int thread_count )
{
  int            xstep, ystep, zstep;
  int            x, y, z;
  int            norm_size;
  int            offset;
  int            z_start, z_limit;
  T              *dptr;
  unsigned char  *gptr;
  unsigned short *nptr;
  float          nx, ny, nz, t;
  float          gvalue;

  // Compute steps through the volume in x, y, and z
  xstep = 1;
  ystep = encoder->ScalarInputSize[0];
  zstep = encoder->ScalarInputSize[0] * encoder->ScalarInputSize[1];

  // Compute the size of the normal grid - the large grid size is
  // NORM_SQR_SIZE * NORM_SQR_SIZE and the small grid of vertices
  // between each 4 of the large grid is ( NORM_SQR_SIZE - 1 ) * 
  // ( NORM_SQR_SIZE - 1 ).  The first norm_size normal indices have
  // normals with z components that are >= 0, and the normals with
  // indices from norm_size to 2*norm_size - 1 have z components
  // that are <= 0.  (There are NORM_SQR_SIZE * 4 - 4 duplicate
  // normals that have z components = 0 and are in both sets of
  // normals)
  norm_size = NORM_SQR_SIZE * NORM_SQR_SIZE + 
	( NORM_SQR_SIZE - 1 ) * ( NORM_SQR_SIZE - 1 );

  // Compute an offset based on the thread_id. The volume will
  // be broken into large slabs (thread_count slabs). For this thread
  // we need to access the correct slab. Also compute the z plane that
  // this slab starts on, and the z limit of this slab (one past the
  // end of the slab)
  z_start = (int)(( (float)thread_id / (float)thread_count ) *
		  encoder->ScalarInputSize[2] );
  offset = z_start * encoder->ScalarInputSize[0] * encoder->ScalarInputSize[1];
  z_limit = (int)(( (float)(thread_id + 1) / (float)thread_count ) *
		  encoder->ScalarInputSize[2] );

  // Make sure out z_limit didn't get too big - it shouldn't so print an
  // error message if this happens and return
  if ( z_limit > encoder->ScalarInputSize[2] )
    {
    return;
    }


  // Set some pointers
  dptr = data_ptr + offset;
  nptr = encoder->EncodedNormal + offset;
  gptr = encoder->GradientMagnitude + offset;

  // Loop through all the data and compute the encoded normal and
  // gradient magnitude for each scalar location
  for ( z = z_start; z < z_limit; z++ )
    for ( y = 0; y < encoder->ScalarInputSize[1]; y++ )
      for ( x = 0; x < encoder->ScalarInputSize[0]; x++ )
	{
	// Use a central difference method if possible,
	// otherwise use a forward or backward difference if
	// we are on the edge

	// Compute the X component
	if ( x > 0 && x < encoder->ScalarInputSize[0] - 1 )
	  nx = (float)*(dptr-xstep) - (float)*(dptr+xstep); 
	else if ( x == 0 )
	  nx = -((float)*(dptr+xstep));
	else
	  nx =  ((float)*(dptr-xstep));
	
	// Compute the Y component
	if ( y > 0 && y < encoder->ScalarInputSize[1] - 1 )
	  ny = (float)*(dptr-ystep) - (float)*(dptr+ystep); 
	else if ( y == 0 )
	  ny = -((float)*(dptr+ystep));
	else
	  ny =  ((float)*(dptr-ystep));
	
	// Compute the Z component
	if ( z > 0 && z < encoder->ScalarInputSize[2] - 1 )
	  nz = (float)*(dptr-zstep) - (float)*(dptr+zstep); 
	else if ( z == 0 )
	  nz = -((float)*(dptr+zstep));
	else
	  nz =  ((float)*(dptr-zstep));

	// Take care of the aspect ratio of the data
	// Scaling in the vtkVolume is isotropic, so this is the
	// only place we have to worry about non-isotropic scaling.
	nx *= encoder->ScalarInputAspect[1] * encoder->ScalarInputAspect[2];
	ny *= encoder->ScalarInputAspect[0] * encoder->ScalarInputAspect[2];
	nz *= encoder->ScalarInputAspect[0] * encoder->ScalarInputAspect[1];
	
	// Compute the gradient magnitude
	t = sqrt( (double)( nx*nx + ny*ny + nz*nz ) ) / 2.0;
	
	// Encode this into an 8 bit value - this method is changing
	// in the near future
	gvalue = 
	  255.0 * ( t - encoder->GradientMagnitudeRange[0] ) / 
	  ( encoder->GradientMagnitudeRange[1] - 
	    encoder->GradientMagnitudeRange[0] );
	  
	if ( gvalue < 0.0 )
	  *gptr = 0;
	else if ( gvalue > 255.0 )
	  *gptr = 255;
	else 
	  *gptr = (unsigned char) gvalue;
	
	// Normalize the gradient direction
	if ( t )
	  {
	  nx /= t;
	  ny /= t;
	  nz /= t;
	  }

	// Convert the gradient direction into an encoded index value
	// This is done by computing the (x,y) grid position of this 
	// normal in the 2*NORM_SQR_SIZE - 1 grid, then passing this
	// through the IndexTable to look up the 16 bit index value
	if (  fabs((double)nz) + fabs((double)nx) + fabs((double)ny) )
	  {
	  t = 1.0 / ( fabs((double)nz) + fabs((double)nx) + 
		      fabs((double)ny) );
	  
	  nx *= t;
	  ny *= t;
	  }
	
	*nptr = encoder->IndexTable
	  [(int)((float)(nx+1 + 1.0 / (float)(2*(NORM_SQR_SIZE-1))) * 
		 (float)(NORM_SQR_SIZE-1))]
	  [(int)((float)(ny+1 + 1.0 / (float)(2*(NORM_SQR_SIZE-1))) * 
		 (float)(NORM_SQR_SIZE-1))];
	
	// If the z component is less than 0.0, add norm_size to the
	// index 
	if ( nz < 0.0 ) *nptr += norm_size;

	nptr++;
	gptr++;
	dptr++;

	}
}

// Description:
// Construct a vtkNormalEncoder with initial values of NULL for
// the ScalarInput, EncodedNormal, and GradientMagnitude. Also,
// indicate that the IndexTable has not yet been initialized. The
// GradientMagnitudeRange and the GradientMangitudeTable are 
// initialized to default values - these will change in the future
// when magnitude of gradient opacities are included
vtkNormalEncoder::vtkNormalEncoder()
{
  int i;

  this->ScalarInput                = NULL;
  this->EncodedNormal              = NULL;
  this->GradientMagnitude          = NULL;
  this->GradientMagnitudeRange[0]  = 0.0;
  this->GradientMagnitudeRange[1]  = 256.0;
  this->IndexTableInitialized      = 0;
  this->ThreadCount                = this->Threader.GetThreadCount();

  for ( i = 0; i < 256; i++ )
    this->GradientMagnitudeTable[i] = this->GradientMagnitudeRange[0] + 
	(float)i/ 256.0 * ( this->GradientMagnitudeRange[1] - 
	  this->GradientMagnitudeRange[0] );
}

// Description:
// Destruct a vtkNormalEncoder - free up any memory used
vtkNormalEncoder::~vtkNormalEncoder()
{
  if ( this->EncodedNormal )
    delete this->EncodedNormal;

  if ( this->GradientMagnitude )
    delete this->GradientMagnitude;
}

// Description:
// Set the gradient magnitude range - this will disappear soon
void vtkNormalEncoder::SetGradientMagnitudeRange( float v1, float v2 )
{
  int i;
 
  this->GradientMagnitudeRange[0] = v1;
  this->GradientMagnitudeRange[1] = v2;
  this->Modified();
  
  for ( i = 0; i < 256; i++ )
    this->GradientMagnitudeTable[i] = this->GradientMagnitudeRange[0] + 
	(float)i/ 256.0 * ( this->GradientMagnitudeRange[1] - 
	  this->GradientMagnitudeRange[0] );
}

// Description:
// Initialize the index table.  This is a 2*NORM_SQR_SIZE - 1 by
// 2*NORM_SQR_SIZE - 1 entry table that maps (x,y) grid position to
// encoded normal index.  The grid position is obtained by starting
// with an octahedron (comprised of 8 triangles forming a double
// pyramid). Each triangle is then replaced by 4 triangles by joining
// edge midpoints.  This is done recursively until NORM_SQR_SIZE
// vertices exist on each original edge. If you "squish" this octahedron,
// it will look like a diamond.  Then rotate it 45 degrees, it will
// look like a square.  Then look at the pattern of vertices - there
// is a NORM_SQR_SIZE by NORM_SQR_SIZE grid, with a (NORM_SQR_SIZE-1) by
// NORM_SQR_SIZE - 1 grid inside of it.  The vertices all fall on
// (x,y) locatiions in a grid that is 2*NORM_SQR_SIZE - 1 by
// 2*NORM_SQR_SIZE - 1, although not every (x,y) location has a vertex.
void vtkNormalEncoder::InitializeIndexTable( void )
{
  int     i, j, index, max_index;
  float   x, y, z, tmp_x, tmp_y;
  float   norm;

  // Initialize the index
  index = 0;

  // max_index indicates the largest index we will get - the number
  // of vertices in the two-grid square.  This represents half the
  // normals, and max_index is used to offset from one half into the
  // other.  One half of the normals have z components >= 0, and the
  // second half (all with indices above max_index) have z components
  // that are <= 0.
  max_index =  NORM_SQR_SIZE * NORM_SQR_SIZE + 
               (NORM_SQR_SIZE - 1) * (NORM_SQR_SIZE - 1);

  // The outer loop is for 2*NORM_SQR_SIZE - 1 rows
  for ( i = 0; i <= (NORM_SQR_SIZE-1)*2; i++ )
    {
    // Compute the y component for this row
    tmp_y = (float)(2*i)/(float)(NORM_SQR_SIZE*2 - 1) - 1.0;

    // On the odd rows, we are doing the small grid which has
    // NORM_SQR_SIZE - 1 elements in it
    if ( i%2 )
      {
      for ( j = 0; j < NORM_SQR_SIZE - 1; j++ )
	{
	// compute the x component for this column
        tmp_x = (float)(2*j)/(float)(NORM_SQR_SIZE-1) - 
	  1.0 + (1.0/(float)(NORM_SQR_SIZE-1));

	// rotate by 45 degrees
        x = 0.5 * tmp_x - 0.5 * tmp_y;
        y = 0.5 * tmp_x + 0.5 * tmp_y;

	// compute the z based on the x and y values
        if ( x >= 0 && y >= 0 )
          z = 1.0 - x - y;
        else if ( x >= 0 && y < 0 )
          z = 1.0 - x + y;
        else if ( x < 0 && y < 0 )
          z = 1.0 + x + y;
        else 
          z = 1.0 + x - y;

	// Normalize this direction and set the DecodedNormal table for
	// this index to this normal.  Also set the corresponding 
	// entry for this normal with a negative z component
        norm = sqrt( (double)( x*x + y*y + z*z ) );
        this->DecodedNormal[3*index + 0] = x / norm;
        this->DecodedNormal[3*index + 1] = y / norm;
        this->DecodedNormal[3*index + 2] = z / norm;
        this->DecodedNormal[3*(index+max_index) + 0] =   x / norm;
        this->DecodedNormal[3*(index+max_index) + 1] =   y / norm;
        this->DecodedNormal[3*(index+max_index) + 2] = -(z / norm);

	// For this x,y grid location, set the index
	// The grid location ranges between 0 and 2*NORM_SQR_SIZE - 1
	// in both x and y
        this->IndexTable
	  [(int)((float)(x + 1 + 1.0 / 
			 (float)(2*(NORM_SQR_SIZE-1)))*
			 (float)(NORM_SQR_SIZE-1))] 
	  [(int)((float)(y + 1 + 1.0 / 
			 (float)(2*(NORM_SQR_SIZE-1)))*
			 (float)(NORM_SQR_SIZE-1))] 
	    = index;

	// Increment the index
        index++;
	}
      }
    // On the even rows we are doing the big grid which has
    // NORM_SQR_SIZE elements in it
    else
      {
      for ( j = 0; j < NORM_SQR_SIZE; j++ )
	{
	// compute the x component for this column
        tmp_x = (float)(2*j)/(float)(NORM_SQR_SIZE-1) - 1.0;

	// rotate by 45 degrees
        x = 0.5 * tmp_x - 0.5 * tmp_y;
        y = 0.5 * tmp_x + 0.5 * tmp_y;

	// compute the z based on the x and y values
        if ( x >= 0 && y >= 0 )
          z = 1.0 - x - y;
        else if ( x >= 0 && y < 0 )
          z = 1.0 - x + y;
        else if ( x < 0 && y < 0 )
          z = 1.0 + x + y;
        else 
          z = 1.0 + x - y;

	// Normalize this direction and set the DecodedNormal table for
	// this index to this normal.  Also set the corresponding 
	// entry for this normal with a negative z component
        norm = sqrt( (double)( x*x + y*y + z*z ) );
        this->DecodedNormal[3*index + 0] = x / norm;
        this->DecodedNormal[3*index + 1] = y / norm;
        this->DecodedNormal[3*index + 2] = z / norm;
        this->DecodedNormal[3*(index+max_index) + 0] =   x / norm;
        this->DecodedNormal[3*(index+max_index) + 1] =   y / norm;
        this->DecodedNormal[3*(index+max_index) + 2] = -(z / norm);

	// For this x,y grid location, set the index
	// The grid location ranges between 0 and 2*NORM_SQR_SIZE - 1
	// in both x and y
        this->IndexTable
	  [(int)((float)(x + 1 + 1.0 / 
			 (float)(2*(NORM_SQR_SIZE-1)))*
			 (float)(NORM_SQR_SIZE-1))] 
	  [(int)((float)(y + 1 + 1.0 / 
			 (float)(2*(NORM_SQR_SIZE-1)))*
			 (float)(NORM_SQR_SIZE-1))] 
	    = index;

	// Increment the index
        index++;
	}
      }
    }

  this->IndexTableInitialized = 1;
}

// Description:
// This method is used to compute the encoded normal and the
// magnitude of the gradient for each voxel location in the 
// ScalarInput.
void vtkNormalEncoder::UpdateNormals( )
{
  int                scalar_input_size[3];
  float              scalar_input_aspect[3];

  // If we haven't initialized the index table yet, we should do that now
  if ( !this->IndexTableInitialized )
    this->InitializeIndexTable();

  // Get the dimensions of the data and its aspect ration
  this->ScalarInput->GetDimensions( scalar_input_size );
  this->ScalarInput->GetAspectRatio( scalar_input_aspect );

  // If we previously have allocated space for the encoded normals,
  // and this space is no longer the right size, delete it
  if ( this->EncodedNormal &&
	( this->EncodedNormalSize[0] != scalar_input_size[0] ||
	  this->EncodedNormalSize[1] != scalar_input_size[1] ||
          this->EncodedNormalSize[2] != scalar_input_size[2] ) )
    {
    delete this->EncodedNormal;
    this->EncodedNormal = NULL;
    }

  // Allocate space for the encoded normals if necessary
  if ( !this->EncodedNormal )
    {
    this->EncodedNormal = new unsigned short[ scalar_input_size[0] *
					      scalar_input_size[1] *
 					      scalar_input_size[2] ];
    this->EncodedNormalSize[0] = scalar_input_size[0];
    this->EncodedNormalSize[1] = scalar_input_size[1];
    this->EncodedNormalSize[2] = scalar_input_size[2];
    }

  // If we previously have allocated space for the gradient magnitudes,
  // and this space is no longer the right size, delete it
  if ( this->GradientMagnitude &&
	( this->GradientMagnitudeSize[0] != scalar_input_size[0] ||
	  this->GradientMagnitudeSize[1] != scalar_input_size[1] ||
          this->GradientMagnitudeSize[2] != scalar_input_size[2] ) )
    {
    delete this->GradientMagnitude;
    this->GradientMagnitude = NULL;
    }

  // Allocate space for the encoded normals if necessary
  if ( !this->GradientMagnitude )
    {
    this->GradientMagnitude = new unsigned char[ scalar_input_size[0] *
					         scalar_input_size[1] *
 					         scalar_input_size[2] ];
    this->GradientMagnitudeSize[0] = scalar_input_size[0];
    this->GradientMagnitudeSize[1] = scalar_input_size[1];
    this->GradientMagnitudeSize[2] = scalar_input_size[2];
    }

  // Copy info that multi threaded function will need into temp variables
  memcpy( this->ScalarInputSize, scalar_input_size, 3 * sizeof(int) );
  memcpy( this->ScalarInputAspect, scalar_input_aspect, 3 * sizeof(float) );

  this->Threader.SetThreadCount( this->ThreadCount );

  this->Threader.SetSingleMethod( SwitchOnDataType,
				  (vtkObject *)this );

  this->Threader.SingleMethodExecute();

}

VTK_THREAD_RETURN_TYPE SwitchOnDataType( void *arg )
{
  vtkNormalEncoder   *encoder;
  int                thread_count;
  int                thread_id;
  unsigned char      *uc_data_ptr;
  unsigned short     *us_data_ptr;
  short              *s_data_ptr;
  int                *i_data_ptr;
  float              *f_data_ptr;
  char               *data_type;

  thread_id = ((ThreadInfoStruct *)(arg))->ThreadID;
  thread_count = ((ThreadInfoStruct *)(arg))->ThreadCount;
  encoder = (vtkNormalEncoder *)(((ThreadInfoStruct *)(arg))->UserData);

  // Find the data type of the ScalarInput and call the correct 
  // templated function to actually compute the normals and magnitudes
  data_type = 
    encoder->ScalarInput->GetPointData()->GetScalars()->GetDataType();

  if ( strcmp( data_type, "unsigned char" ) == 0 )
    {
    uc_data_ptr = ((vtkUnsignedCharScalars *)
      (encoder->ScalarInput->GetPointData()->GetScalars()))->GetPtr(0);
    ComputeGradients( encoder, uc_data_ptr, thread_id, thread_count );
    }
  else if ( strcmp( data_type, "unsigned short" ) == 0 )
    {
    us_data_ptr = ((vtkUnsignedShortScalars *)
      (encoder->ScalarInput->GetPointData()->GetScalars()))->GetPtr(0);
    ComputeGradients( encoder, us_data_ptr, thread_id, thread_count );
    }
  else if ( strcmp( data_type, "short" ) == 0 )
    {
    s_data_ptr = ((vtkShortScalars *)
      (encoder->ScalarInput->GetPointData()->GetScalars()))->GetPtr(0);
    ComputeGradients( encoder, s_data_ptr, thread_id, thread_count );
    }
  else if ( strcmp( data_type, "int" ) == 0 )
    {
    i_data_ptr = ((vtkIntScalars *)
      (encoder->ScalarInput->GetPointData()->GetScalars()))->GetPtr(0);
    ComputeGradients( encoder, i_data_ptr, thread_id, thread_count );
    }
  else if ( strcmp( data_type, "float" ) == 0 )
    {
    f_data_ptr = ((vtkFloatScalars *)
      (encoder->ScalarInput->GetPointData()->GetScalars()))->GetPtr(0);
    ComputeGradients( encoder, f_data_ptr, thread_id, thread_count );
    }

  return VTK_THREAD_RETURN_VALUE;
}

// Description:
// Build a shading table for a light with the given direction and
// color, for a material of the given type. material[0] = ambient,
// material[1] = diffuse, material[2] = specular, material[3] = 
// specular exponent.  If update_flag is 0, the table is overwritten
// with the new values.  If update_flag is 1, the new intensity values
// are added into the table.  This way multiple light sources can
// be handled.
//
// Note: specular is currently missing and will be added soon
//
void vtkNormalEncoder::BuildShadingTable( float light_direction[3],
					  float light_color[3],
					  float light_intensity,
					  float view_direction[3],
					  float material[4],
					  int   update_flag )
{
  float    lx, ly, lz; 
  float    n_dot_l;   
  int      i;
  float    *nptr;
  float    *sdr_ptr;
  float    *sdg_ptr;
  float    *sdb_ptr;
  float    *ssr_ptr;
  float    *ssg_ptr;
  float    *ssb_ptr;
  float    Ka, Es, Kd_intensity, Ks_intensity;
  float    half_x, half_y, half_z;
  float    mag, n_dot_h, specular_value;

  // Move to local variables
  lx = light_direction[0];
  ly = light_direction[1];
  lz = light_direction[2];

  half_x = lx - view_direction[0];
  half_y = ly - view_direction[1];
  half_z = lz - view_direction[2];

  mag = sqrt( (double)(half_x*half_x + half_y*half_y + half_z*half_z ) );
  
  if( mag != 0.0 )
    {
    half_x /= mag;
    half_y /= mag;
    half_z /= mag;
    }

  Ka = material[0];
  Es = material[3];
  Kd_intensity = material[1] * light_intensity;
  Ks_intensity = material[2] * light_intensity;

  nptr = this->DecodedNormal;

  sdr_ptr = this->ShadingTable[0];
  sdg_ptr = this->ShadingTable[1];
  sdb_ptr = this->ShadingTable[2];

  ssr_ptr = this->ShadingTable[3];
  ssg_ptr = this->ShadingTable[4];
  ssb_ptr = this->ShadingTable[5];

  // For each possible normal, compute the intensity of light at
  // a location with that normal, and the given lighting and
  // material properties
  for ( i = 0; i < 2*(NORM_SQR_SIZE*NORM_SQR_SIZE + 
		      (NORM_SQR_SIZE-1)*(NORM_SQR_SIZE-1)); i++ )
    {
    // The dot product between the normal and the light vector
    n_dot_l = (*(nptr+0) * lx + *(nptr+1) * ly + *(nptr+2) * lz);
    
    // If we are updating, then begin by adding in ambient
    if ( update_flag )
      {
      *(sdr_ptr) += Ka * light_color[0];
      *(sdg_ptr) += Ka * light_color[1];
      *(sdb_ptr) += Ka * light_color[2];
      }
    // Otherwise begin by setting the value to the ambient contribution
    else
      {
      *(sdr_ptr) = Ka * light_color[0];
      *(sdg_ptr) = Ka * light_color[1];
      *(sdb_ptr) = Ka * light_color[2];
      *(ssr_ptr) = 0.0;
      *(ssg_ptr) = 0.0;
      *(ssb_ptr) = 0.0;
      }

    // If there is some diffuse contribution, add it in
    if ( n_dot_l > 0 )
      {
      *(sdr_ptr) += (Kd_intensity * n_dot_l * light_color[0]);
      *(sdg_ptr) += (Kd_intensity * n_dot_l * light_color[1]);
      *(sdb_ptr) += (Kd_intensity * n_dot_l * light_color[2]);

      n_dot_h = (*(nptr+0) * half_x + *(nptr+1) * half_y + *(nptr+2) * half_z);
      if ( n_dot_h > 0.001 )
	{
	specular_value = Ks_intensity * pow( (double)n_dot_h, (double)Es );
	*(ssr_ptr) += specular_value * light_color[0];
	*(ssg_ptr) += specular_value * light_color[1];
	*(ssb_ptr) += specular_value * light_color[2];
	}      
      }
    // Increment all the pointers
    nptr += 3;
    sdr_ptr++;
    sdg_ptr++;
    sdb_ptr++;
    ssr_ptr++;
    ssg_ptr++;
    ssb_ptr++;
    }
}

// Description:
// I still need to fill this in...
int vtkNormalEncoder::GetEncodedNormalIndex( int x_index, int y_index,
					     int z_index )
{
  return 0;
}


// Description:
// Print the vtkNormalEncoder
void vtkNormalEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  if ( this->ScalarInput )
    {
    os << indent << "ScalarInput: (" << this->ScalarInput << ")\n";
    }
  else
    {
    os << indent << "ScalarInput: (none)\n";
    }

  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
}
