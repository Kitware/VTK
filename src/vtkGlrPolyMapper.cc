/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlrPolyMapper.cc
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
#include <stdlib.h>
#include <math.h>
#include "vtkGlrRenderer.hh"
#include "vtkPolygon.hh"
#include "vtkPolyData.hh"
#include "vtkGlrPolyMapper.hh"

// Description:
// Construct empty object.
vtkGlrPolyMapper::vtkGlrPolyMapper()
{
  this->Data = NULL; 
  this->Colors = NULL; 
}

// Description:
// Get the lmcolor property, this is a pretty important little 
// function.  It determines how vertex colors will be handled  
// in gl.  When a primitive has vertex colors it will use this 
// method to determine what lmcolor mode to set.               
int vtkGlrPolyMapper::GetLmcolorMode(vtkProperty *prop)
{
  if (prop->GetAmbient() > prop->GetDiffuse())
    {
    return LMC_AMBIENT;
    }
  else
    {
    return LMC_DIFFUSE;
    }
}

// Description:
// Build the data structure for the gl polygon primitive.
void vtkGlrPolyMapper::Build(vtkPolyData *data, vtkColorScalars *c)
{
  this->Data = data;
  this->Colors = c;

  return;
}

// Description:
// Load poly data into gl graphics library.
void vtkGlrPolyMapper::Draw(vtkRenderer *aren, vtkActor *act)
{
  int npts, idx[3], rep, j, interpolation;
  float fclr[4], polyNorm[3], tran;
  short clr[4];
  void (*bgn_func[4])(),(*end_func[4])();
  void (*aBgn_func)(),(*aEnd_func)();
  vtkProperty *prop;
  vtkPoints *p;
  vtkCellArray *prims[4], *aPrim;
  vtkColorScalars *c;
  vtkNormals *n;
  unsigned char *rgb;
  int *pts;
  vtkPolygon polygon;
  vtkTCoords *t;
  int tDim, primType;

  if ( ! this->Data || (npts=this->Data->GetNumberOfPoints()) < 1)
    return;

  // get the property 
  prop = act->GetProperty();

  // get the transparency 
  tran = prop->GetOpacity();
  
  // if the polygons are invisable then get out of here 
  if (tran <= 0.0) return;
  clr[3] = (short) ((float)tran*255);

  // get the representation 
  rep = prop->GetRepresentation();

  switch (rep) 
    {
    case VTK_POINTS:
      bgn_func[0] = bgnpoint;
      bgn_func[1] = bgnpoint;
      bgn_func[2] = bgnpoint;
      bgn_func[3] = bgnpoint;
      end_func[0] = endpoint;
      end_func[1] = endpoint;
      end_func[2] = endpoint;
      end_func[3] = endpoint;
      break;
    case VTK_WIREFRAME:
      bgn_func[0] = bgnpoint;
      bgn_func[1] = bgnline;
      bgn_func[2] = bgnline;
      bgn_func[3] = bgnclosedline;
      end_func[0] = endpoint;
      end_func[1] = endline;
      end_func[2] = endline;
      end_func[3] = endclosedline;
      break;
    case VTK_SURFACE:
      bgn_func[0] = bgnpoint;
      bgn_func[1] = bgnline;
      bgn_func[2] = bgntmesh;
      bgn_func[3] = bgnpolygon;
      end_func[0] = endpoint;
      end_func[1] = endline;
      end_func[2] = endtmesh;
      end_func[3] = endpolygon;
      break;
    default: 
      vtkErrorMacro(<< "Bad glr_poly representation sent\n");
      bgn_func[0] = bgnpoint;
      bgn_func[1] = bgnline;
      bgn_func[2] = bgntmesh;
      bgn_func[3] = bgnpolygon;
      end_func[0] = endpoint;
      end_func[1] = endline;
      end_func[2] = endtmesh;
      end_func[3] = endpolygon;
      break;
    }

  // get the shading interpolation 
  interpolation = prop->GetInterpolation();

  // and draw the display list
  p = this->Data->GetPoints();
  c = this->Colors;
  prims[0] = this->Data->GetVerts();
  prims[1] = this->Data->GetLines();
  prims[2] = this->Data->GetStrips();
  prims[3] = this->Data->GetPolys();

  t = this->Data->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetDimension();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  n = this->Data->GetPointData()->GetNormals();
  if (interpolation == VTK_FLAT) n = 0;

  // if we are doing vertex colors then set lmcolor to adjust 
  // the current materials ambient and diffuse values using   
  // vertex color commands otherwise tell it not to.          
  if (this->Colors)
    {
    lmcolor(this->GetLmcolorMode(prop));
    }
  else 
    {
    lmcolor(LMC_NULL);
    }
  
  for (primType = 0; primType < 4; primType++)
    {
    aPrim = prims[primType];
    aBgn_func = bgn_func[primType];
    aEnd_func = end_func[primType];

    // for lines or points
    if (primType < 2 && !c)
      {
      float *bg_color;
      float ambient;
      // if a line is being drawn without normals and with the  
      // ambient intensity set to zero, then lets pretend that  
      // the ambient intensity is 1.0 because otherwise the line
      // would either not show up or be screwed up              
      ambient = prop->GetAmbient();
      if (ambient <= 0.0)
	{
	// get the color from the property and set it 
	bg_color = prop->GetColor();
	fclr[0] = bg_color[0]; 
	fclr[1] = bg_color[1]; 
	fclr[2] = bg_color[2];
	fclr[3]  = tran;
	lmcolor(LMC_COLOR);
	bgnpoint();
	c4f(fclr);
	endpoint();
	}
      else
	lmcolor(LMC_NULL);
      }
    
    for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); )
      { 
      (*aBgn_func)();
      
      if ((primType > 1) && (!n))
	polygon.ComputeNormal(p,npts,pts,polyNorm);
      
      for (j = 0; j < npts; j++) 
	{
	if (c) 
	  {
	  rgb = c->GetColor(pts[j]);
	  clr[0] = rgb[0]; 
	  clr[1] = rgb[1]; 
	  clr[2] = rgb[2];
	  c4s(clr);
	  }
	
	if (t)
	  {
	  t2f (t->GetTCoord(pts[j]));
	  }
	
	if (n) 
	  {
	  n3f(n->GetNormal(pts[j]));
	  }
	else 
	  {
	  if (primType == 3) 
	    {
	    n3f(polyNorm);
	    }
	  if (primType == 2)
	    {
	    if ( j > 2)
	      {
	      if (j % 2)
		{
		idx[0] = pts[j-2]; idx[1] = pts[j]; idx[2] = pts[j-1]; 
		polygon.ComputeNormal(p, 3, idx, polyNorm);
		}
	      else
		{
		idx[0] = pts[j-2]; idx[1] = pts[j-1]; idx[2] = pts[j]; 
		polygon.ComputeNormal(p, 3, idx, polyNorm);
		}
	      }
	    else if ( j == 0 )
	      {
	      polygon.ComputeNormal(p, 3, pts, polyNorm);
	      }
	    n3f(polyNorm);
	    }
	  }
	
	v3f(p->GetPoint(pts[j]));
	}
      (*aEnd_func)();

      // if its wireframe, then draw the top and bottom edges
      // of any tstrips
      if (primType == 2 && rep == VTK_WIREFRAME) 
	{
	// draw first line
	bgnline();
	for (j = 0; j < npts; j += 2) 
	  {
	  if (c) 
	    {
	    rgb = c->GetColor(pts[j]);
	    clr[0] = rgb[0]; 
	    clr[1] = rgb[1]; 
	    clr[2] = rgb[2];
	    c4s(clr);
	    }
	  
	  if (n) 
	    {
	    n3f(n->GetNormal(pts[j]));
	    }
	  else 
	    {
	    if ( j && j < (npts-1) )
	      {
	      polygon.ComputeNormal(p->GetPoint(pts[j - 1]), 
				    p->GetPoint(pts[j]), 
				    p->GetPoint(pts[j+1]), polyNorm); 
	      }
	    n3f(polyNorm);
	    }
	  
	  if (t)
	    {
	    t2f (t->GetTCoord(pts[j]));
	    }
	  
	  v3f(p->GetPoint(pts[j]));
	  }
	endline();
	
	// draw second line
	bgnline();
	for (j = 1; j < npts; j += 2) 
	  {
	  if (c) 
	    {
	    rgb = c->GetColor(pts[j]);
	    clr[0] = rgb[0]; 
	    clr[1] = rgb[1]; 
	    clr[2] = rgb[2];
	    c4s(clr);
	    }
	  
	  if (n) 
	    {
	    n3f(n->GetNormal(pts[j]));
	    }
	  else 
	    {
	    if (j < npts-1)
	      {
	      polygon.ComputeNormal(p->GetPoint(pts[j + 1]), 
				    p->GetPoint(pts[j]), 
				    p->GetPoint(pts[j - 1]), polyNorm); 
	      }
	    n3f(polyNorm);
	    }
	  if (t)
	    {
	    t2f (t->GetTCoord(pts[j]));
	    }
	  v3f(p->GetPoint(pts[j]));
	  }
	endline();
	}
      }
    }
}
