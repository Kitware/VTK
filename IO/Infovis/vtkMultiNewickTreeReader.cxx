/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiNewickTreeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiNewickTreeReader.h"

#include "vtkNew.h"
#include "vtkTree.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkNewickTreeReader.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkSmartPointer.h"

#include <iostream>
#include <fstream>

vtkStandardNewMacro(vtkMultiNewickTreeReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
vtkMultiNewickTreeReader::vtkMultiNewickTreeReader()
{
  vtkMultiPieceDataSet * output = vtkMultiPieceDataSet::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkMultiNewickTreeReader::~vtkMultiNewickTreeReader()
{
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet * vtkMultiNewickTreeReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet * vtkMultiNewickTreeReader::GetOutput(int idx)
{
  return vtkMultiPieceDataSet::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkMultiNewickTreeReader::SetOutput(vtkMultiPieceDataSet * output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
// I do not think this should be here, but I do not want to remove it now.
int vtkMultiNewickTreeReader::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int piece, numPieces;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
  {
    return 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMultiNewickTreeReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  vtkDebugMacro(<<"Reading Multiple Newick trees ...");

  if(this->GetFileName() == NULL || strcmp(this->GetFileName(), "") == 0)
  {
    vtkErrorMacro(<<"Input filename not set");
    return 1;
  }

  std::ifstream ifs( this->GetFileName(), std::ifstream::in );
  if(!ifs.good())
  {
    vtkErrorMacro(<<"Unable to open " << this->GetFileName() << " for reading");
    return 1;
  }

  vtkMultiPieceDataSet * const output = vtkMultiPieceDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Read the input file into a char *
  int fileSize;
  ifs.seekg(0, std::ios::end);
  fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  char * buffer = new char[fileSize+1];
  ifs.read(buffer, fileSize);
  ifs.close();
  buffer[fileSize] = '\0';

  //use the separator ";" to decide how many trees are contained in the file
  char * current = buffer;
  unsigned int NumOfTrees = 0;
  while ( *current != '\0')
  {
    while (*current == '\n' || *current == ' ')
    {//ignore extra \n and spaces
      current ++;
    }

    char * currentTreeStart = current; //record the starting char of the tree
    unsigned int singleTreeLength = 0;
    while ( *current != ';' &&  *current != '\0')
    {
      singleTreeLength++;
      current++;
    }

    if (*current == ';')  // each newick tree string ends with ";"
    {
      char * singleTreeBuffer = new char[singleTreeLength+1];
      for (unsigned int i = 0; i < singleTreeLength; i++)
      {
         singleTreeBuffer[i] = * (currentTreeStart +i);
      }
      singleTreeBuffer[singleTreeLength] = '\0';
      current ++;//skip ';'

      vtkNew<vtkNewickTreeReader> treeReader;
      vtkSmartPointer<vtkTree> tree = vtkSmartPointer<vtkTree>::New();
      treeReader->ReadNewickTree(singleTreeBuffer, *tree);

      output->SetPiece(NumOfTrees,tree);
      NumOfTrees++;

      delete [] singleTreeBuffer;
    }
  }
  delete [] buffer;

  return 1;
}




//----------------------------------------------------------------------------
int vtkMultiNewickTreeReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiPieceDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkMultiNewickTreeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
