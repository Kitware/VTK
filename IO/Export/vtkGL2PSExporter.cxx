/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGL2PSExporter.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkActorCollection.h"
#include "vtkActor.h"
#include "vtkActor2DCollection.h"
#include "vtkActor2D.h"
#include "vtkVolumeCollection.h"
#include "vtkVolume.h"
#include "vtkIntArray.h"
#include "vtk_gl2ps.h"


void _SavePropVisibility(vtkRendererCollection *renCol,
                         vtkIntArray *volVis, vtkIntArray *actVis,
                         vtkIntArray *act2dVis)
{
  int nRen = renCol->GetNumberOfItems();
  vtkRenderer *ren;
  vtkVolumeCollection *vCol;
  vtkActorCollection *aCol;
  vtkActor2DCollection *a2Col;
  vtkVolume *vol;
  vtkActor *act;
  vtkActor2D *act2d;
  int j;

  volVis->SetNumberOfComponents(nRen);
  actVis->SetNumberOfComponents(nRen);
  act2dVis->SetNumberOfComponents(nRen);
  
  renCol->InitTraversal();
  for (int i=0; i<nRen; ++i)
    {
    ren = renCol->GetNextItem();
    vCol = ren->GetVolumes();
    aCol = ren->GetActors();
    a2Col = ren->GetActors2D();
      
    volVis->SetNumberOfTuples(vCol->GetNumberOfItems());
    for (vCol->InitTraversal(), j=0; (vol = vCol->GetNextVolume()); ++j) 
      {
      volVis->SetComponent(i, j, vol->GetVisibility());
      }
    
    actVis->SetNumberOfTuples(aCol->GetNumberOfItems());
    for (aCol->InitTraversal(), j=0; (act = aCol->GetNextActor()); ++j)
      {
      actVis->SetComponent(i, j, act->GetVisibility());
      }

    act2dVis->SetNumberOfTuples(a2Col->GetNumberOfItems());
    for (a2Col->InitTraversal(), j=0; (act2d = a2Col->GetNextActor2D()); 
         ++j)
      {
      act2dVis->SetComponent(i, j, act2d->GetVisibility());
      }
    }
}

void _Turn3DPropsOff(vtkRendererCollection *renCol)
{
  vtkRenderer *ren;
  vtkVolumeCollection *vCol;
  vtkActorCollection *aCol;
  vtkVolume *vol;
  vtkActor *act;
  
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    vCol = ren->GetVolumes();
    aCol = ren->GetActors();
    for (vCol->InitTraversal(); (vol = vCol->GetNextVolume());) 
      {
      vol->SetVisibility(0);
      }

    for (aCol->InitTraversal(); (act = aCol->GetNextActor());)
      {
      act->SetVisibility(0);
      }
    }
}

void _Turn3DPropsOn(vtkRendererCollection *renCol, vtkIntArray *volVis, 
                    vtkIntArray *actVis)
{
  int nRen = renCol->GetNumberOfItems();
  vtkRenderer *ren;
  vtkVolumeCollection *vCol;
  vtkActorCollection *aCol;
  vtkVolume *vol;
  vtkActor *act;
  int j;
  
  renCol->InitTraversal();
  for (int i=0; i<nRen; ++i)
    {
    ren = renCol->GetNextItem();
    vCol = ren->GetVolumes();
    aCol = ren->GetActors();
      
    for (vCol->InitTraversal(), j=0; (vol = vCol->GetNextVolume()); ++j) 
      {
      vol->SetVisibility(static_cast<int>(volVis->GetComponent(i, j)));
      }

    for (aCol->InitTraversal(), j=0; (act = aCol->GetNextActor()); ++j)
      {
      act->SetVisibility(static_cast<int>(actVis->GetComponent(i, j)));
      }
    }  
}

void _Turn2DPropsOff(vtkRendererCollection *renCol)
{
  vtkRenderer *ren;
  vtkActor2DCollection *a2Col;
  vtkActor2D *act2d;
  
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    a2Col = ren->GetActors2D();
    for (a2Col->InitTraversal(); (act2d = a2Col->GetNextActor2D());) 
      {
      act2d->SetVisibility(0);
      }
    }
}

void _Turn2DPropsOn(vtkRendererCollection *renCol, vtkIntArray *act2dVis)
{
  int nRen = renCol->GetNumberOfItems();
  vtkRenderer *ren;
  vtkActor2DCollection *a2Col;
  vtkActor2D *act2d;
  int j;
  
  renCol->InitTraversal();
  for (int i=0; i<nRen; ++i)
    {
    ren = renCol->GetNextItem();
    a2Col = ren->GetActors2D();

    for (a2Col->InitTraversal(), j=0; (act2d = a2Col->GetNextActor2D()); 
         ++j)
      {
      act2d->SetVisibility(static_cast<int>(act2dVis->GetComponent(i, j)));
      }
    }
}

