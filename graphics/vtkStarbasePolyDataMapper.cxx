/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStarbasePolyDataMapper.cxx
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
#include <stdlib.h>
#include <math.h>
#include "vtkStarbasePolyDataMapper.h"
#include "vtkStarbaseRenderer.h"
#include "vtkPolyData.h"

// Description:
// Construct empty object.
vtkStarbasePolyDataMapper::vtkStarbasePolyDataMapper()
{
  this->Colors = NULL; 
  this->Prim = NULL;
}

vtkStarbasePolyDataMapper::~vtkStarbasePolyDataMapper()
{
  // if Prim is set the free it
  if (this->Prim)
    {
    delete [] this->Prim;
    }
}

//
// Receives from Actor -> maps data to primitives
//
void vtkStarbasePolyDataMapper::Render(vtkRenderer *ren, vtkActor *act)
{
  int numPts;
  vtkPolyData *input= (vtkPolyData *)this->Input;
//
// make sure that we've been properly initialized
//
  if ( input == NULL ) 
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    input->Update();
    numPts = input->GetNumberOfPoints();
    } 

  if (numPts == 0)
    {
    vtkDebugMacro(<< "No points!");
    return;
    }
  
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

  //
  // if something has changed regenrate colors and display lists
  // if required
  //
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime || 
       this->LookupTable->GetMTime() > this->BuildTime ||
       act->GetProperty()->GetMTime() > this->BuildTime)
    {
    // sets this->Colors as side effect
    this->GetColors();
    this->Build(input,this->Colors);
    this->BuildTime.Modified();
    }

  // want to draw the primitives here
  this->Draw(ren,act);
}

// Description:
// Build the data structure for the starbase polygon PolyDataMapper.
void vtkStarbasePolyDataMapper::Build(vtkPolyData *data, vtkColorScalars *c)
{
  vtkNormals *normals;
  vtkTCoords *t;
  int maxSize;
  vtkPolyData *input= (vtkPolyData *)this->Input;

  this->Colors = c;

  normals = input->GetPointData()->GetNormals();
  t = input->GetPointData()->GetTCoords();

  this->DataFlag = 0;
  if (this->Colors)
    {
    this->DataFlag += 3;
    }
  if (normals)
    {
    this->DataFlag += 3;
    }
  if (t)
    {
    this->DataFlag += 2;
    }

  // allocate storage
  if (this->Prim)
    {
    delete [] this->Prim;
    }
  
  maxSize = data->GetVerts()->GetMaxCellSize();
  if (maxSize < data->GetLines()->GetMaxCellSize())
    maxSize = data->GetLines()->GetMaxCellSize();
  if (maxSize < data->GetPolys()->GetMaxCellSize())
    maxSize = data->GetPolys()->GetMaxCellSize();
  if (maxSize < data->GetStrips()->GetMaxCellSize())
    maxSize = data->GetStrips()->GetMaxCellSize();

  this->Prim = 
    new float [(this->DataFlag+4) * maxSize];

  return;
}

// Description:
// Load polydata into starbase graphics library.
void vtkStarbasePolyDataMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  vtkStarbaseRenderer *ren = (vtkStarbaseRenderer *)aren;
  int npts, j;
  float tran;
  vtkProperty *prop;
  vtkPoints *p;
  vtkCellArray *prims[4], *aPrim;
  vtkColorScalars *c;
  vtkNormals *n;
  int *pts;
  int fd, primType;
  float *poly;
  unsigned char *rgb;
  float clr[3];
  vtkTCoords *t;
  int vflags = 0;
  vtkPolyData *input= (vtkPolyData *)this->Input;

  // get the fd
  fd = ren->GetFd();

  // get the property 
  prop = act->GetProperty();

  // get the transparency 
  tran = prop->GetOpacity();
  
  // if the polygons are invisable then get out of here 
  if (tran <= 0.0) return;

  // and draw the display list
  p = input->GetPoints();
  c = this->Colors;
  n = input->GetPointData()->GetNormals();
  prims[0] = input->GetVerts();
  prims[1] = input->GetLines();
  prims[2] = input->GetStrips();
  prims[3] = input->GetPolys();

  t = input->GetPointData()->GetTCoords();
  if ( t ) 
    {
    if (t->GetDimension() != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  // set the flags 
  if (c)
    {
    vflags |= VERTEX_COLOR;
    }
  if (n)
    {
    vflags |= VERTEX_NORMAL;
    }
  if (t)
    {
    vflags |= TEXTURE_MAP;
    }

  // due to a bug in starbase, if we have vertex colors and we want
  // two sided lighting then we must do it the wrong way in order
  // to see the vertex coloring
  if (c && ren->GetTwoSidedLighting())
    {
    bf_control(fd, TRUE, FALSE);
    }
  
  for (primType = 0; primType < 4; primType++)
    {
    aPrim = prims[primType];
    if (primType == 1) vflags |= MD_FLAGS;
    if (primType == 2) vflags = vflags & (~MD_FLAGS);
    
    for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); )
      { 
      poly = this->Prim;
      
      for (j = 0; j < npts; j++) 
	{
	memcpy(poly,p->GetPoint(pts[j]),sizeof(float)*3);
	poly += 3;
	
	if (c)
	  {
	  rgb = c->GetColor(pts[j]);
	  clr[0] = rgb[0]/255.0;
	  clr[1] = rgb[1]/255.0;
	  clr[2] = rgb[2]/255.0;
	  memcpy(poly,clr,sizeof(float)*3);
	  poly += 3;
	  }
        
	if (n)
	  {
	  memcpy(poly,n->GetNormal(pts[j]),sizeof(float)*3);
	  poly += 3;
	  }
	
	if (t)
	  {
	  memcpy(poly,t->GetTCoord(pts[j]),sizeof(float)*2);
	  poly += 2;
	  }
	
	// set move/draw flag
	if (primType == 1)
	  {
	  if (!j) 
	    {
	    *poly = 0.0;
	    } 
	  else
	    {
	    *poly = 1.0;
	    }
	  poly++;
	  }
	}

      switch (primType) 
	{
	case 0:
	  polymarker_with_data3d(fd, this->Prim, npts, this->DataFlag, vflags);
	  break;
	case 1:
	  polyline_with_data3d(fd, this->Prim, npts, this->DataFlag + 1, 
			       vflags, 0);
	  break;
	case 2:
	  triangular_strip_with_data(fd, this->Prim, npts, NULL, 
				     this->DataFlag, vflags, 0);
	  break;
	case 3: 
	  polygon_with_data3d (fd, this->Prim, npts, this->DataFlag, 
			       vflags, 0);
	  break;
	}
      }
    }

  // reset the lighting to how it was before
  if (c && ren->GetTwoSidedLighting())
    {
    bf_control(fd, FALSE, TRUE);
    }
}
