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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLStructuredGridReader.h"

vtkCxxRevisionMacro(vtkTestMultiBlockDataReader, "1.1");
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

int vtkTestMultiBlockDataReader::RequestCompositeInformation(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestCompositeInformation(
        request, inputVector, outputVector))
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
  for (i=0; i<numLevels; i++)
    {
    for (int j=0; j<numBlocks[i]; j++)
      {
      vtkInformation* subInfo = compInfo->GetInformation(i, j);
      subInfo->Set(vtkCompositeDataPipeline::UPDATE_COST(), (double)0.0);
      }
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(), compInfo);
  compInfo->Delete();

  return 1;
}

int vtkTestMultiBlockDataReader::RequestCompositeData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_SET());
  vtkMultiBlockDataSet* mb = 
    vtkMultiBlockDataSet::SafeDownCast(doOutput);
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
    mb->AddDataSet(sg);
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

