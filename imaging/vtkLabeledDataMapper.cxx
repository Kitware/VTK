/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledDataMapper.cxx
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
#include "vtkLabeledDataMapper.h"
#include "vtkDataSet.h"

// Description:
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
  sprintf(this->LabelFormat,"%g");
  this->LabeledComponent = (-1);
  this->FieldDataArray = 0;
 
  this->NumberOfLabels = 0;
  this->TextMappers = NULL;
}

vtkLabeledDataMapper::~vtkLabeledDataMapper()
{
  if (this->LabelFormat) delete [] this->LabelFormat;

  if (this->TextMappers != NULL )
    {
    for (int i=0; i < this->NumberOfLabels; i++)
      {
      this->TextMappers[i]->Delete();
      }
    delete [] this->TextMappers;
    }
}

void vtkLabeledDataMapper::Render(vtkViewport *viewport, vtkActor2D *actor)
{
  int i, j, numComp, pointIdLabels, activeComp;
  char string[1024], format[1024];
  float val, x[3];
  vtkDataSet *input=this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkDataArray *data;
  float *tuple;
  vtkFieldData *fd;

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels");
    return;
    }
  else
    {
    input->Update();
    }

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
  input->GetMTime() > this->BuildTime ) 
    {
    vtkDebugMacro(<<"Rebuilding labels");

    // Delete previously constructed objects
    if (this->TextMappers != NULL )
      {
      for (int i=0; i < this->NumberOfLabels; i++)
	{
	this->TextMappers[i]->Delete();
	}
      delete [] this->TextMappers;
      }

    // figure out what to label, and if we can label it
    pointIdLabels = 0;
    data = NULL;
    switch (this->LabelMode)
      {
      case VTK_LABEL_IDS:
	pointIdLabels = 1;
	break;
      case VTK_LABEL_SCALARS:
	if ( pd->GetScalars() ) data = pd->GetScalars()->GetData();
	break;
      case VTK_LABEL_VECTORS:   
	if ( pd->GetVectors() ) data = pd->GetVectors()->GetData();
	break;
      case VTK_LABEL_NORMALS:    
	if ( pd->GetNormals() ) data = pd->GetNormals()->GetData();
	break;
      case VTK_LABEL_TCOORDS:    
	if ( pd->GetTCoords() ) data = pd->GetTCoords()->GetData();
	break;
      case VTK_LABEL_TENSORS:    
	if ( pd->GetTensors() ) data = pd->GetTensors()->GetData();
	break;
      case VTK_LABEL_FIELD_DATA:
	if ( (fd=pd->GetFieldData()) )
	  {
	  int arrayNum = (this->FieldDataArray < fd->GetNumberOfArrays() ?
		      this->FieldDataArray : fd->GetNumberOfArrays() - 1);
	  data = pd->GetFieldData()->GetArray(arrayNum);
	  }
	break;
      }

    // determine number of components and check input
    if ( pointIdLabels )
      ;
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
    this->TextMappers = new vtkTextMapper * [this->NumberOfLabels];
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
	if ( numComp == 1) sprintf(string, this->LabelFormat, tuple[activeComp]);
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

      this->TextMappers[i] = vtkTextMapper::New();
      this->TextMappers[i]->SetInput(string);
      this->TextMappers[i]->SetFontSize(this->FontSize);
      this->TextMappers[i]->SetBold(this->Bold);
      this->TextMappers[i]->SetItalic(this->Italic);
      this->TextMappers[i]->SetShadow(this->Shadow);
      this->TextMappers[i]->SetFontFamily(this->FontFamily);
      }

    if ( data ) delete [] tuple;

    this->BuildTime.Modified();
    }

  for (i=0; i<this->NumberOfLabels; i++)
    {
    this->Input->GetPoint(i,x);
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(x);
    this->TextMappers[i]->Render(viewport, actor);
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
  if ( this->LabelMode == VTK_LABEL_IDS ) os << "Label Ids\n";
  else if ( this->LabelMode == VTK_LABEL_SCALARS ) os << "Label Scalars\n";
  else if ( this->LabelMode == VTK_LABEL_VECTORS ) os << "Label Vectors\n";
  else if ( this->LabelMode == VTK_LABEL_NORMALS ) os << "Label Normals\n";
  else if ( this->LabelMode == VTK_LABEL_TCOORDS ) os << "Label TCoords\n";
  else if ( this->LabelMode == VTK_LABEL_TENSORS ) os << "Label Tensors\n";
  else os << "Label Field Data\n";

  os << indent << "Font Family: ";
  if ( this->FontFamily == VTK_ARIAL ) os << "Arial\n";
  else if ( this->FontFamily == VTK_COURIER ) os << "Courier\n";
  else os << "Times\n";

  os << indent << "Font Size: " << this->FontSize << "\n";
  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "Label Format: " << this->LabelFormat << "\n";

  os << indent << "Labeled Component: ";
  if ( this->LabeledComponent < 0 ) os << "(All Components)\n";
  else os << this->LabeledComponent << "\n";

  os << indent << "Field Data Array: " << this->FieldDataArray << "\n";
}
