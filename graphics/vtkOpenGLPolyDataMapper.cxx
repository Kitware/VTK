/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper.cxx
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

#include "vtkOpenGLPolyDataMapper.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkOpenGLRenderWindow.h"
#endif
#include "vtkOpenGLRenderer.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"

// Description:
// Construct empty object.
vtkOpenGLPolyDataMapper::vtkOpenGLPolyDataMapper()
{
  this->ListId = 0;
}

// Description:
// Construct empty object.
vtkOpenGLPolyDataMapper::~vtkOpenGLPolyDataMapper()
{
      // free any old display lists
      if (this->ListId)
        {
        glDeleteLists(this->ListId,1);
        this->ListId = 0;
        }
}

// Description:
// Get the lmcolor property, this is a pretty important little 
// function.  It determines how vertex colors will be handled  
// in gl.  When a PolyDataMapper has vertex colors it will use this 
// method to determine what lmcolor mode to set.               
GLenum vtkOpenGLPolyDataMapper::GetLmcolorMode(vtkProperty *prop)
{
  if (prop->GetAmbient() > prop->GetDiffuse())
    {
    return GL_AMBIENT;
    }
  else
    {
    return GL_DIFFUSE;
    }
}

//
// Receives from Actor -> maps data to primitives
//
void vtkOpenGLPolyDataMapper::Render(vtkRenderer *ren, vtkActor *act)
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

// make sure our window is current
#ifdef _WIN32
  ((vtkWin32OpenGLRenderWindow *)(ren->GetRenderWindow()))->MakeCurrent();
#else
  ((vtkOpenGLRenderWindow *)(ren->GetRenderWindow()))->MakeCurrent();
#endif

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

    if (!this->ImmediateModeRendering)
      {
      // free any old display lists
      if (this->ListId)
        {
        glDeleteLists(this->ListId,1);
        this->ListId = 0;
        }
      // get a unique display list id
      this->ListId = glGenLists(1);
      glNewList(this->ListId,GL_COMPILE_AND_EXECUTE);
      this->Draw(ren,act);
      glEndList();
      }
    this->BuildTime.Modified();
    }
  // if nothing changed but we are using display lists, draw it
  else
    {
    if (!this->ImmediateModeRendering)
      {
      glCallList(this->ListId);
      }
    }
   
  // if we are in immediate mode rendering we always
  // want to draw the primitives here
  if (this->ImmediateModeRendering)
    {
    this->Draw(ren,act);
    }
}

