/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper.cxx
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
#include <string.h>
#include <math.h>

#include "vtkMesaPolyDataMapper.h"

#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"
#include "vtkPlane.h"

#include "vtkTimerLog.h"

// Construct empty object.
vtkMesaPolyDataMapper::vtkMesaPolyDataMapper()
{
  this->ListId = 0;
  this->RenderWindow = 0;
}

// Destructor (don't call ReleaseGraphicsResources() since it is virtual
vtkMesaPolyDataMapper::~vtkMesaPolyDataMapper()
{
  if (this->RenderWindow)
    {
    // This renderwindow should be a valid pointer (even though we do not
    // increase the renderwindow's reference count).  If the renderwindow
    // had been deleted before the mapper,  then ReleaseGraphicsResources()
    // would have been called on the mapper and these resources would
    // have been released already.
    this->RenderWindow->MakeCurrent();
  
    // free any old display lists
    if (this->ListId)
      {
      glDeleteLists(this->ListId,1);
      this->ListId = 0;
      }
    }

  this->RenderWindow = NULL;
}


// Release the graphics resources used by this mapper.  In this case, release
// the display list if any.
void vtkMesaPolyDataMapper::ReleaseGraphicsResources(vtkWindow *vtkNotUsed(renWin))
{
  if (this->ListId)
    {
    glDeleteLists(this->ListId,1);
    this->ListId = 0;
    }
  this->RenderWindow = NULL; 
}


// Get the lmcolor property, this is a pretty important little 
// function.  It determines how vertex colors will be handled  
// in gl.  When a PolyDataMapper has vertex colors it will use this 
// method to determine what lmcolor mode to set.               
GLenum vtkMesaPolyDataMapper::GetLmcolorMode(vtkProperty *prop)
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
void vtkMesaPolyDataMapper::Render(vtkRenderer *ren, vtkActor *act)
{
  int numPts;
  vtkPolyData *input= this->GetInput();
  vtkTimerLog *timer;
  vtkPlaneCollection *clipPlanes;
  vtkPlane *plane;
  int i,numClipPlanes;
  double planeEquation[4];

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
      if ( this->StartMethod )
        {
        (*this->StartMethod)(this->StartMethodArg);
        }
      input->Update();
      if ( this->EndMethod )
        {
        (*this->EndMethod)(this->EndMethodArg);
        }
      }
    else
      {
      if ( this->StartMethod )
        {
        (*this->StartMethod)(this->StartMethodArg);
        }
      input->Update();
      if ( this->EndMethod )
        {
        (*this->EndMethod)(this->EndMethodArg);
        }
      }
    numPts = input->GetNumberOfPoints();
    } 

  if (numPts == 0)
    {
    vtkDebugMacro(<< "No points!");
    return;
    }
  
  if ( this->LookupTable == NULL )
    {
    this->CreateDefaultLookupTable();
    }

// make sure our window is current
#ifdef _WIN32
  ((vtkWin32MesaRenderWindow *)(ren->GetRenderWindow()))->MakeCurrent();
#else
  ((vtkMesaRenderWindow *)(ren->GetRenderWindow()))->MakeCurrent();
