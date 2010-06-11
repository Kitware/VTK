/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOOGLExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOOGLExporter.h"

#include "vtkObjectFactory.h"
#include "vtkActorCollection.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTexture.h"
#include "vtkMatrix4x4.h"

vtkStandardNewMacro(vtkOOGLExporter);

vtkOOGLExporter::vtkOOGLExporter()
{
  this->FileName = NULL;
}

vtkOOGLExporter::~vtkOOGLExporter()
{
  this->SetFileName( 0 );
}

static char indent[256];
static int indent_now = 0;

#define VTK_INDENT_MORE { indent[indent_now] = ' '; \
                                          indent_now += 4; \
                                      indent[indent_now] = 0; }
#define VTK_INDENT_LESS { indent[indent_now] = ' '; \
                                          indent_now -= 4; \
                                      indent[indent_now] = 0; }

void vtkOOGLExporter::WriteData()
{
  vtkRenderer *ren;
  FILE *fp;
  int i, j;
  vtkActorCollection *ac;
  vtkActor *anActor, *aPart;
  vtkLightCollection *lc;
  vtkLight *aLight;
  vtkCamera *cam;
  int count;
  vtkMatrix4x4 *mat;
   
  for (i=0;i<256;i++)
    {
    indent[i] = ' ';
    }
  indent[indent_now] = 0;

  // make sure the user specified a filename
  if ( this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
    }

  // first make sure there is only one renderer in this rendering window
  if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
    {
    vtkErrorMacro(<< "Support for only one renderer per window.");
    return;
    }

  // get the renderer
  ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  
  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
    {
    vtkErrorMacro(<< "no actors found for writing Geomview OOGL file.");
    return;
    }
    
  // try opening the files
  fp = fopen(this->FileName,"w");
  if (!fp)
    {
    vtkErrorMacro(<< "unable to open Geomview OOGL file " << this->FileName);
    return;
    }
  
  //
  //  Write header
  //
  vtkDebugMacro("Writing Geomview OOGL file");
  fprintf(fp,"# Geomview OOGL file written by the visualization toolkit\n\n");
  fprintf(fp,"%s( progn\n", indent);
   
   
  VTK_INDENT_MORE;

  //
  // Write out the camera
  // 
  cam = ren->GetActiveCamera();
 
  fprintf(fp, "%s(camera \"Camera\" camera {\n", indent);
  
  VTK_INDENT_MORE;
   
  mat = cam->GetViewTransformMatrix();
  fprintf(fp, "%sworldtocam transform {\n", indent);
  
  VTK_INDENT_MORE;
   
  for (i=0; i<4;i++)
     {
     fprintf(fp, "%s", indent);
     for (j=0; j < 4; j++)
       {
       fprintf(fp, "%f ", mat->GetElement(j,i));
       }
     fprintf(fp, "\n");
     }
   
  VTK_INDENT_LESS; 
  fprintf(fp, "%s}\n", indent);
   
   
  fprintf(fp, "%sperspective %d stereo %d\n", indent, 
          !cam->GetParallelProjection(), 0);
  fprintf(fp, "%sfov 40\n", indent);
  fprintf(fp, "%sframeaspect 1\n", indent);
  fprintf(fp, "%sfocus %f\n", indent, cam->GetDistance());
  fprintf(fp, "%snear %f\n", indent, cam->GetClippingRange()[0]);
  fprintf(fp, "%sfar  %f\n", indent, cam->GetClippingRange()[1]);
   
  VTK_INDENT_LESS; 
  
  fprintf(fp, "%s}\n", indent);

  VTK_INDENT_LESS;
   
  fprintf(fp, "%s)\n", indent);
   
   
  //
  // Write the background colour
  // 
   
  fprintf(fp, "( backcolor \"Camera\" %f %f %f )\n", ren->GetBackground()[0],
          ren->GetBackground()[1],
          ren->GetBackground()[2]);
             
  //
  // Write out default properties
  //

  fprintf(fp, "( merge-baseap appearance {\n");
  
  VTK_INDENT_MORE;
   
  fprintf(fp, "%sface\n%s-edge\n%svect\n%s-transparent\n%severt\n"
          "%sshading flat\n%s-normal\n%snormscale 1\n%slinewidth 1\n"
          "%spatchdice 10 10\n",
          indent, indent, indent, indent, indent,
          indent, indent, indent, indent, indent);
  fprintf(fp, "%slighting {\n", indent);
   
  VTK_INDENT_MORE;
   
  fprintf(fp, "%sambient %f %f %f\n", indent, ren->GetAmbient()[0],
          ren->GetAmbient()[1], ren->GetAmbient()[2]);
  fprintf(fp, "%slocalviewer 1\n%sattenconst 1\n%sattenmult 0\n"
          "%s#replacelights\n",
          indent, indent, indent, indent);
   
  // make sure we have a default light
  // if we dont then use a headlight
  lc = ren->GetLights();
  vtkCollectionSimpleIterator sit;
  for (lc->InitTraversal(sit); (aLight = lc->GetNextLight(sit)); )
    {
    this->WriteALight(aLight, fp);
    }
   
  VTK_INDENT_LESS;
  
  fprintf(fp, "%s}\n", indent);
   
  VTK_INDENT_LESS;
   
  fprintf(fp, "%s})\n", indent);
   
   
  // do the actors now
  ac = ren->GetActors();
  vtkAssemblyPath *apath;
  count = 0;
  vtkCollectionSimpleIterator ait;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
    {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
      {
      count++;
      aPart=static_cast<vtkActor *>(apath->GetLastNode()->GetViewProp());
      this->WriteAnActor(aPart, fp, count);
      }
    }

  VTK_INDENT_LESS;

  fclose(fp);
}

