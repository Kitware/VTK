/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkParallelCoordinatesRepresentation.cxx

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

#include "vtkParallelCoordinatesRepresentation.h"

#include "vtkAbstractArray.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAnnotationLink.h"
#include "vtkArray.h"
#include "vtkArrayData.h"
#include "vtkArrayExtents.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkArrayToTable.h"
#include "vtkAxisActor2D.h"
#include "vtkBivariateLinearTableThreshold.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkConeSource.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetMapper.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkIntArray.h"
#include "vtkInteractorObserver.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineCornerSource.h"
#include "vtkParallelCoordinatesView.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPolyLine.h"
#include "vtkPropCollection.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSCurveSpline.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStdString.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkThreshold.h"
#include "vtkTimeStamp.h"
#include "vtkUnsignedIntArray.h"
#include "vtkViewTheme.h"

#include <vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkParallelCoordinatesRepresentation);

//------------------------------------------------------------------------------
// Esoteric template function that figures out the point positions for a single
// array in the plot.  It would be easier (for me) to loop through row at-a-time 
// instead of array at-a-time, but this is more efficient.
template <typename iterT>
void vtkParallelCoordinatesRepresentationBuildLinePoints(iterT* it, 
                                                         vtkIdTypeArray* idsToPlot,
                                                         int positionIdx,
                                                         double xPosition,
                                                         int numPositions,
                                                         double ymin, 
                                                         double ymax, 
                                                         double amin, 
                                                         double amax, 
                                                         vtkPoints* points)
{
  vtkIdType numTuples = it->GetNumberOfTuples();
  vtkIdType numComponents = it->GetNumberOfComponents();
  double arange = amax - amin;
  double yrange = ymax - ymin;
  double x[3] = {xPosition,ymin+0.5*yrange,0.0};

  // if there are no specific ids to plot, plot them all
  if (!idsToPlot)
    {
    if (arange == 0.0)
      {
      for (vtkIdType ptId=positionIdx,i=0; i<numTuples; i++,ptId+=numPositions)
        {
        points->SetPoint(ptId,x);
        }
      }
    else
      {
      // just a little optimization
      double ydiva = yrange / arange;
      vtkIdType ptId = positionIdx;

      for (vtkIdType i=0,arrayId=0; i<numTuples; i++,ptId+=numPositions,arrayId+=numComponents)
        {
        // map data value to screen position
        vtkVariant v(it->GetValue(arrayId));
        x[1] = ymin + ( v.ToDouble() - amin ) * ydiva;
        points->SetPoint(ptId,x);
        }
      }
    }
  // received a list of ids to plot, so only do those.
  else
    {

    int numIdsToPlot = idsToPlot->GetNumberOfTuples();

    if (arange == 0.0)
      {
      for (vtkIdType ptId=positionIdx,i=0; i<numIdsToPlot; i++,ptId+=numPositions)
        {
        points->SetPoint(ptId,x);
        }
      }
    else
      {
      // just a little optimization
      double ydiva = yrange / arange;
      vtkIdType ptId = positionIdx;

      for (vtkIdType i=0,arrayId=0; i<numIdsToPlot; i++,ptId+=numPositions)
        {
        // map data value to screen position
        arrayId = idsToPlot->GetValue(i)*numComponents;
        vtkVariant v(it->GetValue(arrayId));
        x[1] = ymin + ( v.ToDouble() - amin ) * ydiva;
        points->SetPoint(ptId,x);
        }
      }
    }
}

//------------------------------------------------------------------------------
// Class that houses the STL ivars.  There can be an arbitrary number of
// selections, so it easier to use STL than to be reallocating arrays.
class vtkParallelCoordinatesRepresentation::Internals
{
public:
  std::vector< vtkSmartPointer<vtkPolyData> > SelectionData;
  std::vector< vtkSmartPointer<vtkPolyDataMapper2D> > SelectionMappers;
  std::vector< vtkSmartPointer<vtkActor2D> > SelectionActors;
  static const double Colors[10][3];
  static const unsigned int NumberOfColors = 10;
  double *GetColor(unsigned int idx) 
    {
      idx = (idx >= NumberOfColors) ? NumberOfColors-1 : idx;
      return const_cast<double*>(Colors[idx]);
    }
};

//------------------------------------------------------------------------------
// The colors used for the selections
const double vtkParallelCoordinatesRepresentation::Internals::Colors[10][3] = { {1.0,0.0,0.0}, // red
                                                              {0.0,1.0,0.0}, // green
                                                              {0.0,.8,1.0}, // cyan
                                                              {.8,.8,0.0}, // yellow
                                                              {.8,0.0,.8}, // magenta
                                                              {.2,.2,1.0}, // blue
                                                              {1.0,.65,0.0}, // orange
                                                              {.5,.5,.5}, // gray
                                                              {.6,.2,.2}, // maroon
                                                              {.3,.3,.3} }; // dark gray

//------------------------------------------------------------------------------
vtkParallelCoordinatesRepresentation::vtkParallelCoordinatesRepresentation()
{
  this->SetNumberOfInputPorts(vtkParallelCoordinatesRepresentation::NUM_INPUT_PORTS);
  //DBG
  this->SetNumberOfOutputPorts(1);
  //DBG

  this->I = new Internals;
  this->AxisTitles = vtkSmartPointer<vtkStringArray>::New();
  this->PlotData = vtkSmartPointer<vtkPolyData>::New();
  this->PlotActor = vtkSmartPointer<vtkActor2D>::New();

  this->PlotMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  this->PlotMapper.TakeReference(InitializePlotMapper(this->PlotData,this->PlotActor,true));

  this->InverseSelection = vtkSmartPointer<vtkSelection>::New();

  this->InputArrayTable = vtkSmartPointer<vtkTable>::New();
  this->LinearThreshold = vtkSmartPointer<vtkBivariateLinearTableThreshold>::New();
  this->LinearThreshold->SetInput(this->InputArrayTable);

  this->Axes = NULL;
  this->NumberOfAxisLabels = 2;


  this->PlotTitleMapper = vtkSmartPointer<vtkTextMapper>::New();
  this->PlotTitleMapper->SetInput("Parallel Coordinates Plot");
  this->PlotTitleMapper->GetTextProperty()->SetJustificationToCentered();

  this->PlotTitleActor = vtkSmartPointer<vtkActor2D>::New();
  this->PlotTitleActor->SetMapper(this->PlotTitleMapper);
//  this->PlotTitleActor->SetTextScaleModeToViewport();
  this->PlotTitleActor->GetActualPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  this->PlotTitleActor->SetPosition(.5,.95);


  this->FunctionTextMapper = vtkSmartPointer<vtkTextMapper>::New();
  this->FunctionTextMapper->SetInput("No functino selected.");
  this->FunctionTextMapper->GetTextProperty()->SetJustificationToLeft();
  this->FunctionTextMapper->GetTextProperty()->SetVerticalJustificationToTop();
//  this->FunctionTextActor->SetInput("No function selected.");
  this->FunctionTextMapper->GetTextProperty()->SetFontSize(
    this->PlotTitleMapper->GetTextProperty()->GetFontSize()/2);

  this->FunctionTextActor = vtkSmartPointer<vtkActor2D>::New();
//  this->FunctionTextActor->SetTextScaleModeToViewport();
  this->FunctionTextActor->GetActualPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  this->FunctionTextActor->SetPosition(.01,.99);
  this->FunctionTextActor->VisibilityOff();

  this->NumberOfAxes = 0;
  this->NumberOfSamples = 0;
  this->YMin = .1;
  this->YMax = .9;
  this->Xs = NULL;
  this->Mins = NULL;
  this->Maxs = NULL;
  this->MinOffsets = NULL;
  this->MaxOffsets = NULL;

  this->CurveResolution = 20;
  this->UseCurves = 0;

  this->AngleBrushThreshold = .03;
  this->FunctionBrushThreshold = .1;
  this->SwapThreshold = 0.0;
  
  this->FontSize = 1.0;

  // Apply default theme
  this->LineOpacity = 1.0;
  this->LineColor[0] = this->LineColor[1] = this->LineColor[2] = 0.0;
  this->AxisColor[0] = this->AxisColor[1] = this->AxisColor[2] = 0.0;
  this->AxisLabelColor[0] = this->AxisLabelColor[1] = this->AxisLabelColor[2] = 0.0;

  vtkViewTheme* theme = vtkViewTheme::New();
  theme->SetCellOpacity(1.0);
  theme->SetCellColor(1.0,1.0,1.0);
  theme->SetEdgeLabelColor(1.0,.8,.3);
  this->ApplyViewTheme(theme);
  theme->Delete();

  this->InternalHoverText = 0;
}