#endif

  timer = vtkTimerLog::New();

  clipPlanes = this->ClippingPlanes;

  if (clipPlanes == NULL)
    {
    numClipPlanes = 0;
    }
  else
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "Mesa guarantees at most 6 additional clipping planes");
      }
    }

  for (i = 0; i < numClipPlanes; i++)
    {
     glEnable((GLenum)(GL_CLIP_PLANE0+i));
    }

  for (i = 0; i < numClipPlanes; i++)
    {    
    plane = (vtkPlane *)clipPlanes->GetItemAsObject(i);

    planeEquation[0] = plane->GetNormal()[0]; 
    planeEquation[1] = plane->GetNormal()[1]; 
    planeEquation[2] = plane->GetNormal()[2];
    planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
			 planeEquation[1]*plane->GetOrigin()[1]+
			 planeEquation[2]*plane->GetOrigin()[2]);
    glClipPlane((GLenum)(GL_CLIP_PLANE0+i),planeEquation);
    }
  //
  // if something has changed regenrate colors and display lists
  // if required
  //
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime ||
       act->GetProperty()->GetMTime() > this->BuildTime ||
       ren->GetRenderWindow() != this->RenderWindow)
    {
    // sets this->Colors as side effect
    this->GetColors();

    if (!this->ImmediateModeRendering && 
	!this->GetGlobalImmediateModeRendering())
      {
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->RenderWindow = ren->GetRenderWindow();
      
      // get a unique display list id
      this->ListId = glGenLists(1);
      glNewList(this->ListId,GL_COMPILE_AND_EXECUTE);

      // Time the actual drawing
      timer->StartTimer();
      this->Draw(ren,act);
      timer->StopTimer();      

      glEndList();
      }
    else
      {
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->RenderWindow = ren->GetRenderWindow();
      }
    this->BuildTime.Modified();
    }
  // if nothing changed but we are using display lists, draw it
  else
    {
    if (!this->ImmediateModeRendering && 
	!this->GetGlobalImmediateModeRendering())
      {
      // Time the actual drawing
      timer->StartTimer();
      glCallList(this->ListId);
      timer->StopTimer();      
      }
    }
   
  // if we are in immediate mode rendering we always
  // want to draw the primitives here
  if (this->ImmediateModeRendering ||
      this->GetGlobalImmediateModeRendering())
    {
    // Time the actual drawing
    timer->StartTimer();
    this->Draw(ren,act);
    timer->StopTimer();      
    }

  this->TimeToDraw = (float)timer->GetElapsedTime();

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable((GLenum)(GL_CLIP_PLANE0+i));
    }

  timer->Delete();
}


//
// Helper routine which starts a poly, triangle or quad based upon
// the number of points in the polygon and whether triangles or quads
// were the last thing being drawn (we can get better performance if we
// can draw several triangles within a single glBegin(GL_TRIANGLES) or
// several quads within a single glBegin(GL_QUADS). 
//
void vtkMesaBeginPolyTriangleOrQuad(GLenum aGlFunction,
				      GLenum &previousGlFunction,
				      int npts)
{
  if (aGlFunction == GL_POLYGON)
    {
    switch (npts)
      {
      case 3:  // Need to draw a triangle.
	if (previousGlFunction != GL_TRIANGLES)
	  {
	  // we were not already drawing triangles, were we drawing quads?
	  if (previousGlFunction == GL_QUADS)
	    {
	    // we were previously drawing quads, close down the quads.
	    glEnd();
	    }
	  // start drawing triangles
	  previousGlFunction = GL_TRIANGLES;
	  glBegin(GL_TRIANGLES);
	  }
	  break;
      case 4:  // Need to draw a quad
	if (previousGlFunction != GL_QUADS)
	  {
	  // we were not already drawing quads, were we drawing triangles?
	  if (previousGlFunction == GL_TRIANGLES)
	    {
	    // we were previously drawing triangles, close down the triangles.
	    glEnd();
	    }
	  // start drawing quads
	  previousGlFunction = GL_QUADS;
	  glBegin(GL_QUADS);
	  }
	break;
      default:
	// if we were supposed to be drawing polygons but were really
	// drawing triangles or quads, then we need to close down the
	// triangles or quads and begin a polygon
	if (previousGlFunction != GL_INVALID_VALUE
	    && previousGlFunction != GL_POLYGON)
	  {
	  glEnd();
	  }
	previousGlFunction = GL_POLYGON;
	glBegin(aGlFunction);
	break;
      }
    }
  else if (aGlFunction == GL_POINTS)
    {
    // we are supposed to be drawing points
    if (previousGlFunction != GL_POINTS)
      {
      // We were not drawing points before this, switch to points.
      // We don't need to worry about switching from triangles or quads
      // since draw all points before drawing any polygons (i.e. in the polys
      // case we switch to triangles and quads as an optimization, there is
      // nothing to switch to that is below points).
      previousGlFunction = GL_POINTS;
      glBegin(GL_POINTS);
      }
    }
  else
    {
    previousGlFunction = aGlFunction;
    glBegin(aGlFunction);
    }
}


