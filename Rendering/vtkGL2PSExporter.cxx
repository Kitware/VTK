/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSExporter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGL2PSExporter.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "gl2ps.h"

vtkCxxRevisionMacro(vtkGL2PSExporter, "1.4");
vtkStandardNewMacro(vtkGL2PSExporter);

vtkGL2PSExporter::vtkGL2PSExporter()
{
  this->FilePrefix = NULL;
  this->FileFormat = EPS_FILE;
  this->Sort = SIMPLE_SORT;
  this->DrawBackground = 1;
  this->SimpleLineOffset = 1;
  this->Silent = 0;
  this->BestRoot = 1;
  this->Text = 1;
  this->Landscape = 0;
  this->PS3Shading = 1;
  this->OcclusionCull = 1;
}

vtkGL2PSExporter::~vtkGL2PSExporter()
{
  if ( this->FilePrefix )
    {
    delete [] this->FilePrefix;
    }
}

void vtkGL2PSExporter::WriteData()
{
  FILE *fpObj;
  char *fName;
  GLint format;
  GLint sort;
  GLint options = GL2PS_NONE;
  int buffsize = 0;
  int state = GL2PS_OVERFLOW;
  GLint viewport[4];
  int *sz = this->RenderWindow->GetSize();

  // Setting this to the entire window size for now.
  viewport[0] = 0;
  viewport[1] = 0;
  viewport[2] = sz[0];
  viewport[3] = sz[1];

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

  // gl2ps segfaults if sorting is performed when TeX output is
  // chosen.  Sorting is irrelevant for Tex Output anyway.
  if (this->FileFormat == TEX_FILE)
    {
    sort = GL2PS_NO_SORT;
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
  fName = new char [strlen(this->FilePrefix) + 4] ;
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
  else if (this->FileFormat == TEX_FILE)
    {
    sprintf(fName, "%s.tex", this->FilePrefix);
    format = GL2PS_TEX;
    }
  
  fpObj = fopen(fName, "w");
  if (!fpObj)
    {
    vtkErrorMacro(<< "unable to open file: " << fName);
    return;
    }

  // Writing the file using GL2PS.
  vtkDebugMacro(<<"Writing file using GL2PS");

  // Call gl2ps to generate the file.
  while(state == GL2PS_OVERFLOW)
    {
    buffsize += 1024*1024;
    gl2psBeginPage(this->RenderWindow->GetWindowName(), "VTK", viewport,
                   format, sort, options, GL_RGBA, 0, 
                   NULL, 0, 0, 0, buffsize, fpObj, fName);
    this->RenderWindow->Render();
    state = gl2psEndPage();
    }
  fclose(fpObj);

  // GL2PS versions upto 0.9.0 do not reset the render mode.
  if (this->FileFormat == TEX_FILE)
    {
    glRenderMode(GL_RENDER);
    }

  delete[] fName;

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
}
