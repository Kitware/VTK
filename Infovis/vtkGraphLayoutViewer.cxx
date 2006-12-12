/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutViewer.cxx

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
#include "vtkGraphLayoutViewer.h"

#include "vtkAbstractGraph.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGeometryFilter.h"
#include "vtkGraph.h"
#include "vtkGraphLayout.h"
#include "vtkGraphLayoutStrategy.h"
#include "vtkGraphToPolyData.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleImage.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRandomLayoutStrategy.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkThreshold.h"
#include "vtkThresholdPoints.h"
#include "vtkTreeLevelsFilter.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkGraphLayoutViewer, "1.9");
vtkStandardNewMacro(vtkGraphLayoutViewer);


//----------------------------------------------------------------------------
vtkGraphLayoutViewer::vtkGraphLayoutViewer()
{
  this->Input                 = NULL;
  this->RenderWindow          = NULL;
  this->GraphLayoutStrategy   = NULL;
  this->InteractorStyle       = vtkInteractorStyleImage::New();
  this->GraphLayout           = vtkSmartPointer<vtkGraphLayout>::New();
  this->GraphToPolyData       = vtkSmartPointer<vtkGraphToPolyData>::New();
  this->PolyDataMapper        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->Renderer              = vtkSmartPointer<vtkRenderer>::New();
  this->Actor                 = vtkSmartPointer<vtkActor>::New();
  this->LabelActor            = vtkSmartPointer<vtkActor2D>::New();
  this->ColorLUT              = vtkSmartPointer<vtkLookupTable>::New();
  this->LabeledDataMapper     = vtkSmartPointer<vtkLabeledDataMapper>::New();
  this->ArcWeightField        = 0;
  
  // Set up some the default parameters
  this->LabeledDataMapper->SetFieldDataName("name");
  this->LabeledDataMapper->SetLabelFormat("%s");
  this->LabeledDataMapper->SetLabelModeToLabelFieldData();
  this->LabeledDataMapper->GetLabelTextProperty()->SetColor(1,1,1);
  this->LabeledDataMapper->GetLabelTextProperty()->SetJustificationToCentered();
  this->LabeledDataMapper->GetLabelTextProperty()->SetFontSize(14);
  this->SetLayoutStrategy("Simple2D");

  // Okay setup the internal pipeline
  this->SetupPipeline();
}

//----------------------------------------------------------------------------
vtkGraphLayoutViewer::~vtkGraphLayoutViewer()
{
  // Unregister vtk objects that were passed in
  this->SetRenderWindow(NULL);
  this->SetInput(NULL);

  if (this->InteractorStyle)
    {
    this->InteractorStyle->Delete();
    this->InteractorStyle = NULL;
    }
    
  this->SetArcWeightField(0);
  
  // Smart pointers will handle the rest of
  // vtk pipeline objects :)
}

void vtkGraphLayoutViewer::SetFontSize(const int size)
{
  this->LabeledDataMapper->GetLabelTextProperty()->SetFontSize(size);
}

int vtkGraphLayoutViewer::GetFontSize()
{
  return this->LabeledDataMapper->GetLabelTextProperty()->GetFontSize();
}

void vtkGraphLayoutViewer::SetLabelFieldName(const char *field)
{
  this->LabeledDataMapper->SetFieldDataName(field); 
}

char* vtkGraphLayoutViewer::GetLabelFieldName()
{
  return this->LabeledDataMapper->GetFieldDataName();
}

void vtkGraphLayoutViewer::SetLabelsOn()
{
  this->LabelActor->VisibilityOn();
}

void vtkGraphLayoutViewer::SetLabelsOff()
{
  this->LabelActor->VisibilityOff();
}


int vtkGraphLayoutViewer::IsLayoutComplete()
{
  if (this->GraphLayout)
    {
    return this->GraphLayout->IsLayoutComplete();
    }
    
  // If I don't have a strategy I guess it's
  // better to say I'm done than not done :)
  return 1;
}
  

void vtkGraphLayoutViewer::UpdateLayout()
{
  // See if the graph layout is complete
  // if it's not then set it as modified
  // and call a render on the render window
  if (!this->IsLayoutComplete())
    {
    this->GraphLayout->Modified();
    if (this->RenderWindow)
      {
      this->Renderer->ResetCamera();
      this->RenderWindow->Render();
      }
    }
}

