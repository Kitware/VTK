/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDynamic2DLabelMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDynamic2DLabelMapper.h"

#include "vtkActor2D.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkKdTree.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkViewport.h"

#include <vtksys/ios/fstream>
using vtksys_ios::ofstream;

#if defined(_WIN32) && !defined(__CYGWIN__)
# define SNPRINTF _snprintf
#else
# define SNPRINTF snprintf
#endif

vtkCxxRevisionMacro(vtkDynamic2DLabelMapper, "1.2");
vtkStandardNewMacro(vtkDynamic2DLabelMapper);

//----------------------------------------------------------------------------
// Creates a new label mapper

vtkDynamic2DLabelMapper::vtkDynamic2DLabelMapper()
{
  this->LabelWidth = NULL;
  this->LabelHeight = NULL;
  this->Cutoff = NULL;
  
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "priority");
  this->ReversePriority = false;
  this->LabelHeightPadding = 50;
  this->LabelWidthPadding = 10;

  // Set new default property
  vtkTextProperty* prop = vtkTextProperty::New();
  prop->SetFontSize(12);
  prop->SetBold(1);
  prop->SetItalic(0);
  prop->SetShadow(1);
  prop->SetFontFamilyToArial();
  prop->SetJustificationToCentered();
  prop->SetVerticalJustificationToCentered();
  prop->SetColor(1, 1, 1);
  this->SetLabelTextProperty(prop);
  prop->Delete();
}

