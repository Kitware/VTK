/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestMultiBlockDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestMultiBlockDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkHierarchicalDataInformation.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLStructuredGridReader.h"

vtkCxxRevisionMacro(vtkTestMultiBlockDataReader, "1.4");
vtkStandardNewMacro(vtkTestMultiBlockDataReader);

vtkTestMultiBlockDataReader::vtkTestMultiBlockDataReader()
{
  this->FileName = 0;

  this->SetNumberOfInputPorts(0);
} 

vtkTestMultiBlockDataReader::~vtkTestMultiBlockDataReader()
{
  this->SetFileName(0);
}

int vtkTestMultiBlockDataReader::RequestInformation(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
    return 0;
    }

  const int numLevels = 1;
  int numBlocks[numLevels] = { 3 };

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
  compInfo->Delete();

  return 1;
}

int vtkTestMultiBlockDataReader::SetUpdateBlocks(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::SafeDownCast(info->Get(
      vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));
  if (!compInfo)
    {
    vtkErrorMacro("Expected information not found. "
                  "Cannot provide update extent.");
    return 0;
    }

  if (!compInfo || 
      !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) ||
      !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
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
      vtkInformation* info = updateInfo->GetInformation(j, i);
      info->Set(vtkCompositeDataPipeline::MARKED_FOR_UPDATE(), 1);
      }
    }
  updateInfo->Delete();

  return 1;
}

int vtkTestMultiBlockDataReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalDataSet* mb = 
    vtkHierarchicalDataSet::SafeDownCast(doOutput);
  if (!mb)
    {
    return 0;
    }

  if (!this->FileName)
    {
    vtkErrorMacro("No filename has been specified. Cannot execute");
    return 0;
    }

  // We will read three files and collect them together in one
  // multi-block dataset. I broke the combustor dataset into
  // three pieces and wrote them out separately.
  int i;
  vtkXMLStructuredGridReader* reader = vtkXMLStructuredGridReader::New();

  for (i=0; i<3; i++)
    {
    // Here we load the three separate files (each containing
    // a structured grid dataset)
    char* fstr = new char [strlen(this->FileName) + 
                          strlen(".vts") + 10];
    sprintf(fstr,"%s_%i.vts",this->FileName, i);
    reader->SetFileName(fstr);
    // We have to update since we are working without a VTK pipeline.
    // This will read the file and the output of the reader will be
    // a valid structured grid data.
    reader->Update();
    delete[] fstr;

    // We create a copy to avoid adding the same data three
    // times (the output object of the reader does not change
    // when the filename changes)
    vtkStructuredGrid* sg = vtkStructuredGrid::New();
    sg->ShallowCopy(reader->GetOutput());

    // Add the structured grid to the multi-block dataset
    mb->SetDataSet(0, i, sg);
    sg->Delete();
    }
  reader->Delete();

  return 1;
}

void vtkTestMultiBlockDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " << 
    (this->FileName ? this->FileName : "(none)") << "\n";
}

