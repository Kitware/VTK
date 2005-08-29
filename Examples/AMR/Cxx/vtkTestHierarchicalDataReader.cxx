/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkTestHierarchicalDataReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestHierarchicalDataReader.h"

#include "vtkAMRBox.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkExecutive.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHierarchicalDataInformation.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkXMLImageDataReader.h"

vtkCxxRevisionMacro(vtkTestHierarchicalDataReader, "1.1");
vtkStandardNewMacro(vtkTestHierarchicalDataReader);

vtkTestHierarchicalDataReader::vtkTestHierarchicalDataReader()
{
  this->FileName = 0;

  this->SetNumberOfInputPorts(0);
} 

vtkTestHierarchicalDataReader::~vtkTestHierarchicalDataReader()
{
  this->SetFileName(0);
}

// Provide information about the dataset:
// * Number of levels
// * Number of boxes / level
// * AMRBox (extent) of each box
int vtkTestHierarchicalDataReader::RequestInformation(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
    return 0;
    }

  const int numLevels = 3;
  int numBlocks[numLevels] = { 1, 1, 14 };

  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::New();
  compInfo->SetNumberOfLevels(numLevels);
  int i;
  for (i=0; i<numLevels; i++)
    {
    compInfo->SetNumberOfDataSets(i, numBlocks[i]);
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(), compInfo);

  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();

  for (i=0; i<16; i++)
    {
    // Here we load the 16 separate files (each containing
    // an image dataset -uniform rectilinear grid-)
    char* fstr = this->GetBlockFileName(i);
    reader->SetFileName(fstr);

    reader->UpdateInformation();

    delete[] fstr;

    // Each sub-dataset in a vtkHierarchicalBoxDataSet has an associated
    // vtkAMRBox. This is similar to extent but is stored externally
    // since it is possible to have sub-dataset nodes with NULL
    // vtkUniformGrid pointers.
    vtkAMRBox box;

    // This is a hack (do not do this at home). Normally, the
    // region (box) information should be available in the file.
    // In this case, since there is no such information available,
    // we obtain it by looking at each image data's extent.
    // -- begin hack
    int extent[6];
    double spacing[3];
    double origin[3];

    vtkInformation* outInfo = reader->GetExecutive()->GetOutputInformation(0);
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    outInfo->Get(vtkDataObject::SPACING(), spacing);
    outInfo->Get(vtkDataObject::ORIGIN(), origin);

    int j;
    for (j=0; j<3; j++)
      {
      int num = static_cast<int>(floor(origin[j]/spacing[j] + 0.5));
      box.LoCorner[j] = num + extent[2*j];
      box.HiCorner[j] = num + extent[2*j+1] - 1;
      }
  
    int level;
    int dsindex;

    this->GetBlockIdx(i, level, dsindex);

    vtkInformation* subInfo = compInfo->GetInformation(level, dsindex);
    subInfo->Set(vtkHierarchicalBoxDataSet::BOX(), 
                 box.LoCorner[0], box.LoCorner[1], box.LoCorner[2],
                 box.HiCorner[0], box.HiCorner[1], box.HiCorner[2]);
    }

  reader->Delete();
  compInfo->Delete();

  return 1;
}

int vtkTestHierarchicalDataReader::SetUpdateBlocks(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  if (!info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) ||
      !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    vtkErrorMacro("Expected information not found. "
                  "Cannot provide update extent.");
    return 0;
    }

  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  if (!compInfo)
    {
    vtkErrorMacro("Expected information not found. "
                  "Cannot provide update extent.");
    return 0;
    }

  vtkHierarchicalDataInformation* updateInfo = 
    vtkHierarchicalDataInformation::New();
  info->Set(
    vtkCompositeDataPipeline::UPDATE_BLOCKS(), updateInfo);
  updateInfo->SetNumberOfLevels(compInfo->GetNumberOfLevels());

  unsigned int updatePiece = static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  unsigned int updateNumPieces =  static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));

  unsigned int numLevels = updateInfo->GetNumberOfLevels();
  for (unsigned int j=0; j<numLevels; j++)
    {
    updateInfo->SetNumberOfDataSets(j, compInfo->GetNumberOfDataSets(j));
    unsigned int numBlocks = updateInfo->GetNumberOfDataSets(j);
    unsigned int numBlocksPerPiece = 1;
    if (updateNumPieces < numBlocks)
      {
      numBlocksPerPiece = numBlocks / updateNumPieces;
      }
    unsigned int minBlock = numBlocksPerPiece*updatePiece;
    unsigned int maxBlock = numBlocksPerPiece*(updatePiece+1);
    if (updatePiece == updateNumPieces - 1)
      {
      maxBlock = numBlocks;
      }
    for (unsigned int i=minBlock; i<maxBlock; i++)
      {
      vtkInformation* blockInfo = updateInfo->GetInformation(j, i);
      blockInfo->Set(vtkCompositeDataPipeline::MARKED_FOR_UPDATE(), 1);
      }
    }
  updateInfo->Delete();
  return 1;
}

int vtkTestHierarchicalDataReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  int i;

  if (!this->FileName)
    {
    return 0;
    }

  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalBoxDataSet* hb = 
    vtkHierarchicalBoxDataSet::SafeDownCast(doOutput);
  if (!hb)
    {
    return 0;
    }

  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  hb->SetHierarchicalDataInformation(compInfo);

  // Since there is no AMR reader avaible yet, we will load a
  // collection of VTK files and create our own vtkHierarchicalBoxDataSet.
  // To create the files, I loaded a Chombo file with an experimental
  // Chombo reader and wrote the datasets separately.
  vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();

  for (i=0; i<16; i++)
    {
    // Here we load the 16 separate files (each containing
    // an image dataset -uniform rectilinear grid-)
    char* fstr = this->GetBlockFileName(i);
    reader->SetFileName(fstr);

    // We have to update since we are working without a VTK pipeline.
    // This will read the file and the output of the reader will be
    // a valid image data.
    reader->Update();
    delete[] fstr;
    
    // We now create a vtkUniformGrid. This is essentially a simple
    // vtkImageData (not a sub-class though) with blanking. Since
    // VTK readers do not know vtkUniformGrid, we simply create our
    // own by copying from the image data.
    vtkUniformGrid* ug = vtkUniformGrid::New();
    ug->ShallowCopy(reader->GetOutput());

    int level;
    int dsindex;

    this->GetBlockIdx(i, level, dsindex);

    // Given the level, index and box, add the sub-dataset to
    // hierarchical dataset.
    hb->SetDataSet(level, dsindex, ug);

    ug->Delete();
    }
  reader->Delete();
  
  // I hard-coded the refinement ratios. These should normally
  // be available in the file.
  hb->SetRefinementRatio(0, 2);
  hb->SetRefinementRatio(1, 2);

  // This call generates visibility (blanking) arrays that mask
  // regions of lower level datasets that overlap with regions
  // of higher level datasets (it is assumed that, when available,
  // higher level information should always be used instead of
  // lower level information)
  hb->GenerateVisibilityArrays();
  
  return 1;
}

void vtkTestHierarchicalDataReader::GetBlockIdx(
  int blockId, int& level, int& dsindex)
{
  // Similarly, the level of each sub-dataset is normally 
  // available in the file. Since this is not the case, I
  // hard-coded this into the example program.
  // Level 0 = { 0 }, Level 1 = { 1 }, 
  // Level 2 = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
  if (blockId == 0)
    {
    level = 0;
    dsindex = 0;
    }
  else if (blockId == 1)
    {
    level = 1;
    dsindex = 0;
    }
  else
    {
    level = 2;
    dsindex = blockId-2;
    }
}

char* vtkTestHierarchicalDataReader::GetBlockFileName(int blockId)
{
  size_t len = strlen(this->FileName);
  size_t pos;

  // Search from the tail end of the filename until we
  // find a '.' indicating an extension, or we find a
  // path separator or the beginning of the string.
  for (pos=len-1; pos!=0; --pos)
    {
    if (this->FileName[pos] == '.')
      {
      break;
      }

    if (1==pos || this->FileName[pos] == '/')
      {
      // No extension on this->FileName; use the whole
      // thing as the base name
      pos= len;
      break;
      }
    }

  char* fname = new char[pos+1];
  strncpy(fname, this->FileName, pos);
  fname[pos] = '\0';

  // Here we load the 16 separate files (each containing
  // an image dataset -uniform rectilinear grid-)
  char* fstr = new char [strlen(fname) + 
                         strlen(".vti") + 10];
  sprintf(fstr,"%s_%i.vti",fname, blockId);
  delete[] fname;

  return fstr;
}

int vtkTestHierarchicalDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkHierarchicalBoxDataSet");
  return 1;
}

void vtkTestHierarchicalDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " << 
    (this->FileName ? this->FileName : "(none)") << "\n";
}