//----------------------------------------------------------------------------
vtkDynamic2DLabelMapper::~vtkDynamic2DLabelMapper()
{
  if (this->LabelWidth)
    {
    delete[] this->LabelWidth;
    this->LabelWidth = NULL;
    }
  if (this->LabelHeight)
    {
    delete[] this->LabelHeight;
    this->LabelHeight = NULL;
    }
  if (this->Cutoff)
    {
    delete[] this->Cutoff;
    this->Cutoff = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::SetPriorityArrayName(const char* name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name);
}

//----------------------------------------------------------------------------
template<typename T>
void vtkDynamic2DLabelMapper_PrintComponent(char *output, const char *format, int index, const T *array)
{
  sprintf(output, format, array[index]);
}

//----------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::RenderOpaqueGeometry(vtkViewport *viewport, 
                                                vtkActor2D *actor)
{
  int i, j, numComp = 0, pointIdLabels, activeComp = 0;
  double x[3];
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
              vtkTemplateMacro(vtkDynamic2DLabelMapper_PrintComponent(TempString, LiveFormatString, activeComp, static_cast<VTK_TT *>(rawData)));
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
                  vtkDynamic2DLabelMapper_PrintComponent(TempString, 
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

    //
    // Perform the label layout preprocessing
    //
    
    // Calculate height and width padding
    float widthPadding=0, heightPadding = 0;
    if (this->NumberOfLabels > 0)
      {
      widthPadding = this->TextMappers[0]->GetHeight(viewport) *
              this->LabelWidthPadding/100.0;
      heightPadding = this->TextMappers[0]->GetHeight(viewport) *
               this->LabelHeightPadding/100.0;
      }

    // Calculate label widths / heights
    if (this->LabelWidth != NULL)
      {
      delete[] this->LabelWidth;
      }
    this->LabelWidth = new float[this->NumberOfLabels];
    for (i = 0; i < this->NumberOfLabels; i++)
      {
      this->LabelWidth[i] = this->TextMappers[i]->GetWidth(viewport)+
                            widthPadding;
      }

    if (this->LabelHeight != NULL)
      {
      delete[] this->LabelHeight;
      }
    this->LabelHeight = new float[this->NumberOfLabels];
    for (i = 0; i < this->NumberOfLabels; i++)
      {
      this->LabelHeight[i] = this->TextMappers[i]->GetHeight(viewport)+
                             heightPadding;
      }

    // Determine cutoff scales of each point
    if (this->Cutoff != NULL)
      {
      delete[] this->Cutoff;
      }
    this->Cutoff = new float[this->NumberOfLabels];

    //vtkTimerLog* timer = vtkTimerLog::New();
    //timer->StartTimer();

    vtkCoordinate* coord = vtkCoordinate::New();
    coord->SetViewport(viewport);
    vtkPoints* pts = vtkPoints::New();
    for (i = 0; i < this->NumberOfLabels; i++)
      {
      double* dc;
      double pti[3];
      input->GetPoint(i, pti);
      coord->SetValue(pti);
      dc = coord->GetComputedDoubleDisplayValue(0);
      pts->InsertNextPoint(dc[0], dc[1], 0);
      }

    //timer->StopTimer();
    //cerr << "vtkDynamic2DLabelMapper computed display coordinates for " << timer->GetElapsedTime() << "s" << endl;
    //timer->StartTimer();

    // Announce progress
    double progress = 0;
    this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));
    int current = 0;
    int total = this->NumberOfLabels*(this->NumberOfLabels - 1)/2;

    // Create an index array to store the offsets of the sorted elements.
    vtkIdTypeArray* index = vtkIdTypeArray::New();
    index->SetNumberOfValues(this->NumberOfLabels);
    for (i = 0; i < this->NumberOfLabels; i++)
      {
      index->SetValue(i, i);
      }
    
    // If the array is found, sort it and rearrange the corresponding index array.
    vtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, input);
    if (arr)
      {
      //cerr << "array is not null" << endl;
      vtkSortDataArray::Sort(arr, index);
      }
    
    // We normally go from highest (at the end) to lowest (at the beginning).
    // If priorities are reversed, we go from lowest to highest.
    // If no sorted array was used, we just go from index 0 to index n-1.
    vtkIdType begin = this->NumberOfLabels - 1;
    vtkIdType end = -1;
    vtkIdType step = -1;
    if ((this->ReversePriority && arr) || (!this->ReversePriority && !arr))
      {
      begin = 0;
      end = this->NumberOfLabels;
      step = 1;
      }
    for (i = begin; i != end; i += step)
      {
      vtkIdType indexI = index->GetValue(i);
      float* pti = reinterpret_cast<float*>(pts->GetVoidPointer(3*indexI));
      this->Cutoff[indexI] = VTK_FLOAT_MAX;
      for (j = begin; j != i; j += step)
        {
        vtkIdType indexJ = index->GetValue(j);
        float* ptj = reinterpret_cast<float*>(pts->GetVoidPointer(3*indexJ));
        float absX = (pti[0] - ptj[0]) > 0 ? (pti[0] - ptj[0]) : -(pti[0] - ptj[0]);
        float absY = (pti[1] - ptj[1]) > 0 ? (pti[1] - ptj[1]) : -(pti[1] - ptj[1]);
        float xScale = 2*absX/(float)(this->LabelWidth[indexI] + this->LabelWidth[indexJ]);
        float yScale = 2*absY/(float)(this->LabelHeight[indexI] + this->LabelHeight[indexJ]);
        float maxScale = xScale < yScale ? yScale : xScale;
        if (maxScale < this->Cutoff[indexJ] && maxScale < this->Cutoff[indexI])
          {
          this->Cutoff[indexI] = maxScale;
          }
        if (current % 100000 == 0)
          {
          progress = static_cast<double>(current)/total;
          this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));
          }
        current++;
        }
      }
    index->Delete();
    progress = 1.0;
    this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));

    pts->Delete();

    // Determine the reference scale
    // Transform 0,0 and 0,1 into screen coordinates
    coord->SetValue(0.0, 0.0, 0.0);
    double* dc = coord->GetComputedDoubleDisplayValue(0);
    double pos0 = dc[0];
    coord->SetValue(1.0, 0.0, 0.0);
    dc = coord->GetComputedDoubleDisplayValue(0);
    double pos1 = dc[0];
    this->ReferenceScale = (float)(pos1 - pos0);
    coord->Delete();

    //timer->StopTimer();
    //cerr << "vtkDynamic2DLabelMapper computed label cutoffs for " << timer->GetElapsedTime() << "s" << endl;
    //timer->Delete();
    }

  //
  // Draw labels visible in the current scale
  //

  // Determine the current scale
  // Transform 0,0 and 0,1 into screen coordinates
  vtkCoordinate* coord = vtkCoordinate::New();
  coord->SetViewport(viewport);
  coord->SetValue(0.0, 0.0, 0.0);
  double* dc = coord->GetComputedDoubleDisplayValue(0);
  double pos0 = dc[0];
  coord->SetValue(1.0, 0.0, 0.0);
  dc = coord->GetComputedDoubleDisplayValue(0);
  double pos1 = dc[0];
  float scale = (float)(pos1 - pos0) / this->ReferenceScale;
  coord->Delete();

  for (i = 0; i < this->NumberOfLabels; i++)
    {
    input->GetPoint(i,x);
    if ((1.0 / scale) < this->Cutoff[i])
      {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(x);
      this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
      }
    }
}

