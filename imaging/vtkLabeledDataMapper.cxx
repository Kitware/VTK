/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledDataMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkLabeledDataMapper.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkLabeledDataMapper* vtkLabeledDataMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLabeledDataMapper");
  if(ret)
    {
    return (vtkLabeledDataMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLabeledDataMapper;
}




// Instantiate object with font size 12 of font Arial (bolding,
// italic, shadows on) and %%-#6.3g label format. By default, point ids
// are labeled.
vtkLabeledDataMapper::vtkLabeledDataMapper()
{
  this->Input = NULL;
  this->LabelMode = VTK_LABEL_IDS;

  this->FontSize = 12;
  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->LabelFormat = new char[8]; 
  strcpy(this->LabelFormat,"%g");
  this->LabeledComponent = (-1);
  this->FieldDataArray = 0;
 
  this->NumberOfLabels = 0;
  this->NumberOfLabelsAllocated = 50;

  this->TextMappers = new vtkTextMapper * [this->NumberOfLabelsAllocated];
  for (int i=0; i<this->NumberOfLabelsAllocated; i++)
    {
    this->TextMappers[i] = vtkTextMapper::New();
    }
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkLabeledDataMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->TextMappers != NULL )
    {
    for (int i=0; i < this->NumberOfLabelsAllocated; i++)
      {
      this->TextMappers[i]->ReleaseGraphicsResources(win);
      }
    }
}

vtkLabeledDataMapper::~vtkLabeledDataMapper()
{
  if (this->LabelFormat)
    {
    delete [] this->LabelFormat;
    }

  if (this->TextMappers != NULL )
    {
    for (int i=0; i < this->NumberOfLabelsAllocated; i++)
      {
      this->TextMappers[i]->Delete();
      }
    delete [] this->TextMappers;
    }
  
  this->SetInput(NULL);
}

void vtkLabeledDataMapper::RenderOverlay(vtkViewport *viewport, 
					 vtkActor2D *actor)
{
  int i;
  float x[3];
  vtkDataSet *input=this->GetInput();

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels");
    return;
    }
  for (i=0; i<this->NumberOfLabels; i++)
    {
    this->Input->GetPoint(i,x);
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(x);
    this->TextMappers[i]->RenderOverlay(viewport, actor);
    }
}

