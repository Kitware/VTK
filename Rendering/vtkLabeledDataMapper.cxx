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
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkLabeledDataMapper, "1.49");
vtkStandardNewMacro(vtkLabeledDataMapper);

vtkCxxSetObjectMacro(vtkLabeledDataMapper,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkLabeledDataMapper,Transform,vtkTransform);

#if defined(_WIN32) && !defined(__CYGWIN__)
# define SNPRINTF _snprintf
#else
# define SNPRINTF snprintf
#endif

// ----------------------------------------------------------------------

template<typename T>
void vtkLabeledDataMapper_PrintComponent(char *output, const char *format, int index, const T *array)
{
  sprintf(output, format, array[index]);
}


//----------------------------------------------------------------------------
// Creates a new label mapper

vtkLabeledDataMapper::vtkLabeledDataMapper()
{
  this->Input = NULL;
  this->LabelMode = VTK_LABEL_IDS;

  this->LabelFormat = NULL;

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
  
  this->Transform = 0;
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
  this->SetFieldDataName(NULL);
  this->SetTransform(NULL);
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
    double* pos = x;
    if (this->Transform)
      {
      pos = this->Transform->TransformDoublePoint(x);
      }
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(pos);
    this->TextMappers[i]->RenderOverlay(viewport, actor);
    }
}

//----------------------------------------------------------------------------
void vtkLabeledDataMapper::RenderOpaqueGeometry(vtkViewport *viewport, 
                                                vtkActor2D *actor)
{
  int i, j, numComp = 0, pointIdLabels = 0, activeComp = 0;
  vtkAbstractArray *abstractData = NULL;
  vtkDataArray *numericData = NULL;
  vtkStringArray *stringData = NULL;
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
      {
        pointIdLabels = 1;
      }; 
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
        vtkDebugMacro(<<"Labeling field data array " << this->FieldDataName);
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
      numComp = 1;
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
    else 
      {
      if ( stringData )
        {
        numComp = stringData->GetNumberOfComponents();
        }
      else
        {
        vtkErrorMacro(<<"Need input data to render labels (3)");
        return;
        }
      }

    vtkStdString FormatString;
    if (this->LabelFormat)
      {
      // The user has specified a format string.  
      vtkDebugMacro(<<"Using user-specified format string " << this->LabelFormat);
      FormatString = this->LabelFormat;
      }
    else
      {
      // Try to come up with some sane default.
      if (pointIdLabels)
        {
        FormatString = "%d"; 
        }
      else if (numericData)
        {
        switch (numericData->GetDataType())
          {
          case VTK_VOID: FormatString = "0x%x"; break;

          case VTK_BIT:
          case VTK_SHORT:
          case VTK_UNSIGNED_SHORT:
          case VTK_INT:
          case VTK_UNSIGNED_INT:
            FormatString = "%d"; break;

          case VTK_CHAR:
          case VTK_SIGNED_CHAR:
          case VTK_UNSIGNED_CHAR:
            FormatString = "%c"; break;

          case VTK_LONG:
          case VTK_UNSIGNED_LONG:
          case VTK_ID_TYPE:
            FormatString = "%ld"; break;

          case VTK_LONG_LONG:
          case VTK_UNSIGNED_LONG_LONG:
          case VTK___INT64:
          case VTK_UNSIGNED___INT64:
            FormatString = "%lld"; break;

          case VTK_FLOAT:
            FormatString = "%f"; break;

          case VTK_DOUBLE:
            FormatString = "%g"; break;

          default:
            FormatString = "BUG - UNKNOWN DATA FORMAT"; break;
          }
        }
      else if (stringData)
        {
        FormatString = ""; // we'll use vtkStdString::operator+ instead of sprintf
        }
      else
        {
        FormatString = "BUG - COULDN'T DETECT DATA TYPE"; 
        }

      vtkDebugMacro(<<"Using default format string " << FormatString.c_str());

      } // Done building default format string
    
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
    

    // ----------------------------------------
    // Now we actually construct the label strings
    //

    const char *LiveFormatString = FormatString.c_str();
    char TempString[1024];

    for (i=0; i < this->NumberOfLabels; i++)
      {
      vtkStdString ResultString; 

      if ( pointIdLabels )
        {
        sprintf(TempString, LiveFormatString, i);
        ResultString = TempString;
        }
      else 
        {
        if ( numericData )
          {
          void *rawData = numericData->GetVoidPointer(i);
          
          if ( numComp == 1 )
            {
            switch (numericData->GetDataType())
              {
              vtkTemplateMacro(vtkLabeledDataMapper_PrintComponent(TempString, LiveFormatString, activeComp, static_cast<VTK_TT *>(rawData)));
              }
            ResultString = TempString;
            } 
          else // numComp != 1
            {
            ResultString = "(";
            
            // Print each component in turn and add it to the string.
            for (j = 0; j < numComp; ++j)
              {
              switch (numericData->GetDataType())
                {
                vtkTemplateMacro(
                  vtkLabeledDataMapper_PrintComponent(TempString, 
                                                      LiveFormatString, 
                                                      j, 
                                                      static_cast<VTK_TT *>(rawData)));
                }
              ResultString += TempString;
              
              if (j < (numComp-1))
                {
                ResultString += ' ';
                }
              else
                {
                ResultString += ')';
                }
              }
            }
          }
        else // rendering string data
          {
          // If the user hasn't given us a custom format string then
          // we'll sidestep a lot of sprintf nonsense.
          if (this->LabelFormat == NULL)
            {
            ResultString = stringData->GetValue(i);
            }
          else // the user specified a label format
            {
            SNPRINTF(TempString, 1023, LiveFormatString, 
                     stringData->GetValue(i).c_str());
              ResultString = TempString;
            } // done printing strings with label format
          } // done printing strings
        } // done creating string 

      this->TextMappers[i]->SetInput(ResultString.c_str());
      this->TextMappers[i]->SetTextProperty(tprop);
      }

    this->BuildTime.Modified();
    }

  double x[3];
  for (i=0; i<this->NumberOfLabels; i++)
    {
    input->GetPoint(i,x);
    double* pos = x;
    if (this->Transform)
      {
      pos = this->Transform->TransformDoublePoint(x);
      }
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(pos);
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

  os << indent << "Label Format: " << (this->LabelFormat ? this->LabelFormat : "Null") << "\n";

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
  
  os << indent << "Transform: " << (this->Transform ? "" : "(none)") << endl;
  if (this->Transform)
    {
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
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