//------------------------------------------------------------------------------
vtkParallelCoordinatesRepresentation::~vtkParallelCoordinatesRepresentation()
{
  if (I)
    delete I;

  if (this->Maxs)
    delete [] this->Maxs;
  
  if (this->Mins)
    delete [] this->Mins;
  
  if (this->MaxOffsets)
    delete [] this->MaxOffsets;
  
  if (this->MinOffsets)
    delete [] this->MinOffsets;

  if (this->Axes)
    delete [] this->Axes;

  if (this->Xs)
    delete [] this->Xs;

  this->SetInternalHoverText(0);
}

//------------------------------------------------------------------------------
// I should fill this out.
const char* vtkParallelCoordinatesRepresentation::GetHoverText(vtkView* view, 
                                                               int x, 
                                                               int y)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv && this->NumberOfAxes > 0)
    {
    int* s = rv->GetRenderer()->GetSize();

    double p[2] = {0.0,0.0};
    p[0] = static_cast<double>(x) / s[0];
    p[1] = static_cast<double>(y) / s[1];
    
    int position = this->GetPositionNearXCoordinate(p[0]);
    
    if (fabs(p[0] - this->Xs[position]) < .05 &&
        p[1] <= this->YMax &&
        p[1] >= this->YMin)
      {
      double pct = (p[1] - this->YMin) / (this->YMax - this->YMin);

      double r[2] = {0.0,0.0};
      this->GetRangeAtPosition(position,r);

      double v = pct * (r[1]-r[0]) + r[0];
      vtkVariant var(v);

      this->SetInternalHoverText(vtkVariant(v).ToString());
      return this->GetInternalHoverText();
      }
    else if (p[0] > this->Xs[0] &&
             p[1] < this->Xs[this->NumberOfAxes-1] &&
             p[1] <= this->YMax &&
             p[1] >= this->YMin)
      {
      this->UpdateHoverHighlight(view,x,y);
      return this->GetInternalHoverText();
      }
    }
  return 0;
}

//------------------------------------------------------------------------------
// Not sure what this function is for
void vtkParallelCoordinatesRepresentation::UpdateHoverHighlight(vtkView* view, 
                                                                int x, 
                                                                int y)
{
  // Make sure we have a context.
  vtkRenderer* r = vtkRenderView::SafeDownCast(view)->GetRenderer();
  vtkRenderWindow* win = r->GetRenderWindow();
  if (!win)
    {
    return;
    }
  win->MakeCurrent();

  if (!win->IsCurrent())
    {
    return;
    }

  // Use the hardware picker to find a point in world coordinates.

  if (x > 0 && y > 0)
    {
        vtksys_ios::ostringstream str;
    int* size = win->GetSize();
    int linesFound = 0;
    vtkCellArray* lines = this->PlotData->GetLines();

    int lineNum = 0;
    vtkIdType* pts = 0;
    vtkIdType  npts = 0;
    double p[3] = {x,y,0.0};
    p[0] /= size[0];
    p[1] /= size[1];

    if (p[0] < this->Xs[0] || 
        p[0] > this->Xs[this->NumberOfAxes-1] ||
        p[1] < this->YMin ||
        p[1] > this->YMax)
      return;

    double p1[3];
    double p2[3];
    double dist;

    int position = this->ComputePointPosition(p);

    for (lines->InitTraversal(); lines->GetNextCell(npts,pts); lineNum++)
      {
      if (!pts) break;

      this->PlotData->GetPoints()->GetPoint(pts[position],p1);
      this->PlotData->GetPoints()->GetPoint(pts[position+1],p2);

      dist = fabs((p2[1]-p1[1])/(p2[0]-p1[0])*(p[0]-p1[0])+p1[1]-p[1]);

      if (dist < .01)
        {
        str << lineNum << " ";
        linesFound++;

        if (linesFound > 2)
          {
          str << "...";
          break;
          }
        }
      }

    this->SetInternalHoverText(str.str().c_str());
    }
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkDebugMacro(<<"begin request data.\n");

  // get the info objects and input
  vtkInformation* inDataInfo = inputVector[INPUT_DATA]->GetInformationObject(0);
  vtkInformation* inTitleInfo = inputVector[INPUT_TITLES]->GetInformationObject(0);

  if (!inDataInfo)
    return 0;

  vtkDataObject *inputData = inDataInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!inputData)
    return 0;

  // pull out the title string array
  vtkStringArray* titles = NULL;
  if (inTitleInfo)
    {
    vtkTable* inputTitles = vtkTable::SafeDownCast(inTitleInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (inputTitles && inputTitles->GetNumberOfColumns() > 0)
      {
      titles = vtkStringArray::SafeDownCast(inputTitles->GetColumn(0));
      }
    }
  // build the input array table.  This is convenience table that gets used
  // later when building the plots.
  if (this->GetInput()->GetMTime() > this->BuildTime)
    {
    if (inputData->IsA("vtkArrayData"))
      {
      vtkSmartPointer<vtkArrayToTable> att = vtkSmartPointer<vtkArrayToTable>::New();
      att->SetInput(inputData);
      att->Update();

      this->InputArrayTable->ShallowCopy(att->GetOutput());
      }
    else
      {
      vtkInformationVector *inArrayVec = 
        this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
 
      if (!inArrayVec)
        {
        vtkErrorMacro(<<"No input arrays specified.  Use SetInputArrayToProcess(i,...).");
        return 0;
        }

      int numberOfInputArrays = inArrayVec->GetNumberOfInformationObjects();

      if (numberOfInputArrays <= 0)
        {
        vtkErrorMacro(<<"No input arrays specified.  Use SetInputArrayToProcess(i,...).");
        return 0;
        }

      this->InputArrayTable->Initialize();

      for (int i=0; i<numberOfInputArrays; i++)
        {
        vtkDataArray* a = this->GetInputArrayToProcess(i,inputVector);
        if (a)
          {
          this->InputArrayTable->AddColumn(a);
          }
        }
      }
    }

  if (this->InputArrayTable->GetNumberOfColumns() <= 0)
    {
    vtkErrorMacro(<<"No valid input arrays specified.");
    return 0;
    }

  vtkDebugMacro(<<"begin compute data properties.\n");  
  if (!this->ComputeDataProperties())
    return 0;
  
  vtkDebugMacro(<<"begin axis placement.\n");
  if (!this->PlaceAxes())
    return 0;

  vtkDebugMacro(<<"begin line placement.\n");

  this->UpdateSelectionActors();

  vtkIdTypeArray* unselectedRows = NULL;
  if (this->InverseSelection->GetNode(0))
    unselectedRows = vtkIdTypeArray::SafeDownCast(this->InverseSelection->GetNode(0)->GetSelectionList());

  if (this->UseCurves)
    {
    if (!this->PlaceCurves(this->PlotData,this->InputArrayTable,unselectedRows))
      return 0;
    }
  else 
    {
    if (!this->PlaceLines(this->PlotData,this->InputArrayTable,unselectedRows))
      return 0;
    }

  vtkDebugMacro(<<"begin selection line placement.\n");
  vtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();
  if (selection)
    {

    for (unsigned int i=0; i<selection->GetNumberOfNodes(); i++)
      {
      if (!this->PlaceSelection(this->I->SelectionData[i],this->InputArrayTable,selection->GetNode(i)))
        return 0;
      if (i > 0)
        continue;
      }
    }

  vtkDebugMacro(<<"begin update plot properties.\n");
  if (!this->UpdatePlotProperties(titles))
    return 0;

  this->BuildTime.Modified();

  return 1;
}

