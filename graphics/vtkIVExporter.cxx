/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIVExporter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Jon A. Webb of Visual Interface Inc.


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
#include "vtkIVExporter.h"
#include "vtkGeometryFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkMath.h"

vtkIVExporter::vtkIVExporter()
{
  this->FileName = NULL;
}

vtkIVExporter::~vtkIVExporter()
{
  if ( this->FileName ) delete [] this->FileName;
}

static char indent[256];
int indent_now = 0;
#define indent_more { indent[indent_now] = ' '; \
					  indent_now += 4; \
				      indent[indent_now] = 0; }
#define indent_less { indent[indent_now] = ' '; \
					  indent_now -= 4; \
				      indent[indent_now] = 0; }

void vtkIVExporter::WriteData()
{
  vtkRenderer *ren;
  FILE *fp;
  vtkActorCollection *ac;
  vtkActor *anActor, *aPart;
  vtkLightCollection *lc;
  vtkLight *aLight;
  vtkCamera *cam;
  float *tempf;
  
  for (int i=0;i<256;i++) indent[i] = ' ';
  indent[indent_now] = 0;

  // make sure the user specified a filename
  if ( this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
    }

  // first make sure there is only one renderer in this rendering window
  if (this->Input->GetRenderers()->GetNumberOfItems() > 1)
    {
    vtkErrorMacro(<< "OpenInventor files only support one renderer per window.");
    return;
    }

  // get the renderer
  this->Input->GetRenderers()->InitTraversal();
  ren = this->Input->GetRenderers()->GetNextItem();
  
  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
    {
    vtkErrorMacro(<< "no actors found for writing OpenInventor file.");
    return;
    }
    
  // try opening the files
  fp = fopen(this->FileName,"w");
  if (!fp)
    {
    vtkErrorMacro(<< "unable to open OpenInventor file " << this->FileName);
    return;
    }
  
  //
  //  Write header
  //
  vtkDebugMacro("Writing OpenInventor file");
  fprintf(fp,"#Inventor V2.0 ascii\n");
  fprintf(fp,"# OpenInventor file written by the visualization toolkit\n\n");

  fprintf(fp, "Separator {\n");
  indent_more;

  // do the camera
  cam = ren->GetActiveCamera();
  if (cam->GetParallelProjection()) 
    {
    fprintf(fp,"%sOrthographicCamera\n%s{\n", indent, indent);
    } 
  else 
    {
    // this assumes the aspect ratio is 1
    fprintf(fp,"%sPerspectiveCamera\n%s{\n%s    heightAngle %f\n",
	    indent, indent, indent,
	    cam->GetViewAngle()*3.1415926/180.0);
    }
  indent_more;
  fprintf(fp,"%snearDistance %f\n",indent, cam->GetClippingRange()[0]);
  fprintf(fp,"%sfarDistance %f\n",indent, cam->GetClippingRange()[1]);
  fprintf(fp,"%sfocalDistance %f\n",indent, cam->GetDistance());
  fprintf(fp,"%sposition %f %f %f\n", indent, cam->GetPosition()[0],
    	  cam->GetPosition()[1], cam->GetPosition()[2]);
  tempf = cam->GetOrientationWXYZ();
  fprintf(fp,"%sorientation %g %g %g %g\n%s}\n", indent,
	  tempf[1], tempf[2], tempf[3], tempf[0]*3.1415926/180.0, indent);
  indent_less;

  // do the lights first the ambient then the others
  fprintf(fp,"# The following environment information is disabled\n");
  fprintf(fp,"# because a popular viewer (Template Graphics Software SceneViewer) has\n");
  fprintf(fp,"# trouble (access violations under Windows NT) with it.\n");
  fprintf(fp,"#%sEnvironment {\n", indent);
  // couldn't figure out a way to do headlight -- seems to be a property of the
  // viewer not the model
  indent_more;
  fprintf(fp,"#%sambientIntensity 1.0 # ambient light\n", indent);
  fprintf(fp,"#%sambientColor %f %f %f }\n\n", indent,
	  ren->GetAmbient()[0], ren->GetAmbient()[1], ren->GetAmbient()[2]);
  indent_less;

  // make sure we have a default light
  // if we dont then use a headlight
  lc = ren->GetLights();
  for (lc->InitTraversal(); (aLight = lc->GetNextItem()); )
    {
    this->WriteALight(aLight, fp);
    }

  // do the actors now
  ac = ren->GetActors();
  for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
    {
    for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart()); )
      {
      this->WriteAnActor(aPart, fp);
      }
    }

  indent_less;
  fprintf(fp, "}\n"); // close Separator

  fclose(fp);
}