void vtkMesaDraw01(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		     vtkTCoords *, vtkMesaRenderer *ren, int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );

    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES)
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawN013(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &, vtkPoints *p, vtkNormals *n, 
		       vtkScalars *, vtkTCoords *, vtkMesaRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;

  GLenum previousGlFunction=GL_INVALID_VALUE;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    {
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCN013(vtkCellArray *aPrim, GLenum aGlFunction,
			int &cellNum, vtkPoints *p, vtkNormals *n, 
			vtkScalars *, vtkTCoords *, vtkMesaRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawS01(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawNS013(vtkCellArray *aPrim, GLenum aGlFunction,
			int &, vtkPoints *p, vtkNormals *n, 
			vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCNS013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &cellNum, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawT01(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *, vtkTCoords *t, vtkMesaRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawNT013(vtkCellArray *aPrim, GLenum aGlFunction,
			int &, vtkPoints *p, vtkNormals *n, 
			vtkScalars *, vtkTCoords *t, vtkMesaRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;
  
  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
	{
	noAbort = 0;
	}
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCNT013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &cellNum, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *, vtkTCoords *t, vtkMesaRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
      glEnd();
      }
    
    if (count == 100)
      {
      count = 0;
      if (ren->GetRenderWindow()->CheckAbortStatus())
	{
	noAbort = 0;
	}
      }
    }
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawST01(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &, vtkPoints *p, vtkNormals *, 
		       vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawNST013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCNST013(vtkCellArray *aPrim, GLenum aGlFunction,
			  int &cellNum, vtkPoints *p, vtkNormals *n, 
			  vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
			  int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    {
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCS01(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &cellNum, vtkPoints *p, vtkNormals *, 
		       vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawNCS013(vtkCellArray *aPrim, GLenum aGlFunction,
			 int &cellNum, vtkPoints *p, vtkNormals *n, 
			 vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
			 int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glColor4ubv(c->GetColor(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCNCS013(vtkCellArray *aPrim, GLenum aGlFunction,
			  int &cellNum, vtkPoints *p, vtkNormals *n, 
			  vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
			  int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glColor4ubv(c->GetColor(cellNum));
    glNormal3fv(n->GetNormal(cellNum));
    
    for (j = 0; j < npts; j++) 
      {
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCST01(vtkCellArray *aPrim, GLenum aGlFunction,
			int &cellNum, vtkPoints *p, vtkNormals *, 
			vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
			int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawNCST013(vtkCellArray *aPrim, GLenum aGlFunction,
			  int &cellNum, vtkPoints *p, vtkNormals *n, 
			  vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
			  int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(n->GetNormal(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCNCST013(vtkCellArray *aPrim, GLenum aGlFunction,
			   int &cellNum, vtkPoints *p, vtkNormals *n, 
			   vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
			   int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    glColor4ubv(c->GetColor(cellNum));
    glNormal3fv(n->GetNormal(cellNum));
      
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}


void vtkMesaDraw3(vtkCellArray *aPrim, GLenum aGlFunction,
		    int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		    vtkTCoords *, vtkMesaRenderer *ren, int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    {
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawS3(vtkCellArray *aPrim, GLenum aGlFunction,
		    int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawT3(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *, vtkTCoords *t, vtkMesaRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawST3(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(pts[j]));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES)
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCS3(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &cellNum, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);

    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES)
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCST3(vtkCellArray *aPrim, GLenum aGlFunction,
		       int &cellNum, vtkPoints *p, vtkNormals *, 
		       vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
		       int &noAbort)
{
  int j, npts, *pts;
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
    vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
    
    for (j = 0; j < npts; j++) 
      {
      glColor4ubv(c->GetColor(cellNum));
      glTexCoord2fv(t->GetTCoord(pts[j]));
      glNormal3fv(polyNorm);
      glVertex3fv(p->GetPoint(pts[j]));
      }

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}


  
void vtkMesaDraw2(vtkCellArray *aPrim, GLenum aGlFunction,
		    int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		    vtkTCoords *, vtkMesaRenderer *ren, int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
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

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawS2(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
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

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawT2(vtkCellArray *aPrim, GLenum aGlFunction,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *, vtkTCoords *t, vtkMesaRenderer *ren,
		     int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
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

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawST2(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
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

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCS2(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &cellNum, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
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

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}

void vtkMesaDrawCST2(vtkCellArray *aPrim, GLenum aGlFunction,
		      int &cellNum, vtkPoints *p, vtkNormals *, 
		      vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
		      int &noAbort)
{
  int j, npts, *pts;
  int idx[3];
  int count = 0;
  float polyNorm[3];
  
  GLenum previousGlFunction=GL_INVALID_VALUE;

  for (aPrim->InitTraversal(); noAbort && aPrim->GetNextCell(npts,pts); 
       count++, cellNum++)
    { 
    vtkMesaBeginPolyTriangleOrQuad( aGlFunction, previousGlFunction, npts );
    
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

    if ((previousGlFunction != GL_TRIANGLES) 
	&& (previousGlFunction != GL_QUADS)
	&& (previousGlFunction != GL_POINTS))
      {
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
  if ((previousGlFunction == GL_TRIANGLES)
      || (previousGlFunction == GL_QUADS)
      || (previousGlFunction == GL_POINTS))
    {
    glEnd();
    }
}


  

 
  
void vtkMesaDrawW(vtkCellArray *aPrim, GLenum,
		   int &, vtkPoints *p, vtkNormals *, vtkScalars *, 
		   vtkTCoords *, vtkMesaRenderer *ren, int &noAbort)
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

void vtkMesaDrawNW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *n, 
		     vtkScalars *, vtkTCoords *, vtkMesaRenderer *ren,
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


void vtkMesaDrawSW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
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

void vtkMesaDrawNSW(vtkCellArray *aPrim, GLenum,
		      int &, vtkPoints *p, vtkNormals *n, 
		      vtkScalars *c, vtkTCoords *, vtkMesaRenderer *ren,
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

void vtkMesaDrawTW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *, vtkTCoords *t, vtkMesaRenderer *ren,
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

void vtkMesaDrawNTW(vtkCellArray *aPrim, GLenum,
		      int &, vtkPoints *p, vtkNormals *n, 
		      vtkScalars *, vtkTCoords *t, vtkMesaRenderer *ren,
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

void vtkMesaDrawSTW(vtkCellArray *aPrim, GLenum,
		     int &, vtkPoints *p, vtkNormals *, 
		     vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
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

void vtkMesaDrawNSTW(vtkCellArray *aPrim, GLenum,
		      int &, vtkPoints *p, vtkNormals *n, 
		      vtkScalars *c, vtkTCoords *t, vtkMesaRenderer *ren,
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



// Draw method for Mesa.
void vtkMesaPolyDataMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  vtkMesaRenderer *ren = (vtkMesaRenderer *)aren;
  int rep, interpolation;
  float tran;
  GLenum glFunction[4], aGlFunction;
  vtkProperty *prop;
  vtkPoints *p;
  vtkCellArray *prims[4], *aPrim;
  vtkScalars *c=NULL;
  vtkNormals *n;
  vtkTCoords *t;
  int tDim;
  int noAbort = 1;
  vtkPolyData *input = this->GetInput();
  int cellScalars = 0;
  int cellNum = 0;
  int cellNormals = 0;
  
  
  // get the property 
  prop = act->GetProperty();

  // get the transparency 
  tran = prop->GetOpacity();
  
  // if the primitives are invisable then get out of here 
  if (tran <= 0.0)
    {
    return;
    }

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
  if (interpolation == VTK_FLAT)
    {
    n = 0;
    }
  
  cellNormals = 0;
  if (input->GetCellData()->GetNormals())
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
		vtkScalars *, vtkTCoords *, vtkMesaRenderer *, int &);

  int idx;
  if (n && !cellNormals)
    {
    idx = 1;
    }
  else
    {
    idx = 0;
    }
  if (c)
    {
    idx += 2;
    }
  if (t)
    {
    idx += 4;
    }
  if (cellScalars)
    {
    idx += 8;
    }
  if (cellNormals)
    {
    idx += 16;
    }

  switch (idx) 
    {
    case 0: draw0 = vtkMesaDraw01; break;
    case 1: draw0 = vtkMesaDrawN013; break;
    case 2: draw0 = vtkMesaDrawS01; break;
    case 3: draw0 = vtkMesaDrawNS013; break;
    case 4: draw0 = vtkMesaDrawT01; break;
    case 5: draw0 = vtkMesaDrawNT013; break;
    case 6: draw0 = vtkMesaDrawST01; break;
    case 7: draw0 = vtkMesaDrawNST013; break;
    case 10: draw0 = vtkMesaDrawCS01; break;
    case 11: draw0 = vtkMesaDrawNCS013; break;
    case 14: draw0 = vtkMesaDrawCST01; break;
    case 15: draw0 = vtkMesaDrawNCST013; break;

    case 16: draw0 = vtkMesaDrawCN013; break;
    case 18: draw0 = vtkMesaDrawCNS013; break;
    case 20: draw0 = vtkMesaDrawCNT013; break;
    case 22: draw0 = vtkMesaDrawCNST013; break;
    case 26: draw0 = vtkMesaDrawCNCS013; break;
    case 30: draw0 = vtkMesaDrawCNCST013; break;
    }

  // how do we draw lines
  void (*draw1)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		vtkScalars *, vtkTCoords *, vtkMesaRenderer *, int &);
  switch (idx) 
    {
    case 0: draw1 = vtkMesaDraw01; break;
    case 1: draw1 = vtkMesaDrawN013; break;
    case 2: draw1 = vtkMesaDrawS01; break;
    case 3: draw1 = vtkMesaDrawNS013; break;
    case 4: draw1 = vtkMesaDrawT01; break;
    case 5: draw1 = vtkMesaDrawNT013; break;
    case 6: draw1 = vtkMesaDrawST01; break;
    case 7: draw1 = vtkMesaDrawNST013; break;
    case 10: draw1 = vtkMesaDrawCS01; break;
    case 11: draw1 = vtkMesaDrawNCS013; break;
    case 14: draw1 = vtkMesaDrawCST01; break;
    case 15: draw1 = vtkMesaDrawNCST013; break;
    case 16: draw1 = vtkMesaDrawCN013; break;
    case 18: draw1 = vtkMesaDrawCNS013; break;
    case 20: draw1 = vtkMesaDrawCNT013; break;
    case 22: draw1 = vtkMesaDrawCNST013; break;
    case 26: draw1 = vtkMesaDrawCNCS013; break;
    case 30: draw1 = vtkMesaDrawCNCST013; break;
    }

  // how do we draw tstrips
  void (*draw2)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		vtkScalars *, vtkTCoords *, vtkMesaRenderer *, int &);
  void (*draw2W)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		 vtkScalars *, vtkTCoords *, vtkMesaRenderer *, int &);
  switch (idx) 
    {
    case 0: draw2 = vtkMesaDraw2; break;
    case 1: draw2 = vtkMesaDrawN013; break;
    case 2: draw2 = vtkMesaDrawS2; break;
    case 3: draw2 = vtkMesaDrawNS013; break;
    case 4: draw2 = vtkMesaDrawT2; break;
    case 5: draw2 = vtkMesaDrawNT013; break;
    case 6: draw2 = vtkMesaDrawST2; break;
    case 7: draw2 = vtkMesaDrawNST013; break;
    case 10: draw2 = vtkMesaDrawCS2; break;
    case 11: draw2 = vtkMesaDrawNCS013; break;
    case 14: draw2 = vtkMesaDrawCST2; break;
    case 15: draw2 = vtkMesaDrawNCST013; break;
    case 16: draw2 = vtkMesaDraw2; break;
    case 18: draw2 = vtkMesaDrawS2; break;
    case 20: draw2 = vtkMesaDrawT2; break;
    case 22: draw2 = vtkMesaDrawST2; break;
    case 26: draw2 = vtkMesaDrawCS2; break;
    case 30: draw2 = vtkMesaDrawCST2; break;
    }
  switch (idx)
    {
    case 0: draw2W = vtkMesaDrawW; break;
    case 1: draw2W = vtkMesaDrawNW; break;
    case 2: draw2W = vtkMesaDrawSW; break;
    case 3: draw2W = vtkMesaDrawNSW; break;
    case 4: draw2W = vtkMesaDrawTW; break;
    case 5: draw2W = vtkMesaDrawNTW; break;
    case 6: draw2W = vtkMesaDrawSTW; break;
    case 7: draw2W = vtkMesaDrawNSTW; break;
    case 10: draw2W = vtkMesaDrawW; break;
    case 11: draw2W = vtkMesaDrawNW; break;
    case 14: draw2W = vtkMesaDrawTW; break;
    case 15: draw2W = vtkMesaDrawNTW; break;
    case 16: draw2W = vtkMesaDrawW; break;
    case 18: draw2W = vtkMesaDrawSW; break;
    case 20: draw2W = vtkMesaDrawTW; break;
    case 22: draw2W = vtkMesaDrawSTW; break;
    case 26: draw2W = vtkMesaDrawW; break;
    case 30: draw2W = vtkMesaDrawTW; break;
    }
  
  // how do we draw polys
  void (*draw3)(vtkCellArray *, GLenum, int &, vtkPoints *, vtkNormals *, 
		vtkScalars *, vtkTCoords *, vtkMesaRenderer *, int &);
  switch (idx) 
    {
    case 0: draw3 = vtkMesaDraw3; break;
    case 1: draw3 = vtkMesaDrawN013; break;
    case 2: draw3 = vtkMesaDrawS3; break;
    case 3: draw3 = vtkMesaDrawNS013; break;
    case 4: draw3 = vtkMesaDrawT3; break;
    case 5: draw3 = vtkMesaDrawNT013; break;
    case 6: draw3 = vtkMesaDrawST3; break;
    case 7: draw3 = vtkMesaDrawNST013; break;
    case 10: draw3 = vtkMesaDrawCS3; break;
    case 11: draw3 = vtkMesaDrawNCS013; break;
    case 14: draw3 = vtkMesaDrawCST3; break;
    case 15: draw3 = vtkMesaDrawNCST013; break;
    case 16: draw3 = vtkMesaDrawCN013; break;
    case 18: draw3 = vtkMesaDrawCNS013; break;
    case 20: draw3 = vtkMesaDrawCNT013; break;
    case 22: draw3 = vtkMesaDrawCNST013; break;
    case 26: draw3 = vtkMesaDrawCNCS013; break;
    case 30: draw3 = vtkMesaDrawCNCST013; break;
    }

  // do verts
  aPrim = prims[0];
  aGlFunction = glFunction[0];

  // For verts or lines that have no normals, disable shading.
  // This will fall back on the color set in the glColor4fv() 
  // call in vtkMesaProperty::Render() - the color returned
  // by vtkProperty::GetColor() with alpha set to 1.0.
  if (!n)
    {
    glDisable( GL_LIGHTING);
    }
  
  // draw all the elements
  draw0(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  
  // do lines
  aPrim = prims[1];
  aGlFunction = glFunction[1];
  
  // draw all the elements
  draw1(aPrim, aGlFunction, cellNum, p, n, c, t, ren, noAbort);
  
  // reset the lighting if we turned it off
  if (!n)
    {
    glEnable( GL_LIGHTING);
    }

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
