/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkGraphMapper.h"

#include "vtkAbstractGraph.h"
#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkGraphToPolyData.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkGlyph2D.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPNGReader.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkViewTheme.h"

vtkCxxRevisionMacro(vtkGraphMapper, "1.4");
vtkStandardNewMacro(vtkGraphMapper);

//----------------------------------------------------------------------------
vtkGraphMapper::vtkGraphMapper()
{
  this->GraphToPoly       = vtkSmartPointer<vtkGraphToPolyData>::New();
  this->VertexGlyph       = vtkSmartPointer<vtkVertexGlyphFilter>::New();
  this->IconGlyph         = vtkSmartPointer<vtkGlyph2D>::New();
  this->EdgeMapper        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->VertexMapper      = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->OutlineMapper     = vtkSmartPointer<vtkPolyDataMapper>::New(); 
  this->EdgeActor         = vtkSmartPointer<vtkActor>::New();
  this->VertexActor       = vtkSmartPointer<vtkActor>::New();
  this->OutlineActor      = vtkSmartPointer<vtkActor>::New();
  this->VertexLookupTable = vtkSmartPointer<vtkLookupTable>::New();
  this->EdgeLookupTable   = vtkSmartPointer<vtkLookupTable>::New();
  this->IconTexture       = vtkSmartPointer<vtkTexture>::New();
  this->VertexColorArrayNameInternal = 0;
  this->EdgeColorArrayNameInternal = 0;
  
  this->VertexMapper->SetScalarModeToUsePointData();
  this->VertexMapper->SetLookupTable(this->VertexLookupTable);
  this->VertexMapper->SetScalarVisibility(false);
  this->VertexActor->PickableOff();
  this->VertexActor->GetProperty()->SetPointSize(this->GetVertexPointSize());
  this->OutlineActor->PickableOff();
  this->OutlineActor->GetProperty()->SetPointSize(this->GetVertexPointSize()+2);
  this->OutlineActor->SetPosition(0, 0, -0.001);
  this->OutlineMapper->SetScalarVisibility(false);
  this->EdgeMapper->SetScalarModeToUseCellData();
  this->EdgeMapper->SetLookupTable(this->EdgeLookupTable);
  this->EdgeMapper->SetScalarVisibility(false);
  this->EdgeActor->SetPosition(0, 0, -0.003);
  this->EdgeActor->GetProperty()->SetLineWidth(this->GetEdgeLineWidth());
  
  this->VertexGlyph->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);
  this->OutlineMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->EdgeMapper->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);
  
  // Set default parameters
  this->SetVertexColorArrayName("VertexDegree");
  this->ColorVerticesOff();
  this->SetEdgeColorArrayName("weight");
  this->ColorEdgesOff();
  this->VertexPointSize = 5;
  this->EdgeLineWidth = 1;
  
  // FIXME: Total hack for testing textures
#if 0
  vtkPNGReader *PNGReader = vtkPNGReader::New();
  PNGReader->SetFileName("C:/Work/vtk_stuff/vtkSNL/Applications/DataBaseView/Node.png");
  PNGReader->Update();

  this->IconTexture->SetInputConnection(PNGReader->GetOutputPort());
  PNGReader->Delete();
  this->IconTexture->InterpolateOn();
  //this->VertexActor->SetTexture(this->IconTexture);
  

  vtkPlaneSource *planeSource = vtkPlaneSource::New();
  planeSource->Update();

  this->IconGlyph->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->IconGlyph->SetSource(planeSource->GetOutput());
  this->VertexMapper->SetInputConnection(this->IconGlyph->GetOutputPort());
#endif
}

//----------------------------------------------------------------------------
vtkGraphMapper::~vtkGraphMapper()
{
  // Delete internally created objects.
  // Note: All of the smartpointer objects 
  //       will be deleted for us
    
  this->SetVertexColorArrayNameInternal(0);
  this->SetEdgeColorArrayNameInternal(0);
}

