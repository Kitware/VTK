/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXglrTexture.cc
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
#include <math.h>
#include "vtkXglrRenderWindow.hh"
#include "vtkXglrRenderer.hh"
#include "vtkXglrTexture.hh"

// shared increasing counter
long vtkXglrTexture::GlobalIndex = 0;

// the system state for XGL
extern Xgl_sys_state xglr_sys_state;

// Description:
// Initializes an instance, generates a unique index.
vtkXglrTexture::vtkXglrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
  this->MipMap = NULL;
  this->TMap = NULL;
  this->Switch = 1;
}

vtkXglrTexture::~vtkXglrTexture()
{
  if (this->MipMap)
    {
    xgl_object_destroy(this->MipMap);
    }
  if (this->TMap)
    {
    xgl_object_destroy(this->TMap);
    }
}

// Description:
// Implement base class method.
void vtkXglrTexture::Load(vtkTexture *txt, vtkRenderer *ren)
{
  this->Load(txt, (vtkXglrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vtkXglrTexture::Load(vtkTexture *txt, vtkXglrRenderer *ren)
{
  // need to reload the texture
  if (txt->GetInput()->GetMTime() > this->LoadTime.GetMTime())
    {
    int bytesPerPixel;
    int *size;
    vtkScalars *scalars;
    unsigned char *dataPtr;
    Xgl_usgn32 xsize, ysize;
    Xgl_object setRas;
    Xgl_usgn32 *input, *bptr;
    Xgl_texture_boundary uBound, vBound;
    Xgl_render_component_desc *rDesc;
    unsigned int yloop, xloop;
    
    // get some info
    size = txt->GetInput()->GetDimensions();
    scalars = (txt->GetInput()->GetPointData())->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
      }

    bytesPerPixel = scalars->GetNumberOfValuesPerScalar();

    // make sure using unsigned char data of color scalars type
    if ( strcmp(scalars->GetDataType(),"unsigned char") ||
    strcmp(scalars->GetScalarType(),"ColorScalar") )
      {
      vtkErrorMacro(<< "Cannot do quick coversion to unsigned char.\n");
      return;
      }

    dataPtr = ((vtkColorScalars *)scalars)->GetPtr(0);    

    // we only support 2d texture maps right now
    // so one of the three sizes must be 1, but it 
    // could be any of them, so lets find it
    if (size[0] == 1)
      {
      xsize = size[1]; ysize = size[2];
      }
    else
      {
      xsize = size[0];
      if (size[1] == 1)
	{
	ysize = size[2];
	}
      else
	{
	ysize = size[1];
	if (size[2] != 1)
	  {
	  vtkErrorMacro(<< "3D texture maps currently are not supported!\n");
	  return;
	  }
	}
      }

    if (txt->GetRepeat())
      {
      uBound = XGL_TEXTURE_BOUNDARY_WRAP;
      vBound = XGL_TEXTURE_BOUNDARY_WRAP;
      }
    else
      {
      uBound = XGL_TEXTURE_BOUNDARY_TRANSPARENT;
      vBound = XGL_TEXTURE_BOUNDARY_TRANSPARENT;
      }

    if (this->MipMap)
      {
      xgl_object_destroy(this->MipMap);
      }
    this->MipMap = xgl_object_create(xglr_sys_state, XGL_MIPMAP_TEXTURE, 
				     NULL, 0);
    xgl_object_set(this->MipMap,XGL_MIPMAP_TEXTURE_LEVELS, 1, NULL);

    setRas = xgl_object_create (xglr_sys_state, 
				XGL_MEM_RAS, 0,
				XGL_DEV_COLOR_TYPE, 
				XGL_COLOR_RGB, 
				XGL_RAS_WIDTH, xsize,
				XGL_RAS_HEIGHT, ysize,
				XGL_RAS_DEPTH, 32,
				0);
  
    // Get the memory rasters pixel data 
    xgl_object_get (setRas, XGL_MEM_RAS_IMAGE_BUFFER_ADDR, &input);
    
    for (yloop = 0; yloop < ysize; yloop++)
      {
      // upside down ? or is OpenGL upside down
      // bptr = input + (ysize - yloop - 1)*xsize;
      bptr = input + yloop*xsize;
      
      for (xloop = 0; xloop < xsize; xloop++)
	{
	if (bytesPerPixel < 3)
	  {
	  *(bptr)  = *(dataPtr);
	  *(bptr) += ((Xgl_usgn32)(*(dataPtr)))<<8;
	  *(bptr) += ((Xgl_usgn32)(*(dataPtr++)))<<16;
	  }
	else
	  {
	  *(bptr)  = *(dataPtr++);
	  *(bptr) += ((Xgl_usgn32)(*(dataPtr++)))<<8;
	  *(bptr) += ((Xgl_usgn32)(*(dataPtr++)))<<16;
	  }
	if ((bytesPerPixel == 4)||(bytesPerPixel == 2))
	  {
	  *(bptr) += ((Xgl_usgn32)(*(dataPtr++)))<<24;
	  }
	bptr++;
	}
      }

    xgl_mipmap_texture_build(this->MipMap, setRas, uBound, vBound);
    xgl_object_destroy(setRas);
    
    if (this->TMap)
      {
      xgl_object_destroy(this->TMap);
      }
    this->TMap = xgl_object_create(xglr_sys_state, XGL_TMAP, NULL, NULL);

    this->TDesc.texture_type = XGL_TEXTURE_TYPE_MIPMAP;
    this->TDesc.texture_info.mipmap.texture_map = this->MipMap;
    this->TDesc.texture_info.mipmap.u_boundary = uBound;
    this->TDesc.texture_info.mipmap.v_boundary = vBound;
    
    if (txt->GetInterpolate())
      {
      this->TDesc.texture_info.mipmap.interp_info.filter1 = 
	XGL_TEXTURE_INTERP_BILINEAR;
      this->TDesc.texture_info.mipmap.interp_info.filter2 = 
	XGL_TEXTURE_INTERP_BILINEAR;
      }
    else
      {
      this->TDesc.texture_info.mipmap.interp_info.filter1 = 
	XGL_TEXTURE_INTERP_POINT;
      this->TDesc.texture_info.mipmap.interp_info.filter2 = 
	XGL_TEXTURE_INTERP_POINT;
      }

    /* Set the depth adjustment factor to 0 */
    this->TDesc.texture_info.mipmap.depth_interp_factor = 0.0;
    
    /* Set the orientation matrix to identity */
    this->TDesc.texture_info.mipmap.orientation_matrix[0][0] = 1.0;
    this->TDesc.texture_info.mipmap.orientation_matrix[0][1] = 0.0;
    this->TDesc.texture_info.mipmap.orientation_matrix[1][0] = 0.0;
    this->TDesc.texture_info.mipmap.orientation_matrix[1][1] = 1.0;
    this->TDesc.texture_info.mipmap.orientation_matrix[2][0] = 0.0;
    this->TDesc.texture_info.mipmap.orientation_matrix[2][1] = 0.0;

    this->TDesc.comp_info.color_info.channel_number[0] = 0;
    rDesc = &(this->TDesc.comp_info.color_info.render_component_desc[0]);
    rDesc->comp = XGL_RENDER_COMP_DIFFUSE_COLOR;
    if ((bytesPerPixel == 3)||(bytesPerPixel == 1))
      {
      this->TDesc.comp_info.color_info.num_render_comp_desc = 1;
      this->TDesc.comp_info.color_info.num_channels[0] = 3;
      rDesc->texture_op = XGL_TEXTURE_OP_REPLACE;
      //      rDesc->texture_op = XGL_TEXTURE_OP_MODULATE;
      }
    else
      {
      this->TDesc.comp_info.color_info.num_render_comp_desc = 2;
      this->TDesc.comp_info.color_info.num_channels[0] = 3;
      this->TDesc.comp_info.color_info.num_channels[1] = 1;
      this->TDesc.comp_info.color_info.channel_number[1] = 3;
      rDesc->texture_op = XGL_TEXTURE_OP_REPLACE;
      //      rDesc->texture_op = XGL_TEXTURE_OP_MODULATE;
      //      rDesc->texture_op = XGL_TEXTURE_OP_DECAL_INTRINSIC;
      rDesc = &(this->TDesc.comp_info.color_info.render_component_desc[1]);
      rDesc->texture_op = XGL_TEXTURE_OP_MODULATE;
      rDesc->texture_op = XGL_TEXTURE_OP_REPLACE;
      }
    
    xgl_object_set(this->TMap, XGL_TMAP_DESCRIPTOR, &this->TDesc, 0);
    
    // modify the load time to the current time
    this->LoadTime.Modified();
    }

  // now bind it 
  xgl_object_set(*ren->GetContext(),
		 XGL_3D_CTX_SURF_FRONT_TMAP_NUM, 1,
		 XGL_3D_CTX_SURF_FRONT_TMAP, &this->TMap,
		 XGL_3D_CTX_SURF_FRONT_TMAP_SWITCHES, &(this->Switch),
		 NULL);
}