vtkStandardNewMacro(vtkGL2PSExporter);

static float vtkGL2PSExporterGlobalPointSizeFactor = 5.0/7.0;
static float vtkGL2PSExporterGlobalLineWidthFactor = 5.0/7.0;

vtkGL2PSExporter::vtkGL2PSExporter()
{
  this->FilePrefix = NULL;
  this->FileFormat = EPS_FILE;
  this->Sort = SIMPLE_SORT;
  this->Compress = 1;
  this->DrawBackground = 1;
  this->SimpleLineOffset = 1;
  this->Silent = 0;
  this->BestRoot = 1;
  this->Text = 1;
  this->Landscape = 0;
  this->PS3Shading = 1;
  this->OcclusionCull = 1;
  this->Write3DPropsAsRasterImage = 0;
}

vtkGL2PSExporter::~vtkGL2PSExporter()
{
  if ( this->FilePrefix )
    {
    delete [] this->FilePrefix;
    }
}

void vtkGL2PSExporter::SetGlobalPointSizeFactor(float val)
{
    vtkGL2PSExporterGlobalPointSizeFactor = fabs(val);
}

float vtkGL2PSExporter::GetGlobalPointSizeFactor()
{
    return vtkGL2PSExporterGlobalPointSizeFactor;
}

void vtkGL2PSExporter::SetGlobalLineWidthFactor(float val)
{
    vtkGL2PSExporterGlobalLineWidthFactor = fabs(val);
}

float vtkGL2PSExporter::GetGlobalLineWidthFactor()
{
    return vtkGL2PSExporterGlobalLineWidthFactor;
}

void vtkGL2PSExporter::WriteData()
{
  FILE *fpObj;
  char *fName;
  GLint format = GL2PS_EPS;
  GLint sort;
  GLint options = GL2PS_NONE;
  int buffsize = 0;
  int state = GL2PS_OVERFLOW;
  GLint viewport[4];
  int *winsize = this->RenderWindow->GetSize();

  vtkRendererCollection *renCol = this->RenderWindow->GetRenderers();
  // Stores visibility of actors/volumes.
  vtkIntArray *volVis = vtkIntArray::New();
  vtkIntArray *actVis = vtkIntArray::New();
  vtkIntArray *act2dVis = vtkIntArray::New();
  float *floatpixels = 0; // for writing 3d props as an image.

  // Setting this to the entire window size for now.
  viewport[0] = 0;
  viewport[1] = 0;
  viewport[2] = winsize[0];
  viewport[3] = winsize[1];

  // make sure the user specified a file prefix
  if (this->FilePrefix == NULL)
    {
    vtkErrorMacro(<< "Please specify a file prefix to use");
    return;
    }

  // Set the options based on user's choices.
  if (this->Sort == NO_SORT)
    {
    sort = GL2PS_NO_SORT;
    }
  else if (this->Sort == SIMPLE_SORT)
    {
    sort = GL2PS_SIMPLE_SORT;
    }
  else
    {
    sort = GL2PS_BSP_SORT;
    }

  if (this->Compress == 1)
    {
    options = options | GL2PS_COMPRESS;
    }  
  if (this->DrawBackground == 1)
    {
    options = options | GL2PS_DRAW_BACKGROUND;
    }
  if (this->SimpleLineOffset == 1)
    {
    options = options | GL2PS_SIMPLE_LINE_OFFSET;
    }
  if (this->Silent == 1)
    {
    options = options | GL2PS_SILENT;
    }
  if (this->BestRoot == 1)
    {
    options = options | GL2PS_BEST_ROOT;
    }
  if (this->Text == 0)
    {
    options = options | GL2PS_NO_TEXT;
    }
  if (this->Landscape == 1)
    {
    options = options | GL2PS_LANDSCAPE;
    }
  if (this->PS3Shading == 0)
    {
    options = options | GL2PS_NO_PS3_SHADING;
    }
  if (this->OcclusionCull == 1)
    {
    options = options | GL2PS_OCCLUSION_CULL;
    }  

  // Setup the file.
  fName = new char [strlen(this->FilePrefix) + 5] ;
  if (this->FileFormat == PS_FILE)
    {
    sprintf(fName, "%s.ps", this->FilePrefix);
    format = GL2PS_PS;
    }
  else if (this->FileFormat == EPS_FILE)
    {
    sprintf(fName, "%s.eps", this->FilePrefix);
    format = GL2PS_EPS;
    }
  else if (this->FileFormat == PDF_FILE)
    {
    sprintf(fName, "%s.pdf", this->FilePrefix);
    format = GL2PS_PDF;
    }
  else if (this->FileFormat == TEX_FILE)
    {
    sprintf(fName, "%s.tex", this->FilePrefix);
    format = GL2PS_TEX;
    }
  else if (this->FileFormat == SVG_FILE)
    {
    sprintf(fName, "%s.svg", this->FilePrefix);
    format = GL2PS_SVG;
    }
  
  fpObj = fopen(fName, "wb");
  if (!fpObj)
    {
    vtkErrorMacro(<< "unable to open file: " << fName);
    return;
    }

  // Ready to write file.

  // Need to render once before rendering into the feedback buffer to
  // prevent problems.
  this->RenderWindow->Render();  

  // Write out a raster image without the 2d actors
  if (this->Write3DPropsAsRasterImage)
    {
    _SavePropVisibility(renCol, volVis, actVis, act2dVis);
    _Turn2DPropsOff(renCol);

    int numpix= winsize[0]*winsize[1]*3;
    floatpixels = new float [numpix];
    unsigned char *charpixels;
    int offscreen = this->RenderWindow->GetOffScreenRendering();

    this->RenderWindow->OffScreenRenderingOn();
    this->RenderWindow->Render();
    charpixels = this->RenderWindow->GetPixelData(0, 0, winsize[0] - 1,
                                                  winsize[1] - 1, 1);
    
    for (int i=0; i<numpix; i++)
      {
      floatpixels[i] = (float)(charpixels[i]/255.0);
      }
    this->RenderWindow->SetOffScreenRendering(offscreen);
    delete [] charpixels;
    }
  
  // Writing the file using GL2PS.
  vtkDebugMacro(<<"Writing file using GL2PS");

  // Call gl2ps to generate the file.
  while(state == GL2PS_OVERFLOW)
    {
    buffsize += 2048*2048;
    gl2psBeginPage(this->RenderWindow->GetWindowName(), "VTK", viewport,
                   format, sort, options, GL_RGBA, 0, 
                   NULL, 0, 0, 0, buffsize, fpObj, fName);

    if (this->Write3DPropsAsRasterImage)
      {
      // Dump the rendered image without 2d actors as a raster image.
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glPushMatrix();
      glRasterPos3f(-1.0, -1.0, -1.0);
      gl2psDrawPixels(winsize[0], winsize[1], 0, 0, GL_RGB, 
                      GL_FLOAT, floatpixels);
      glPopMatrix();  

      // Render the 2d actors alone in a vector graphic format.
      _Turn3DPropsOff(renCol);
      _Turn2DPropsOn(renCol, act2dVis);
      this->RenderWindow->Render();
      }
    else
      {
      this->RenderWindow->Render();
      }

    state = gl2psEndPage();
    }
  fclose(fpObj);

  // Clean up.
  if (this->Write3DPropsAsRasterImage)
    {
    // Reset the visibility.
    _Turn3DPropsOn(renCol, volVis, actVis);
    // memory
    delete [] floatpixels;
    // Re-render the scene to show all actors.
    this->RenderWindow->Render();
    }
  delete[] fName;
  volVis->Delete();
  actVis->Delete();
  act2dVis->Delete();

  vtkDebugMacro(<<"Finished writing file using GL2PS");
}