//----------------------------------------------------------------------------
void vtkGraphMapper::SetVertexColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->VertexMapper->SetScalarModeToUsePointFieldData();
  this->VertexMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphMapper::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkGraphMapper::SetColorVertices(bool vis)
{
  this->VertexMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphMapper::GetColorVertices()
{
  return this->VertexMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkGraphMapper::ColorVerticesOn()
{
  this->VertexMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkGraphMapper::ColorVerticesOff()
{
  this->VertexMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkGraphMapper::SetEdgeColorArrayName(const char* name)
{
  this->SetEdgeColorArrayNameInternal(name);
  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphMapper::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkGraphMapper::SetColorEdges(bool vis)
{
  this->EdgeMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphMapper::GetColorEdges()
{
  return this->EdgeMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkGraphMapper::ColorEdgesOn()
{
  this->EdgeMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkGraphMapper::ColorEdgesOff()
{
  this->EdgeMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkGraphMapper::SetVertexPointSize(int size)
{
  this->VertexPointSize = size;
  this->VertexActor->GetProperty()->SetPointSize(this->GetVertexPointSize());
  this->OutlineActor->GetProperty()->SetPointSize(this->GetVertexPointSize()+2);
}
  
//----------------------------------------------------------------------------
void vtkGraphMapper::SetEdgeLineWidth(int width)
{
  this->EdgeLineWidth = width;
  this->EdgeActor->GetProperty()->SetLineWidth(this->GetEdgeLineWidth());
}

//----------------------------------------------------------------------------
void vtkGraphMapper::SetInput(vtkAbstractGraph *input)
{
  if(input)
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
vtkAbstractGraph *vtkGraphMapper::GetInput()
{
  vtkAbstractGraph *inputGraph =
  vtkAbstractGraph::SafeDownCast(this->Superclass::GetInputAsDataSet());
  return inputGraph;
}

//----------------------------------------------------------------------------
void vtkGraphMapper::ReleaseGraphicsResources( vtkWindow *renWin )
{
  if (this->EdgeMapper)
    {
    this->EdgeMapper->ReleaseGraphicsResources( renWin );
    }
}

//----------------------------------------------------------------------------
// Receives from Actor -> maps data to primitives
//
void vtkGraphMapper::Render(vtkRenderer *ren, vtkActor * vtkNotUsed(act))
{
  // make sure that we've been properly initialized
  if ( !this->GetInput() )
    {
    vtkErrorMacro(<< "No input!\n");
    return;
    } 
    
  // Update the pipeline up until the graph to poly data
  this->GraphToPoly->SetInput(this->GetInput());
  this->GraphToPoly->Update();
  vtkPolyData* pd = this->GraphToPoly->GetOutput();
 
  // Try to find the range the user-specified color array.
  // If we cannot find that array, use the scalar range.
  double range[2];
  vtkDataArray* arr = 0; 
  if (this->GetColorEdges())
    {
    if (this->GetEdgeColorArrayName())
      {
      arr = pd->GetCellData()->GetArray(this->GetEdgeColorArrayName());
      }
    if (!arr)
      {
      arr = pd->GetCellData()->GetScalars();
      }
    if (arr)
      {
      arr->GetRange(range);    
      this->EdgeMapper->SetScalarRange(range[0], range[1]);
      }
    }

  // Do the same thing for the vertex array.
  arr = 0; 
  if (this->GetColorVertices())
    {
    if (this->GetVertexColorArrayName())
      {
      arr = pd->GetPointData()->GetArray(this->GetVertexColorArrayName());
      }
    if (!arr)
      {
      arr = pd->GetPointData()->GetScalars();
      }
    if (arr)
      {
      arr->GetRange(range);    
      this->VertexMapper->SetScalarRange(range[0], range[1]);
      }
    }

  this->EdgeActor->RenderOpaqueGeometry(ren);
  this->VertexActor->RenderOpaqueGeometry(ren);
  this->OutlineActor->RenderOpaqueGeometry(ren);
  this->EdgeActor->RenderTranslucentPolygonalGeometry(ren);
  this->VertexActor->RenderTranslucentPolygonalGeometry(ren);
  this->OutlineActor->RenderTranslucentPolygonalGeometry(ren);
  this->TimeToDraw = this->EdgeMapper->GetTimeToDraw() +
                     this->VertexMapper->GetTimeToDraw() +
                     this->OutlineMapper->GetTimeToDraw();
}

void vtkGraphMapper::ApplyViewTheme(vtkViewTheme* theme)
{
  this->VertexActor->GetProperty()->SetColor(theme->GetPointColor());
  this->VertexActor->GetProperty()->SetOpacity(theme->GetPointOpacity());
  this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());
  this->VertexLookupTable->SetHueRange(theme->GetPointHueRange()); 
  this->VertexLookupTable->SetSaturationRange(theme->GetPointSaturationRange()); 
  this->VertexLookupTable->SetValueRange(theme->GetPointValueRange()); 
  this->VertexLookupTable->SetAlphaRange(theme->GetPointAlphaRange()); 
  this->VertexLookupTable->Build();

  this->EdgeActor->GetProperty()->SetColor(theme->GetCellColor());
  this->EdgeActor->GetProperty()->SetOpacity(theme->GetCellOpacity());
  this->EdgeLookupTable->SetHueRange(theme->GetCellHueRange()); 
  this->EdgeLookupTable->SetSaturationRange(theme->GetCellSaturationRange()); 
  this->EdgeLookupTable->SetValueRange(theme->GetCellValueRange()); 
  this->EdgeLookupTable->SetAlphaRange(theme->GetCellAlphaRange()); 
  this->EdgeLookupTable->Build();
}

//----------------------------------------------------------------------------
void vtkGraphMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->EdgeMapper )
    {
    os << indent << "EdgeMapper: (" << this->EdgeMapper << ")\n";
    }
  else
    {
    os << indent << "EdgeMapper: (none)\n";
    }
 if ( this->VertexMapper )
    {
    os << indent << "VertexMapper: (" << this->VertexMapper << ")\n";
    }
  else
    {
    os << indent << "VertexMapper: (none)\n";
    }
 if ( this->OutlineMapper )
    {
    os << indent << "OutlineMapper: (" << this->OutlineMapper << ")\n";
    }
  else
    {
    os << indent << "OutlineMapper: (none)\n";
    }
  if ( this->EdgeActor )
    {
    os << indent << "EdgeActor: (" << this->EdgeActor << ")\n";
    }
  else
    {
    os << indent << "EdgeActor: (none)\n";
    }
 if ( this->VertexActor )
    {
    os << indent << "VertexActor: (" << this->VertexActor << ")\n";
    }
  else
    {
    os << indent << "VertexActor: (none)\n";
    }
 if ( this->OutlineActor )
    {
    os << indent << "OutlineActor: (" << this->OutlineActor << ")\n";
    }
  else
    {
    os << indent << "OutlineActor: (none)\n";
    }

  if ( this->GraphToPoly )
    {
    os << indent << "GraphToPoly: (" << this->GraphToPoly << ")\n";
    }
  else
    {
    os << indent << "GraphToPoly: (none)\n";
    }
  os << indent << "VertexPointSize: " << this->VertexPointSize << "\n";
  os << indent << "EdgeLineWidth: " << this->EdgeLineWidth << "\n";
}

//----------------------------------------------------------------------------
unsigned long vtkGraphMapper::GetMTime()
{
  unsigned long mTime=this->vtkMapper::GetMTime();
  unsigned long time;

  if ( this->LookupTable != NULL )
    {
    time = this->LookupTable->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkGraphMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAbstractGraph");
  return 1;
}

#if 0
//----------------------------------------------------------------------------
void vtkGraphMapper::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->GraphToPoly,
                            "GraphToPoly");
  vtkGarbageCollectorReport(collector, this->VertexGlyph,
                            "VertexGlyph");           
  vtkGarbageCollectorReport(collector, this->EdgeMapper,
                            "EdgeMapper");
  vtkGarbageCollectorReport(collector, this->VertexMapper,
                            "VertexMapper");
  vtkGarbageCollectorReport(collector, this->OutlineMapper,
                            "OutlineMapper");
  vtkGarbageCollectorReport(collector, this->EdgeActor,
                            "EdgeActor");
  vtkGarbageCollectorReport(collector, this->VertexActor,
                            "VertexActor");
  vtkGarbageCollectorReport(collector, this->OutlineActor,
                            "OutlineActor");
  vtkGarbageCollectorReport(collector, this->EdgeLookupTable,
                            "EdgeLookupTable");
  vtkGarbageCollectorReport(collector, this->VertexLookupTable,
                            "VertexLookupTable");
}

#endif
