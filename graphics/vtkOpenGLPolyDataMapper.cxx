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
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }
  
  if ( input == NULL ) 
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    if ( input->GetDataReleased() )
      {
      input->ForceUpdate();
      }
    else
      {
      input->Update();
      }
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
       act->GetProperty()->GetMTime() > this->BuildTime)
    {
    // sets this->Colors as side effect
    this->GetColors();

    if (!this->ImmediateModeRendering && 
	!this->GetGlobalImmediateModeRendering())
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
    if (!this->ImmediateModeRendering && 
	!this->GetGlobalImmediateModeRendering())
      {
      glCallList(this->ListId);
      }
    }
   
  // if we are in immediate mode rendering we always
  // want to draw the primitives here
  if (this->ImmediateModeRendering ||
      this->GetGlobalImmediateModeRendering())
    {
    this->Draw(ren,act);
    }
}



void vtkOpenGLDraw01(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		     vtkTCoords *, vtkOpenGLRenderer *ren, int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawN013(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &, vtkPoints *p, vtkNormals *n, 
		       vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCN013(vtkCellArray *aPrim, GLenum aGlFunction,
			int &cellNum, vtkPoints *p, vtkNormals *n, 
			vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawS01(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNS013(vtkCellArray *aPrim, GLenum aGlFunction,
			int &, vtkPoints *p, vtkNormals *n, 
			vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCNS013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &cellNum, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawT01(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *, vtkTCoords *t, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNT013(vtkCellArray *aPrim, GLenum aGlFunction,
			int &, vtkPoints *p, vtkNormals *n, 
			vtkScalars *, vtkTCoords *t, vtkOpenGLRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
	{
	noAbort = 0;
	}
      }
    }
}

void vtkOpenGLDrawCNT013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &cellNum, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *, vtkTCoords *t, vtkOpenGLRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
	{
	noAbort = 0;
	}
      }
    }
}

void vtkOpenGLDrawST01(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &, vtkPoints *p, vtkNormals *, 
		       vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNST013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCNST013(vtkCellArray *aPrim, GLenum aGlFunction,
			  int &cellNum, vtkPoints *p, vtkNormals *n, 
			  vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
			  int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCS01(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &cellNum, vtkPoints *p, vtkNormals *, 
		       vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNCS013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &cellNum, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    glColor4ubv(c->GetColor(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCNCS013(vtkCellArray *aPrim, GLenum aGlFunction,
			  int &cellNum, vtkPoints *p, vtkNormals *n, 
			  vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
			  int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    glColor4ubv(c->GetColor(cellNum));
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCST01(vtkCellArray *aPrim, GLenum aGlFunction,
			int &cellNum, vtkPoints *p, vtkNormals *, 
			vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNCST013(vtkCellArray *aPrim, GLenum aGlFunction,
			  int &cellNum, vtkPoints *p, vtkNormals *n, 
			  vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
			  int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCNCST013(vtkCellArray *aPrim, GLenum aGlFunction,
			   int &cellNum, vtkPoints *p, vtkNormals *n, 
			   vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
			   int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    glColor4ubv(c->GetColor(cellNum));
    glNormal3fv(n->GetNormal(cellNum));
      
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}


void vtkOpenGLDraw3(vtkCellArray *aPrim, GLenum aGlFunction,
		    int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		    vtkTCoords *, vtkOpenGLRenderer *ren, int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawS3(vtkCellArray *aPrim, GLenum aGlFunction,
		    int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawT3(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *, vtkTCoords *t, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawST3(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCS3(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &cellNum, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCST3(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &cellNum, vtkPoints *p, vtkNormals *, 
		       vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}


  
void vtkOpenGLDraw2(vtkCellArray *aPrim, GLenum aGlFunction,
		    int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		    vtkTCoords *, vtkOpenGLRenderer *ren, int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
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
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawS2(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
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
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawT2(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *, vtkTCoords *t, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
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
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawST2(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(aGlFunction);
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
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
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCS2(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &cellNum, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
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
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawCST2(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &cellNum, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    glBegin(aGlFunction);
    vtkTriangle::ComputeNormal(p,3,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
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
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}


  

 
  
void vtkOpenGLDrawW(vtkCellArray *aPrim, GLenum,
		   int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		   vtkTCoords *, vtkOpenGLRenderer *ren, int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      if ( j == 0 )
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
      
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      if (j == 1)
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
  
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
}

void vtkOpenGLDrawNW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *n, 
		     vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}


void vtkOpenGLDrawSW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      if ( j == 0 )
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      if (j == 1)
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNSW(vtkCellArray *aPrim, GLenum,
		      int &, vtkPoints *p, vtkNormals *n, 
		      vtkScalars *c, vtkTCoords *, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawTW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *, vtkTCoords *t, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      if ( j == 0 )
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      if (j == 1)
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNTW(vtkCellArray *aPrim, GLenum,
		      int &, vtkPoints *p, vtkNormals *n, 
		      vtkScalars *, vtkTCoords *t, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawSTW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      if ( j == 0 )
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
      
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      if (j == 1)
	{
	vtkTriangle::ComputeNormal(p, 3, pts, polyNorm);
	}
      else
	{
	idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
	vtkTriangle::ComputeNormal(p, 3, idx, polyNorm);
	}
      glNormal3fv(polyNorm);
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
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
}

void vtkOpenGLDrawNSTW(vtkCellArray *aPrim, GLenum,
		      int &, vtkPoints *p, vtkNormals *n, 
		      vtkScalars *c, vtkTCoords *t, vtkOpenGLRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    // draw first line
    glBegin(GL_LINE_STRIP);
    for (j = 0; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
    
    // draw second line
    glBegin(GL_LINE_STRIP);
    for (j = 1; j < npts; j += 2) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }
    glEnd();
  
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
}



// Description:
// Draw method for OpenGL.
void vtkOpenGLPolyDataMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  vtkOpenGLRenderer *ren = (vtkOpenGLRenderer *)aren;
  int rep, interpolation;
  float fclr[4], tran;
  GLenum glFunction[4], aGlFunction;
  vtkProperty *prop;
  vtkPoints *p;
  vtkCellArray *prims[4], *aPrim;
  vtkScalars *c=NULL;
  vtkNormals *n;
  vtkTCoords *t;
  int tDim;
  int noAbort = 1;
  vtkPolyData *input = (vtkPolyData *)this->Input;
  float diff;
  int cellScalars = 0;
  int cellNum = 0;
  int cellNormals = 0;
  
  
  // get the property 
  prop = act->GetProperty();

  // get the transparency 
  tran = prop->GetOpacity();
  diff = prop->GetDiffuse();
  
  // if the primitives are invisable then get out of here 
  if (tran <= 0.0) return;

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
  
  // are they cell or point scalars
  if ( this->Colors )
    {
    c = this->Colors;
    c->InitColorTraversal(tran, this->LookupTable, this->ColorMode);
    if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
    !input->GetPointData()->GetScalars() )	 
      {
      cellScalars = 1;
      }
    }
    
  prims[0] = input->GetVerts();
  prims[1] = input->GetLines();
  prims[2] = input->GetStrips();
  prims[3] = input->GetPolys();

  t = input->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetNumberOfComponents();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  n = input->GetPointData()->GetNormals();
  if (interpolation == VTK_FLAT) n = 0;
  
  if (!n && input->GetCellData()->GetNormals())
    {
    cellNormals = 1;
    n = input->GetCellData()->GetNormals();
    }
  

  // if we are doing vertex colors then set lmcolor to adjust 
  // the current materials ambient and diffuse values using   
  // vertex color commands otherwise tell it not to.          
  glDisable( GL_COLOR_MATERIAL );
  if (c)
    {
    glColorMaterial( GL_FRONT_AND_BACK, this->GetLmcolorMode(prop));
    glEnable( GL_COLOR_MATERIAL );
    }
  
  // how do we draw points
  void (*draw0)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *, int &);
  int idx;
  if (n) idx = 1;
  else idx = 0;
  if (c) idx += 2;
  if (t) idx += 4;
  if (cellScalars) idx += 8;
  if (cellNormals) idx += 16;
  
  switch (idx) 
    {
    case 0: draw0 = vtkOpenGLDraw01; break;
    case 1: draw0 = vtkOpenGLDrawN013; break;
    case 2: draw0 = vtkOpenGLDrawS01; break;
    case 3: draw0 = vtkOpenGLDrawNS013; break;
    case 4: draw0 = vtkOpenGLDrawT01; break;
    case 5: draw0 = vtkOpenGLDrawNT013; break;
    case 6: draw0 = vtkOpenGLDrawST01; break;
    case 7: draw0 = vtkOpenGLDrawNST013; break;
    case 10: draw0 = vtkOpenGLDrawCS01; break;
    case 11: draw0 = vtkOpenGLDrawNCS013; break;
    case 14: draw0 = vtkOpenGLDrawCST01; break;
    case 15: draw0 = vtkOpenGLDrawNCST013; break;

    case 17: draw0 = vtkOpenGLDrawCN013; break;
    case 19: draw0 = vtkOpenGLDrawCNS013; break;
    case 21: draw0 = vtkOpenGLDrawCNT013; break;
    case 23: draw0 = vtkOpenGLDrawCNST013; break;
    case 27: draw0 = vtkOpenGLDrawCNCS013; break;
    case 31: draw0 = vtkOpenGLDrawCNCST013; break;
    }
  
  // how do we draw lines
  void (*draw1)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *, int &);
  switch (idx) 
    {
    case 0: draw1 = vtkOpenGLDraw01; break;
    case 1: draw1 = vtkOpenGLDrawN013; break;
    case 2: draw1 = vtkOpenGLDrawS01; break;
    case 3: draw1 = vtkOpenGLDrawNS013; break;
    case 4: draw1 = vtkOpenGLDrawT01; break;
    case 5: draw1 = vtkOpenGLDrawNT013; break;
    case 6: draw1 = vtkOpenGLDrawST01; break;
    case 7: draw1 = vtkOpenGLDrawNST013; break;
    case 10: draw1 = vtkOpenGLDrawCS01; break;
    case 11: draw1 = vtkOpenGLDrawNCS013; break;
    case 14: draw1 = vtkOpenGLDrawCST01; break;
    case 15: draw1 = vtkOpenGLDrawNCST013; break;
    case 17: draw1 = vtkOpenGLDrawCN013; break;
    case 19: draw1 = vtkOpenGLDrawCNS013; break;
    case 21: draw1 = vtkOpenGLDrawCNT013; break;
    case 23: draw1 = vtkOpenGLDrawCNST013; break;
    case 27: draw1 = vtkOpenGLDrawCNCS013; break;
    case 31: draw1 = vtkOpenGLDrawCNCST013; break;
    }

  // how do we draw tstrips
  void (*draw2)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *, int &);
  void (*draw2W)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		 vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *, int &);
  switch (idx) 
    {
    case 0: draw2 = vtkOpenGLDraw2; break;
    case 1: draw2 = vtkOpenGLDrawN013; break;
    case 2: draw2 = vtkOpenGLDrawS2; break;
    case 3: draw2 = vtkOpenGLDrawNS013; break;
    case 4: draw2 = vtkOpenGLDrawT2; break;
    case 5: draw2 = vtkOpenGLDrawNT013; break;
    case 6: draw2 = vtkOpenGLDrawST2; break;
    case 7: draw2 = vtkOpenGLDrawNST013; break;
    case 10: draw2 = vtkOpenGLDrawCS2; break;
    case 11: draw2 = vtkOpenGLDrawNCS013; break;
    case 14: draw2 = vtkOpenGLDrawCST2; break;
    case 15: draw2 = vtkOpenGLDrawNCST013; break;
    case 17: draw2 = vtkOpenGLDraw2; break;
    case 19: draw2 = vtkOpenGLDrawS2; break;
    case 21: draw2 = vtkOpenGLDrawT2; break;
    case 23: draw2 = vtkOpenGLDrawST2; break;
    case 27: draw2 = vtkOpenGLDrawCS2; break;
    case 31: draw2 = vtkOpenGLDrawCST2; break;
    }
  switch (idx)
    {
    case 0: draw2W = vtkOpenGLDrawW; break;
    case 1: draw2W = vtkOpenGLDrawNW; break;
    case 2: draw2W = vtkOpenGLDrawSW; break;
    case 3: draw2W = vtkOpenGLDrawNSW; break;
    case 4: draw2W = vtkOpenGLDrawTW; break;
    case 5: draw2W = vtkOpenGLDrawNTW; break;
    case 6: draw2W = vtkOpenGLDrawSTW; break;
    case 7: draw2W = vtkOpenGLDrawNSTW; break;
    case 10: draw2W = vtkOpenGLDrawW; break;
    case 11: draw2W = vtkOpenGLDrawNW; break;
    case 14: draw2W = vtkOpenGLDrawTW; break;
    case 15: draw2W = vtkOpenGLDrawNTW; break;
    case 17: draw2W = vtkOpenGLDrawW; break;
    case 19: draw2W = vtkOpenGLDrawSW; break;
    case 21: draw2W = vtkOpenGLDrawTW; break;
    case 23: draw2W = vtkOpenGLDrawSTW; break;
    case 27: draw2W = vtkOpenGLDrawW; break;
    case 31: draw2W = vtkOpenGLDrawTW; break;
    }
  
  // how do we draw polys
  void (*draw3)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		vtkScalars *, vtkTCoords *, vtkOpenGLRenderer *, int &);
  switch (idx) 
    {
    case 0: draw3 = vtkOpenGLDraw3; break;
    case 1: draw3 = vtkOpenGLDrawN013; break;
    case 2: draw3 = vtkOpenGLDrawS3; break;
    case 3: draw3 = vtkOpenGLDrawNS013; break;
    case 4: draw3 = vtkOpenGLDrawT3; break;
    case 5: draw3 = vtkOpenGLDrawNT013; break;
    case 6: draw3 = vtkOpenGLDrawST3; break;
    case 7: draw3 = vtkOpenGLDrawNST013; break;
    case 10: draw3 = vtkOpenGLDrawCS3; break;
    case 11: draw3 = vtkOpenGLDrawNCS013; break;
    case 14: draw3 = vtkOpenGLDrawCST3; break;
    case 15: draw3 = vtkOpenGLDrawNCST013; break;
    case 17: draw3 = vtkOpenGLDrawCN013; break;
    case 19: draw3 = vtkOpenGLDrawCNS013; break;
    case 21: draw3 = vtkOpenGLDrawCNT013; break;
    case 23: draw3 = vtkOpenGLDrawCNST013; break;
    case 27: draw3 = vtkOpenGLDrawCNCS013; break;
    case 31: draw3 = vtkOpenGLDrawCNCST013; break;
    }

  // do verts
  aPrim = prims[0];
  aGlFunction = glFunction[0];
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
  
  // draw all the elements
  draw0(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  
  // do lines
  aPrim = prims[1];
  aGlFunction = glFunction[1];
  
  // draw all the elements
  draw1(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  
  // reset the lighting if we turned it off
  if (!n) glEnable( GL_LIGHTING);

  // do tstrips
  aPrim = prims[2];
  aGlFunction = glFunction[2];
  draw2(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  if (rep == VTK_WIREFRAME)   
    {
    draw2W(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
    }

  // do polys
  aPrim = prims[3];
  aGlFunction = glFunction[3];
  draw3(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
}
