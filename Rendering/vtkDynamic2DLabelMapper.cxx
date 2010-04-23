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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkDynamic2DLabelMapper.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkKdTree.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkTypeTraits.h"
#include "vtkUnicodeStringArray.h"
#include "vtkViewport.h"

#include <vtksys/ios/fstream>
using vtksys_ios::ofstream;

#if defined(_WIN32) && !defined(__CYGWIN__)
# define SNPRINTF _snprintf
#else
# define SNPRINTF snprintf
#endif

vtkStandardNewMacro(vtkDynamic2DLabelMapper);

//----------------------------------------------------------------------------
// Creates a new label mapper

vtkDynamic2DLabelMapper::vtkDynamic2DLabelMapper()
{
  this->LabelWidth = NULL;
  this->LabelHeight = NULL;
  this->Cutoff = NULL;
  
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "priority");
  this->ReversePriority = false;
  this->LabelHeightPadding = 50;
  this->LabelWidthPadding = 10;
  this->ReferenceScale = 1.0;

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
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name);
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
  vtkUnicodeStringArray* uStringData;
  vtkDataObject *input = this->GetExecutive()->GetInputData(0, 0);

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels (2)");
    return;
    }

  vtkTextProperty *tprop = this->GetLabelTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render labels");
    return;
    }

  input->Update();

  // Input might have changed
  input = this->GetExecutive()->GetInputData(0, 0);

  vtkDataSet *dsInput = vtkDataSet::SafeDownCast(input);
  vtkGraph *gInput = vtkGraph::SafeDownCast(input);
  if (!dsInput && !gInput)
    {
    vtkErrorMacro(<<"Input must be vtkDataSet or vtkGraph.");
    return;
    }
  vtkDataSetAttributes *pd = 
    dsInput ? dsInput->GetPointData() : gInput->GetVertexData();

  // If no labels we are done
  vtkIdType numItems = dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();
  if (numItems == 0)
    {
    return;
    }

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<<"Rebuilding labels");

    vtkIntArray *typeArr = vtkIntArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(0, input));

    // figure out what to label, and if we can label it
    pointIdLabels = 0;
    abstractData = NULL;
    numericData = NULL;
    stringData = NULL;
    uStringData = NULL;
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
      uStringData = vtkUnicodeStringArray::SafeDownCast(abstractData);
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
    else if ( uStringData )
      {
      vtkWarningMacro( "Unicode string arrays are not adequately supported by the vtkDynamic2DLabelMapper.  Unicode strings will be converted to vtkStdStrings for rendering.");
      }
    else if ( !stringData )
      {
      if (this->FieldDataName)
        {
        vtkWarningMacro(<< "Could not find label array ("
                        << this->FieldDataName << ") "
                        << "in input.");
        }
      else 
        {
        vtkWarningMacro(<< "Could not find label array ("
                        << "index " << this->FieldDataArray << ") "
                        << "in input.");
        }

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

          // dont use vtkTypeTraits::ParseFormat for character types as parse formats
          // aren't the same as print formats for these types.
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
            FormatString = vtkTypeTraits<long>::ParseFormat(); break;
          case VTK_UNSIGNED_LONG:
            FormatString = vtkTypeTraits<unsigned long>::ParseFormat(); break;

          case VTK_ID_TYPE:
            FormatString = vtkTypeTraits<vtkIdType>::ParseFormat(); break;

#if defined(VTK_TYPE_USE_LONG_LONG)
          case VTK_LONG_LONG:
            FormatString = vtkTypeTraits<long long>::ParseFormat(); break;
          case VTK_UNSIGNED_LONG_LONG:
            FormatString = vtkTypeTraits<unsigned long long>::ParseFormat(); break;
#endif

#if defined(VTK_TYPE_USE___INT64)
          case VTK___INT64:
            FormatString = vtkTypeTraits<__int64>::ParseFormat(); break;
          case VTK_UNSIGNED___INT64:
            FormatString = vtkTypeTraits<unsigned __int64>::ParseFormat(); break;
#endif

          case VTK_FLOAT:
            FormatString = vtkTypeTraits<float>::ParseFormat(); break;

          case VTK_DOUBLE:
            FormatString = vtkTypeTraits<double>::ParseFormat(); break;

          default:
            FormatString = "BUG - UNKNOWN DATA FORMAT"; break;
          }
        }
      else if (stringData)
        {
        FormatString = ""; // we'll use vtkStdString::operator+ instead of sprintf
        }
      else if (uStringData)
        {
        FormatString = "unicode"; // we'll use vtkStdString::operator+ instead of sprintf
        }
      else
        {
        FormatString = "BUG - COULDN'T DETECT DATA TYPE"; 
        }

      vtkDebugMacro(<<"Using default format string " << FormatString.c_str());
      } // Done building default format string

    this->NumberOfLabels = 
      dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();
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
            if( uStringData )
              {
              ResultString = uStringData->GetValue(i).utf8_str();
              }
            else
              {
              ResultString = stringData->GetValue(i);
              }
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

      // Find the correct property type
      int type = 0;
      if (typeArr)
        {
        type = typeArr->GetValue(i);
        }
      vtkTextProperty* prop = this->GetLabelTextProperty(type);
      if (!prop)
        {
        prop = this->GetLabelTextProperty(0);
        }
      this->TextMappers[i]->SetTextProperty(prop);
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

    vtkTimerLog* timer = vtkTimerLog::New();
    timer->StartTimer();

    vtkCoordinate* coord = vtkCoordinate::New();
    coord->SetViewport(viewport);
    vtkPoints* pts = vtkPoints::New();
    for (i = 0; i < this->NumberOfLabels; i++)
      {
      double* dc;
      double pti[3];
      if (dsInput)
        {
        dsInput->GetPoint(i, pti);
        }
      else
        {
        gInput->GetPoint(i, pti);
        }
      coord->SetValue(pti);
      dc = coord->GetComputedDoubleDisplayValue(0);
      pts->InsertNextPoint(dc[0], dc[1], 0);
      }
    coord->Delete();

    timer->StopTimer();
    vtkDebugMacro("vtkDynamic2DLabelMapper computed display coordinates for " << timer->GetElapsedTime() << "s");
    timer->StartTimer();

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
    vtkAbstractArray* inputArr = this->GetInputAbstractArrayToProcess(1, input);
    if (inputArr)
      {
      // Don't sort the original array, instead make a copy.
      vtkAbstractArray* arr = vtkAbstractArray::CreateArray(inputArr->GetDataType());
      arr->DeepCopy(inputArr);
      vtkSortDataArray::Sort(arr, index);
      arr->Delete();
      }
    
    // We normally go from highest (at the end) to lowest (at the beginning).
    // If priorities are reversed, we go from lowest to highest.
    // If no sorted array was used, we just go from index 0 to index n-1.
    vtkIdType begin = this->NumberOfLabels - 1;
    vtkIdType end = -1;
    vtkIdType step = -1;
    if ((this->ReversePriority && inputArr) || (!this->ReversePriority && !inputArr))
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
        float xScale = 2*absX/(this->LabelWidth[indexI] + this->LabelWidth[indexJ]);
        float yScale = 2*absY/(this->LabelHeight[indexI] + this->LabelHeight[indexJ]);
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
    this->ReferenceScale = this->GetCurrentScale(viewport);    

    timer->StopTimer();
    vtkDebugMacro("vtkDynamic2DLabelMapper computed label cutoffs for " << timer->GetElapsedTime() << "s");
    timer->Delete();
    }

  //
  // Draw labels visible in the current scale
  //

  // Determine the current scale
  double scale = 1.0;
  if (this->ReferenceScale != 0.0)
    {
    scale = this->GetCurrentScale(viewport) / this->ReferenceScale;    
    }

  for (i = 0; i < this->NumberOfLabels; i++)
    {
    if (dsInput)
      {
      dsInput->GetPoint(i,x);
      }
    else
      {
      gInput->GetPoint(i,x);
      }
    if ((1.0 / scale) < this->Cutoff[i])
      {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(x);
      this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
      }
    }
}