void vtkIVExporter::WriteALight(vtkLight *aLight, FILE *fp)
{
  float *pos, *focus, *color;
  float dir[3];
  
  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  color = aLight->GetColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  vtkMath::Normalize(dir);
    
  if (aLight->GetPositional())
    {
    float *attn;
    
    if (aLight->GetConeAngle() >= 180.0)
      {
      fprintf(fp,"%sPointLight {\n", indent);
      indent_more;
      }
    else
      { 
      fprintf(fp,"%sSpotLight {\n", indent);
	  indent_more;
      fprintf(fp,"%sdirection %f %f %f\n", indent, dir[0], dir[1], dir[2]);
      fprintf(fp,"%scutOffAngle %f\n", indent, aLight->GetConeAngle());
	  // the following ignores linear and quadratic attenuation values
	  attn = aLight->GetAttenuationValues();
	  fprintf(fp,"%sdropOffRate %f\n", indent, attn[0]);
      }
    fprintf(fp,"%slocation %f %f %f\n", indent, pos[0], pos[1], pos[2]);
    }
  else
    {
    fprintf(fp,"%sDirectionalLight {\n", indent);
	indent_more;
    fprintf(fp,"%sdirection %f %f %f\n", indent, dir[0], dir[1], dir[2]);
    }

  fprintf(fp,"%scolor %f %f %f\n", indent, color[0], color[1], color[2]);
  fprintf(fp,"%sintensity %f\n", indent, aLight->GetIntensity());
  if (aLight->GetSwitch())
    {
    fprintf(fp,"%son TRUE\n%s}\n", indent, indent);
    }
  else
    {
    fprintf(fp,"%son FALSE\n%s}\n", indent, indent);
    }
  indent_less;
}