void vtkOOGLExporter::WriteALight(vtkLight *aLight, FILE *fp)
{
  double *pos, *focus, *color;
  float dir[3];
  
  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  color = aLight->GetDiffuseColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  vtkMath::Normalize(dir);
       
   fprintf(fp, "%slight {\n", indent);
   
   VTK_INDENT_MORE;

   fprintf(fp, "%sambient 0.00 0.00 0.00\n", indent);
   fprintf(fp, "%scolor   %f %f %f\n", indent, color[0], color[1], color[2]);
   fprintf(fp, "%sposition %f %f %f %f\n", indent, pos[0], pos[1], pos[2],
           0.0);
   
   VTK_INDENT_LESS;
   
   fprintf(fp, "%s}\n", indent);

   return;
}

void vtkOOGLExporter::WriteAnActor(vtkActor *anActor, FILE *fp, int count)
{
  vtkDataSet *ds;
  vtkPolyData *pd;
  vtkGeometryFilter *gf = NULL;
  vtkPoints *points;
  int i;
  vtkProperty *prop;
  static double defcolor[3] = {  1.0, 1.0, 1.0 };
  double *tempd = defcolor;
  vtkCellArray *cells;
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  double tempf2=0;
  vtkPolyDataMapper *pm;
  vtkUnsignedCharArray *colors;
   
  double p[3];
  unsigned char *c;
   
  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == NULL)
    {
    return;
    }
      
  fprintf(fp,"%s(new-geometry \"[g%d]\"\n", indent, count);

  VTK_INDENT_MORE;

  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();
  
  // we really want polydata
  if ( ds->GetDataObjectType() != VTK_POLY_DATA )
    {
    gf = vtkGeometryFilter::New();
    gf->SetInput(ds);
    gf->Update();
    pd = gf->GetOutput();
    }
  else
    {
    ds->Update();
    pd = static_cast<vtkPolyData *>(ds);
    }

  pm = vtkPolyDataMapper::New();
  pm->SetInput(pd);
  pm->SetScalarRange(anActor->GetMapper()->GetScalarRange());
  pm->SetScalarVisibility(anActor->GetMapper()->GetScalarVisibility());
  pm->SetLookupTable(anActor->GetMapper()->GetLookupTable());

  points = pd->GetPoints();

  // usage of GetColors() has been deprecated in VTK 4.0
  colors  = pm->MapScalars(1.0);
   
  // Get the material properties
  prop = anActor->GetProperty();
   
  // is there a texture map (for the moment, we don't deal with textures)
  if ( 1 == 2 /*anActor->GetTexture()*/)
    {
    vtkTexture *aTexture = anActor->GetTexture();
    int *size, xsize, ysize, bpp;
    vtkDataArray *scalars;
    vtkUnsignedCharArray *mappedScalars;
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
    scalars = aTexture->GetInput()->GetPointData()->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
      }

    // make sure using unsigned char data of color scalars type
    if (aTexture->GetMapColorScalarsThroughLookupTable () ||
        (scalars->GetDataType() != VTK_UNSIGNED_CHAR) )
      {
      mappedScalars = aTexture->GetMappedScalars ();
      }
    else
      {
      mappedScalars = static_cast<vtkUnsignedCharArray*>(scalars);
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
    VTK_INDENT_MORE;
    bpp = mappedScalars->GetNumberOfComponents();
    fprintf(fp, "%simage %d %d %d\n", indent, xsize, ysize, bpp);
    VTK_INDENT_MORE;
    txtrData =  static_cast<vtkUnsignedCharArray *>(mappedScalars)->GetPointer(0);
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
      if (i%8 == 0)
        {
        fprintf(fp,"\n%s    ", indent);
        }
      else
        {
        fprintf(fp," ");
        }
      }
    VTK_INDENT_LESS;
    fprintf(fp, "%s}\n", indent);
    VTK_INDENT_LESS;
    }
   
   // start an INST object
   fprintf(fp, "%s{ INST\n", indent);
   
   VTK_INDENT_MORE
   
   // start a LIST object
   fprintf(fp, "%sgeom { LIST\n", indent);
   
   VTK_INDENT_MORE;

   // extract vector information
   if (pd->GetNumberOfLines())
     {
     fprintf(fp, "%s{ VECT\n", indent);

     VTK_INDENT_MORE;
     
     // write out the header line
     cells = pd->GetLines();
     i = 0;
     for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
       {
       i += npts;
       }
     fprintf(fp, "%s%d %d %d\n", indent, static_cast<int>(pd->GetNumberOfLines()), i, 1);
     cells = pd->GetLines();
     fprintf(fp, "%s", indent);
     for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
       {
       fprintf(fp, "%d ", static_cast<int>(npts));
       }
     fprintf(fp, "\n");
     
     // write out # of color information
     fprintf(fp, "%s1 ", indent);
     for (i=1; i<pd->GetNumberOfLines(); i++)
       {
       fprintf(fp, "0 ");
       }
     fprintf(fp, "\n");
     
     
     // write out points
     cells = pd->GetLines();
     for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
       {
       fprintf(fp, "%s", indent);
       for (i=0;i<npts;i++)
         {
         double *pt = points->GetPoint(indx[i]);
         fprintf(fp, "%s%f %f %f ", indent, pt[0], pt[1], pt[2]);
         }
       fprintf(fp, "\n");
       }
     
     // write out color indices
     tempd = prop->GetColor();
     fprintf(fp, "%f %f %f 1\n", tempd[0], tempd[1], tempd[2]);
     fprintf(fp, "}\n");
     
     VTK_INDENT_LESS;
     }
   
   // extract polygon information (includes triangle strips)

   if (pd->GetNumberOfPolys() || pd->GetNumberOfStrips())
     {
     fprintf(fp, "%s{ %sOFF\n", indent, (colors) ? "C" : "");
   
     VTK_INDENT_MORE;
      
     // write header
     if (pd->GetNumberOfPolys())
       {
       fprintf(fp, "%s%d %d %d\n", indent, static_cast<int>(points->GetNumberOfPoints()),
               static_cast<int>(pd->GetNumberOfPolys()), 0);
       }
     else
       {
       // Handle triangle strips
       // 
       i = 0;
       cells = pd->GetStrips();
       for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
         {
         i += (npts-2);
         }
       fprintf(fp, "%s%d %d %d\n", indent, static_cast<int>(points->GetNumberOfPoints()), i, 0);
       }
      
      // write points
      if (colors)
        {
        for (i = 0; i < points->GetNumberOfPoints(); i++)
          {
          double *pt = points->GetPoint(i);
          c = static_cast<unsigned char *>(colors->GetPointer(4*i));
           
          fprintf (fp,"%s%g %g %g %g %g %g %g\n", indent,
                   pt[0], pt[1], pt[2],
                   c[0]/255., c[1]/255., c[2]/255., c[3]/255.);
          }
        }
      else
        {
        for (i = 0; i < points->GetNumberOfPoints(); i++)
          {
          double *pt = points->GetPoint(i);
          fprintf (fp,"%s%g %g %g\n", indent, pt[0], pt[1], pt[2]);
          }
        }
      
      // write polys
      if (pd->GetNumberOfPolys())
        {
        cells = pd->GetPolys();
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
          {
          fprintf(fp, "%s%i ", indent, static_cast<int>(npts));
          for (i = 0; i < npts; i++)
            {
#ifdef VTK_USE_64BIT_IDS
            fprintf(fp, "%lld ", indx[i]);
#else  // VTK_USE_64BIT_IDS
            fprintf(fp, "%d ", indx[i]);
#endif // VTK_USE_64BIT_IDS
            }
          fprintf(fp, "\n");
          }
        fprintf(fp, "%s}\n", indent); // finish of polygon list
         
        VTK_INDENT_LESS; 
        
        }
        
      else if (pd->GetNumberOfStrips())
        { // write triangle strips
        cells = pd->GetStrips();
         
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
          {
          int pt1, pt2, pt3;
          
          pt1 = indx[0];
          pt2 = indx[1];
          
          for (i = 0; i < (npts - 2); i++)
            {
            pt3 = indx[i+2];
            if (i % 2)
              {
              fprintf(fp, "%s3 %d %d %d\n", indent, pt2, pt1, pt3);
              }
            else
              {
              fprintf(fp, "%s3 %d %d %d\n", indent, pt1, pt2, pt3);
              }
            pt1 = pt2;
            pt2 = pt3;      
            }          
          }
        fprintf(fp, "%s}\n", indent); // Finish off triangle strips
         
        VTK_INDENT_LESS;
        }
     }  

   fprintf(fp, "%s}\n", indent); // End of list object
   
   VTK_INDENT_LESS;

   // Get the actor's position
   anActor->GetPosition(p);

   fprintf(fp, "transform {1 0 0 0 0 1 0 0 0 0 1 0 %f %f %f 1}\n", p[0], p[1], p[2]);

   VTK_INDENT_LESS;
   
   fprintf(fp, "%s}\n", indent); // Finish off INST command
   
   VTK_INDENT_LESS;
   
   fprintf(fp, "%s)\n", indent); // Finish off new-geometry command
   
   // turn off the bounding box, set normalization to none
   fprintf(fp, "( bbox-draw \"[g%d]\" off )\n", count);
   fprintf(fp, "( normalization \"[g%d]\" none )\n", count);
   
   fprintf(fp, "( merge-ap \"[g%d]\" appearance {\n", count);
   
   VTK_INDENT_MORE;
   
   // Set shading model
   if (prop->GetInterpolation() > 0)
     {
     fprintf(fp, "%sshading smooth\n", indent);
     }

   // Set transparency
   if (prop->GetOpacity() < 1)
     {
     fprintf(fp, "%s+transparent\n", indent);
     }

   // Set representation - no way to render points
   if (prop->GetRepresentation() != 2)
     {
     fprintf(fp, "%s+edge\n%s-face\n", indent, indent);
     }

   // Set edge information
   fprintf(fp, "%slinewidth %d\n", indent, static_cast<int>(prop->GetLineWidth()));

   // Now the material information
   fprintf(fp, "%smaterial {\n", indent);
   
   VTK_INDENT_MORE; 
   
  // Indicate whether edges are shown or not 
   if (prop->GetEdgeVisibility())
     {
     tempd = prop->GetEdgeColor();
     }
   if (prop->GetRepresentation() != 2)
     {
     tempd = prop->GetColor();
     } 
  if (prop->GetEdgeVisibility() || (prop->GetRepresentation() != 2))
    {
    fprintf(fp, "%sedgecolor %f %f %f\n", indent,
            tempd[0], tempd[1], tempd[2]);
    }
      
  tempf2 = prop->GetAmbient();
  tempd = prop->GetAmbientColor();
  fprintf(fp, "%ska %f\n", indent, tempf2);
  fprintf(fp, "%sambient %f %f %f\n", indent, tempd[0], tempd[1], tempd[2]);
  tempf2 = prop->GetDiffuse();
  tempd = prop->GetDiffuseColor();
  fprintf(fp, "%skd %f\n", indent, tempf2);
  fprintf(fp, "%sdiffuse %f %f %f\n", indent, tempd[0], tempd[1], tempd[2]);
  tempf2 = prop->GetSpecular();
  tempd = prop->GetSpecularColor();
  fprintf(fp, "%sks %f\n", indent, tempf2);
  fprintf(fp, "%sspecular %f %f %f\n", indent, tempd[0], tempd[1], tempd[2]);
  if (prop->GetOpacity() < 1)
    {
    fprintf(fp, "%salpha %f\n", indent, prop->GetOpacity());
    }
   
  fprintf(fp, "%s}\n", indent);
   
  VTK_INDENT_LESS;
   
  fprintf(fp, "%s}\n", indent);
   
  VTK_INDENT_LESS;
   
  fprintf(fp, ")\n");
   
  if (gf)
    {
    gf->Delete();
    }
  pm->Delete();
}



void vtkOOGLExporter::PrintSelf(ostream& os, vtkIndent ind)
{
  vtkExporter::PrintSelf(os,ind);
 
  if (this->FileName)
    {
    os << ind << "FileName: " << this->FileName << "\n";
    }
  else
    {
    os << ind << "FileName: (null)\n";
    }
}