//----------------------------------------------------------------------------
double vtkDynamic2DLabelMapper::GetCurrentScale(vtkViewport *viewport)
{
  // The current scale is the size on the screen of 1 unit in the xy plane

  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  if ( ! ren )
    {
    vtkErrorMacro("vtkDynamic2DLabelMapper only works in a vtkRenderer or subclass");
    return 1.0;
    }
  vtkCamera* camera = ren->GetActiveCamera();
  if ( camera->GetParallelProjection() )
    {
    // For parallel projection, the scale depends on the parallel scale 
    double scale = ( ren->GetSize()[1] / 2.0 ) / camera->GetParallelScale();
    return scale;
    }
  else
    {
    // For perspective projection, the scale depends on the view angle
    double viewAngle = camera->GetViewAngle();
    double distZ = camera->GetPosition()[2] > 0 ? camera->GetPosition()[2] : -camera->GetPosition()[2];
    double unitAngle = vtkMath::DegreesFromRadians( atan2( 1.0, distZ ) );
    double scale = ren->GetSize()[1] * unitAngle / viewAngle;
    return scale;
    }
}

//----------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::RenderOverlay(vtkViewport *viewport, 
                                            vtkActor2D *actor)
{
  int i;
  double x[3];
  vtkDataObject *input = this->GetExecutive()->GetInputData(0, 0);
  vtkGraph *gInput = vtkGraph::SafeDownCast(input);
  vtkDataSet *dsInput = vtkDataSet::SafeDownCast(input);
  vtkIdType numPts = dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();

  // Determine the current scale
  double scale = 1.0;
  if (this->ReferenceScale != 0.0)
    {
    scale = this->GetCurrentScale(viewport) / this->ReferenceScale;
    }

  if ( ! input )
    {
    vtkErrorMacro(<<"Need input data to render labels (1)");
    return;
    }
  
  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();
  
  for (i=0; i<this->NumberOfLabels && i<numPts; i++)
    {
    if (dsInput)
      {
      dsInput->GetPoint(i, x);
      }
    else
      {
      gInput->GetPoint(i, x);
      }
    actor->SetPosition(x);
    double* display = actor->GetPositionCoordinate()->GetComputedDoubleDisplayValue(viewport);
    double screenX = display[0];
    double screenY = display[1];

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
      this->TextMappers[i]->RenderOverlay(viewport, actor);
      }
    }

  timer->StopTimer();
  vtkDebugMacro("vtkDynamic2DLabelMapper interactive time: " << timer->GetElapsedTime() << "s");
  timer->Delete();
}

//----------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ReversePriority: " << (this->ReversePriority ? "on" : "off") << endl;
  os << indent << "LabelHeightPadding: " << (this->LabelHeightPadding ? "on" : "off") << endl;
  os << indent << "LabelWidthPadding: " << (this->LabelWidthPadding ? "on" : "off") << endl;
}
