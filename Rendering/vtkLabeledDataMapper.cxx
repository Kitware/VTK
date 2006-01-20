/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLabeledDataMapper.h"

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkActor2D.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"

vtkCxxRevisionMacro(vtkLabeledDataMapper, "1.42");
vtkStandardNewMacro(vtkLabeledDataMapper);

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
  this->FieldDataName = NULL;

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
  
  this->SetLabelTextProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkLabeledDataMapper::SetInput(vtkDataSet *input)
{
  if (input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(0, 0);
    }
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkLabeledDataMapper::GetInput()
{
  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this mapper.
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
  double x[3];
  vtkDataSet *input=this->GetInput();
  vtkIdType numPts = input->GetNumberOfPoints();

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels (1)");
    return;
    }
  for (i=0; i<this->NumberOfLabels && i<numPts; i++)
    {
    input->GetPoint(i,x);
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
  double val, x[3];
  vtkAbstractArray *abstractData;
  vtkDataArray *numericData;
  vtkStringArray *stringData;
  vtkDataSet *input=this->GetInput();

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels (2)");
    return;
    }

  vtkTextProperty *tprop = this->LabelTextProperty;
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render labels");
    return;
    }

  input->Update();

  // Input might have changed
  input = this->GetInput();
  vtkPointData *pd=input->GetPointData();

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime ||
       tprop->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<<"Rebuilding labels");

    // figure out what to label, and if we can label it
    pointIdLabels = 0;
    abstractData = NULL;
    numericData = NULL;
    stringData = NULL;
    switch (this->LabelMode)
      {
      case VTK_LABEL_IDS:
        pointIdLabels = 1;
        break;
      case VTK_LABEL_SCALARS:
        if ( pd->GetScalars() )
          {
          numericData = pd->GetScalars();
          }
        break;
      case VTK_LABEL_VECTORS:   
        if ( pd->GetVectors() )
          {
          numericData = pd->GetVectors();
          }
        break;
      case VTK_LABEL_NORMALS:    
        if ( pd->GetNormals() )
          {
          numericData = pd->GetNormals();
          }
        break;
      case VTK_LABEL_TCOORDS:    
        if ( pd->GetTCoords() )
          {
          numericData = pd->GetTCoords();
          }
        break;
      case VTK_LABEL_TENSORS:    
        if ( pd->GetTensors() )
          {
          numericData = pd->GetTensors();
          }
        break;
      case VTK_LABEL_FIELD_DATA:
      {
      int arrayNum;
      if (this->FieldDataName != NULL)
        {
        abstractData = pd->GetAbstractArray(this->FieldDataName, arrayNum);
        }
      else
        {
        arrayNum = (this->FieldDataArray < pd->GetNumberOfArrays() ?
                    this->FieldDataArray : pd->GetNumberOfArrays() - 1);
        abstractData = pd->GetAbstractArray(arrayNum);
        }
      numericData = vtkDataArray::SafeDownCast(abstractData);
      stringData = vtkStringArray::SafeDownCast(abstractData);
      }; break;
      }

    // determine number of components and check input
    if ( pointIdLabels )
      {
      ;
      }
    else if ( numericData )
      {
      numComp = numericData->GetNumberOfComponents();
      activeComp = 0;
      if ( this->LabeledComponent >= 0 )
        {
        activeComp = (this->LabeledComponent < numComp ? 
                      this->LabeledComponent : numComp - 1);
        numComp = 1;
        }
      }
    else if ( !stringData )
      {
      vtkErrorMacro(<<"Need input data to render labels (3)");
      return;
      }

    this->NumberOfLabels = input->GetNumberOfPoints();
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
        if ( numericData )
          {
          if ( numComp == 1 )
            {
            if (numericData->GetDataType() == VTK_CHAR) 
              {
              if (strcmp(this->LabelFormat,"%c") != 0) 
                {
                vtkErrorMacro(<<"Label format must be %c to use with char");
                return;
                }
              sprintf(string, this->LabelFormat, 
                      (char)numericData->GetComponent(i, activeComp));
              } 
            else 
              {
              sprintf(string, this->LabelFormat, 
                      numericData->GetComponent(i, activeComp));
              }
            }
          else
            {
            strcpy(format, "("); strcat(format, this->LabelFormat);
            for (j=0; j<(numComp-1); j++)
              {
              sprintf(string, format, numericData->GetComponent(i, j));
              strcpy(format,string); strcat(format,", ");
              strcat(format, this->LabelFormat);
              }
            sprintf(string, format, numericData->GetComponent(i, numComp-1));
            strcat(string, ")");
            }
          }
        else // rendering string data
          {
          if (strcmp(this->LabelFormat,"%s") != 0) 
            {
            vtkErrorMacro(<<"Label format must be %s to use with strings");
            return;
            }
          sprintf(string, this->LabelFormat, 
                  stringData->GetValue(i).c_str());
          }
        } // not point labels
      this->TextMappers[i]->SetInput(string);
      this->TextMappers[i]->SetTextProperty(tprop);
      }

    this->BuildTime.Modified();
    }

  for (i=0; i<this->NumberOfLabels; i++)
    {
    input->GetPoint(i,x);
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(x);
    this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
    }
}

//----------------------------------------------------------------------------
int vtkLabeledDataMapper::FillInputPortInformation(
  int vtkNotUsed( port ), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
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
  os << indent << "Field Data Name: " << (this->FieldDataName ? this->FieldDataName : "Null") << "\n";
}

// ----------------------------------------------------------------------
void
vtkLabeledDataMapper::SetFieldDataArray(int arrayIndex)
{
  if (this->FieldDataName)
    {
    delete [] this->FieldDataName;
    this->FieldDataName = NULL;
    }

  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting FieldDataArray to " << arrayIndex ); 

  if (this->FieldDataArray != (arrayIndex < 0 ? 0 : 
                               (arrayIndex > VTK_LARGE_INTEGER ? VTK_LARGE_INTEGER : arrayIndex)))
    {
    this->FieldDataArray = ( arrayIndex < 0 ? 0 : 
                             (arrayIndex > VTK_LARGE_INTEGER ? VTK_LARGE_INTEGER : arrayIndex ));
    this->Modified();
    }
}

// ----------------------------------------------------------------------
void
vtkLabeledDataMapper::SetFieldDataName(const char *arrayName)
{
  vtkDebugMacro(<< this->GetClassName() 
                << " (" << this << "): setting " << "FieldDataName" 
                << " to " << (arrayName?arrayName:"(null)") ); 

  if ( this->FieldDataName == NULL && arrayName == NULL) { return; } 
  if ( this->FieldDataName && arrayName && (!strcmp(this->FieldDataName,arrayName))) { return;} 
  if (this->FieldDataName) { delete [] this->FieldDataName; } 
  if (arrayName) 
    { 
    this->FieldDataName = new char[strlen(arrayName)+1]; 
    strcpy(this->FieldDataName,arrayName); 
    } 
   else 
    { 
    this->FieldDataName = NULL; 
    } 
  this->Modified(); 
}