void vtkGL2PSExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
 
  if (this->FilePrefix)
    {
    os << indent << "FilePrefix: " << this->FilePrefix << "\n";
    }
  else
    {
    os << indent << "FilePrefix: (null)\n";      
    }

  os << indent << "FileFormat: " 
     << this->GetFileFormatAsString() << "\n";
  os << indent << "Sort: "
     << this->GetSortAsString() << "\n";
  os << indent << "Compress: "
     << (this->Compress ? "On\n" : "Off\n");
  os << indent << "DrawBackground: "
     << (this->DrawBackground ? "On\n" : "Off\n");
  os << indent << "SimpleLineOffset: "
     << (this->SimpleLineOffset ? "On\n" : "Off\n");
  os << indent << "Silent: "
     << (this->Silent ? "On\n" : "Off\n");
  os << indent << "BestRoot: "
     << (this->BestRoot ? "On\n" : "Off\n");
  os << indent << "Text: "
     << (this->Text ? "On\n" : "Off\n");
  os << indent << "Landscape: "
     << (this->Landscape ? "On\n" : "Off\n");
  os << indent << "PS3Shading: "
     << (this->PS3Shading ? "On\n" : "Off\n");
  os << indent << "OcclusionCull: "
     << (this->OcclusionCull ? "On\n" : "Off\n");
  os << indent << "Write3DPropsAsRasterImage: "
     << (this->Write3DPropsAsRasterImage ? "On\n" : "Off\n");
  os << indent << "GlobalPointSizeFactor:" 
     << vtkGL2PSExporterGlobalPointSizeFactor << endl;
  os << indent << "GlobalLineWidthFactor:" 
     << vtkGL2PSExporterGlobalLineWidthFactor << endl;
}