void vtkIVExporter::WriteAnActor(vtkActor *anActor, FILE *fp)
{
  vtkDataSet *ds;
  vtkPolyData *pd;
  vtkGeometryFilter *gf = NULL;
  vtkPointData *pntData;
  vtkPoints *points = NULL;
  vtkNormals *normals = NULL;
  vtkTCoords *tcoords = NULL;
  int i;
  vtkProperty *prop;
  float *tempf;
  vtkCellArray *cells;
  int npts, *indx;
  float tempf2;
  vtkPolyDataMapper *pm;
  vtkColorScalars *colors;
  float *p;
  unsigned char *c;
  vtkTransform *trans;
  
  fprintf(fp,"%sSeparator {\n", indent);
  indent_more;

  // first stuff out the transform
  trans = new vtkTransform;
  trans->SetMatrix(anActor->vtkProp::GetMatrix());
  
  fprintf(fp,"%sTransform {\n", indent);
  indent_more;
  tempf = trans->GetPosition();
  fprintf(fp,"%stranslation %g %g %g\n", indent, tempf[0], tempf[1], tempf[2]);
  tempf = trans->GetOrientationWXYZ();
  fprintf(fp,"%srotation %g %g %g %g\n", indent, tempf[1], tempf[2], 
	  tempf[3], tempf[0]*3.1415926/180.0);
  tempf = trans->GetScale();
  fprintf(fp,"%sscaleFactor %g %g %g\n", indent, tempf[0], tempf[1], tempf[2]);
  fprintf(fp,"%s}\n", indent);
  indent_less;
  trans->Delete();
  
  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();
  
  // we really want polydata
  if (strcmp(ds->GetClassName(),"vtkPolyData"))
    {
    gf = new vtkGeometryFilter;
    gf->SetInput(ds);
    gf->Update();
    pd = gf->GetOutput();
    }
  else
    {
    ds->Update();
    pd = (vtkPolyData *)ds;
    }

  pm = new vtkPolyDataMapper;
  pm->SetInput(pd);
  pm->SetScalarRange(anActor->GetMapper()->GetScalarRange());
  pm->SetScalarVisibility(anActor->GetMapper()->GetScalarVisibility());
  pm->SetLookupTable(anActor->GetMapper()->GetLookupTable());

  points = pd->GetPoints();
  pntData = pd->GetPointData();
  normals = pntData->GetNormals();
  tcoords = pntData->GetTCoords();
  colors  = pm->GetColors();
  
  fprintf(fp,"%sMaterial {\n", indent);
  indent_more;
  
  // write out the material properties to the mat file
  prop = anActor->GetProperty();
  // the following is based on a guess about how VTK's GetAmbient
  // property corresponds to SoMaterial's ambientColor
  tempf2 = prop->GetAmbient();
  tempf = prop->GetAmbientColor();
  fprintf(fp,"%sambientColor %g %g %g\n", indent,
	  tempf[0]*tempf2, tempf[1]*tempf2, tempf[2]*tempf2);
  tempf2 = prop->GetDiffuse();
  tempf = prop->GetDiffuseColor();
  fprintf(fp,"%sdiffuseColor %g %g %g\n", indent,
	  tempf[0]*tempf2, tempf[1]*tempf2, tempf[2]*tempf2);
  tempf2 = prop->GetSpecular();
  tempf = prop->GetSpecularColor();
  fprintf(fp,"%sspecularColor %g %g %g\n", indent,
	  tempf[0]*tempf2, tempf[1]*tempf2, tempf[2]*tempf2);
  fprintf(fp,"%sshininess %g\n", indent,prop->GetSpecularPower()/128.0);
  fprintf(fp,"%stransparency %g\n", indent,1.0 - prop->GetOpacity());
  fprintf(fp,"%s}\n", indent); // close matrial
  indent_less;

  // is there a texture map
  if (anActor->GetTexture())
    {
    vtkTexture *aTexture = anActor->GetTexture();
    int *size, xsize, ysize, bpp;
    vtkScalars *scalars;
    vtkColorScalars *mappedScalars;
    unsigned char *txtrData;
    int totalValues;
    
    // make sure it is updated and then get some info
    if (aTexture->GetInput() == NULL)
      {
      vtkErrorMacro(<< "texture has no input!\n");
      return;
      }
    aTexture->GetInput()->Update();
    size = aTexture->GetInput()->GetDimensions();
    scalars = (aTexture->GetInput()->GetPointData())->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
      }

    // make sure using unsigned char data of color scalars type
    if (aTexture->GetMapColorScalarsThroughLookupTable () ||
        (strcmp(scalars->GetDataType(),"unsigned char") ||
        strcmp(scalars->GetScalarType(),"ColorScalar")) )
      {
      mappedScalars = aTexture->GetMappedScalars ();
      }
    else
      {
      mappedScalars = (vtkColorScalars *) scalars;
      }

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

    fprintf(fp, "%sTexture2 {\n", indent);
    indent_more;
    bpp = mappedScalars->GetNumberOfValuesPerScalar();
    fprintf(fp, "%simage %d %d %d\n", indent, xsize, ysize, bpp);
    indent_more;
    txtrData = mappedScalars->GetPointer(0);
    totalValues = xsize*ysize;
    fprintf(fp,"%s",indent);
    for (i = 0; i < totalValues; i++)
      {
      fprintf(fp,"%.2x",*txtrData);
      txtrData++;
      if (bpp > 1)
	{
	fprintf(fp,"%.2x",*txtrData);
	txtrData++;
	}
      if (bpp > 2) 
	{
	fprintf(fp,"%.2x",*txtrData);
	txtrData++;
	}
      if (bpp > 3) 
	{
	fprintf(fp,"%.2x",*txtrData);
	txtrData++;
	}
      if (i%8 == 0) fprintf(fp,"\n%s    ", indent);
      else fprintf(fp," ");
      }
    indent_less;
    fprintf(fp, "%s}\n", indent);
    indent_less;
    }

  // write out point data if any
  this->WritePointData(points, normals, tcoords, colors, fp);

  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
    {
    fprintf(fp,"%sIndexedFaceSet {\n", indent);
	indent_more;
    fprintf(fp,"%scoordIndex  [\n", indent);
	indent_more;
    
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"%s", indent);
      for (i = 0; i < npts; i++)
	{
	fprintf(fp,"%i, ",indx[i]);
	if (((i+1)%10) == 0) fprintf(fp, "\n%s    ", indent);
	  }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"%s]\n", indent);
	indent_less;
    fprintf(fp,"%s}\n", indent);
	indent_less;
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
    {
    fprintf(fp,"%sIndexedTriangleStripSet {\n", indent);
    indent_more;
    fprintf(fp,"%scoordIndex  [\n", indent);
    indent_more;
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"%s", indent);
      for (i = 0; i < npts; i++)
	{
	fprintf(fp,"%i, ",indx[i]);
	if (((i+1)%10) == 0) fprintf(fp, "\n%s    ", indent);
	}
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"%s]\n", indent);
    indent_less;
    fprintf(fp,"%s}\n", indent);
    indent_less;
    }
  
  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
    {
    fprintf(fp,"%sIndexedLineSet {\n", indent);
    indent_more;
    fprintf(fp,"%scoordIndex  [\n", indent);
    indent_more;
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"%s", indent);
      for (i = 0; i < npts; i++)
	{
	fprintf(fp,"%i, ",indx[i]);
	if (((i+1)%10) == 0) fprintf(fp, "\n%s    ", indent);
	}
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"%s]\n", indent);
    indent_less;
    fprintf(fp,"%s}\n", indent);
    indent_less;
    }
  
  // write out verts if any
  // (more complex because there is no IndexedPointSet)
  if (pd->GetNumberOfVerts() > 0)
    {
    fprintf(fp, "%sSeparator {\n", indent);
    indent_more;
    fprintf(fp, "%sCoordinate3 {\n", indent);
    indent_more;
    fprintf(fp,"%spoint [", indent);
    indent_more;
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      for (i = 0; i < npts; i++)
	{
	p = points->GetPoint(indx[i]);
	fprintf (fp,"%s%g %g %g,\n", indent, p[0], p[1], p[2]);
	}
      }
    fprintf(fp,"%s]\n", indent);
    indent_less;
    fprintf(fp,"%s}\n", indent);
    indent_less;
    if (colors)
      {
      fprintf(fp,"%sPackedColor {", indent);
      indent_more;
      fprintf(fp,"%srgba [\n", indent);
      indent_more;
      for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
	{
	fprintf(fp,"%s", indent);
	for (i = 0; i < npts; i++)
	  {
	  c = colors->GetColor(indx[i]);
	  fprintf (fp,"%#lx, ", 
		   ((unsigned long)255 << 24) // opaque
		   |	(((unsigned long)c[2]) << 16) 
		   | (((unsigned long)c[1]) << 8) 
		   |	 ((unsigned long)c[0]));
	  if (((i+1)%5) == 0) fprintf(fp, "\n%s", indent);
	  }
	}
      fprintf(fp,"\n%s]\n", indent);
      indent_less;
      fprintf(fp,"%s}\n", indent);
      indent_less;
      fprintf(fp,"%sMaterialBinding { value PER_VERTEX_INDEXED }\n", indent);
      }
    
    fprintf(fp, "%sPointSet {\n", indent);
    indent_more;
    fprintf(fp, "%snumPoints %d\n", indent, npts);
    indent_more;
    fprintf(fp, "%s}\n", indent);
    indent_less;
    fprintf(fp,"%s}\n", indent); // close the Separator
    indent_less;
    }  
  fprintf(fp, "%s}\n", indent);
  indent_less;
  if (gf) gf->Delete();
  pm->Delete();
}

