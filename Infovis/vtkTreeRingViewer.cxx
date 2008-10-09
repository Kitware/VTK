/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingViewer.cxx

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
#include "vtkTreeRingViewer.h"

#include "vtkActor.h"
//#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkInteractorStyle.h"
#include "vtkInteractorStyleTreeRingHover.h"
//#include "vtkLabeledTreeRingDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkThreshold.h"
#include "vtkThresholdPoints.h"
#include "vtkTreeFieldAggregator.h"
//#include "vtkTreeLevelsFilter.h"
#include "vtkTreeRingLayout.h"
#include "vtkTreeRingLayoutStrategy.h"
#include "vtkTreeRingDefaultLayoutStrategy.h"
#include "vtkTreeRingReversedLayoutStrategy.h"
#include "vtkTreeRingToPolyData.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkTreeRingViewer, "1.1");
vtkStandardNewMacro(vtkTreeRingViewer);

//----------------------------------------------------------------------------
vtkTreeRingViewer::vtkTreeRingViewer()
{
  this->Input                 = NULL;
  this->RenderWindow          = NULL;
  this->InteractorStyle       = vtkInteractorStyleTreeRingHover::New();
//  this->TreeLevelsFilter      = vtkSmartPointer<vtkTreeLevelsFilter>::New();
  this->TreeFieldAggregator   = vtkSmartPointer<vtkTreeFieldAggregator>::New();
  this->TreeRingLayout         = vtkSmartPointer<vtkTreeRingLayout>::New();
  this->TreeRingToPolyData     = vtkSmartPointer<vtkTreeRingToPolyData>::New();
  this->PolyDataMapper        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->Renderer              = vtkSmartPointer<vtkRenderer>::New();
  this->Actor                 = vtkSmartPointer<vtkActor>::New();
//  this->LabelActor            = vtkSmartPointer<vtkActor2D>::New();
  this->ColorLUT              = vtkSmartPointer<vtkLookupTable>::New();
//  this->LabeledDataMapper     = vtkSmartPointer<vtkLabeledTreeRingDataMapper>::New();
  
  // Set up some the default parameters
  this->SetAggregationFieldName("size");
  this->InteractorStyle->SetLabelField("name");
//   this->LabeledDataMapper->SetFieldDataName("name");
//   this->LabeledDataMapper->SetLabelFormat("%s");
//   this->LabeledDataMapper->SetLabelModeToLabelFieldData();
//   this->LabeledDataMapper->SetClipTextMode(0);
//  this->SetFontSizeRange(24,10);
  
  // Okay setup the internal pipeline
  this->SetupPipeline();
}

//----------------------------------------------------------------------------
vtkTreeRingViewer::~vtkTreeRingViewer()
{
  // Unregister vtk objects that were passed in
  this->SetRenderWindow(NULL);
  this->SetInput(NULL);

  if (this->InteractorStyle)
    {
    this->InteractorStyle->Delete();
    this->InteractorStyle = NULL;
    }
  
  // Smart pointers will handle the rest of
  // vtk pipeline objects :)
}

void vtkTreeRingViewer::SetAggregationFieldName(const char *field)
{
  this->TreeFieldAggregator->SetField(field);
}

char* vtkTreeRingViewer::GetAggregationFieldName()
{
  return this->TreeFieldAggregator->GetField();
}

// void vtkTreeRingViewer::SetFontSizeRange(const int maxSize, const int minSize)
// {
//   this->LabeledDataMapper->SetFontSizeRange(maxSize, minSize);
// }

void vtkTreeRingViewer::SetLabelFieldName(const char *field)
{
  this->InteractorStyle->SetLabelField(field);
//  this->LabeledDataMapper->SetFieldDataName(field); 
}

char* vtkTreeRingViewer::GetLabelFieldName()
{
  return this->InteractorStyle->GetLabelField();
}