// Description:
// Draw method for OpenGL.
void vtkOpenGLPolyDataMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  vtkOpenGLRenderer *ren = (vtkOpenGLRenderer *)aren;
  int npts, rep, j, interpolation, idx[3];
  float fclr[4], polyNorm[3], tran;
  short clr[4];
  GLenum glFunction[4], aGlFunction;
  vtkProperty *prop;
  vtkPoints *p;
  vtkCellArray *prims[4], *aPrim;
  vtkColorScalars *c;
  vtkNormals *n;
  int *pts;
  vtkTCoords *t;
  int tDim, primType;
  int count = 0;
  int noAbort = 1;
  vtkPolyData *input = (vtkPolyData *)this->Input;
  
  // get the property 
  prop = act->GetProperty();

  // get the transparency 
  tran = prop->GetOpacity();
  
  // if the primitives are invisable then get out of here 
  if (tran <= 0.0) return;
  clr[3] = (short) ((float)tran*255);

  // get the representation (e.g., surface / wireframe / points)
  rep = prop->GetRepresentation();

  switch (rep) 
    {
    case VTK_POINTS:
      glFunction[0]  = GL_POINTS;
      glFunction[1]  = GL_POINTS;
      glFunction[2]  = GL_POINTS;
      glFunction[3]  = GL_POINTS;
      break;
    case VTK_WIREFRAME:
      glFunction[0] = GL_POINTS;
      glFunction[1] = GL_LINE_STRIP;
      glFunction[2] = GL_LINE_STRIP;
      glFunction[3] = GL_LINE_LOOP;
      break;
    case VTK_SURFACE:
      glFunction[0] = GL_POINTS;
      glFunction[1] = GL_LINE_STRIP;
      glFunction[2] = GL_TRIANGLE_STRIP;
      glFunction[3] = GL_POLYGON;
      break;
    default: 
      vtkErrorMacro(<< "Bad representation sent\n");
      glFunction[0] = GL_POINTS;
      glFunction[1] = GL_LINE_STRIP;
      glFunction[2] = GL_TRIANGLE_STRIP;
      glFunction[3] = GL_POLYGON;
      break;
    }

  // get the shading interpolation 
  interpolation = prop->GetInterpolation();

  // and draw the display list
  p = input->GetPoints();
  c = this->Colors;
  prims[0] = input->GetVerts();
  prims[1] = input->GetLines();
  prims[2] = input->GetStrips();
  prims[3] = input->GetPolys();

  t = input->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetDimension();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  n = input->GetPointData()->GetNormals();
  if (interpolation == VTK_FLAT) n = 0;

  // if we are doing vertex colors then set lmcolor to adjust 
  // the current materials ambient and diffuse values using   
  // vertex color commands otherwise tell it not to.          
  glDisable( GL_COLOR_MATERIAL );
  if (c)
    {
    glColorMaterial( GL_FRONT_AND_BACK, this->GetLmcolorMode(prop));
    glEnable( GL_COLOR_MATERIAL );
    }
  
  for (primType = 0; primType < 4; primType++)
    {
    aPrim = prims[primType];
    aGlFunction = glFunction[primType];
    
    // for lines or points
    if (primType < 2)
      {
      if (!n)
	{
	glDisable( GL_LIGHTING);
	if (!c)
	  {
	  float *bg_color;
	  // if a line is being drawn without normals and with the  
	  // ambient intensity set to zero, then lets pretend that  
	  // the ambient intensity is 1.0 because otherwise the line
	  // would either not show up or be screwed up              
	  // get the color from the property and set it 
	  bg_color = prop->GetColor();
	  fclr[0] = bg_color[0]; 
	  fclr[1] = bg_color[1]; 
	  fclr[2] = bg_color[2];
	  fclr[3]  = tran;
	  glBegin( GL_POINTS );
	  glColor4fv(fclr);
	  glEnd();
	  }
	}
      }
    
    for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
	 count++)
      { 
      glBegin(aGlFunction);
      
      if ((primType > 1) && (!n))
        {
        if ( primType == 3 ) vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
	else vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
        }
      
      for (j = 0; j < npts; j++) 
	{
	if (c) 
	  {
	  glColor4ubv(c->GetColor(pts[j]));
	  }
	
	if (t)
	  {
	  glTexCoord2fv(t->GetTCoord(pts[j]));
	  }
	
	if (n) 
	  {
	  glNormal3fv(n->GetNormal(pts[j]));
	  }
	else 
	  {
	  if (primType == 3) 
	    {
	    glNormal3fv(polyNorm);
	    }
	  if (primType == 2)
	    {
	    if ( j > 2)
	      {
	      if (j % 2)
		{
		idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
		vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
		}
	      else
		{
		idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
		vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
		}
	      }
	    else if ( j == 0 )
	      {
	      vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	      }
	    glNormal3fv(polyNorm);
	    }
	  }
	
	glVertex3fv(p->GetPoint(pts[j]));
	}
      glEnd();
      
      // if its wireframe, then draw the top and bottom edges
      // of any tstrips
      if (primType == 2 && rep == VTK_WIREFRAME) 
	{
	// draw first line
	glBegin(GL_LINE_STRIP);
	for (j = 0; j < npts; j += 2) 
	  {
	  if (c) 
	    {
	    glColor4ubv(c->GetColor(pts[j]));
	    }
	  
	  if (n) 
	    {
	    glNormal3fv(n->GetNormal(pts[j]));
	    }
	  else 
	    {
	    if ( j && j < (npts-1) )
	      {
              idx[0] = pts[j-1]; idx[1] = pts[j]; idx[2] = pts[j+1]; 
              vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	      }
	    glNormal3fv(polyNorm);
	    }
	  
	  if (t)
	    {
	    glTexCoord2fv(t->GetTCoord(pts[j]));
	    }
	  
	  glVertex3fv(p->GetPoint(pts[j]));
	  }
	glEnd();
	
	// draw second line
	glBegin(GL_LINE_STRIP);
	for (j = 1; j < npts; j += 2) 
	  {
	  if (c) 
	    {
	    glColor4ubv(c->GetColor(pts[j]));
	    }
	  
	  if (n) 
	    {
	    glNormal3fv(n->GetNormal(pts[j]));
	    }
	  else 
	    {
	    if (j < npts-1)
	      {
              idx[0] = pts[j+1]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
              vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	      }
	    glNormal3fv(polyNorm);
	    }
	  if (t)
	    {
	    glTexCoord2fv(t->GetTCoord(pts[j]));
	    }
	  glVertex3fv(p->GetPoint(pts[j]));
	  }
	glEnd();
	}

      // check for abort condition
      if (count == 100)
	{
	count = 0;
	if (ren->GetRenderWindow()->CheckAbortStatus())
	  {
	  noAbort = 0;
	  }
	}
      }
    if (primType < 2)
      {
      // reset the lighting if we turned it off
      if (!n) glEnable( GL_LIGHTING);
      }
    }
}