//------------------------------------------------------------------------------
// Add all of the plot actors to the view
bool vtkParallelCoordinatesRepresentation::AddToView(vtkView* view)
{
  this->Superclass::AddToView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    rv->GetRenderer()->AddActor(this->PlotTitleActor);
    rv->GetRenderer()->AddActor(this->FunctionTextActor);
    rv->GetRenderer()->AddActor(this->PlotActor);

    for (int i=0; i<this->NumberOfAxes; i++)
      {
      rv->GetRenderer()->AddActor(this->Axes[i]);
      }
    for (int i=0; i<(int)this->I->SelectionActors.size(); i++)
      {
      rv->GetRenderer()->AddActor(this->I->SelectionActors[i]);
      }

    // not sure what these are for
    //rv->RegisterProgress(...);
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
// Remove all of the plot actors from the view
bool vtkParallelCoordinatesRepresentation::RemoveFromView(vtkView* view)
{
  this->Superclass::RemoveFromView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    rv->GetRenderer()->RemoveActor(this->PlotTitleActor);
    rv->GetRenderer()->RemoveActor(this->FunctionTextActor);
    rv->GetRenderer()->RemoveActor(this->PlotActor);

    for (int i=0; i<this->NumberOfAxes; i++)
      {
      rv->GetRenderer()->RemoveActor(this->Axes[i]);
      }

    for (int i=0; i<(int)this->I->SelectionActors.size(); i++)
      {
      rv->GetRenderer()->RemoveActor(this->I->SelectionActors[i]);
      }

    // not sure what these are for
    //rv->UnRegisterProgress(this->OutlineMapper);
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::PrepareForRendering(vtkRenderView* view)
{
  this->Superclass::PrepareForRendering(view);

  
  // Make hover highlight up to date

  // Add/remove graph actors as necessary as input connections are added/removed
  
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  double opacity = std::max(0.0,std::min(1.0,theme->GetCellOpacity()));
  this->SetLineOpacity(opacity);
  this->SetLineColor(theme->GetCellColor());
  this->SetAxisColor(theme->GetEdgeLabelColor());
  this->SetAxisLabelColor(theme->GetCellColor());
  this->SetLineOpacity(theme->GetCellOpacity());
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::FillInputPortInformation(int port, 
                                                                   vtkInformation* info)
{
  if (port == vtkParallelCoordinatesRepresentation::INPUT_DATA)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    return 1;
    }
  else if (port == vtkParallelCoordinatesRepresentation::INPUT_TITLES)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
    }

  return 0;
}
//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::SetAxisTitles(vtkAlgorithmOutput* ao)
{
  this->SetInputConnection(1,ao);
}
//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::SetAxisTitles(vtkStringArray* sa)
{
  vtkSmartPointer<vtkTable> t = vtkSmartPointer<vtkTable>::New();
  t->AddColumn(sa);
  this->SetInput(1,t);
}
//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::PrintSelf(ostream& os, 
                                                     vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfAxes: " << this->NumberOfAxes << endl;
  os << "NumberOfSamples: " << this->NumberOfSamples << endl;
  os << "NumberOfAxisLabels: " << this->NumberOfAxisLabels << endl;
  os << "YMin: " << this->YMin << endl;
  os << "YMax: " << this->YMax << endl;
  os << "CurveResolution: " << this->CurveResolution << endl;
  os << "UseCurves: " << this->UseCurves << endl;
  os << "AngleBrushThreshold: " << this->AngleBrushThreshold << endl;
  os << "FunctionBrushThreshold: " << this->FunctionBrushThreshold << endl;
  os << "SwapThreshold: " << this->SwapThreshold << endl;
  os << "LineOpacity: " << this->LineOpacity << endl;
  os << "FontSize: " << this->FontSize << endl;
  os << "LineColor: " << 
    this->LineColor[0] << this->LineColor[1] << this->LineColor[2] << endl;
  os << "AxisColor: " << 
    this->AxisColor[0] << this->AxisColor[1] << this->AxisColor[2] << endl;
  os << "AxisLabelColor: " << 
    this->AxisLabelColor[0] << 
    this->AxisLabelColor[1] << 
    this->AxisLabelColor[2] << endl;

  os << "Xs: ";
  for (int i=0; i<this->NumberOfAxes; i++)
    os << this->Xs[i];
  os << endl;

  os << "Mins: ";
  for (int i=0; i<this->NumberOfAxes; i++)
    os << this->Mins[i];
  os << endl;

  os << "Maxs: ";
  for (int i=0; i<this->NumberOfAxes; i++)
    os << this->Maxs[i];
  os << endl;

  os << "MinOffsets: ";
  for (int i=0; i<this->NumberOfAxes; i++)
    os << this->MinOffsets[i];
  os << endl;

  os << "MaxOffsets: ";
  for (int i=0; i<this->NumberOfAxes; i++)
    os << this->MaxOffsets[i];
  os << endl;
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::ComputeDataProperties()
{
  // if the data hasn't changed, there's no reason to recompute 
  if (this->BuildTime > this->GetInput()->GetMTime())
    {
    return 1;
    }

  int numberOfInputArrays = this->InputArrayTable->GetNumberOfColumns();
  int newNumberOfAxes = 0;
  int newNumberOfSamples = 0;

  // stores the array names, if there are any
  vtkSmartPointer<vtkStringArray> newtitles = vtkSmartPointer<vtkStringArray>::New();

  for (int i=0; i<numberOfInputArrays; i++)
    {
    vtkAbstractArray* array = this->InputArrayTable->GetColumn(i);
    int numTuples = array->GetNumberOfTuples();

    if (i > 0 && newNumberOfSamples != numTuples)
      {
      vtkErrorMacro(<<"Error: all arrays must have the same number of values!");
      return 0;
      }
    else
      {
      newNumberOfSamples = numTuples;
      }

    newNumberOfAxes++;

    if (array->GetName())
      {
      newtitles->InsertNextValue(array->GetName());
      }  
    }

  if (newNumberOfAxes <= 0 || 
      newNumberOfSamples <= 0) 
    {
    return 0;
    }

  // did the number of axes change? reinitialize EVERYTHING.
  if (newNumberOfAxes != this->NumberOfAxes || 
      newNumberOfSamples != this->NumberOfSamples)
    {
    // make sure that the old ones get removed
    for (int i=0; i<this->NumberOfAxes; i++)
      {
      this->RemovePropOnNextRender(this->Axes[i]);
      }

    this->NumberOfAxes = newNumberOfAxes;
    this->NumberOfSamples = newNumberOfSamples;

    this->ReallocateInternals();
    }


  if (this->AxisTitles->GetNumberOfValues() != this->NumberOfAxes ||
      newtitles->GetNumberOfValues() == this->NumberOfAxes)
    {
    this->AxisTitles->Initialize();
    this->AxisTitles->DeepCopy(newtitles);
    }

  // compute axis ranges
  for (int i=0; i<numberOfInputArrays; i++)
    {
    vtkDataArray* array = vtkDataArray::SafeDownCast(this->InputArrayTable->GetColumn(i));
    double *r = array->GetRange(0);
    this->Mins[i] = r[0];
    this->Maxs[i] = r[1];
    }

  return 1;
}

//------------------------------------------------------------------------------
// update colors and such.
int vtkParallelCoordinatesRepresentation::UpdatePlotProperties(vtkStringArray* inputTitles)
{

  this->PlotActor->GetProperty()->SetColor(this->LineColor);
  this->PlotActor->GetProperty()->SetOpacity(this->LineOpacity);
  this->PlotTitleActor->GetProperty()->SetColor(this->AxisLabelColor);
  
  if (inputTitles)
    {
    this->AxisTitles->DeepCopy(inputTitles);
    }
  // make sure we have sufficient plot titles
  if (this->NumberOfAxes != this->AxisTitles->GetNumberOfValues())
    {
    vtkWarningMacro( << "Warning: wrong number of axis titles, using default labels." );

    this->AxisTitles->Initialize();
    for (int i=0; i<this->NumberOfAxes; i++)
      {
      char title[16];
      sprintf(title,"%c",i+65);
      this->AxisTitles->InsertNextValue(title);
      }
    }

  // set everything on the axes
  for (int i=0; i<this->NumberOfAxes; i++)
    {
    this->Axes[i]->SetTitle(this->AxisTitles->GetValue(i));
    this->Axes[i]->SetRange ( this->Mins[i]+this->MinOffsets[i],this->Maxs[i]+this->MaxOffsets[i] );
    this->Axes[i]->GetProperty()->SetColor(this->AxisColor);
    this->Axes[i]->GetTitleTextProperty()->SetColor(this->AxisLabelColor);
    this->Axes[i]->GetLabelTextProperty()->SetColor(this->AxisLabelColor);
    this->Axes[i]->AdjustLabelsOff();
    this->Axes[i]->GetProperty()->SetLineWidth ( 2.0 );
    this->Axes[i]->SetLabelFactor ( 0.5 );
    this->Axes[i]->TickVisibilityOff();
    this->Axes[i]->SetNumberOfLabels ( this->NumberOfAxisLabels );
    this->Axes[i]->SetTitlePosition ( -.05 );
    this->Axes[i]->GetTitleTextProperty()->SetJustificationToRight();
    this->Axes[i]->GetTitleTextProperty()->ItalicOff();
    this->Axes[i]->GetTitleTextProperty()->BoldOff();
    this->Axes[i]->GetLabelTextProperty()->ItalicOff();
    this->Axes[i]->GetLabelTextProperty()->BoldOff();
    this->Axes[i]->SetFontFactor(this->FontSize);
    this->Axes[i]->GetTitleTextProperty()->Modified();
    }

  for (int i=0; i<(int)this->I->SelectionActors.size(); i++)
    {
    this->I->SelectionActors[i]->GetProperty()->SetOpacity(this->LineOpacity);
    this->I->SelectionActors[i]->GetProperty()->SetColor(this->I->GetColor(i));
    }
  
  return 1;
}

//------------------------------------------------------------------------------
// Clear out all of the arrays and intialize them to defaults where appropriate.
int vtkParallelCoordinatesRepresentation::ReallocateInternals()
{
  if (this->Maxs) delete [] this->Maxs;
  if (this->Mins) delete [] this->Mins;
  if (this->MaxOffsets) delete [] this->MaxOffsets;
  if (this->MinOffsets) delete [] this->MinOffsets;
  if (this->Axes)  delete [] this->Axes;
  if (this->Xs) delete [] this->Xs;

  this->Maxs = new double[this->NumberOfAxes];
  this->Mins = new double[this->NumberOfAxes];
  this->MaxOffsets = new double[this->NumberOfAxes];
  this->MinOffsets = new double[this->NumberOfAxes];
  this->Axes = new vtkSmartPointer<vtkAxisActor2D>[this->NumberOfAxes];
  this->Xs = new double[this->NumberOfAxes];

  for (int i=0; i<this->NumberOfAxes; i++)
    {
    this->Maxs[i] = -VTK_DOUBLE_MAX;
    this->Mins[i] = VTK_DOUBLE_MAX;
    this->MaxOffsets[i] = 0.0;
    this->MinOffsets[i] = 0.0;
    this->Axes[i] = vtkSmartPointer<vtkAxisActor2D>::New();
    this->Xs[i] = -1.0;

    this->AddPropOnNextRender(this->Axes[i]);
    }

  // the x positions of axes
  double p1[] = {.1,.1};
  double p2[] = {.8,.8};
  double width = ( p2[0] ) / static_cast<double>(this->NumberOfAxes-1);
  this->SwapThreshold = width * .1;

  // figure out where each axis should go
  for ( int i=0; i<this->NumberOfAxes; i++ )
    {
    this->Xs[i] = p1[0] + i * width;
    }
  return 1;
}

//------------------------------------------------------------------------------
// put the axes where they're supposed to go, which is defined in this->Xs.
int vtkParallelCoordinatesRepresentation::PlaceAxes()
{
//  int axisI;

  // Get the location of the corners of the box
  double p1[2],p2[2];
  p1[0] = p1[1] = p2[0] = p2[1] = 0.;
  this->GetPositionAndSize(p1,p2);

  // Specify the positions for the axes
  this->YMin = p1[1];
  this->YMax = p1[1]+p2[1];

  // do the placement
  for ( int pos=0; pos<this->NumberOfAxes; pos++ )
    {
    //axisI = this->AxisOrder[pos];
    this->Axes[pos]->GetPositionCoordinate()->SetValue ( this->Xs[pos],this->YMin );
    this->Axes[pos]->GetPosition2Coordinate()->SetValue ( this->Xs[pos],this->YMax );

    this->Axes[pos]->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    this->Axes[pos]->GetPosition2Coordinate()->SetCoordinateSystemToNormalizedViewport();
    }

  return 1;
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::AllocatePolyData(vtkPolyData* polyData, 
                                                           int numLines, 
                                                           int numPointsPerLine, 
                                                           int numStrips, 
                                                           int numPointsPerStrip, 
                                                           int numQuads, 
                                                           int numPoints,
                                                           int numCellScalars,
                                                           int numPointScalars)
{
  // if there are lines requested, make room and fill in some default cells
  if (numLines)
    {
    vtkCellArray* lines = polyData->GetLines();
    if (!lines || 
        lines->GetSize() != lines->EstimateSize( numLines,numPointsPerLine ) ||
        lines->GetNumberOfCells() != numLines)
      {
      lines = vtkCellArray::New();
      lines->Allocate(lines->EstimateSize( numLines, numPointsPerLine));
      polyData->SetLines(lines);
      lines->Delete();

      // prepare the cell array. might as well initialize it now and only
      // recompute it when something actually changes.
      vtkIdType *ptIds = new vtkIdType[numPointsPerLine];

      lines->InitTraversal();
      for (int i=0; i<numLines; i++)
        {
        for (int j=0; j<numPointsPerLine; j++)
          {
          ptIds[j] = i*numPointsPerLine + j;
          }
        lines->InsertNextCell(numPointsPerLine,ptIds);
        }
      delete [] ptIds;
      }
    }
  else
    {
    polyData->SetLines(NULL);
    }

  // if there are strips requested, make room and fill in some default cells
  if (numStrips)
    {
    vtkCellArray* strips = polyData->GetStrips();
    if (!strips || 
        strips->GetSize() != strips->EstimateSize( numStrips,numPointsPerStrip ) ||
        strips->GetNumberOfCells() != numStrips)
      {
      strips = vtkCellArray::New();
      strips->Allocate(strips->EstimateSize( numStrips, numPointsPerStrip));
      polyData->SetStrips(strips);
      strips->Delete();

      // prepare the cell array. might as well initialize it now and only
      // recompute it when something actually changes.
      vtkIdType *ptIds = new vtkIdType[numPointsPerStrip];

      strips->InitTraversal();
      for (int i=0; i<numStrips; i++)
        {
        for (int j=0; j<numPointsPerStrip; j++)
          {
          ptIds[j] = i*numPointsPerStrip + j;
          }
        strips->InsertNextCell(numPointsPerStrip,ptIds);
        }
      delete [] ptIds;
      }
    }
  else
    {
    polyData->SetStrips(NULL);
    }

  // if there are quads requested, make room and fill in some default cells
  if (numQuads)
    {
    vtkCellArray* quads = polyData->GetPolys();
    if (!quads || 
        quads->GetSize() != quads->EstimateSize( numQuads,4 ) ||
        quads->GetNumberOfCells() != numQuads)
      {
      quads = vtkCellArray::New();
      quads->Allocate(quads->EstimateSize( numQuads, 4));
      polyData->SetPolys(quads);
      quads->Delete();

      // prepare the cell array. might as well initialize it now and only
      // recompute it when something actually changes.
      vtkIdType *ptIds = new vtkIdType[4];

      quads->InitTraversal();
      for (int i=0; i<numQuads; i++)
        {
        for (int j=0; j<4; j++)
          {
          ptIds[j] = i*4 + j;
          }
        quads->InsertNextCell(4,ptIds);
        }
      delete [] ptIds;
      }
    }
  else
    {
    polyData->SetPolys(NULL);
    }

  // if there are points requested, make room.  don't fill in defaults, as that's
  // what the Place*** functions are for.
  if (numPoints)
    {
    vtkPoints* points = polyData->GetPoints();
    // check if we need to (re)allocate space for the points
    if (!points || points->GetNumberOfPoints() != (numPoints))
      {
      points = vtkPoints::New();
      points->SetNumberOfPoints(numPoints);
      polyData->SetPoints(points);
      points->Delete();
      }
    }
  else
    {
    polyData->SetPoints(NULL);
    }
  
  // if there are scalars requested, make room. defaults everything to 0.  
  // scalars are all vtkDoubleArrays.
  if (numCellScalars)
    {
    vtkDoubleArray* scalars =
      vtkDoubleArray::SafeDownCast(polyData->GetCellData()->GetScalars());

    if (!scalars)
      {
      scalars = vtkDoubleArray::New();
      polyData->GetCellData()->SetScalars(scalars);
      scalars->Delete();
      }

    if (scalars->GetNumberOfTuples() != numCellScalars)
      {
      scalars->SetNumberOfTuples(numCellScalars);
      scalars->FillComponent(0,0);
      }
    }
  else
    {
    polyData->GetCellData()->SetScalars(NULL);
    }
  
  // if there are scalars requested, make room. defaults everything to 0.  
  // scalars are all vtkDoubleArrays.
  if (numPointScalars)
    {
    vtkDoubleArray* scalars =
      vtkDoubleArray::SafeDownCast(polyData->GetPointData()->GetScalars());

    if (!scalars)
      {
      scalars = vtkDoubleArray::New();
      polyData->GetPointData()->SetScalars(scalars);
      scalars->Delete();
      }

    if (scalars->GetNumberOfTuples() != numPointScalars)
      {
      scalars->SetNumberOfTuples(numPointScalars);
      scalars->FillComponent(0,0);
      }
    }
  else
    {
    polyData->GetPointData()->SetScalars(NULL);
    }

  polyData->BuildCells();
  return 1;
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::PlaceLines(vtkPolyData* polyData, 
                                                     vtkTable* data,
                                                     vtkIdTypeArray* idsToPlot)
{
  if (!polyData)
    return 0;

  if (!data || data->GetNumberOfColumns() != this->NumberOfAxes)
    {
    polyData->Initialize();
    return 0;
    }

//  int axisI;
  int position;

  int numPointsPerSample = this->NumberOfAxes;
  int numSamples = (idsToPlot) ? idsToPlot->GetNumberOfTuples() : data->GetNumberOfRows();//this->NumberOfSamples;

  this->AllocatePolyData(polyData,
                         numSamples,
                         numPointsPerSample,0,0,0,
                         numSamples*numPointsPerSample,
                         0,0); // no scalars

  vtkPoints* points = polyData->GetPoints();

  for (position=0; position<this->NumberOfAxes; position++)
    {
    // figure out which axis is at this position
    //axisI = this->AxisOrder[position];
      
    // get the relevant array information
//    vtkDataArray* array = this->GetInputArrayAtPosition(position);
    vtkDataArray* array = vtkDataArray::SafeDownCast(data->GetColumn(position));
    if (!array)
      return 0;

    // start the iterator
    vtkArrayIterator* iter = array->NewIterator();
    switch(array->GetDataType())
      {
      vtkArrayIteratorTemplateMacro(
        vtkParallelCoordinatesRepresentationBuildLinePoints(
          static_cast<VTK_TT*>(iter),
          idsToPlot,
          position,
          this->Xs[position],
          this->NumberOfAxes,
          this->YMin,
          this->YMax,
          this->Mins[position] + this->MinOffsets[position],
          this->Maxs[position] + this->MaxOffsets[position],
          points));
      }
    iter->Delete();
    }

  return 1;
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::PlaceCurves(vtkPolyData* polyData, 
                                                      vtkTable* data,
                                                      vtkIdTypeArray* idsToPlot)
{
  if (!polyData)
    return 0;

  if (!data || data->GetNumberOfColumns() != this->NumberOfAxes)
    {
    polyData->Initialize();
    return 0;
    }

//  int axisI;
  int sampleI, position;
  double x[3] = {0,0,0};

  int numPointsPerSample = (this->NumberOfAxes-1)*this->CurveResolution + 1;
  int numSamples = (idsToPlot) ? idsToPlot->GetNumberOfTuples() : data->GetNumberOfRows();//this->NumberOfSamples;

  this->AllocatePolyData(polyData,
                         numSamples,
                         numPointsPerSample,0,0,0,
                         numSamples*numPointsPerSample,
                         0,0);//numSamples);

  vtkPoints* points = polyData->GetPoints();

  // same as PlaceLines(...), except the number of positions argument has changed.
  for (position=0; position<this->NumberOfAxes; position++)
    {
    // figure out which axis is at this position
    //axisI = this->AxisOrder[position];
      
    // get the relevant array information
//    vtkDataArray* array = this->GetInputArrayAtPosition(position);
    vtkDataArray* array = vtkDataArray::SafeDownCast(data->GetColumn(position));
    if (!array)
      {
      return 0;
      }

    // start the iterator
    // this fills out a subset of the actual points, namely just the points
    // on the axes.  These get used later to fill in the rest
    vtkArrayIterator* iter = array->NewIterator();
    switch(array->GetDataType())
      {
      vtkArrayIteratorTemplateMacro(
        vtkParallelCoordinatesRepresentationBuildLinePoints(
          static_cast<VTK_TT*>(iter),
          idsToPlot,
          this->CurveResolution*position,
          this->Xs[position],
          numPointsPerSample,
          this->YMin,
          this->YMax,
          this->Mins[position] + this->MinOffsets[position],
          this->Maxs[position] + this->MaxOffsets[position],
          points));
      }
    }

  // make a s-curve from (0,0) to (1,1) with the right number of segments.
  // this curve gets transformed based on data values later.
  vtkSmartPointer<vtkDoubleArray> defSplineValues = vtkSmartPointer<vtkDoubleArray>::New();
  this->BuildDefaultSCurve(defSplineValues,this->CurveResolution);

  // now go through what just got filled in and build splines.
  // specifically, the points sitting exactly on the axes are correct,
  // but nothing else is.  Just use that information to build the
  // splines per sample and fill in everything in between.
  vtkIdType ptId=0;
  double pL[3] = {0,0,0};
  double pR[3] = {0,0,0};
  for (sampleI=0; sampleI<numSamples; sampleI++)
    {
    // build the spline for this sample
    for (position=0; position<this->NumberOfAxes-1; position++)
      {
      points->GetPoint(position*this->CurveResolution + sampleI*numPointsPerSample,pL);
      points->GetPoint((position+1)*this->CurveResolution + sampleI*numPointsPerSample,pR);
      double dy = pR[1] - pL[1];
      double dx = (this->Xs[position+1] - this->Xs[position]) / static_cast<double>(this->CurveResolution);
      for (int curvePosition=0; curvePosition<this->CurveResolution; curvePosition++)
        {
        x[0] = this->Xs[position] + curvePosition*dx;
        x[1] = defSplineValues->GetValue(curvePosition) * dy + pL[1];
        points->SetPoint( ptId++, x );
        }
      }
    ptId++;
    }

  return 1;
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::BuildDefaultSCurve(vtkDoubleArray* defArray, 
                                                              int numValues)
{
  if (!defArray)
    return;

  // build a default spline, going from (0,0) to (1,1), 
  vtkSmartPointer<vtkSCurveSpline> defSpline = vtkSmartPointer<vtkSCurveSpline>::New();
  defSpline->SetParametricRange(0,1);
  defSpline->AddPoint(0,0);
  defSpline->AddPoint(1,1);

  // fill in an array with the interpolated curve values
  defArray->Initialize();
  defArray->SetNumberOfValues(numValues);
  for (int i=0; i<numValues; i++)
    {
    defArray->SetValue(i,defSpline->Evaluate(static_cast<double>(i)/numValues));
    }
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::PlaceSelection(vtkPolyData* polyData, 
                                                         vtkTable* data,
                                                         vtkSelectionNode* selectionNode)
{
  vtkIdTypeArray* selectedIds = vtkIdTypeArray::SafeDownCast(selectionNode->GetSelectionList());

  if (!selectedIds)
    return 0;

  if (this->UseCurves)
    {
    return this->PlaceCurves(polyData,data,selectedIds);
    }
  else
    {
    return this->PlaceLines(polyData,data,selectedIds);
    }
}
//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::SetPlotTitle(const char* title)
{
  if (title && title[0] != '\0')
    {
    this->PlotTitleActor->VisibilityOn();
    this->PlotTitleMapper->SetInput(title);
    }
  else
    {
    this->PlotTitleActor->VisibilityOff();
    }
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::SetNumberOfAxisLabels(int num)
{
  if (num > 0)
    {
    this->NumberOfAxisLabels = num;
    for (int i=0; i<this->NumberOfAxes; i++)
      {
      this->Axes[i]->SetNumberOfLabels(num);
      }
    }
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::SwapAxisPositions(int position1, 
                                                            int position2)
{
  if (position1 < 0 ||
      position2 < 0 ||
      position1 >= this->NumberOfAxes ||
      position2 >= this->NumberOfAxes)
    {
    return 0;
    }

  // for some reason there's no SetColumn(...)
  if (this->InputArrayTable->GetNumberOfColumns() > 0)
    {
    vtkSmartPointer<vtkTable> oldTable = vtkSmartPointer<vtkTable>::New();
    for (int i=0; i<this->NumberOfAxes; i++)
      {
      oldTable->AddColumn(this->InputArrayTable->GetColumn(i));
      }

    vtkAbstractArray* a1 = this->InputArrayTable->GetColumn(position1);
    vtkAbstractArray* a2 = this->InputArrayTable->GetColumn(position2);
    this->InputArrayTable->Initialize();
    for (int i=0; i<this->NumberOfAxes; i++)
      {
      if (i == position1)
        this->InputArrayTable->AddColumn(a2);
      else if (i == position2)
        this->InputArrayTable->AddColumn(a1);
      else
        this->InputArrayTable->AddColumn(oldTable->GetColumn(i));
      }
    this->InputArrayTable->Modified();
    }

  double tmp;
  tmp = this->Mins[position1];
  this->Mins[position1] = this->Mins[position2];
  this->Mins[position2] = tmp;

  tmp = this->Maxs[position1];
  this->Maxs[position1] = this->Maxs[position2];
  this->Maxs[position2] = tmp;

  tmp = this->MinOffsets[position1];
  this->MinOffsets[position1] = this->MinOffsets[position2];
  this->MinOffsets[position2] = tmp;

  tmp = this->MaxOffsets[position1];
  this->MaxOffsets[position1] = this->MaxOffsets[position2];
  this->MaxOffsets[position2] = tmp;

  vtkAxisActor2D* axtmp = this->Axes[position1];
  this->Axes[position1] = this->Axes[position2];
  this->Axes[position2] = axtmp;

  vtkStdString tmpStr = this->AxisTitles->GetValue(position1);
  this->AxisTitles->SetValue(position1,this->AxisTitles->GetValue(position2));
  this->AxisTitles->SetValue(position2,tmpStr);

  // make sure everything's sufficiently far apart
  for (int pos=1; pos<this->NumberOfAxes; pos++)
    {
    double diff = fabs(this->Xs[pos]-this->Xs[pos-1]);
    if (diff < this->SwapThreshold)
      {
      this->Xs[pos] += (this->SwapThreshold-diff)+this->SwapThreshold*.1;
      }
    }

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::SetXCoordinateOfPosition(int position, 
                                                                   double xcoord)
{
  if (position < 0 || 
      position >= this->NumberOfAxes)
    {
    return -1;
    }

  this->Xs[position] = xcoord;
  this->Modified();

  
  if (position > 0 && 
      (this->Xs[position] - this->Xs[position-1]) < this->SwapThreshold)
    {
    this->SwapAxisPositions(position,position-1);
    return position-1;
    }
  else if (position < this->NumberOfAxes-1 &&
           (this->Xs[position+1] - this->Xs[position]) < this->SwapThreshold)
    {
    this->SwapAxisPositions(position,position+1);
    return position+1;
    }


  return position;
}

//------------------------------------------------------------------------------
double vtkParallelCoordinatesRepresentation::GetXCoordinateOfPosition(int position)
{
  if (position >= 0 && position < this->NumberOfAxes)
    {
    return this->Xs[position];
    }
  else
    {
    return -1.0;
    }
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::GetXCoordinatesOfPositions(double *coords)
{
  for (int i=0; i<this->NumberOfAxes; i++)
    {
    coords[i] = this->Xs[i];
    }
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::GetPositionNearXCoordinate(double xcoord)
{
  double minDist = VTK_DOUBLE_MAX;
  int nearest = -1;
  for (int i=0; i<this->NumberOfAxes; i++)
    {
    double dist = fabs(this->Xs[i] - xcoord);
    if (dist < minDist)
      {
      nearest = i;
      minDist = dist;
      }
    }

  return nearest;
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::LassoSelect(int brushClass, 
                                                       int brushOperator, 
                                                       vtkPoints* brushPoints)
{
  if (brushPoints->GetNumberOfPoints() < 2)
    return;

  int position=-1, prevPosition=-1;

  vtkSmartPointer<vtkBivariateLinearTableThreshold> threshold = NULL;//vtkSmartPointer<vtkBivariateLinearTableThreshold>::New();
  vtkSmartPointer<vtkIdTypeArray> allIds = vtkSmartPointer<vtkIdTypeArray>::New();

  // for every point in the brush, compute a line in XY space.  A point in XY space satifies
  // the threshold if it is contained WITHIN all such lines.
  vtkSmartPointer<vtkPoints> posPoints = vtkSmartPointer<vtkPoints>::New();
  for (int i=0; i<brushPoints->GetNumberOfPoints()-1; i++)
    {
    double *p = brushPoints->GetPoint(i);
    position = this->ComputePointPosition(p);

    // if we have a valid position
    if (position >= 0 && position < this->NumberOfAxes)
      {
      // position has changed, that means we need to create a new threshold object.
      if (prevPosition != position && i > 0)
        {
        this->LassoSelectInternal(posPoints,allIds);
        posPoints->Initialize();
        }

        posPoints->InsertNextPoint(p);
      }
    prevPosition = position;
    }

  if (posPoints->GetNumberOfPoints() > 0)
    {
    this->LassoSelectInternal(posPoints,allIds);
    }

  this->FunctionTextMapper->SetInput("No function selected.");
  this->FunctionTextActor->VisibilityOff();
  this->SelectRows(brushClass,brushOperator,allIds);
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::LassoSelectInternal(vtkPoints* brushPoints,
                                                               vtkIdTypeArray* outIds)
{
  if (brushPoints->GetNumberOfPoints() <= 0)
    return;

  double *p = brushPoints->GetPoint(0);
  int position = this->ComputePointPosition(p);

  if (position < 0 || position >= this->NumberOfAxes)
    return;

  double leftAxisRange[2],rightAxisRange[2];
  leftAxisRange[0] = leftAxisRange[1] = rightAxisRange[0] = rightAxisRange[1] = 0;
  this->GetRangeAtPosition(position,leftAxisRange);
  this->GetRangeAtPosition(position+1,rightAxisRange);

  double dLeft = leftAxisRange[1] - leftAxisRange[0];
  double dRight = rightAxisRange[1] - rightAxisRange[0];
  double dy = this->YMax - this->YMin;

  this->LinearThreshold->Initialize();
  this->LinearThreshold->SetLinearThresholdTypeToBetween();
  this->LinearThreshold->SetDistanceThreshold(this->AngleBrushThreshold);
  this->LinearThreshold->UseNormalizedDistanceOn();
  this->LinearThreshold->SetColumnRanges(dLeft,dRight);
  this->LinearThreshold->AddColumnToThreshold(position,0);
  this->LinearThreshold->AddColumnToThreshold(position+1,0);

  // add a line equation for each brush point
  for (int i=0; i<brushPoints->GetNumberOfPoints(); i++)
    {
    p = brushPoints->GetPoint(i);

    // normalize p into [0,1]x[0,1]
    double pn[2] = {(p[0]-this->Xs[position]) / (this->Xs[position+1]-this->Xs[position]),
                    (p[1]-this->YMin) / dy};
      
    // now compute actual data values for two PC lines passing through pn, 
    // starting from the endpoints of the left axis
    double q[2] = { leftAxisRange[0],
                    rightAxisRange[0] + pn[1]/pn[0]*dRight };

    double r[2] = { leftAxisRange[1],
                    rightAxisRange[0] + (1.0 + (pn[1]-1.0)/pn[0])*dRight };

    this->LinearThreshold->AddLineEquation(q,r);
    }

  this->LinearThreshold->Update();
  vtkIdTypeArray* ids = this->LinearThreshold->GetSelectedRowIds();
  for (int i=0; i<ids->GetNumberOfTuples(); i++)
    {
    outIds->InsertNextTuple(i,ids);
    }
}
//------------------------------------------------------------------------------
// All lines that have the same slope in PC space represent a set of points 
// that define a line in XY space.  PC lines that have similar slope are all
// near the same XY line.  
void vtkParallelCoordinatesRepresentation::AngleSelect(int brushClass, 
                                                       int brushOperator, 
                                                       double* p1, 
                                                       double* p2)
{
  int position = this->ComputeLinePosition(p1,p2);

  if (position >= 0 && position < this->NumberOfAxes)
    {
    // convert the points into data values
    double leftAxisRange[2],rightAxisRange[2];
    leftAxisRange[0] = leftAxisRange[1] = rightAxisRange[0] = rightAxisRange[1] = 0;
    this->GetRangeAtPosition(position,leftAxisRange);
    this->GetRangeAtPosition(position+1,rightAxisRange);

    double dLeft = leftAxisRange[1] - leftAxisRange[0];
    double dRight = rightAxisRange[1] - rightAxisRange[0];
    double dy = this->YMax-this->YMin;

    // compute point-slope line definition in XY space
    double xy[2] = {
      (p1[1] - this->YMin)/dy*dLeft + leftAxisRange[0],
      (p2[1] - this->YMin)/dy*dRight + rightAxisRange[0]};

    // oddly enough, the slope of the XY line is completely
    // independent of the line drawn in PC space.
    double slope = dRight / dLeft;

    this->LinearThreshold->Initialize();
    this->LinearThreshold->SetLinearThresholdTypeToNear();
    this->LinearThreshold->SetDistanceThreshold(this->AngleBrushThreshold);
    this->LinearThreshold->UseNormalizedDistanceOn();
    this->LinearThreshold->SetColumnRanges(dLeft,dRight);
    this->LinearThreshold->AddLineEquation(xy,slope);
    this->LinearThreshold->AddColumnToThreshold(position,0);
    this->LinearThreshold->AddColumnToThreshold(position+1,0);
    this->LinearThreshold->Update();

    char buf[256];
    double b = xy[1] - slope*xy[0];
    sprintf(buf,"%s = %f * %s %s %f\n",
            this->AxisTitles->GetValue(position+1).c_str(),
            slope,
            this->AxisTitles->GetValue(position).c_str(),
            (b < 0) ? "-" : "+",
            fabs(b));
            
    this->FunctionTextMapper->SetInput(buf);
    this->FunctionTextActor->VisibilityOn();

    this->SelectRows(brushClass,brushOperator,this->LinearThreshold->GetSelectedRowIds());
    }
}

//------------------------------------------------------------------------------
// Line that match a linear function can be found by defining that linear
// function and selecting all points that are near the line.  the linear
// function can be specified by two XY points, equivalent to two PC lines.
void vtkParallelCoordinatesRepresentation::FunctionSelect(int brushClass, 
                                                          int brushOperator, 
                                                          double* p1, 
                                                          double* p2, 
                                                          double* q1, 
                                                          double* q2)
{
  int position = this->ComputeLinePosition(p1,p2);
  int position2 = this->ComputeLinePosition(q1,q2);

  if (position != position2)
    return;

  if (position >= 0 && position < this->NumberOfAxes)
    {
    // convert the points into data values
    double leftAxisRange[2],rightAxisRange[2];
    leftAxisRange[0] = leftAxisRange[1] = rightAxisRange[0] = rightAxisRange[1] = 0;
    this->GetRangeAtPosition(position,leftAxisRange);
    this->GetRangeAtPosition(position+1,rightAxisRange);

    double dLeft = leftAxisRange[1] - leftAxisRange[0];
    double dRight = rightAxisRange[1] - rightAxisRange[0];
    double dy = this->YMax-this->YMin;

    double xy1[2] = {
      (p1[1] - this->YMin)/dy*dLeft + leftAxisRange[0],
      (p2[1] - this->YMin)/dy*dRight + rightAxisRange[0]};

    double xy2[2] = {
      (q1[1] - this->YMin)/dy*dLeft + leftAxisRange[0],
      (q2[1] - this->YMin)/dy*dRight + rightAxisRange[0]};

    this->LinearThreshold->Initialize();
    this->LinearThreshold->SetLinearThresholdTypeToNear();
    this->LinearThreshold->SetDistanceThreshold(this->AngleBrushThreshold);
    this->LinearThreshold->UseNormalizedDistanceOn();
    this->LinearThreshold->SetColumnRanges(dLeft,dRight);
    this->LinearThreshold->AddLineEquation(xy1,xy2);
    this->LinearThreshold->AddColumnToThreshold(position,0);
    this->LinearThreshold->AddColumnToThreshold(position+1,0);
    this->LinearThreshold->Update();

    double m = (xy1[1]-xy2[1])/(xy1[0]-xy2[0]);
    double b = xy1[1] - (xy1[1]-xy2[1])/(xy1[0]-xy2[0])*xy1[0];
    char buf[256];
    sprintf(buf,"%s = %f * %s %s %f\n",
            this->AxisTitles->GetValue(position+1).c_str(),
            m,
            this->AxisTitles->GetValue(position).c_str(),
            (b < 0) ? "-" : "+",
            fabs(b));
            
    this->FunctionTextMapper->SetInput(buf);
    this->FunctionTextActor->VisibilityOn();


    this->SelectRows(brushClass,brushOperator,this->LinearThreshold->GetSelectedRowIds());
    }
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::RangeSelect(int vtkNotUsed(brushClass), 
                                                       int vtkNotUsed(brushOperator), 
                                                       double* vtkNotUsed(p1), 
                                                       double* vtkNotUsed(p2))
{
  // stubbed out for now
}

void vtkParallelCoordinatesRepresentation::UpdateSelectionActors()
{
  vtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();
  int numNodes = selection->GetNumberOfNodes();

  for (int i=0; i<numNodes; i++)
    {
    while (i >= (int)this->I->SelectionData.size())
      {
      // initialize everything for drawing the selection
      vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
      vtkSmartPointer<vtkActor2D> actor = vtkSmartPointer<vtkActor2D>::New();
      vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
      mapper.TakeReference(this->InitializePlotMapper(polyData,actor));

      this->I->SelectionData.push_back(polyData);
      this->I->SelectionMappers.push_back(mapper);
      this->I->SelectionActors.push_back(actor);

      this->AddPropOnNextRender(actor);
      }
    }

  for (int i=numNodes; i<(int)this->I->SelectionData.size(); i++)
    {
    this->RemovePropOnNextRender(this->I->SelectionActors[i]);
    this->I->SelectionData.pop_back();
    this->I->SelectionMappers.pop_back();
    this->I->SelectionActors.pop_back();
    }

  this->BuildInverseSelection();
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::ComputePointPosition(double *p)
{
  if (p[0] < this->Xs[0])
    return -1;

  for (int i=1; i<this->NumberOfAxes; i++)
    {
    if (p[0] < this->Xs[i])
      {
      return i-1;
      }
    }
  return -1;
}

//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::ComputeLinePosition(double* p1, 
                                                              double* p2)
{
  double eps = .0001;
  for (int i=0; i<this->NumberOfAxes-1; i++)
    {
    if (p1[0] < this->Xs[i]+eps &&
        p2[0] > this->Xs[i+1]-eps)
      {
      return i;
      }
    }
  return -1;
}

//------------------------------------------------------------------------------
vtkSelection* vtkParallelCoordinatesRepresentation::ConvertSelection(vtkView* vtkNotUsed(view), 
                                                                     vtkSelection* selection)
{
  return selection;
}

//------------------------------------------------------------------------------
// does the actual selection, including joining the new selection with the
// old selection of the same class with various set operations.
void vtkParallelCoordinatesRepresentation::SelectRows(vtkIdType brushClass, 
                                                      vtkIdType brushOperator, 
                                                      vtkIdTypeArray* newSelectedIds)
{
  // keep making new selection nodes (and initializing them) until a node for
  // brushClass actually exists.
  vtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();
  vtkSelectionNode* node = selection->GetNode(brushClass);
  while (!node)
    {
    vtkSmartPointer<vtkSelectionNode> newnode = vtkSmartPointer<vtkSelectionNode>::New();
    newnode->GetProperties()->Set(
      vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::PEDIGREEIDS);
    newnode->GetProperties()->Set(
      vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::ROW);
    selection->AddNode(newnode);

    // initialize the selection data
    vtkSmartPointer<vtkIdTypeArray> selectedIds = vtkSmartPointer<vtkIdTypeArray>::New();
    newnode->SetSelectionList(selectedIds);

    // initialize everything for drawing the selection
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkActor2D> actor = vtkSmartPointer<vtkActor2D>::New();
    vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    mapper.TakeReference(this->InitializePlotMapper(polyData,actor));

    this->I->SelectionData.push_back(polyData);
    this->I->SelectionMappers.push_back(mapper);
    this->I->SelectionActors.push_back(actor);

    this->AddPropOnNextRender(actor);

    node = selection->GetNode(brushClass);
    }

  vtkIdTypeArray* oldSelectedIds = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
  
  // no selection list yet? that shouldn't be possible...it was allocated above
  if (!oldSelectedIds)
    {
    return;
    }

  vtkSmartPointer<vtkIdTypeArray> outSelectedIds = vtkSmartPointer<vtkIdTypeArray>::New();

  int numOldIds = oldSelectedIds->GetNumberOfTuples();
  int numNewIds = newSelectedIds->GetNumberOfTuples();
  switch(brushOperator)
    {
    case vtkParallelCoordinatesView::VTK_BRUSHOPERATOR_ADD:
      // add all of the old ones, clobbering the class if it's in the new array
      for (int i=0; i<numOldIds; i++)
        {
        outSelectedIds->InsertNextValue(oldSelectedIds->GetValue(i));
        }

      // add all of the new ones, as long as they aren't in the old array
      for (int i=0; i<numNewIds; i++)
        {
        if (oldSelectedIds->LookupValue(newSelectedIds->GetValue(i)) == -1)
          {
          outSelectedIds->InsertNextValue(newSelectedIds->GetValue(i));
          }
        }
      break;

    case vtkParallelCoordinatesView::VTK_BRUSHOPERATOR_SUBTRACT:
      // if an old id is in the new array and it has the current brush class, skip it
      for (int i=0; i<numOldIds; i++)
        {
        if (newSelectedIds->LookupValue(oldSelectedIds->GetValue(i)) == -1)
          {
          outSelectedIds->InsertNextValue(oldSelectedIds->GetValue(i));
          }
        }
      break;

    case vtkParallelCoordinatesView::VTK_BRUSHOPERATOR_INTERSECT:
      // if an old id isn't in the new array and has the current brush class, skip it
      for (int i=0; i<numOldIds; i++)
        {
        if (newSelectedIds->LookupValue(oldSelectedIds->GetValue(i)) >= 0)
          {
          outSelectedIds->InsertNextValue(oldSelectedIds->GetValue(i));
          }
        }

      break;

    case vtkParallelCoordinatesView::VTK_BRUSHOPERATOR_REPLACE:
      // add all of the new ones, 
      for (int i=0; i<numNewIds; i++)
        {
        outSelectedIds->InsertNextValue(newSelectedIds->GetValue(i));
        }
      break;
    }

  vtkSortDataArray::Sort(outSelectedIds);
  node->SetSelectionList(outSelectedIds);

  this->BuildInverseSelection();

  this->Modified();
  this->UpdateSelection(selection);
}

//------------------------------------------------------------------------------
//
void vtkParallelCoordinatesRepresentation::BuildInverseSelection()
{
  vtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();

  this->InverseSelection->RemoveAllNodes();


  int numNodes = selection->GetNumberOfNodes();
  if (numNodes <= 0)
    {
    return;
    }

  vtkSmartPointer<vtkIdTypeArray> unselected = vtkSmartPointer<vtkIdTypeArray>::New();
  std::vector<int> idxs(numNodes,0);
 
  for (int i=0; i<this->NumberOfSamples; i++)
    {
    bool found = false;
    for (int j=0; j<numNodes; j++)
      {

      vtkIdTypeArray* a = vtkIdTypeArray::SafeDownCast(selection->GetNode(j)->GetSelectionList());
      if (!a || idxs[j] >= a->GetNumberOfTuples())
        {
        continue;
        }

      int numRows = a->GetNumberOfTuples();
      while (idxs[j] < numRows &&
             a->GetValue(idxs[j]) < i)
        {
        idxs[j]++;
        }

      if (idxs[j] < numRows && 
          a->GetValue(idxs[j]) == i)
        {
        found=true;
        break;
        }
      }
    
    if (!found)
      {
      unselected->InsertNextValue(i);
      }
    }
 

  vtkSmartPointer<vtkSelectionNode> totalSelection = vtkSmartPointer<vtkSelectionNode>::New();
  totalSelection->SetSelectionList(unselected);

  if (unselected->GetNumberOfTuples())
    this->InverseSelection->AddNode(totalSelection);
}

//------------------------------------------------------------------------------
// get the value range of an axis
int vtkParallelCoordinatesRepresentation::GetRangeAtPosition(int position, 
                                                             double range[2])
{
  if (position < 0 || position >= this->NumberOfAxes)
    {
    return -1;
    }

//  int axis = this->AxisOrder[position];
  range[0] = this->Mins[position]+this->MinOffsets[position];
  range[1] = this->Maxs[position]+this->MaxOffsets[position];
  
  return 1;
}

//------------------------------------------------------------------------------
// set the value range of an axis
int vtkParallelCoordinatesRepresentation::SetRangeAtPosition(int position, 
                                                             double range[2])
{
  if (position < 0 || position >= this->NumberOfAxes)
    {
    return -1;
    }

//  int axis = this->AxisOrder[position];
  this->MinOffsets[position] = range[0]-this->Mins[position];
  this->MaxOffsets[position] = range[1]-this->Maxs[position];
  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
void vtkParallelCoordinatesRepresentation::ResetAxes()
{
  this->YMin = .1;
  this->YMax = .9;

  for (int i=0; i<this->NumberOfAxes; i++)
    this->RemovePropOnNextRender(this->Axes[i]);

  this->ReallocateInternals();

  this->GetInput()->Modified();

  this->Modified();
  this->Update();
}

//------------------------------------------------------------------------------
// get position and size of the entire plot
int vtkParallelCoordinatesRepresentation::GetPositionAndSize(double* position, 
                                                             double* size)
{
  if (!this->Xs)
    return 0;

  position[0] = this->Xs[0];
  position[1] = this->YMin;

  size[0] = this->Xs[this->NumberOfAxes-1] - this->Xs[0];;
  size[1] = this->YMax - this->YMin;
  return 1;
}

//------------------------------------------------------------------------------
// set position and size of the entire plot
int vtkParallelCoordinatesRepresentation::SetPositionAndSize(double* position, 
                                                             double* size)
{
  // rescale the Xs so that they fit into the range prescribed by position and size
  double oldPos[2],oldSize[2];
  oldPos[0] = oldPos[1] = oldSize[0] = oldSize[1] = 0.;
  this->GetPositionAndSize(oldPos,oldSize);

  for (int i=0; i<this->NumberOfAxes; i++)
    {
    this->Xs[i] = position[0] + size[0]*(this->Xs[i]-oldPos[0])/oldSize[0];
    }

  this->YMin = position[1];
  this->YMax = position[1] + size[1];

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
vtkPolyDataMapper2D* vtkParallelCoordinatesRepresentation::InitializePlotMapper(vtkPolyData* input, vtkActor2D* actor, bool vtkNotUsed(forceStandard))
{
  vtkPolyDataMapper2D* mapper = vtkPolyDataMapper2D::New();

  // this tells all the mappers to use the normalized viewport coordinate system
  vtkSmartPointer<vtkCoordinate> dummyCoord = vtkSmartPointer<vtkCoordinate>::New();
  dummyCoord->SetCoordinateSystemToNormalizedViewport();

  mapper->SetInput(input);
  mapper->SetTransformCoordinate(dummyCoord);
  mapper->ScalarVisibilityOff();
  actor->SetMapper(mapper);  
  
  return mapper;
}
//------------------------------------------------------------------------------
vtkPolyDataMapper2D* vtkParallelCoordinatesRepresentation::GetSelectionMapper(int idx)
{
  if (idx >= 0 && idx < (int)this->I->SelectionMappers.size())
    {
    return this->I->SelectionMappers[idx];
    }
  else
    {
    return NULL;
    }
}
//------------------------------------------------------------------------------
int vtkParallelCoordinatesRepresentation::GetNumberOfSelections()
{
  return (int)this->I->SelectionActors.size();
}
