/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainterPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPainterPolyDataMapper.h"

#include "vtkChooserPainter.h"
#include "vtkClipPlanesPainter.h"
#include "vtkCoincidentTopologyResolutionPainter.h"
#include "vtkCommand.h"
#include "vtkDefaultPainter.h"
#include "vtkDisplayListPainter.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericVertexAttributeMapping.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyData.h"
#include "vtkPrimitivePainter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkStandardPolyDataPainter.h"
#include "vtkgl.h"

vtkStandardNewMacro(vtkPainterPolyDataMapper);
//-----------------------------------------------------------------------------
class vtkPainterPolyDataMapperObserver : public vtkCommand
{
public:
  static vtkPainterPolyDataMapperObserver* New()
    { return new vtkPainterPolyDataMapperObserver; }

  virtual void Execute(vtkObject* caller, unsigned long event, void*)
    {
    vtkPainter* p = vtkPainter::SafeDownCast(caller);
    if (this->Target && p && event == vtkCommand::ProgressEvent)
      {
      this->Target->UpdateProgress(p->GetProgress());
      }
    }
  vtkPainterPolyDataMapperObserver()
    {
    this->Target = 0;
    }
  vtkPainterPolyDataMapper* Target;
};


//-----------------------------------------------------------------------------
vtkPainterPolyDataMapper::vtkPainterPolyDataMapper()
{
  this->Painter = 0;

  this->PainterInformation = vtkInformation::New();

  this->Observer = vtkPainterPolyDataMapperObserver::New();
  this->Observer->Target = this;

  vtkDefaultPainter* dp = vtkDefaultPainter::New();
  this->SetPainter(dp);
  dp->Delete();

  vtkChooserPainter* cp = vtkChooserPainter::New();
  this->Painter->SetDelegatePainter(cp);
  cp->Delete();

  this->SelectionPainter = 0;
  vtkPainter* selPainter = vtkHardwareSelectionPolyDataPainter::New();
  this->SetSelectionPainter(selPainter);
  selPainter->Delete();
}

//-----------------------------------------------------------------------------
vtkPainterPolyDataMapper::~vtkPainterPolyDataMapper()
{
  this->SetPainter(NULL);
  this->SetSelectionPainter(0);
  this->Observer->Target = NULL;
  this->Observer->Delete();
  this->PainterInformation->Delete();
}

//---------------------------------------------------------------------------
void vtkPainterPolyDataMapper::MapDataArrayToVertexAttribute(
  const char* vertexAttributeName,
  const char* dataArrayName, 
  int field,
  int componentno)
{
  vtkGenericVertexAttributeMapping* mappings = 0;
  if( this->PainterInformation->Has(
      vtkPrimitivePainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()) )
    {
    mappings = vtkGenericVertexAttributeMapping::SafeDownCast(
      this->PainterInformation->Get(
        vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()));
    }

  if (mappings==NULL)
    {
    mappings = vtkGenericVertexAttributeMapping::New();
    this->PainterInformation->Set(
      vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE(), mappings);
    mappings->Delete();
    }

  mappings->AddMapping(
    vertexAttributeName, dataArrayName, field, componentno);
}