void vtkTreeRingViewer::SetInput(vtkTree *tree)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the InputInitialize() call in the middle :)
  if (tree != this->Input)
    {
    vtkTree *tmp = this->Input;
    this->Input = tree;
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

void vtkTreeRingViewer::InputInitialize()
{
  // Pipeline setup
//  this->TreeLevelsFilter->SetInput(this->Input);
  this->TreeFieldAggregator->SetInput(this->Input);
  this->Actor->VisibilityOn();
//  this->LabelActor->VisibilityOn();
  
  // Get and set the range of data for this mapper
  double range[2]; 
  this->TreeRingToPolyData->Update();
  this->TreeRingToPolyData->GetOutput()->GetScalarRange(range);
  this->PolyDataMapper->SetScalarRange( range[0], range[1] );
  
  if (this->RenderWindow)
    {
    this->Renderer->ResetCamera();
    this->RenderWindow->Render();
    }
}

// This method is a cut and paste of vtkCxxSetObjectMacro
// except for the AddRenderer and ResetCamera() calls in the middle :)
void vtkTreeRingViewer::SetRenderWindow(vtkRenderWindow *arg)
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
      this->RenderWindow->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
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
void vtkTreeRingViewer::SetupPipeline()
{
  // Set various properties
  this->TreeFieldAggregator->SetLeafVertexUnitSize(false);
  this->TreeFieldAggregator->SetMinValue(1.0); // Treat a size of 0 to be a size of 1
  this->TreeFieldAggregator->SetLogScale(true);
  this->SetLayoutStrategy("Tree Ring Default Layout");
//  this->TreeRingToPolyData->SetLevelsFieldName("level");
  this->Renderer->SetBackground(.3,.3,.3);
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  this->ColorLUT->SetHueRange( 0.667, 0 );
  this->ColorLUT->Build();
  this->InteractorStyle->SetLayout(this->TreeRingLayout);
//  this->InteractorStyle->SetTreeRingToPolyData(this->TreeRingToPolyData);
  
  // Wire up the pipeline
  
  // Set the input to NULL and turn the 
  // visibility of the actors off for now.
  // When SetInput() is called by the application
  // the input is set and the actors are turned on
//  this->TreeLevelsFilter->SetInput(NULL);
  this->TreeFieldAggregator->SetInput(NULL);
  this->Actor->VisibilityOff();
//  this->LabelActor->VisibilityOff();

    
//   this->TreeFieldAggregator->SetInputConnection(0, 
//     this->TreeLevelsFilter->GetOutputPort(0));
//  this->TreeFieldAggregator->SetInput(this->Input);
      
  this->TreeRingLayout->SetInputConnection(0, 
    this->TreeFieldAggregator->GetOutputPort(0));
  
  this->TreeRingToPolyData->SetInputConnection(0,
    this->TreeRingLayout->GetOutputPort(0)); 

  this->PolyDataMapper->SetLookupTable(ColorLUT);
  this->PolyDataMapper->SetInputConnection(0, 
                                           this->TreeRingToPolyData->GetOutputPort(0));

//   this->LabeledDataMapper->SetInputConnection(this->TreeRingLayout->
//                                  GetOutputPort(0)); 
//   this->LabelActor->SetPickable(false);
//   this->LabelActor->SetMapper(this->LabeledDataMapper);
  this->Actor->SetMapper(this->PolyDataMapper);
  this->Renderer->AddActor(this->Actor);
//  this->Renderer->AddActor(this->LabelActor);

}

void vtkTreeRingViewer::SetColorFieldName(const char *field)
{
  // Sanity Check
  if (!strcmp(field,"")) return;
  if (!strcmp(field,"No Filter")) return;
  
  this->PolyDataMapper->SetScalarModeToUseCellFieldData();
  this->PolyDataMapper->SelectColorArray(field);
  
  // Okay now get the range of the data field
  double range[2]; 
  this->TreeRingToPolyData->Update();
  vtkDataArray *array =
    this->TreeRingToPolyData->GetOutput()->GetCellData()->GetArray(field);
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

char* vtkTreeRingViewer::GetColorFieldName()
{
  return this->PolyDataMapper->GetArrayName();
}

bool vtkTreeRingViewer::GetLogScale()
{
  return this->TreeFieldAggregator->GetLogScale();
}

void vtkTreeRingViewer::SetLogScale(bool value)
{
  if (value == this->GetLogScale())
    {
    return;
    }

  this->TreeFieldAggregator->SetLogScale(value);

  if (this->RenderWindow)
    {
    this->RenderWindow->GetInteractor()->Render();
    }
}

// Set up the layout strategy for the treemap
void vtkTreeRingViewer::SetLayoutStrategy(int strategy_enum)
{
  vtkTreeRingLayoutStrategy* strategy;
  switch (strategy_enum)
    {
    case TREE_RING_DEFAULT_LAYOUT:
      strategy = vtkTreeRingDefaultLayoutStrategy::New();
      break;
    case TREE_RING_REVERSED_LAYOUT:
      strategy = vtkTreeRingReversedLayoutStrategy::New();
      break;
    default:
      // Just use the default of box
      vtkWarningMacro(<<"Unknown layout strategy enum: " << strategy_enum);
      strategy = vtkTreeRingDefaultLayoutStrategy::New();
    }
     
  // Actually set strategy
  this->TreeRingLayout->SetLayoutStrategy(strategy);
  strategy->Delete();
    
  // Reset camera
  this->Renderer->ResetCamera();
        
  // Tell render window to explicitly update
  if (this->RenderWindow)
    {
    this->RenderWindow->Render();
    
    // Refresh the selection bounding box
    // to reflect the new layout geometry
    this->InteractorStyle->HighLightCurrentSelectedItem();
    }
}

int vtkTreeRingViewer::GetLayoutStrategy()
{
  vtkTreeRingLayoutStrategy* strategy = this->TreeRingLayout->GetLayoutStrategy();
  if (strategy->IsA("vtkTreeRingDefaultLayoutStrategy"))
    {
    return TREE_RING_DEFAULT_LAYOUT;
    }
  else if (strategy->IsA("vtkTreeRingReversedLayoutStrategy"))
    {
    return TREE_RING_REVERSED_LAYOUT;
    }
  vtkWarningMacro(<< "Unknown layout strategy");
  return -1;
}

static const char *StrategyNames[vtkTreeRingViewer::NUMBER_OF_LAYOUTS] = {
  "Tree Ring Default Layout",
  "Tree Ring Reversed Layout"
};

const char *vtkTreeRingViewer::GetLayoutStrategyName(int strategy)
{
  if ((strategy < 0) || (strategy >= NUMBER_OF_LAYOUTS))
    {
    cout << "Bad Layout Strategy enum: " << strategy;
    return NULL;
    }

  return StrategyNames[strategy];
}

// Set up the layout strategy for the treemap
void vtkTreeRingViewer::SetLayoutStrategy(const char *layoutType)
{

  // Find the enumerated layout type and call the method
  for(int i=0; i<vtkTreeRingViewer::NUMBER_OF_LAYOUTS; i++)
    {
    if (!strcmp(layoutType, StrategyNames[i]))
      {
      this->SetLayoutStrategy(i);
      }
    }
}

// Highlight the tree item that matches the pedigree id
void vtkTreeRingViewer::HighLightItem(vtkIdType id)
{
  this->InteractorStyle->HighLightItem(id);
}

//----------------------------------------------------------------------------
void vtkTreeRingViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input Tree: " << (this->Input ? "" : "(none)") << endl;
  if (this->Input)
    {
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }

  os << indent << "RenderWindow: " << (this->RenderWindow ? "" : "(none)") << endl;
  if (this->RenderWindow)
    {
    this->RenderWindow->PrintSelf(os,indent.GetNextIndent());
    }

//   os << indent << "TreeLevelsFilter: " << (this->TreeLevelsFilter ? "" : "(none)") << endl;
//   if (this->TreeLevelsFilter)
//     {
//     this->TreeLevelsFilter->PrintSelf(os,indent.GetNextIndent());
//     }
  
  os << indent << "TreeFieldAggregator: " << (this->TreeFieldAggregator ? "" : "(none)") << endl;
  if (this->TreeFieldAggregator)
    {
    this->TreeFieldAggregator->PrintSelf(os,indent.GetNextIndent()); 
    }
  
  os << indent << "TreeRingLayout: " << (this->TreeRingLayout ? "" : "(none)") << endl;
  if (this->TreeRingLayout)
    {
    this->TreeRingLayout->PrintSelf(os,indent.GetNextIndent()); 
    }
  
  os << indent << "TreeRingToPolyData: " << (this->TreeRingToPolyData ? "" : "(none)") << endl;
  if (this->TreeRingToPolyData)
    {
    this->TreeRingToPolyData->PrintSelf(os,indent.GetNextIndent()); 
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
  if (this->Actor)
    {
    this->Actor->PrintSelf(os,indent.GetNextIndent());
    }
  
  os << indent << "InteractorStyle: " << (this->InteractorStyle ? "" : "(none)") << endl;
  if (this->InteractorStyle)
    {
    this->InteractorStyle->PrintSelf(os,indent.GetNextIndent());
    }

}
  
// void vtkTreeRingViewer::SetLabelLevelRange(int start, int end)
// {
//   this->LabeledDataMapper->SetLevelRange(start, end);
// }

// void vtkTreeRingViewer::GetLabelLevelRange(int range[2])
// {
//   this->LabeledDataMapper->GetLevelRange(range);
// }

// void vtkTreeRingViewer::SetDynamicLabelLevel(int level)
// {
//   this->LabeledDataMapper->SetDynamicLevel(level);
// }

// int vtkTreeRingViewer::GetDynamicLabelLevel()
// {
//   return this->LabeledDataMapper->GetDynamicLevel();
// }

// void vtkTreeRingViewer::SetChildLabelMotion(int mode)
// {
//   this->LabeledDataMapper->SetChildMotion(mode);
// }

// int vtkTreeRingViewer::GetChildLabelMotion()
// {
//   return this->LabeledDataMapper->GetChildMotion();
// }

// void vtkTreeRingViewer::SetLabelClipMode(int mode)
// {
//   this->LabeledDataMapper->SetClipTextMode(mode);
// }

// int vtkTreeRingViewer::GetLabelClipMode()
// {
//   return this->LabeledDataMapper->GetClipTextMode();
// }
