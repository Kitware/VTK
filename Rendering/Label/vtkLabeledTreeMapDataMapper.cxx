/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledTreeMapDataMapper.cxx

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

#include "vtkActor2D.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkLabeledTreeMapDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkTreeDFSIterator.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkLabeledTreeMapDataMapper);

vtkLabeledTreeMapDataMapper::vtkLabeledTreeMapDataMapper()
  : CurrentViewPort(0), FontHeights(0), FontWidths(0), MaxFontLevel(0),
    MaxTreeLevels(100), ClipTextMode(0), ChildMotion(0),
    StartLevel(0), EndLevel(-1), DynamicLevel(0)
{
  this->BoxTrans[0][0] = this->BoxTrans[1][0] = 0.0;
  this->BoxTrans[0][1] = this->BoxTrans[1][1] = 1.0;
  this->WindowLimits[0][0] = this->WindowLimits[1][0] = 0.0;
  this->WindowLimits[0][1] = this->WindowLimits[1][1] = 1.0;
  this->VCoord = vtkCoordinate::New();
  this->VertexList = vtkIdList::New();
  this->VertexList->SetNumberOfIds(this->NumberOfLabelsAllocated);
  this->TextPoints = vtkPoints::New();
  this->TextPoints->Allocate(this->NumberOfLabelsAllocated);
  this->VerticalLabelProperty = vtkTextProperty::New();
  this->VerticalLabelProperty->SetFontSize(12);
  this->VerticalLabelProperty->SetBold(1);
  this->VerticalLabelProperty->SetItalic(1);
  this->VerticalLabelProperty->SetShadow(1);
  this->VerticalLabelProperty->SetFontFamilyToArial();
  this->VerticalLabelProperty->SetJustificationToCentered();
  this->GetLabelTextProperty()->SetJustificationToCentered();
  this->VerticalLabelProperty->SetVerticalJustificationToCentered();
  this->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->VerticalLabelProperty->SetOrientation(90.0);
  this->VerticalLabelProperty->SetColor(1, 1, 1);
  this->GetLabelTextProperty()->SetColor(1, 1, 1);
  this->GetLabelTextProperty()->SetFontSize(12);
  this->SetFontSizeRange(24, 10);
  this->ChildrenCount = new int[this->MaxTreeLevels+1];
  this->LabelMasks = new float[this->MaxTreeLevels+1][4];
  this->SetRectanglesArrayName("area");
  this->SetLabelFormat("%s");

  // Take control of the TextMappers array.
  // The superclass just created new TextMapper instances
  // up to the currently allocated amount (default 50).
  // Instead, we will store NULL values until we need them.
  // This class will manage the maintenance and deletion of
  // this array.
  for (int i = 0; i < this->NumberOfLabelsAllocated; i++)
  {
    this->TextMappers[i]->Delete();
    this->TextMappers[i] = NULL;
  }
}