//---------------------------------------------------------------------------
void vtkPainterPolyDataMapper::MapDataArrayToMultiTextureAttribute(
  int unit,
  const char* dataArrayName, 
  int field,
  int componentno)
{
  vtkGenericVertexAttributeMapping* mappings = 0;
  if( this->PainterInformation->Has(
      vtkPrimitivePainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()) )
    {
    mappings = vtkGenericVertexAttributeMapping::SafeDownCast(
      this->PainterInformation->Get(
        vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()));
    }

  if (mappings==NULL)
    {
    mappings = vtkGenericVertexAttributeMapping::New();
    this->PainterInformation->Set(
      vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE(), mappings);
    mappings->Delete();
    }

  mappings->AddMapping(
    unit, dataArrayName, field, componentno);
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::RemoveAllVertexAttributeMappings()
{
  vtkGenericVertexAttributeMapping* mappings = 0;
  if( this->PainterInformation->Has(
      vtkPrimitivePainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()) )
    {
    mappings = vtkGenericVertexAttributeMapping::SafeDownCast(
      this->PainterInformation->Get(
        vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()));
    mappings->RemoveAllMappings();
    }
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::RemoveVertexAttributeMapping(
  const char* vertexAttributeName)
{
  vtkGenericVertexAttributeMapping* mappings = 0;
  if( this->PainterInformation->Has(
      vtkPrimitivePainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()) )
    {
    mappings = vtkGenericVertexAttributeMapping::SafeDownCast(
      this->PainterInformation->Get(
        vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE()));
    mappings->RemoveMapping(vertexAttributeName);
    }
}


//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::SetPainter(vtkPainter* p)
{
  if (this->Painter)
    {
    this->Painter->RemoveObservers(vtkCommand::ProgressEvent, this->Observer);
    this->Painter->SetInformation(0);
    }
  vtkSetObjectBodyMacro(Painter, vtkPainter, p);

   if (this->Painter)
    {
    this->Painter->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    this->Painter->SetInformation(this->PainterInformation);
    }
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::SetSelectionPainter(vtkPainter* p)
{
  if (this->SelectionPainter)
    {
    this->SelectionPainter->SetInformation(0);
    this->SelectionPainter->RemoveObservers(vtkCommand::ProgressEvent, this->Observer);
    }
  vtkSetObjectBodyMacro(SelectionPainter, vtkPainter, p);
   if (this->SelectionPainter)
    {
    this->SelectionPainter->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    this->SelectionPainter->SetInformation(this->PainterInformation);
    }
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Painter, "Painter");
  vtkGarbageCollectorReport(collector, this->SelectionPainter, "SelectionPainter");
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::ReleaseGraphicsResources(vtkWindow *w)
{
  if (this->Painter)
    {
    this->Painter->ReleaseGraphicsResources(w);
    }
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::UpdatePainterInformation()
{
  vtkInformation* info = this->PainterInformation;

  info->Set(vtkPainter::STATIC_DATA(), this->Static);

  info->Set(vtkScalarsToColorsPainter::USE_LOOKUP_TABLE_SCALAR_RANGE(),
    this->GetUseLookupTableScalarRange());
  info->Set(vtkScalarsToColorsPainter::SCALAR_RANGE(), 
    this->GetScalarRange(), 2);
  info->Set(vtkScalarsToColorsPainter::SCALAR_MODE(), this->GetScalarMode());
  info->Set(vtkScalarsToColorsPainter::COLOR_MODE(), this->GetColorMode());
  info->Set(vtkScalarsToColorsPainter::INTERPOLATE_SCALARS_BEFORE_MAPPING(),
    this->GetInterpolateScalarsBeforeMapping());
  info->Set(vtkScalarsToColorsPainter::LOOKUP_TABLE(), this->LookupTable);
  info->Set(vtkScalarsToColorsPainter::SCALAR_VISIBILITY(), 
    this->GetScalarVisibility());
  info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(), 
    this->ArrayAccessMode);
  info->Set(vtkScalarsToColorsPainter::ARRAY_ID(), this->ArrayId);
  info->Set(vtkScalarsToColorsPainter::ARRAY_NAME(), this->ArrayName);
  info->Set(vtkScalarsToColorsPainter::ARRAY_COMPONENT(), this->ArrayComponent);
  info->Set(vtkScalarsToColorsPainter::SCALAR_MATERIAL_MODE(), 
    this->GetScalarMaterialMode());
  
  info->Set(vtkClipPlanesPainter::CLIPPING_PLANES(), this->ClippingPlanes);

  info->Set(vtkCoincidentTopologyResolutionPainter::RESOLVE_COINCIDENT_TOPOLOGY(),
    this->GetResolveCoincidentTopology());
  info->Set(vtkCoincidentTopologyResolutionPainter::Z_SHIFT(),
    this->GetResolveCoincidentTopologyZShift());
  double p[2];
  this->GetResolveCoincidentTopologyPolygonOffsetParameters(p[0], p[1]);
  info->Set(vtkCoincidentTopologyResolutionPainter::POLYGON_OFFSET_PARAMETERS(),
    p, 2);
  info->Set(vtkCoincidentTopologyResolutionPainter::POLYGON_OFFSET_FACES(),
    this->GetResolveCoincidentTopologyPolygonOffsetFaces());

  int immr = (this->ImmediateModeRendering || 
              vtkMapper::GetGlobalImmediateModeRendering());
  info->Set(vtkDisplayListPainter::IMMEDIATE_MODE_RENDERING(), immr);
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::RenderPiece(vtkRenderer* ren, vtkActor* act)
{
  vtkDataObject *input= this->GetInputDataObject(0, 0);

   vtkStandardPolyDataPainter * painter =
    vtkStandardPolyDataPainter::SafeDownCast(this->Painter);
  if (painter != NULL && vtkPolyData::SafeDownCast(input))
    {
    // FIXME: This is not supported currently for composite datasets.
    vtkInformationVector *inArrayVec =
      this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
    int numArrays = inArrayVec->GetNumberOfInformationObjects();

    for(int i = 0; i < numArrays; i++)
      {
      painter->AddMultiTextureCoordsArray(this->GetInputArrayToProcess(i,input));
      }
    }

  //
  // make sure that we've been properly initialized
  //
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  if ( input == NULL )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    if (!this->Static)
      {
      input->Update();
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    // This check is unnecessary since the mapper will be cropped out by culling
    // if it returns invalid bounds which is what will happen when input has no
    // points.
    // vtkIdType numPts = input->GetNumberOfPoints();
    // if (numPts == 0)
    //   {
    //   vtkDebugMacro(<< "No points!");
    //   return;
    //   }
    }

  // Update Painter information if obsolete.
  if (this->PainterUpdateTime < this->GetMTime())
    {
    this->UpdatePainterInformation();
    this->PainterUpdateTime.Modified();
    }

  // make sure our window is current
  ren->GetRenderWindow()->MakeCurrent();
  this->TimeToDraw = 0.0;

  // If we are rendering in selection mode, then we use the selection painter
  // instead of the standard painter.
  if (this->SelectionPainter && ren->GetSelector())
    {
    this->SelectionPainter->SetInput(input);
    this->SelectionPainter->Render(ren, act, 0xff,
      (this->ForceCompileOnly==1));
    this->TimeToDraw = this->SelectionPainter->GetTimeToDraw();
    }
  else if (this->SelectionPainter && this->SelectionPainter != this->Painter)
    {
    this->SelectionPainter->ReleaseGraphicsResources(ren->GetRenderWindow());
    }

  if (this->Painter && ren->GetSelector() == 0)
    {
    // Pass polydata.
    this->Painter->SetInput(input);
    this->Painter->Render(ren, act, 0xff,this->ForceCompileOnly==1);
    this->TimeToDraw = this->Painter->GetTimeToDraw();
    }

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }

  this->UpdateProgress(1.0);
}

//-------------------------------------------------------------------------
void vtkPainterPolyDataMapper::ComputeBounds()
{
  this->GetInput()->GetBounds(this->Bounds);

  // if the mapper has a painter, update the bounds in the painter
  vtkPainter *painter = this->GetPainter();
  if (painter)
    {
    // Update Painter information if obsolete.
    if (this->PainterUpdateTime < this->GetMTime())
      {
      this->UpdatePainterInformation();
      this->PainterUpdateTime.Modified();
      }
    painter->UpdateBounds(this->Bounds);
    }
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Painter: " ;
  if (this->Painter)
    {
    os << endl;
    this->Painter->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "(none)" << endl;
    }
  os << indent << "SelectionPainter: " << this->SelectionPainter << endl;
}