void vtkLabeledDataMapper::RenderOpaqueGeometry(vtkViewport *viewport, 
						vtkActor2D *actor)
{
  int i, j, numComp = 0, pointIdLabels, activeComp = 0;
  char string[1024], format[1024];
  float val, x[3];
  vtkDataArray *data;
  float *tuple=NULL;
  vtkDataSet *input=this->GetInput();
  vtkPointData *pd=input->GetPointData();

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels");
    return;
    }


  input->Update();

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime ) 
    {
    vtkDebugMacro(<<"Rebuilding labels");

    // figure out what to label, and if we can label it
    pointIdLabels = 0;
    data = NULL;
    switch (this->LabelMode)
      {
      case VTK_LABEL_IDS:
	pointIdLabels = 1;
	break;
      case VTK_LABEL_SCALARS:
	if ( pd->GetScalars() )
	  {
	  data = pd->GetScalars()->GetData();
	  }
	break;
      case VTK_LABEL_VECTORS:   
	if ( pd->GetVectors() )
	  {
	  data = pd->GetVectors()->GetData();
	  }
	break;
      case VTK_LABEL_NORMALS:    
	if ( pd->GetNormals() )
	  {
	  data = pd->GetNormals()->GetData();
	  }
	break;
      case VTK_LABEL_TCOORDS:    
	if ( pd->GetTCoords() )
	  {
	  data = pd->GetTCoords()->GetData();
	  }
	break;
      case VTK_LABEL_TENSORS:    
	if ( pd->GetTensors() )
	  {
	  data = pd->GetTensors()->GetData();
	  }
	break;
      case VTK_LABEL_FIELD_DATA:
	int arrayNum = (this->FieldDataArray < pd->GetNumberOfArrays() ?
			this->FieldDataArray : pd->GetNumberOfArrays() - 1);
	data = pd->GetArray(arrayNum);
	break;
      }

    // determine number of components and check input
    if ( pointIdLabels )
      {
      ;
      }
    else if ( data )
      {
      numComp = data->GetNumberOfComponents();
      tuple = new float[numComp];
      activeComp = 0;
      if ( this->LabeledComponent >= 0 )
	{
	numComp = 1;
	activeComp = (this->LabeledComponent < numComp ? 
		      this->LabeledComponent : numComp - 1);
	}
      }
    else
      {
      vtkErrorMacro(<<"Need input data to render labels");
      return;
      }

    this->NumberOfLabels = this->Input->GetNumberOfPoints();
    if ( this->NumberOfLabels > this->NumberOfLabelsAllocated )
      {
      // delete old stuff
      for (i=0; i < this->NumberOfLabelsAllocated; i++)
	{
	this->TextMappers[i]->Delete();
	}
      delete [] this->TextMappers;

      this->NumberOfLabelsAllocated = this->NumberOfLabels;
      this->TextMappers = new vtkTextMapper * [this->NumberOfLabelsAllocated];
      for (i=0; i<this->NumberOfLabelsAllocated; i++)
	{
	this->TextMappers[i] = vtkTextMapper::New();
	}
      }//if we have to allocate new text mappers
    
    for (i=0; i < this->NumberOfLabels; i++)
      {
      if ( pointIdLabels )
	{
	val = (float)i;
        sprintf(string, this->LabelFormat, val);
	}
      else 
	{
        data->GetTuple(i, tuple);
	if ( numComp == 1)
	  {
	  sprintf(string, this->LabelFormat, tuple[activeComp]);
	  }
	else
	  {
	  strcpy(format, "("); strcat(format, this->LabelFormat);
          for (j=0; j<(numComp-1); j++)
	    {
	    sprintf(string, format, tuple[j]);
	    strcpy(format,string); strcat(format,", ");
            strcat(format, this->LabelFormat);
	    }
	  sprintf(string, format, tuple[numComp-1]);
	  strcat(string, ")");
	  }
	}

      this->TextMappers[i]->SetInput(string);
      this->TextMappers[i]->SetFontSize(this->FontSize);
      this->TextMappers[i]->SetBold(this->Bold);
      this->TextMappers[i]->SetItalic(this->Italic);
      this->TextMappers[i]->SetShadow(this->Shadow);
      this->TextMappers[i]->SetFontFamily(this->FontFamily);
      }

    if ( data )
      {
      delete [] tuple;
      }

    this->BuildTime.Modified();
    }

  for (i=0; i<this->NumberOfLabels; i++)
    {
    this->Input->GetPoint(i,x);
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(x);
    this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
    }
}

void vtkLabeledDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMapper2D::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  os << indent << "Label Mode: ";
  if ( this->LabelMode == VTK_LABEL_IDS )
    {
    os << "Label Ids\n";
    }
  else if ( this->LabelMode == VTK_LABEL_SCALARS )
    {
    os << "Label Scalars\n";
    }
  else if ( this->LabelMode == VTK_LABEL_VECTORS )
    {
    os << "Label Vectors\n";
    }
  else if ( this->LabelMode == VTK_LABEL_NORMALS )
    {
    os << "Label Normals\n";
    }
  else if ( this->LabelMode == VTK_LABEL_TCOORDS )
    {
    os << "Label TCoords\n";
    }
  else if ( this->LabelMode == VTK_LABEL_TENSORS )
    {
    os << "Label Tensors\n";
    }
  else
    {
    os << "Label Field Data\n";
    }

  os << indent << "Font Family: ";
  if ( this->FontFamily == VTK_ARIAL )
    {
    os << "Arial\n";
    }
  else if ( this->FontFamily == VTK_COURIER )
    {
    os << "Courier\n";
    }
  else
    {
    os << "Times\n";
    }

  os << indent << "Font Size: " << this->FontSize << "\n";
  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "Label Format: " << this->LabelFormat << "\n";

  os << indent << "Labeled Component: ";
  if ( this->LabeledComponent < 0 )
    {
    os << "(All Components)\n";
    }
  else
    {
    os << this->LabeledComponent << "\n";
    }

  os << indent << "Field Data Array: " << this->FieldDataArray << "\n";
}
