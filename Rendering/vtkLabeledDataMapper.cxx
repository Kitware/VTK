/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledDataMapper.cxx
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
#include "vtkLabeledDataMapper.h"

#include "vtkActor2D.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"

vtkCxxRevisionMacro(vtkLabeledDataMapper, "1.31");
vtkStandardNewMacro(vtkLabeledDataMapper);

vtkCxxSetObjectMacro(vtkLabeledDataMapper,Input, vtkDataSet);
vtkCxxSetObjectMacro(vtkLabeledDataMapper,LabelTextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Creates a new label mapper

vtkLabeledDataMapper::vtkLabeledDataMapper()
{
  this->Input = NULL;
  this->LabelMode = VTK_LABEL_IDS;

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

  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetFontSize(12);
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();
}

//----------------------------------------------------------------------------
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
  this->SetLabelTextProperty(NULL);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

  vtkTextProperty *tprop = this->LabelTextProperty;
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render labels");
    return;
    }

  input->Update();

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime ||
       tprop->GetMTime() > this->BuildTime)
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
          data = pd->GetScalars();
          }
        break;
      case VTK_LABEL_VECTORS:   
        if ( pd->GetVectors() )
          {
          data = pd->GetVectors();
          }
        break;
      case VTK_LABEL_NORMALS:    
        if ( pd->GetNormals() )
          {
          data = pd->GetNormals();
          }
        break;
      case VTK_LABEL_TCOORDS:    
        if ( pd->GetTCoords() )
          {
          data = pd->GetTCoords();
          }
        break;
      case VTK_LABEL_TENSORS:    
        if ( pd->GetTensors() )
          {
          data = pd->GetTensors();
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
      this->TextMappers[i]->SetTextProperty(tprop);
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

//----------------------------------------------------------------------------
void vtkLabeledDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  if (this->LabelTextProperty)
    {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Label Text Property: (none)\n";
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

//----------------------------------------------------------------------------
// Backward compatibility calls

void vtkLabeledDataMapper::SetFontFamily(int val) 
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetFontFamily(val); 
    }
}

int vtkLabeledDataMapper::GetFontFamily()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetFontFamily(); 
    }
  else
    {
    return 0;
    }
}

void vtkLabeledDataMapper::SetFontSize(int size) 
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetFontSize(size); 
    }
}

int vtkLabeledDataMapper::GetFontSize()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetFontSize(); 
    }
  else
    {
    return 0;
    }
}
  
void vtkLabeledDataMapper::SetBold(int val)
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetBold(val); 
    }
}

int vtkLabeledDataMapper::GetBold()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetBold(); 
    }
  else
    {
    return 0;
    }
}
  
void vtkLabeledDataMapper::SetItalic(int val)
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetItalic(val); 
    }
}

int vtkLabeledDataMapper::GetItalic()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetItalic(); 
    }
  else
    {
    return 0;
    }
}

void vtkLabeledDataMapper::SetShadow(int val)
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetShadow(val); 
    }
}

int vtkLabeledDataMapper::GetShadow()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetShadow(); 
    }
  else
    {
    return 0;
    }
}