//----------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::RenderOverlay(vtkViewport *viewport, 
                                          vtkActor2D *actor)
{
  int i;
  double x[3];
  vtkDataSet *input=this->GetInput();
  vtkIdType numPts = input->GetNumberOfPoints();

  // Determine the current scale
  // Transform 0,0 and 0,1 into screen coordinates
  vtkCoordinate* coord = vtkCoordinate::New();
  coord->SetViewport(viewport);
  coord->SetValue(0.0, 0.0, 0.0);
  double* dc = coord->GetComputedDoubleDisplayValue(0);
  double x0 = dc[0];
  double y0 = dc[1];
  coord->SetValue(1.0, 1.0, 0.0);
  dc = coord->GetComputedDoubleDisplayValue(0);
  double x1 = dc[0];
  double y1 = dc[1];
  double absScaleX = (x1 - x0);
  double absScaleY = (y1 - y0);
  double scale = absScaleX / this->ReferenceScale;

  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels (1)");
    return;
    }
  for (i=0; i<this->NumberOfLabels && i<numPts; i++)
    {
    input->GetPoint(i,x);
    double screenX = x0 + absScaleX*x[0];
    double screenY = y0 + absScaleY*x[1];

    //vtkStringArray* arr = vtkStringArray::SafeDownCast(
    //  input->GetPointData()->GetAbstractArray(0));
    //cerr << "placing label \"" << arr->GetValue(i) << "\" at " << screenX << "," << screenY << endl;
    bool inside = 
      viewport->IsInViewport(
        static_cast<int>(screenX + this->LabelWidth[i]), 
        static_cast<int>(screenY + this->LabelHeight[i]))
      || viewport->IsInViewport(
        static_cast<int>(screenX + this->LabelWidth[i]), 
        static_cast<int>(screenY - this->LabelHeight[i]))
      || viewport->IsInViewport(
        static_cast<int>(screenX - this->LabelWidth[i]), 
        static_cast<int>(screenY + this->LabelHeight[i]))
      || viewport->IsInViewport(
        static_cast<int>(screenX - this->LabelWidth[i]), 
        static_cast<int>(screenY - this->LabelHeight[i]));
    if (inside && (1.0f / scale) < this->Cutoff[i])
      {
      actor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
      actor->GetPositionCoordinate()->SetValue(screenX, screenY, 0);
      this->TextMappers[i]->RenderOverlay(viewport, actor);
      }
    }
  coord->Delete();

  //timer->StopTimer();
  //cerr << "vtkDynamic2DLabelMapper interactive time: " << timer->GetElapsedTime() << "s" << endl;
  //timer->Delete();
}

//----------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ReversePriority: " << (this->ReversePriority ? "on" : "off") << endl;
  os << indent << "LabelHeightPadding: " << (this->LabelHeightPadding ? "on" : "off") << endl;
  os << indent << "LabelWidthPadding: " << (this->LabelWidthPadding ? "on" : "off") << endl;
}
