/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNormalEncoder.h
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

// .NAME vtkNormalEncoder - encode volume gradients and gradient magnitudes, build shading table
// .SECTION Description
// vtkNormalEncode takes vtkStructuredPoints as input and can generate 
// two 3D array data sets - a two-byte per value array which encodes
// normal direction, and a 1 byte per value array which captures gradient
// magnitude information.  Once these structures have been build, a shading
// table can be created for a given directional light source and given
// material properties.

// .SECTION see also
// 

#ifndef __vtkNormalEncoder_h
#define __vtkNormalEncoder_h

#include "vtkObject.h"
#include "vtkStructuredPoints.h"
#include "vtkMultiThreader.h"

// With a recursion depth of 7, you will have 65 vertices on each
// original edge of the octahedron. This leads to a 65x65 grid of
// vertices with a 64x64 grid of vertices in between each 4 vertices 
// in the 65x65 vertex grid.  This number can be 3, 5, 9, 17, 33 or 65
// and still fit in a 16 bit value.  The next number, 129, would lead
// to too many entries for a 16 bit value.
#define NORM_SQR_SIZE          65

class VTK_EXPORT vtkNormalEncoder : public vtkObject
{
public:
  vtkNormalEncoder();
  ~vtkNormalEncoder();
  static vtkNormalEncoder *New() {return new vtkNormalEncoder;};
  const char *GetClassName() {return "vtkNormalEncoder";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Set/Get the scalar input for which the normals will be 
  // calculated
  vtkSetObjectMacro( ScalarInput, vtkStructuredPoints );
  vtkGetObjectMacro( ScalarInput, vtkStructuredPoints );
  
  // Description:
  // This is temporary and will be replaced with a gradient magnitude
  // opacity transfer function
  void SetGradientMagnitudeRange( float values[2] )
    { SetGradientMagnitudeRange( values[0], values[1] ); };
  void SetGradientMagnitudeRange( float value1, float value2 );
  vtkGetVectorMacro(  GradientMagnitudeRange, float, 2 );

  // Description:
  // Recompute the encoded normals and gradient magnitudes.
  void  UpdateNormals( void );

  // Description:
  // Build a shading table for a light with the specified direction,
  // and color for an object of the specified material properties.
  // material[0] = ambient, material[1] = diffuse, material[2] = specular
  // and material[3] = specular exponent.  If the update flag is 0,
  // the shading table is overwritten with these new shading values.
  // If the update_flag is 1, then the computed light contribution is
  // added to the current shading table values.
  void  BuildShadingTable( float light_direction[3],
			   float light_color[3],
			   float light_intensity,
			   float view_direction[3],
			   float material[4],
			   int update_flag );
  
  // Description:
  // Get the red/green/blue shading table.
  float *GetRedDiffuseShadingTable( void )    {return this->ShadingTable[0];};
  float *GetGreenDiffuseShadingTable( void )  {return this->ShadingTable[1];};
  float *GetBlueDiffuseShadingTable( void )   {return this->ShadingTable[2];};
  float *GetRedSpecularShadingTable( void )   {return this->ShadingTable[3];};
  float *GetGreenSpecularShadingTable( void ) {return this->ShadingTable[4];};
  float *GetBlueSpecularShadingTable( void )  {return this->ShadingTable[5];};

  // Description:
  // Get the encoded normals.
  unsigned short  *GetEncodedNormals( void ) { return this->EncodedNormal; };

  // Description:
  // Get the encoded normal at an x,y,z location in the volume
  int   GetEncodedNormalIndex( int xyz_index ) 
                                { return *(this->EncodedNormal+xyz_index); };
  int   GetEncodedNormalIndex( int x_index, int y_index, int z_index );

  // Description:
  // Get the magnitude of the gradient at an x,y,z location in the volume
  float GetGradientMagnitude( int xyz_index ) 
  {return this->GradientMagnitudeTable[*(this->GradientMagnitude+xyz_index)];};

  // Description:
  // Get/Set the number of threads to create when encoding normals
  vtkSetClampMacro( ThreadCount, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( ThreadCount, int );

  // These variables should be protected but are being
  // made public to be accessible to the templated function.
  // We used to have the templated function as a friend, but
  // this does not work with all compilers

  // The input scalar data on which the normals are computed
  vtkStructuredPoints   *ScalarInput;

  // The encoded normals (2 bytes) and the size of the encoded normals
  unsigned short        *EncodedNormal;
  int                   EncodedNormalSize[3];

  // The magnitude of the gradient array and the size of this array
  unsigned char         *GradientMagnitude;
  int                   GradientMagnitudeSize[3];

  // A mapping from 0-255 to opacity - will be replaced
  float                 GradientMagnitudeRange[2];
  float                 GradientMagnitudeTable[256];

  // The time at which the normals were last built
  vtkTimeStamp          BuildTime;

  // The six shading tables (r diffuse ,g diffuse ,b diffuse, 
  // r specular, g specular, b specular ) - with an entry for each
  // encoded normal
  float                 ShadingTable[6][(2*(NORM_SQR_SIZE*NORM_SQR_SIZE+
				     (NORM_SQR_SIZE-1)*(NORM_SQR_SIZE-1)))];

  // The index table which maps (x,y) position in the rotated grid
  // to an encoded normal 
  int                   IndexTable[2*NORM_SQR_SIZE - 1][2*NORM_SQR_SIZE -1];

  // Has this index table been initialized yet?
  int                   IndexTableInitialized;

  // This is a table that maps encoded normal (2 byte value) to a 
  // normal (dx, dy, dz)
  float                 DecodedNormal[3*(2*(NORM_SQR_SIZE*NORM_SQR_SIZE+
				 (NORM_SQR_SIZE-1)*(NORM_SQR_SIZE-1)))];

  // These are temporary variables used to avoid conflicts with
  // multi threading
  int                   ScalarInputSize[3];
  float                 ScalarInputAspect[3];

protected:

  // Method to initialize the index table
  void                  InitializeIndexTable( void );

  // The number of threads to use when encoding normals
  int                        ThreadCount;

  vtkMultiThreader                Threader;
  friend VTK_THREAD_RETURN_TYPE   SwitchOnDataType( void *arg );

}; 


#endif