vtkLabeledTreeMapDataMapper::~vtkLabeledTreeMapDataMapper()
{
  this->VCoord->Delete();
  this->TextPoints->Delete();
  this->VertexList->Delete();
  this->VerticalLabelProperty->Delete();
  int i;
  for (i = 0; i <= this->MaxFontLevel; i++)
  {
    delete [] this->FontWidths[i];
    this->HLabelProperties[i]->Delete();
  }
  delete [] this->FontWidths;
  delete [] this->FontHeights;
  delete [] this->HLabelProperties;
  delete [] this->ChildrenCount;
  delete [] this->LabelMasks;
  if (this->TextMappers)
  {
    for (i=0; i < this->NumberOfLabelsAllocated; i++)
    {
      if (this->TextMappers[i])
      {
        this->TextMappers[i]->Delete();
      }
    }
    delete [] this->TextMappers;
    this->TextMappers = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkLabeledTreeMapDataMapper::SetRectanglesArrayName(const char* name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this mapper.
void vtkLabeledTreeMapDataMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->TextMappers != NULL )
  {
    for (int i=0; i < this->NumberOfLabelsAllocated; i++)
    {
      if (this->TextMappers[i])
      {
        this->TextMappers[i]->ReleaseGraphicsResources(win);
      }
    }
  }
}

vtkTree *vtkLabeledTreeMapDataMapper::GetInputTree()
{
  return vtkTree::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

void vtkLabeledTreeMapDataMapper::UpdateFontSizes()
{
  char test[2];
  test[1] = '\0';
  int tSize[2], i;
  // Make sure that there is a text mapper at index 0
  if (!this->TextMappers[0])
  {
    this->TextMappers[0] = vtkTextMapper::New();
    this->NumberOfLabels = 1;
  }

  for (i = 0; i <= this->MaxFontLevel; i++)
  {
    this->TextMappers[0]->SetTextProperty(this->HLabelProperties[i]);
    this->FontHeights[i] = 0;
    for (test[0] = 32; test[0] < 127; test[0]++)
    {
      this->TextMappers[0]->SetInput(test);
      this->TextMappers[0]->GetSize(this->CurrentViewPort, tSize);
      this->FontWidths[i][test[0]-32] = tSize[0];
      if (this->FontHeights[i] < tSize[1])
      {
        this->FontHeights[i] = tSize[1];
      }
    }
  }
}

void vtkLabeledTreeMapDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ClipTextMode: " << this->ClipTextMode << endl;
  os << indent << "ChildMotion: " << this->ChildMotion << endl;
  os << indent << "DynamicLevel: " << this->DynamicLevel << endl;
  int i;
  os << "Font Sizes: ";
  for (i = 0; i <= this->MaxFontLevel; i++)
  {
    os << this->HLabelProperties[i]->GetFontSize() << " ";
  }
  os << endl;

  os << indent << "Level Range: [" << this->StartLevel
     << ", " << this->EndLevel << "]" << endl;
}

int vtkLabeledTreeMapDataMapper::UpdateWindowInfo(vtkViewport *viewport)
{
  // Assumes that the view is not rotated w/r to the tree map!
  int *dc;
  float p[2][2];
  this->VCoord->SetViewport(viewport);
  // Transform 0,0 and 1,1 into screen coordinates
  this->VCoord->SetValue(0.0, 0.0, 0.0);
  dc = VCoord->GetComputedDisplayValue(0);
  p[0][0] = dc[0];
  p[0][1] = dc[1];
  this->VCoord->SetValue(1.0, 1.0, 0.0);
  dc = this->VCoord->GetComputedDisplayValue(0);
  p[1][0] = dc[0];
  p[1][1] = dc[1];

  // The translation is defined by 0,0's new position
  this->BoxTrans[0][0] = p[0][0];
  this->BoxTrans[1][0] = p[0][1];

  // The scales are defined as p[1] - p[0]
  this->BoxTrans[0][1] = p[1][0] - p[0][0];
  this->BoxTrans[1][1] = p[1][1] - p[0][1];

  // Get the window extents
  vtkWindow * win = viewport->GetVTKWindow();
  int *winPos = win->GetPosition();
  this->WindowLimits[0][0] = winPos[0];
  this->WindowLimits[1][0] = winPos[1];
  int *winSize = win->GetSize();
  this->WindowLimits[0][1] = this->WindowLimits[0][0] + winSize[0];
  this->WindowLimits[1][1] = this->WindowLimits[1][0] + winSize[1];

  // We are done with the coordinate, so release the viewport
  this->VCoord->SetViewport(NULL);

  // Ideally we can compare the new scales with the original and
  // see if the change has exceeded some threshold - in that case
  // we could return 0
  return 1;
}

void vtkLabeledTreeMapDataMapper::GetVertexLabel(vtkIdType vertex,
                                               vtkDataArray *numericData,
                                               vtkStringArray *stringData,
                                               int activeComp, int numComp,
                                               char *string,
                                               size_t stringSize)
{
  char format[1024];
  double val;
  int j;
  if ( numericData )
  {
    if ( numComp == 1 )
    {
      if (numericData->GetDataType() == VTK_CHAR)
      {
        if (strcmp(this->LabelFormat,"%c") != 0)
        {
          vtkErrorMacro(<<"Label format must be %c to use with char");
          string[0] = '\0';
          return;
        }
        snprintf(string, stringSize, this->LabelFormat,
                 static_cast<char>(numericData->GetComponent(vertex, activeComp)));
      }
      else
      {
        snprintf(string, stringSize, this->LabelFormat,
                 numericData->GetComponent(vertex, activeComp));
      }
    }
    else
    {
      strcpy(format, "("); strcat(format, this->LabelFormat);
      for (j=0; j<(numComp-1); j++)
      {
        snprintf(string, stringSize, format, numericData->GetComponent(vertex, j));
        strcpy(format,string); strcat(format,", ");
        strcat(format, this->LabelFormat);
      }
      snprintf(string, stringSize, format, numericData->GetComponent(vertex, numComp-1));
      strcat(string, ")");
    }
  }
  else if (stringData)// rendering string data
  {
    if (strcmp(this->LabelFormat,"%s") != 0)
    {
      vtkErrorMacro(<<"Label format must be %s to use with strings");
      string[0] = '\0';
      return;
    }
    snprintf(string, stringSize, this->LabelFormat,
             stringData->GetValue(vertex).c_str());
  }
  else // Use the vertex id
  {
    val = static_cast<double>(vertex);
    snprintf(string, stringSize, this->LabelFormat, val);
  }
}

//----------------------------------------------------------------------------
void vtkLabeledTreeMapDataMapper::RenderOverlay(vtkViewport *viewport,
                                                vtkActor2D *actor)
{
  int i;
  double x[3];
  for (i=0; i<this->NumberOfLabels; i++)
  {
    this->TextPoints->GetPoint(i,x);
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(x);
    this->TextMappers[i]->RenderOverlay(viewport, actor);
  }
}

//----------------------------------------------------------------------------
void vtkLabeledTreeMapDataMapper::RenderOpaqueGeometry(vtkViewport *viewport,
                                                       vtkActor2D *actor)
{
  int i, numComp = 0, pointIdLabels, activeComp = 0;
  int numVertices;
  double x[3];
  vtkAbstractArray *abstractData;
  vtkDataArray *numericData, *tempData;
  vtkStringArray *stringData;
  vtkTree *input=this->GetInputTree();
  if ( ! input )
  {
    vtkErrorMacro(<<"Need input tree to render labels (2)");
    return;
  }

  vtkTextProperty *tprop = this->GetLabelTextProperty();
  if (!tprop)
  {
    vtkErrorMacro(<<"Need text property to render labels");
    return;
  }

  this->GetInputAlgorithm()->Update();

  // Input might have changed
  input = this->GetInputTree();
  vtkDataSetAttributes *pd = input->GetVertexData();
  // Get the treeMap info
  tempData = this->GetInputArrayToProcess(0, input);
  if (!tempData)
  {
    vtkErrorMacro(<< "Input Tree does not have box information.");
    return;
  }
  vtkFloatArray *boxInfo = vtkArrayDownCast<vtkFloatArray>(tempData);

  // Check to see whether we have to rebuild everything
  if ( this->UpdateWindowInfo(viewport) ||
       this->CurrentViewPort != viewport ||
       this->GetMTime() > this->BuildTime ||
       input->GetMTime() > this->BuildTime ||
       tprop->GetMTime() > this->BuildTime)
  {
    vtkDebugMacro(<<"Rebuilding labels");

    // See if we have to recalculate fonts sizes
    if (this->CurrentViewPort != viewport)
    {
      this->CurrentViewPort = viewport;
      this->UpdateFontSizes();
    }

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
      numericData = vtkArrayDownCast<vtkDataArray>(abstractData);
      stringData = vtkArrayDownCast<vtkStringArray>(abstractData);
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

    // Make sure that the array of TextMappers can accommodate
    // the number of vertices in the tree - Note that we may
    // not create the actual mappers

    numVertices = input->GetNumberOfVertices();
    if ( numVertices > this->NumberOfLabelsAllocated )
    {
      // delete old stuff
      for (i=0; i < this->NumberOfLabelsAllocated; i++)
      {
          if (this->TextMappers[i])
          {
          this->TextMappers[i]->Delete();
          }
      }
      delete [] this->TextMappers;

      this->NumberOfLabelsAllocated = numVertices;
      this->TextMappers = new vtkTextMapper * [numVertices];
      this->VertexList->SetNumberOfIds(numVertices);
      this->TextPoints->Allocate(numVertices);
      for (i = 0; i < numVertices; i++)
      {
        this->TextMappers[i] = 0;
      }
    }

    this->LabelTree(input, boxInfo, numericData, stringData,
                    activeComp, numComp);
  }


  for (i=0; i<this->NumberOfLabels; i++)
  {
    this->TextPoints->GetPoint(i,x);
    actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    actor->GetPositionCoordinate()->SetValue(x);
    this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
  }
}

void vtkLabeledTreeMapDataMapper::LabelTree(vtkTree *tree,
                                            vtkFloatArray *boxInfo,
                                            vtkDataArray *numericData,
                                            vtkStringArray *stringData,
                                            int activeComp,
                                            int numComps)
{

  float blimits[4], blimitsDC[4], textPosWC[3];
  char string[1024];
  int results;
  vtkTextProperty *tprop = NULL;
  vtkIdType vertex, level, root = tree->GetRoot();
  if (root < 0)
  {
    vtkErrorMacro(<< "Input Tree does not have a root.");
    return;
  }

  this->NumberOfLabels = 0;
  vtkTreeDFSIterator* dfs = vtkTreeDFSIterator::New();
  dfs->SetTree(tree);
  while (dfs->HasNext())
  {
    // Are we suppose to display this vertex?
    vertex = dfs->Next();
    level = tree->GetLevel(vertex);

    if (level >= this->StartLevel && (this->EndLevel == -1 || level <= this->EndLevel))
    {
      boxInfo->GetTypedTuple(vertex, blimits); // Get the extents of the vertex
      if (this->ConvertToDC(blimits, blimitsDC))
      {
        continue;
      }

      this->GetVertexLabel(vertex, numericData, stringData, activeComp, numComps,
                           string, sizeof(string));
      results = this->AnalyseLabel(string, level, blimitsDC, textPosWC, &tprop);
      if (results == 1)
      {
        // Label doesn't fit in its box - don't both processing children
        continue;
      }
    }
    else
    {
      // results == 2 from AnalyseLabel means that the label can't be
      // displayed due to reasons other than size - well in this
      // case we can't display due to the level limit we
      // also have to deactive the maks for this level
      this->LabelMasks[level][0] = -1.0;
      results = 2;
    }

    if (!results)
    {
      if (!this->TextMappers[this->NumberOfLabels])
      {
        this->TextMappers[this->NumberOfLabels] = vtkTextMapper::New();
      }
      this->TextMappers[this->NumberOfLabels]->SetInput(string);
      this->TextMappers[this->NumberOfLabels]->
        SetTextProperty(tprop);
      this->TextPoints->SetPoint(this->NumberOfLabels, textPosWC);
      ++(this->NumberOfLabels);
    }

    // Is this level the deepest we can display?
    if (this->EndLevel == level)
    {
      continue;
    }
  }
  dfs->Delete();
  this->BuildTime.Modified();
}

int vtkLabeledTreeMapDataMapper::GetStringSize(char *string, int level)
{

  if (level > this->MaxFontLevel)
  {
    level = this->MaxFontLevel;
  }
  int size = 0, i;
  for(i = 0; string[i] != '\0'; i++)
  {
    if (string[i] < 32)
    {
      continue;
    }

    if (string[i] > 126)
    {
      continue;
    }

    size += this->FontWidths[level][string[i]-32];
  }
  return size;
}

int vtkLabeledTreeMapDataMapper::ConvertToDC(float *binfo, float *newBinfo)
{

  newBinfo[0] = this->BoxTrans[0][0] + (binfo[0]*this->BoxTrans[0][1]);
  newBinfo[1] = this->BoxTrans[0][0] + (binfo[1]*this->BoxTrans[0][1]);
  newBinfo[2] = this->BoxTrans[1][0] + (binfo[2]*this->BoxTrans[1][1]);
  newBinfo[3] = this->BoxTrans[1][0] + (binfo[3]*this->BoxTrans[1][1]);

  // See the comments in AnalyseLabel for why we're comparing against
  // these numbers.
  double windowWidth = this->WindowLimits[0][1] - this->WindowLimits[0][0];
  double windowHeight = this->WindowLimits[1][1] - this->WindowLimits[1][0];

  if (newBinfo[0] >= windowWidth)
  {
    return 1;
  }

  if (newBinfo[1] <= 0)
  {
    return 1;
  }

  if (newBinfo[2] >= windowHeight)
  {
    return 1;
  }

  if (newBinfo[3] <= 0)
  {
    return 1;
  }

  if (!this->ClipTextMode)
  {
    return 0;
  }

  if (newBinfo[0] < 0)
  {
    newBinfo[0] = 0;
  }

  if (newBinfo[1] > windowWidth)
  {
    newBinfo[1] = windowWidth;
  }

  if (newBinfo[2] < 0)
  {
    newBinfo[2] = 0;
  }

  if (newBinfo[3] > windowHeight)
  {
    newBinfo[3] = windowHeight;
  }

  return 0;
}

void vtkLabeledTreeMapDataMapper::SetFontSizeRange(int maxSize, int minSize,
                                                   int delta)
{
  int nLevels = (maxSize - minSize) / delta;
  int i, s;
  if (nLevels < 0)
  {
    vtkErrorMacro(<<"maxSize is smaller than minSize");
    return;
  }

  if ((maxSize - (nLevels * delta)) > minSize)
  {
    ++nLevels;
  }

  if (this->MaxFontLevel != nLevels)
  {
    if (this->MaxFontLevel)
    {
      delete [] this->FontHeights;
      for (i = 0; i <= this->MaxFontLevel; i++)
      {
        delete [] this->FontWidths[i];
        this->HLabelProperties[i]->Delete();
      }
      delete [] this->FontWidths;
      delete [] this->HLabelProperties;
    }
    this->MaxFontLevel = nLevels;
    this->FontHeights = new int[this->MaxFontLevel+1];
    this->FontWidths = new int *[this->MaxFontLevel+1];
    this->HLabelProperties = new vtkTextProperty *[this->MaxFontLevel+1];
    for(i = 0; i <= this->MaxFontLevel; i++)
    {
      this->FontWidths[i] = new int[95];
      this->HLabelProperties[i] = vtkTextProperty::New();
      this->HLabelProperties[i]->SetFontSize(12);
      this->HLabelProperties[i]->SetBold(1);
      this->HLabelProperties[i]->SetItalic(1);
      this->HLabelProperties[i]->SetShadow(1);
      this->HLabelProperties[i]->SetFontFamilyToArial();
      this->HLabelProperties[i]->SetJustificationToCentered();
      this->HLabelProperties[i]->SetVerticalJustificationToCentered();
      this->HLabelProperties[i]->SetColor(1, 1, 1);
    }
  }
  for(i = 0, s = maxSize; i < this->MaxFontLevel; i++, s -= delta)
  {
    this->HLabelProperties[i]->SetFontSize(s);
  }
  this->HLabelProperties[i]->SetFontSize(minSize);

  this->CurrentViewPort = 0;
}

void vtkLabeledTreeMapDataMapper::GetFontSizeRange(int range[3])
{
  range[0] = this->HLabelProperties[0]->GetFontSize();
  range[1] = this->HLabelProperties[this->MaxFontLevel - 1]->GetFontSize();
  range[2] = (range[0] - range[1]) / (this->MaxFontLevel - 1);
}

int vtkLabeledTreeMapDataMapper::AnalyseLabel(char * string, int level,
                                              float *blimitsDC,
                                              float *textPosWC,
                                              vtkTextProperty **tprop)
{
  float sizes[2], tPosDC[2], delta, flimits[4];
  int fsize, trueLevel, oDir; // oDir is the text orientation
  // Calculate the size of the box in DC
  sizes[0] = blimitsDC[1] - blimitsDC[0];
  sizes[1] = blimitsDC[3] - blimitsDC[2];
  trueLevel = level -  this->StartLevel;
  if (trueLevel < 0)
  {
    vtkErrorMacro(<< "Invalid level.");
    trueLevel = 0;
  }
  trueLevel =
    (trueLevel > this->MaxFontLevel) ? this->MaxFontLevel : trueLevel;
   fsize = this->GetStringSize(string, trueLevel);

  // Horizontal label.
  // (Vertical labels don't work due to issues with vtkTextActor.)
  oDir = 0;
  *tprop = this->HLabelProperties[trueLevel];

  // Is this level dynamic or static?
  if (level >= this->DynamicLevel)
  {

    // See if the text will not even fit in the box
    if (sizes[!oDir] < this->FontHeights[trueLevel])
    {
    // Text will not fit
      return 1;
    }

    if (sizes[oDir] < fsize)
    {
      // Text will not fit
      return 1;
    }
  }
  // Calculate the bounding box of the text
  // Determine where to place the text
  tPosDC[0] = 0.5 *(blimitsDC[0] + blimitsDC[1]);
  tPosDC[1] = 0.5 *(blimitsDC[2] + blimitsDC[3]);
  // Compute mask for this level
  delta = 0.5 * 1.05 * (!oDir?fsize:this->FontHeights[trueLevel]);
  flimits[0] = tPosDC[0] - delta;
  flimits[1] = tPosDC[0] + delta;
  delta = 0.5 * 1.05 * (oDir?fsize:this->FontHeights[trueLevel]);
  flimits[2] = tPosDC[1] - delta;
  flimits[3] = tPosDC[1] + delta;

  // If the label is not to be centered based on the clipped form of the
  // vertex's box see if it has been clipped away
  if (!this->ClipTextMode)
  {
    // The 'flimits' variable contains the bounding box of the label
    // in coordinates relative to (0, 0) in the window -- that is, the
    // lower left corner of the window.  These next few lines test to
    // make sure the label is not entirely outside the window.  The
    // coordinates in WindowLimits are actually in the space of the
    // entire screen, not just this application or its OpenGL window.
    double windowWidth = this->WindowLimits[0][1] - this->WindowLimits[0][0];
    double windowHeight = this->WindowLimits[1][1] - this->WindowLimits[0][0];

    if ((flimits[0] >= windowWidth) ||
        (flimits[1] <= 0) ||
        (flimits[2] >= windowHeight) ||
        (flimits[3] <= 0))
    {
      this->LabelMasks[level][0] = -1.0;
      return 2;
    }
  }

  // Apply Masks
  if (level && (level > this->DynamicLevel))
  {
    if (this->ApplyMasks(level, flimits, blimitsDC))
    {
      // This label does not fit based on the masks
      // Since device coordinate can not be < 0 set
      // the first component of the mask to be -1
      // to indicate the mask is not to be used
      this->LabelMasks[level][0] = -1.0;
      return 2;
    }
  }

  this->LabelMasks[level][0] = flimits[0];
  this->LabelMasks[level][1] = flimits[1];
  this->LabelMasks[level][2] = flimits[2];
  this->LabelMasks[level][3] = flimits[3];
  // since applying the masks can shift the label calculate the
  // new position
  tPosDC[0] = 0.5 *(flimits[0] + flimits[1]);
  tPosDC[1] = 0.5 *(flimits[2] + flimits[3]);

  textPosWC[0] = (tPosDC[0] - this->BoxTrans[0][0])/ this->BoxTrans[0][1];
  textPosWC[1] = (tPosDC[1] - this->BoxTrans[1][0])/ this->BoxTrans[1][1];
  textPosWC[2] = 1.0;
  return 0;
}

int vtkLabeledTreeMapDataMapper::ApplyMasks(int level, float flimits[4],
                                            float blimits[4])
{
// Note that all limits and mask information is in Device Coordinates

  float dy = 0.0;
  int changed = 1, dir = 0, l;
  int status = 1;
  if (!this->ChildMotion)
  {
    // If any of the masks intersect the label don't display it
    for (l = 0; l < level; l++)
    {
      // Skip all masks that refer to labels that are not displayed
      // (i.e. that the first component < 0)
      if (this->LabelMasks[l][0] < 0.0)
      {
        continue;
      }
      if (this->LabelMasks[l][0] > flimits[1])
      {
        continue;
      }
      if (this->LabelMasks[l][1] < flimits[0])
      {
        continue;
      }
      if (this->LabelMasks[l][2] > flimits[3])
      {
        continue;
      }
      if (this->LabelMasks[l][3] < flimits[2])
      {
        continue;
      }
      // If we are here the label intersects the mask
      return 1;
    }
    return 0;
  }

  // dir == 0 indicates we are dropping the label and 1 means we are raising it
  while (changed)
  {
    changed = 0;
    for (l = 0; l < level; l++)
    {
      // Skip all masks that refer to labels that are not displayed
      // (i.e. that the first component < 0)
      // or do not interfere in the y-direction
      // (i.e. the second component < 0)
      // or has been already fixed in the y-direction
      // (.i.e the third component < 0)
      if ((this->LabelMasks[l][0] < 0.0) ||
          (this->LabelMasks[l][1] < 0.0) ||
          (this->LabelMasks[l][2] < 0.0))
      {
        continue;
      }

      // If the label passes either x-check it will never
      // interfere with a horizontal label
      if (this->LabelMasks[l][0] > flimits[1])
      {
        // Set the second component to be -(value + 1) - the reason
        // for the offset is to take of the case the original value is 0
        this->LabelMasks[l][1] = -1.0 * (this->LabelMasks[l][1] + 1.0);
        continue;
      }
      if (this->LabelMasks[l][1] < flimits[0])
      {
        this->LabelMasks[l][1] = -1.0 * (this->LabelMasks[l][1] + 1.0);
        continue;
      }
      if (this->LabelMasks[l][2] > (flimits[3]+dy))
      {
        // If dy < 0 then this condition will always be true and
        // this check can be turned off
        if (dy < 0.0)
        {
          this->LabelMasks[l][2] = -1.0 * (this->LabelMasks[l][2] + 1.0);
        }
        continue;
      }
      if (this->LabelMasks[l][3] < (flimits[2]+dy))
      {
        // If dy > 0 then this condition will always be true and
        // this check can be turned off
        if (dy > 0.0)
        {
          this->LabelMasks[l][2] = -1.0 * (this->LabelMasks[l][2] + 1.0);
        }
        continue;
      }
      // If we are here then the mask does clip the label
      // see which direction we are going it - added a cushion of 5 pixels
      if (dir)
      {
        dy = 5 + this->LabelMasks[l][3] - flimits[2];
      }
      else
      {
        dy = this->LabelMasks[l][2] - (5 + flimits[3]);
      }

      // Indicate that we changed something
      changed = 1;
    }

    // See if anything changed - if it did then repeat the mask loop
    if (changed)
    {
      continue;
    }

    // See if the current label position will not fit on the screen
    if ((blimits[2] > (flimits[2] + dy)) || (blimits[3] < (flimits[3] + dy)))
    {
      // Have we been dropping the label?
      if (!dir)
      {
        // Try raising it - we will need to reset all the masks with
        // negative third components
        dir = 1;
        changed = 1;
        for (l = 0; l < level; l++)
        {
          if (this->LabelMasks[l][2] < 0.0)
          {
            this->LabelMasks[l][2] = - 1.0 * (this->LabelMasks[l][2]+ 1.0);
          }
        }
      }
      // in this case there was no way to display the label
      status = 1;
    }
    else
    {
      // Success
      status = 0;
      flimits[2] += dy;
      flimits[3] += dy;
    }
  }

    // Reset masks that were deactivated
    for (l = 0; l < level; l++)
    {
      if (this->LabelMasks[l][1] < 0.0)
      {
        this->LabelMasks[l][1] = - 1.0 * (this->LabelMasks[l][1]+ 1.0);
        continue;
      }

      if (this->LabelMasks[l][2] < 0.0)
      {
        this->LabelMasks[l][2] = - 1.0 * (this->LabelMasks[l][2]+ 1.0);
        continue;
      }
    }

  return status;
}

void vtkLabeledTreeMapDataMapper::SetLevelRange(int start, int end)
{
  if (((end != -1) && (start > end)) || (start < 0))
  {
    vtkErrorMacro(<<"Invalid level range specified.");
    return;
  }

  this->StartLevel = start;
  this->EndLevel = end;
  this->BuildTime.Modified();
}

void vtkLabeledTreeMapDataMapper::GetLevelRange(int range[2])
{
  range[0] = this->StartLevel;
  range[1] = this->EndLevel;
}