void vtkGraphLayoutViewer::SetInput(vtkAbstractGraph *graph)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the InputInitialize() call in the middle :)
  if (graph != this->Input)
    {
    vtkAbstractGraph *tmp = this->Input;
    this->Input = graph;
    if (this->Input != NULL)
      {
      this->Input->Register(this);
      this->InputInitialize();
      }
    if (tmp != NULL)
      {
      tmp->UnRegister(this);
      }
    this->Modified();
    }
}

void vtkGraphLayoutViewer::InputInitialize()
{

  // Pipeline setup
  this->GraphLayout->SetInput(this->Input);
  this->Actor->VisibilityOn();
  this->LabelActor->VisibilityOff(); // Defaulted to off
  
  // Get and set the range of data for this mapper
  double range[2]; 
  this->GraphToPolyData->Update();
  this->GraphToPolyData->GetOutput()->GetScalarRange(range);
  this->PolyDataMapper->SetScalarRange( range[0], range[1] );
 
  if (this->RenderWindow)
    {
    this->Renderer->ResetCamera();
    this->RenderWindow->Render();
    }
}

// This method is a cut and paste of vtkCxxSetObjectMacro
// except for the AddRenderer and ResetCamera() calls in the middle :)
void vtkGraphLayoutViewer::SetRenderWindow(vtkRenderWindow *arg)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the Pipeline setup in the middle :)
  if (arg != this->RenderWindow)
    {
    vtkRenderWindow *tmp = this->RenderWindow;
    this->RenderWindow = arg;
    if (this->RenderWindow != NULL)
      {
      this->RenderWindow->Register(this);
      
      // Set up last part of the pipeline
      this->RenderWindow->AddRenderer(this->Renderer);
      this->Renderer->ResetCamera();
      }
    if (tmp != NULL)
      {
      tmp->UnRegister(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGraphLayoutViewer::SetupPipeline()
{

  // Set various properties
  this->Renderer->SetBackground(.3,.3,.3);
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  this->ColorLUT->SetHueRange( 0.667, 0 );
  this->ColorLUT->Build();
 
  // Wire up the pipeline
 
  // Set the input to NULL and turn the 
  // visibility of the actors off for now.
  // When SetInput() is called by the application
  // the input is set and the actors are turned on
  this->GraphLayout->SetInput(NULL);
  this->Actor->VisibilityOff();
  this->LabelActor->VisibilityOff();
  
  // Send graph to poly data filter and mapper
  this->GraphToPolyData->SetInputConnection(0, 
    this->GraphLayout->GetOutputPort(0)); 
  this->PolyDataMapper->SetLookupTable(ColorLUT);
  this->PolyDataMapper->SetInputConnection(0, 
    this->GraphToPolyData->GetOutputPort(0));
                                           
  // Labels
  this->LabeledDataMapper->SetInputConnection(GraphToPolyData->GetOutputPort());
  this->LabelActor->SetPickable(false);
  this->LabelActor->SetMapper(this->LabeledDataMapper);
  
  // Actor setup
  this->Actor->SetMapper(this->PolyDataMapper);
  this->Renderer->AddActor(this->Actor);
  this->Renderer->AddActor(this->LabelActor); 

}


void vtkGraphLayoutViewer::SetColorFieldName(const char *field)
{
  // Sanity Check
  if (!strcmp(field,"")) return;
  if (!strcmp(field,"No Filter")) return;
  
  this->PolyDataMapper->SetScalarModeToUseCellFieldData();
  this->PolyDataMapper->SelectColorArray(field);
  
  // Okay now get the range of the data field
  double range[2]; 
  this->GraphToPolyData->Update();
  vtkDataArray *array =
    this->GraphToPolyData->GetOutput()->GetCellData()->GetArray(field);
  if (array)
    {
    array->GetRange(range);
    this->PolyDataMapper->SetScalarRange( range[0], range[1] );
    } 

  if (this->RenderWindow)
    {
    this->RenderWindow->GetInteractor()->Render();
    }
}

char* vtkGraphLayoutViewer::GetColorFieldName()
{
  return this->PolyDataMapper->GetArrayName();
}


// Set up the layout strategy for the treemap
void vtkGraphLayoutViewer::SetLayoutStrategy(const char* strategyName)
{
  vtkGraphLayoutStrategy* strategy;
  
  // Set some general parameters
  int MaxIterations = 100;
  int InterationPerLayout = 100;
  if (this->Iterative)
    {
    InterationPerLayout = 5;
    }

  // "Switch" on strategy name to set up the layout
  // strategy and parameters specific to the layout type. 
  if (!strcmp(strategyName, "Random"))
    {
    strategy = vtkRandomLayoutStrategy::New();
    }
  else if (!strcmp(strategyName, "ForceDirected"))
    {
    strategy = vtkForceDirectedLayoutStrategy::New();
    vtkForceDirectedLayoutStrategy::SafeDownCast(strategy)
      ->SetMaxNumberOfIterations(MaxIterations);
    vtkForceDirectedLayoutStrategy::SafeDownCast(strategy)
      ->SetIterationsPerLayout(InterationPerLayout);
    vtkForceDirectedLayoutStrategy::SafeDownCast(strategy)
      ->SetThreeDimensionalLayout(false);
    }
  else if (!strcmp(strategyName, "Simple2D"))
    {
    strategy = vtkSimple2DLayoutStrategy::New();
    vtkSimple2DLayoutStrategy::SafeDownCast(strategy)
      ->SetMaxNumberOfIterations(100);
    vtkSimple2DLayoutStrategy::SafeDownCast(strategy)
      ->SetIterationsPerLayout(100);
    vtkSimple2DLayoutStrategy::SafeDownCast(strategy)
      ->SetInitialTemperature(1);
    }
  else
    {
    // Just use the default of force directed
    vtkWarningMacro(<<"Unknown layout strategy: " << strategyName);
    strategy = vtkForceDirectedLayoutStrategy::New();
    vtkForceDirectedLayoutStrategy::SafeDownCast(strategy)
      ->SetMaxNumberOfIterations(MaxIterations);
    vtkForceDirectedLayoutStrategy::SafeDownCast(strategy)
      ->SetIterationsPerLayout(InterationPerLayout);
    vtkForceDirectedLayoutStrategy::SafeDownCast(strategy)
      ->SetThreeDimensionalLayout(false);
    }

  // Set the strategy for the layout
  this->GraphLayout->SetLayoutStrategy(strategy);
  strategy->Delete();
    
  // Reset camera
  this->Renderer->ResetCamera();
        
  // Tell render window to explicitly update
  if (this->RenderWindow)
    {
    this->RenderWindow->Render();
    }
}

const char* vtkGraphLayoutViewer::GetLayoutStrategy()
{
  vtkGraphLayoutStrategy* strategy = this->GraphLayout->GetLayoutStrategy();
  if (strategy->IsA("vtkRandomLayoutStrategy"))
    {
    return "Random";
    }
  else if (strategy->IsA("vtkForceDirectedLayoutStrategy"))
    {
    return "ForceDirected";
    }
  else if (strategy->IsA("vtkSimple2DLayoutStrategy"))
    {
    return "Simple2D";
    }
  return "";
}

//----------------------------------------------------------------------------
void vtkGraphLayoutViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);


  os << indent << "Input Graph: " << (this->Input ? "" : "(none)") << endl;
  if (this->Input)
    {
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }

  os << indent << "RenderWindow: " << (this->RenderWindow ? "" : "(none)") << endl;
  if (this->RenderWindow)
    {
    this->RenderWindow->PrintSelf(os,indent.GetNextIndent());
    }
  
  os << indent << "PolyDataMapper: " << (this->PolyDataMapper ? "" : "(none)") << endl;
  if (this->PolyDataMapper)
    {
    this->PolyDataMapper->PrintSelf(os,indent.GetNextIndent()); 
    }
    
  os << indent << "Renderer: " << (this->Renderer ? "" : "(none)") << endl;
  if (this->Renderer)
    {
    this->Renderer->PrintSelf(os,indent.GetNextIndent());
    }
  
  os << indent << "Actor: " << (this->Actor ? "" : "(none)") << endl;
  if (this->Actor && this->Input)
    {
    this->Actor->PrintSelf(os,indent.GetNextIndent());
    }
  
  os << indent << "InteractorStyle: " << (this->InteractorStyle ? "" : "(none)") << endl;
  if (this->InteractorStyle)
    {
    this->InteractorStyle->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "Iterative: " 
     << (this->Iterative ? "true" : "false") << endl;
     
  os << indent << "ArcWeightField: " 
     << (this->ArcWeightField ? this->ArcWeightField : "(none)") << endl;
}
