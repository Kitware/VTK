/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkComputeHistogram2DOutliers.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkComputeHistogram2DOutliers.h"
//------------------------------------------------------------------------------
#include "vtkCollection.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageMedian3D.h"
#include "vtkImageGradientMagnitude.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSortDataArray.h"
#include "vtkTable.h"
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkComputeHistogram2DOutliers);
//------------------------------------------------------------------------------
vtkComputeHistogram2DOutliers::vtkComputeHistogram2DOutliers()
{
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(2);

  this->PreferredNumberOfOutliers = 10;
  this->BuildTime.Modified();
}
//------------------------------------------------------------------------------
vtkComputeHistogram2DOutliers::~vtkComputeHistogram2DOutliers()
{
}
//------------------------------------------------------------------------------
int vtkComputeHistogram2DOutliers::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the output
  vtkInformation *outSelectionInfo = outputVector->GetInformationObject(OUTPUT_SELECTED_ROWS);
  vtkSelection* outputSelection = vtkSelection::SafeDownCast(
    outSelectionInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outTableInfo = outputVector->GetInformationObject(OUTPUT_SELECTED_TABLE_DATA);
  vtkTable* outputTable = vtkTable::SafeDownCast(
    outTableInfo->Get(vtkDataObject::DATA_OBJECT()));


  // get the input table data
  vtkInformation *inDataInfo = inputVector[INPUT_TABLE_DATA]->GetInformationObject(0);
  if (!inDataInfo)
    {
    vtkErrorMacro("no input data information.");
    return 0;
    }

  vtkTable *inData = vtkTable::SafeDownCast(inDataInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!inData)
    {
    vtkErrorMacro("no input data table.");
    return 0;
    }

  // get the input histogram data
  // try the repeatable vtkImageData port first
  vtkSmartPointer<vtkCollection> histograms = vtkSmartPointer<vtkCollection>::New();
  int numHistograms = inputVector[INPUT_HISTOGRAMS_IMAGE_DATA]->GetNumberOfInformationObjects();
  if (numHistograms > 0)
    {
    // get the data objects for the input
    for (int i=0; i<numHistograms; i++)
      {
      vtkImageData* im = vtkImageData::SafeDownCast(
        inputVector[INPUT_HISTOGRAMS_IMAGE_DATA]->GetInformationObject(i)->Get(vtkDataObject::DATA_OBJECT()));
      if (!im)
        {
        vtkErrorMacro("invalid input histogram.");
        return 0;
        }
      histograms->AddItem(im);
      }
    }
  // if there wasn't anything on that port, try the vtkMultiBlockDataSet port
  else
    {
    vtkInformation *inHistogramInfo = inputVector[INPUT_HISTOGRAMS_MULTIBLOCK]->GetInformationObject(0);
    if (inHistogramInfo)
      {
      vtkMultiBlockDataSet* ds = vtkMultiBlockDataSet::SafeDownCast(
        inHistogramInfo->Get(vtkDataObject::DATA_OBJECT()));

      if (ds)
        {
        for (int i=0; i<(int)ds->GetNumberOfBlocks(); i++)
          {
          vtkImageData* im = vtkImageData::SafeDownCast(ds->GetBlock(i));
          if (im)
            {
            histograms->AddItem(im);
            }
          }
        }
      }
    }

  if (histograms->GetNumberOfItems() <= 0)
    {
    vtkErrorMacro("No input histograms.");
    return 0;
    }

  // compute the thresholds that contain outliers
  vtkSmartPointer<vtkCollection> outlierThresholds = vtkSmartPointer<vtkCollection>::New();
  if (!this->ComputeOutlierThresholds(histograms,outlierThresholds))
    {
    vtkErrorMacro("Error during outlier bin computation.");
    return 0;
    }

  // take the computed outlier thresholds and extract the input table rows that match
  vtkSmartPointer<vtkIdTypeArray> outlierRowIds = vtkSmartPointer<vtkIdTypeArray>::New();
  if (outlierThresholds->GetNumberOfItems() >= 0 &&
      !this->FillOutlierIds(inData,outlierThresholds,outlierRowIds,outputTable))
    {
    vtkErrorMacro("Error during outlier row retrieval.");
    return 0;
    }

  // print out the table, just for grins
  //outputTable->Dump();

  // generate the selection based on the outlier row ids
  if (outputSelection->GetNumberOfNodes() == 0)
    {
    vtkSmartPointer<vtkSelectionNode> newNode = vtkSmartPointer<vtkSelectionNode>::New();
    newNode->GetProperties()->Set(
//      vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::PEDIGREEIDS);
      vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::INDICES);
    newNode->GetProperties()->Set(
      vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::ROW);
    outputSelection->AddNode(newNode);
    }

  vtkSelectionNode* node = outputSelection->GetNode(0);
  node->SetSelectionList(outlierRowIds);

  this->BuildTime.Modified();

  return 1;
}
//------------------------------------------------------------------------------
int vtkComputeHistogram2DOutliers::FillInputPortInformation(int port,
                                                            vtkInformation* info)
{
  if (port == vtkComputeHistogram2DOutliers::INPUT_TABLE_DATA)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
    }
  else if (port == vtkComputeHistogram2DOutliers::INPUT_HISTOGRAMS_IMAGE_DATA)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
    }
  else if (port == vtkComputeHistogram2DOutliers::INPUT_HISTOGRAMS_MULTIBLOCK)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
    }

  return 0;
}