void vtkIVExporter::WritePointData(vtkPoints *points, vtkNormals *normals,
				     vtkTCoords *tcoords, 
				     vtkColorScalars *colors, FILE *fp)
{
  float *p;
  int i;
  unsigned char *c;
  
  // write out the points
  fprintf(fp,"%sCoordinate3 {\n", indent);
  indent_more;
  fprintf(fp,"%spoint [\n", indent);
  indent_more;
  for (i = 0; i < points->GetNumberOfPoints(); i++)
    {
    p = points->GetPoint(i);
    fprintf (fp,"%s%g %g %g,\n", indent, p[0], p[1], p[2]);
    }
  fprintf(fp,"%s]\n", indent);
  indent_less;
  fprintf(fp,"%s}\n", indent);
  indent_less;
  
  // write out the point data
  if (normals)
    {
    fprintf(fp,"%sNormal {\n", indent);
	indent_more;
    fprintf(fp,"%svector [\n", indent);
	indent_more;
    for (i = 0; i < normals->GetNumberOfNormals(); i++)
      {
      p = normals->GetNormal(i);
      fprintf (fp,"%s%g %g %g,\n", indent, p[0], p[1], p[2]);
      }
    fprintf(fp,"%s]\n", indent);
	indent_less;
    fprintf(fp,"%s}\n", indent);
	indent_less;
  }

  // write out the point data
  if (tcoords)
    {
	fprintf(fp,"%sTextureCoordinateBinding  {\n",indent);
	indent_more;
	fprintf(fp,"%svalue PER_VERTEX_INDEXED\n",indent);
	indent_less;
	fprintf(fp,"%s}\n",indent);
	fprintf(fp,"%sTextureCoordinate2 {\n", indent);
    fprintf(fp,"%spoint [\n", indent);
    for (i = 0; i < tcoords->GetNumberOfTCoords(); i++)
      {
      p = tcoords->GetTCoord(i);
      fprintf (fp,"%s%g %g,\n", indent, p[0], p[1]);
      }
    fprintf(fp,"%s]\n", indent);
	indent_less;
    fprintf(fp,"%s}\n", indent);
	indent_less;
  }

  // write out the point data
  if (colors)
    {
    fprintf(fp,"%sPackedColor {\n", indent);
	indent_more;
    fprintf(fp,"%srgba [\n", indent);
	indent_more;
	fprintf(fp,"%s", indent);
    for (i = 0; i < colors->GetNumberOfColors(); i++)
      {
      c = colors->GetColor(i);
      fprintf (fp,"%#lx, ",
		   ((unsigned long)255 << 24) | // opaque
		  (((unsigned long)c[2])<<16) | 
		  (((unsigned long)c[1])<<8) |
		   ((unsigned long)c[0]));
	  if (((i+1)%5)==0) fprintf(fp, "\n%s", indent);
      }
    fprintf(fp,"\n%s]\n", indent);
	indent_less;
    fprintf(fp,"%s}\n", indent);
	indent_less;
	fprintf(fp,"%sMaterialBinding { value PER_VERTEX_INDEXED }\n", indent);
    }
}


void vtkIVExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExporter::PrintSelf(os,indent);
 
  os << indent << "FileName: " << this->FileName << "\n";
}

