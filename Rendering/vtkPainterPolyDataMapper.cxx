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
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyData.h"
#include "vtkPrimitivePainter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColorsPainter.h"

vtkStandardNewMacro(vtkPainterPolyDataMapper);
vtkCxxRevisionMacro(vtkPainterPolyDataMapper, "1.10")

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

}

//-----------------------------------------------------------------------------
vtkPainterPolyDataMapper::~vtkPainterPolyDataMapper()
{
  this->SetPainter(NULL);
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
void vtkPainterPolyDataMapper::SetPainter(vtkPolyDataPainter* p)
{
  if (this->Painter)
    {
    this->Painter->RemoveObservers(vtkCommand::ProgressEvent, this->Observer);
    }
  vtkSetObjectBodyMacro(Painter, vtkPolyDataPainter, p);

   if (this->Painter)
    {
    this->Painter->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    this->Painter->SetInformation(this->PainterInformation);
    }
}

//-----------------------------------------------------------------------------
void vtkPainterPolyDataMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Painter, "Painter");
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
  vtkPolyData *input= this->GetInput();
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

    vtkIdType numPts = input->GetNumberOfPoints();
    if (numPts == 0)
      {
      vtkDebugMacro(<< "No points!");
      return;
      }
    }
  // make sure our window is current
  ren->GetRenderWindow()->MakeCurrent();
  this->TimeToDraw = 0.0;
  if (this->Painter)
    {
    // Update Painter information if obsolete.
    if (this->PainterUpdateTime < this->MTime)
      {
      this->UpdatePainterInformation();
      this->PainterUpdateTime.Modified();
      }
    // Pass polydata if changed.
    if (this->Painter->GetPolyData() != input)
      {
      this->Painter->SetPolyData(input);
      }
    this->Painter->Render(ren, act, 0xff);
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
void vtkPainterPolyDataMapper::GetBounds(double bounds[6])
{
  this->GetBounds();
  memcpy(bounds,this->Bounds,6*sizeof(double));
}

//-------------------------------------------------------------------------
double* vtkPainterPolyDataMapper::GetBounds()
{
  static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  // do we have an input
  if ( ! this->GetNumberOfInputConnections(0) )
    {
    return bounds;
    }
  else
    {
    if (!this->Static)
      {
      // For proper clipping, this would be this->Piece,
      // this->NumberOfPieces.
      // But that removes all benefites of streaming.
      // Update everything as a hack for paraview streaming.
      // This should not affect anything else, because no one uses this.
      // It should also render just the same.
      // Just remove this lie if we no longer need streaming in paraview :)
      //this->GetInput()->SetUpdateExtent(0, 1, 0);
      //this->GetInput()->Update();

      // first get the bounds from the input
      this->Update();
      this->GetInput()->GetBounds(this->Bounds);

      // if the mapper has a painter, update the bounds in the painter
      vtkPainter *painter = this->GetPainter();

      if( painter )
        {
        painter->UpdateBounds(this->Bounds);
        }
      }
    // if the bounds indicate NAN and subpieces are being used then
    // return NULL
    if (!vtkMath::AreBoundsInitialized(this->Bounds)
        && this->NumberOfSubPieces > 1)
      {
      return NULL;
      }
    return this->Bounds;
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
}