//------------------------------------------------------------------------------
int vtkComputeHistogram2DOutliers::FillOutputPortInformation(int port,
                                                             vtkInformation* info)
{
  if (port == vtkComputeHistogram2DOutliers::OUTPUT_SELECTED_ROWS)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
    return 1;
    }
  else if (port == vtkComputeHistogram2DOutliers::OUTPUT_SELECTED_TABLE_DATA)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
    }

  return 0;
}

//------------------------------------------------------------------------------
void vtkComputeHistogram2DOutliers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "PreferredNumberOfOutliers: " << this->PreferredNumberOfOutliers << endl;
}
//------------------------------------------------------------------------------
// Tries to find the right number of outliers.  Not the smartest thing
// in the world yet.  It basically starts off with a low percentage threshold
// (i.e. outlier bins must have a count smaller than pct * maximum bin cuont),
// find outliers, and grows the percentage if there are too outliers.  The
// growth process is geometric until it finds enough, then it backtracks and
// goes linear.  Very slow.
int vtkComputeHistogram2DOutliers::ComputeOutlierThresholds(vtkCollection* histograms, vtkCollection* thresholds)
{
  if (!histograms || !thresholds)
    return 0;

  int numHistograms = histograms->GetNumberOfItems();

  // compute the maximum bin count
  double maxVal = 0.0;
  double r[2];
  for (int i=0; i<numHistograms; i++)
    {
    vtkImageData* histogram = vtkImageData::SafeDownCast(histograms->GetItemAsObject(i));
    histogram->GetPointData()->GetScalars()->GetRange(r,0);
    if (r[1] > maxVal)
      maxVal = r[1];
    }

  double pctThreshold = .01;
  bool growingSlower = false;
  double slowGrowthInc = 100.0;
  thresholds->RemoveAllItems();

  // grow the percentage threshold until we're at 100% of the maximum bin count or
  // we have enough outliers.
  int numOutliers = 0;
  while (pctThreshold < 1.0)
    {
    int tmpNumOutliers = 0;
    vtkSmartPointer<vtkCollection> tmpThresholdCollection = vtkSmartPointer<vtkCollection>::New();
    // compute outlier ids in all of the histograms
    for (int i=0; i<numHistograms; i++)
      {
      vtkSmartPointer<vtkDoubleArray> tmpThresholds = vtkSmartPointer<vtkDoubleArray>::New();
      tmpThresholds->SetNumberOfComponents(4);

      vtkImageData* histogram = vtkImageData::SafeDownCast(histograms->GetItemAsObject(i));
      tmpNumOutliers += this->ComputeOutlierThresholds(histogram,tmpThresholds,pctThreshold*maxVal);
      tmpThresholdCollection->AddItem(tmpThresholds);
      }

    // Did the number of outliers get closer to the preferred number?  If so, keep them.
    if (abs(tmpNumOutliers - this->PreferredNumberOfOutliers) <=
        abs(numOutliers - this->PreferredNumberOfOutliers))
      {
      thresholds->RemoveAllItems();
      for (int j=0; j<tmpThresholdCollection->GetNumberOfItems(); j++)
        thresholds->AddItem(tmpThresholdCollection->GetItemAsObject(j));
      numOutliers = tmpNumOutliers;
      }
    // got farther from the preferred number, and still in the first pass.  initiate second, slower pass.
    else if (!growingSlower)
      {
      growingSlower = true;
      pctThreshold *= .5;
      slowGrowthInc = pctThreshold / 10.0;
      }
    // got farther from the preferred number, in the second class.  quit.
    else
      {
      break;
      }

    pctThreshold += (growingSlower) ? slowGrowthInc : pctThreshold;
    }

  return 1;
}
//------------------------------------------------------------------------------
// This function actually detects outliers, given a percentage threshold.
// It does a 3x3 median filter operation to find out what pixels disappear,
// and if they disappear and are small enough, the pixel is accepted as an outlier.
int vtkComputeHistogram2DOutliers::ComputeOutlierThresholds(vtkImageData* histogram, vtkDoubleArray* thresholds, double threshold)
{
  if (!histogram || !thresholds)
    return 0;

  vtkSmartPointer<vtkImageMedian3D> median = vtkSmartPointer<vtkImageMedian3D>::New();
  median->SetInputData(histogram);
  median->SetKernelSize(3,3,1);
  median->Update();

  vtkDataArray* histArray = histogram->GetPointData()->GetScalars();
  vtkDataArray* filtArray = median->GetOutput()->GetPointData()->GetScalars();

  int dims[3] = { 0,0,0 };
  double sp[3] = { 0,0,0 };
  double o[3] = {0,0,0};
  histogram->GetDimensions(dims);
  histogram->GetSpacing(sp);
  histogram->GetOrigin(o);

  int x,y,numOutliers=0;
  double hval,fval;
  for (int j=0; j<histArray->GetNumberOfTuples(); j++)
    {
    hval = histArray->GetTuple1(j);
    fval = filtArray->GetTuple1(j);

    if (hval < threshold && hval-fval > 0.0)
      {
      x = j % dims[0];
      y = j / dims[0];
      thresholds->InsertNextTuple4(o[0] + x*sp[0], o[0] + (x+1)*sp[0],
                                   o[1] + y*sp[1], o[1] + (y+1)*sp[1]);
      numOutliers += (int)hval;
      }
    }
  return numOutliers;
}
//------------------------------------------------------------------------------
int vtkComputeHistogram2DOutliers::FillOutlierIds(vtkTable* data, vtkCollection* thresholds, vtkIdTypeArray *rowIds, vtkTable* outTable)
{
  if (!data || !thresholds || !rowIds || !outTable)
    {
    return 0;
    }

  // nothing to threshold, that's fine, just quit
  if (thresholds->GetNumberOfItems() == 0)
    {
    return 1;
    }
  // if there's something to threshold, there better be the correct
  // number of threshold arrays
  else if (data->GetNumberOfColumns()-1 != thresholds->GetNumberOfItems())
    {
    return 0;
    }

  int numColumns = data->GetNumberOfColumns();

  // store the matching rows in a vtkIdList since this list
  // can check for uniqueness, and I don't want duplicate rows.
  vtkSmartPointer<vtkIdList> uniqueRowIds = vtkSmartPointer<vtkIdList>::New();
  for (int i=0; i<numColumns-1; i++)
    {
    vtkDataArray* col1 = vtkDataArray::SafeDownCast(data->GetColumn(i));
    vtkDataArray* col2 = vtkDataArray::SafeDownCast(data->GetColumn(i+1));

    vtkDoubleArray* currThresholds = vtkDoubleArray::SafeDownCast(thresholds->GetItemAsObject(i));
    for (int j=0; j<currThresholds->GetNumberOfTuples(); j++)
      {
      double *t = currThresholds->GetTuple(j);

      for (int k=0; k<col1->GetNumberOfTuples(); k++)
        {
        double v1 = col1->GetComponent(k,0);
        double v2 = col2->GetComponent(k,0);

        if (v1 >= t[0] && v1 < t[1] &&
            v2 >= t[2] && v2 < t[3])
          {
          uniqueRowIds->InsertUniqueId(k);
          }
        }
      }
    }

  rowIds->Initialize();
  for (int i=0; i<uniqueRowIds->GetNumberOfIds(); i++)
    {
    rowIds->InsertNextValue(uniqueRowIds->GetId(i));
    }

  // this probably isn't necessary
  vtkSortDataArray::Sort(rowIds);

  // initialize the output table
  outTable->Initialize();
  for (int i=0; i<numColumns; i++)
    {
    vtkDataArray* a = vtkDataArray::CreateDataArray(data->GetColumn(i)->GetDataType());
    a->SetNumberOfComponents(data->GetColumn(i)->GetNumberOfComponents());
    a->SetName(data->GetColumn(i)->GetName());
    outTable->AddColumn(a);
    a->Delete();
    }

  for (int i=0; i<rowIds->GetNumberOfTuples(); i++)
    {
    outTable->InsertNextRow(data->GetRow(rowIds->GetValue(i)));
    }

  return 1;

}
//------------------------------------------------------------------------------
vtkTable* vtkComputeHistogram2DOutliers::GetOutputTable()
{
  if (this->BuildTime < this->GetMTime())
    this->Update();
  return vtkTable::SafeDownCast(this->GetOutputDataObject(OUTPUT_SELECTED_TABLE_DATA));
}
