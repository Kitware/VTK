/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRCalculatorFilter.cxx

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

#include "vtkObjectFactory.h"
#include "vtkRCalculatorFilter.h"
#include "vtkRInterface.h"
#include "vtkErrorCode.h"
#include "vtkDataSet.h"
#include "vtkGraph.h"
#include "vtkFieldData.h"
#include "vtkArrayData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDoubleArray.h"
#include "vtkTable.h"
#include "vtkTree.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkStringArray.h"

#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>

#define BUFFER_SIZE 32768

vtkStandardNewMacro(vtkRCalculatorFilter);

class ArrNames
{

public:
  ArrNames(const char* Vname, const char* Rname)
  {
    this->VTKArrName = Vname;
    this->RarrName = Rname;
  };

  std::string VTKArrName;
  std::string RarrName;

};

class RVariableNames
{

public:
  RVariableNames()
  {
    this->ResetNameIterator();
  }

  void SetName(std::string name)
  {
    this->Names.push_back(name);
    this->ResetNameIterator();
  }

  void SetNames(vtkStringArray* names)
  {
    this->Names.clear();
    for (vtkIdType i = 0; i < names->GetNumberOfTuples(); ++i)
    {
      this->Names.push_back(names->GetValue(i));
    }

    this->ResetNameIterator();
  }

  void ResetNameIterator()
  {
    this->NameIterator = this->Names.begin();
  }

  std::string NextName()
  {
    return *this->NameIterator++;
  }

  bool HasName()
  {
    return this->NameIterator != this->Names.end();
  }

  void Clear()
  {
    this->Names.clear();
    this->ResetNameIterator();
  }

  int Count()
  {
    return this->Names.size();
  }


  std::vector<std::string> Names;
  std::vector<std::string>::iterator NameIterator;

};

class vtkRCalculatorFilterInternals
{

public:
  std::vector<ArrNames> PutArrNames;
  std::vector<ArrNames> GetArrNames;
  RVariableNames PutTableNames;
  RVariableNames GetTableNames;
  RVariableNames PutTreeNames;
  RVariableNames GetTreeNames;

};

vtkRCalculatorFilter::vtkRCalculatorFilter()
{

  this->ri = 0;
  this->Rscript = 0;
  this->RfileScript = 0;
  this->ScriptFname = 0;
  this->TimeOutput = 1;
  this->Routput = 1;
  this->BlockInfoOutput = 1;
  this->ScriptFname = 0;
  this->CurrentTime = 0;
  this->TimeRange = 0;
  this->TimeSteps = 0;
  this->BlockId = 0;
  this->NumBlocks = 0;
  this->Routput = 1;
  this->rcfi = new vtkRCalculatorFilterInternals();
  this->OutputBuffer = new char[BUFFER_SIZE];
  this->GetInputPortInformation(0)->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);

}

vtkRCalculatorFilter::~vtkRCalculatorFilter()
{

  delete this->rcfi;

  if(this->ri)
  {
    this->ri->Delete();
  }

  delete [] this->Rscript;
  delete [] this->RfileScript;
  delete [] this->ScriptFname;

  if(this->CurrentTime)
  {
    this->CurrentTime->Delete();
  }

  if(this->TimeRange)
  {
    this->TimeRange->Delete();
  }

  if(this->TimeSteps)
  {
    this->TimeSteps->Delete();
  }

  if(this->BlockId)
  {
    this->BlockId->Delete();
  }

  if(this->NumBlocks)
  {
    this->NumBlocks->Delete();
  }

  delete [] this->OutputBuffer;

}

void vtkRCalculatorFilter::PrintSelf(ostream& os, vtkIndent indent)
{

  this->Superclass::PrintSelf(os,indent);

  os << indent << "Rscript: "
     << (this->Rscript ? this->Rscript : "(none)") << endl;

  os << indent << "RfileScript: "
     << (this->RfileScript ? this->RfileScript : "(none)") << endl;

  os << indent << "ScriptFname: "
     << (this->ScriptFname ? this->ScriptFname : "(none)") << endl;

  os << indent << "Routput: "
     << (this->Routput ? "On" : "Off") << endl;

  os << indent << "TimeOutput: "
     << (this->TimeOutput ? "On" : "Off") << endl;

  os << indent << "BlockInfoOutput: "
     << (this->BlockInfoOutput ? "On" : "Off") << endl;

  os << indent << "CurrentTime: " << endl;

  if (this->CurrentTime)
  {
    this->CurrentTime->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(none)" << endl;
  }

  os << indent << "TimeRange: " << endl;

  if (this->TimeRange)
  {
    this->TimeRange->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(none)" << endl;
  }

  os << indent << "TimeSteps: " << endl;

  if (this->TimeSteps)
  {
    this->TimeSteps->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(none)" << endl;
  }

  os << indent << "BlockId: " << endl;

  if (this->BlockId)
  {
    this->BlockId->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(none)" << endl;
  }

  os << indent << "NumBlocks: " << endl;

  if (this->NumBlocks)
  {
    this->NumBlocks->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(none)" << endl;
  }

}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessRequest(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::RequestDataObject(
    vtkInformation*,
    vtkInformationVector** inputVector ,
    vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    //one output port, but multiple InformationObjects()
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

      if (this->HasMultipleGets())
      {
        if (!output || !output->IsA("vtkMultiPieceDataSet"))
        {
          vtkDataObject* newOutput = vtkMultiPieceDataSet::New();
          info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
          newOutput->Delete();
        }
      }
      else
      {
        if (!output || !output->IsA(input->GetClassName()))
        {
          vtkDataObject* newOutput = NULL;
          if (this->rcfi->GetTableNames.Count() > 0)
          {
            newOutput = vtkTable::New();
          }
          else if (this->rcfi->GetTreeNames.Count() > 0)
          {
            newOutput = vtkTree::New();
          }
          else
          {
            newOutput = input->NewInstance();
          }
          info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
          newOutput->Delete();
        }
      }
    }
    return (1);
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                      vtkInformationVector **inputVector,
                                      vtkInformationVector *outputVector)
{

  int result;

  if(!this->ri)
  {
    this->ri = vtkRInterface::New();
  }


  if(this->ScriptFname)
  {
    this->SetRscriptFromFile(this->ScriptFname);
  }

  if( (!this->Rscript) && (!this->RfileScript) )
  {
    return(1);
  }

  if(this->Routput)
  {
    this->ri->OutputBuffer(this->OutputBuffer, BUFFER_SIZE);
  }

  vtkInformation* outinfo = outputVector->GetInformationObject(0);
  vtkInformation* inpinfo = inputVector[0]->GetInformationObject(0);

  vtkDataObject* input = inpinfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outinfo->Get(vtkDataObject::DATA_OBJECT());

  // initialize the output's components if it is a composite data set.
  if (this->HasMultipleGets())
  {
    vtkMultiPieceDataSet* outComposite =
        vtkMultiPieceDataSet::SafeDownCast(output);

    int tableCount = 0;
    int treeCount = 0;
    int itemCount = 0;

    for (int i=0; i < this->rcfi->GetTableNames.Count() - tableCount; i++)
    {
      vtkTable *table = vtkTable::New();
      outComposite->SetPiece(itemCount++, table);
      table->Delete();
    }

    for (int i=0; i < this->rcfi->GetTreeNames.Count() - treeCount; i++)
    {
      vtkTree *tree = vtkTree::New();
      outComposite->SetPiece(itemCount++, tree);
      tree->Delete();
    }
  }
  else if (!output->IsA("vtkTable") && !output->IsA("vtkTree"))
  {
    // some tests assume that input arrays will also be present in the output
    // data set.
    output->ShallowCopy(input);
  }

  // For now: use the first input information for timing
  if(this->TimeOutput)
  {
    if ( inpinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
      int length = inpinfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );

      if(!this->TimeSteps)
      {
        this->TimeSteps = vtkDoubleArray::New();
        this->TimeSteps->SetNumberOfComponents(1);
        this->TimeSteps->SetNumberOfTuples(length);
      }
      else if(this->TimeSteps->GetNumberOfTuples() != length)
      {
        this->TimeSteps->SetNumberOfTuples(length);
      }

      for(int i = 0; i<length; i++)
      {
        this->TimeSteps->InsertValue(i,
          inpinfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() )[i] );
      }

      this->ri->AssignVTKDataArrayToRVariable(this->TimeSteps, "VTK_TIME_STEPS");
    }

    if ( inpinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()) )
    {
      if(!this->TimeRange)
      {
        this->TimeRange = vtkDoubleArray::New();
        this->TimeRange->SetNumberOfComponents(1);
        this->TimeRange->SetNumberOfTuples(2);
      }

      this->TimeRange->InsertValue(0,inpinfo->Get( vtkStreamingDemandDrivenPipeline::TIME_RANGE() )[0] );

      this->TimeRange->InsertValue(1,inpinfo->Get( vtkStreamingDemandDrivenPipeline::TIME_RANGE() )[1] );

      this->ri->AssignVTKDataArrayToRVariable(this->TimeRange, "VTK_TIME_RANGE");
    }

    if ( input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()) )
    {
      if(!this->CurrentTime)
      {
        this->CurrentTime = vtkDoubleArray::New();
        this->CurrentTime->SetNumberOfComponents(1);
        this->CurrentTime->SetNumberOfTuples(1);
      }

      this->CurrentTime->InsertValue(0,input->GetInformation()->Get( vtkDataObject::DATA_TIME_STEP()) );

      this->ri->AssignVTKDataArrayToRVariable(this->CurrentTime, "VTK_CURRENT_TIME");
    }
  }

  // assign vtk variables to R variables
  int numberOfInputs =  inputVector[0]->GetNumberOfInformationObjects();
  rcfi->PutTableNames.ResetNameIterator();
  rcfi->PutTreeNames.ResetNameIterator();
  for ( int i = 0; i < numberOfInputs; i++)
  {
    inpinfo = inputVector[0]->GetInformationObject(i);
    input = inpinfo->Get(vtkDataObject::DATA_OBJECT());
    this->ProcessInputDataObject(input);
  }

  //run scripts
  if(this->Rscript)
  {
    result = this->ri->EvalRscript(this->Rscript);

    if(result)
    {
      vtkErrorMacro(<<"Failed to evaluate command string in R");
      return(1);
    }

    if(this->Routput)
    {
      cout << this->OutputBuffer << endl;
    }
  }

  if(this->RfileScript)
  {
    result = this->ri->EvalRscript(this->RfileScript);

    if(result)
    {
      vtkErrorMacro(<<"Failed to evaluate command string in R");
      return(1);
    }

    if(this->Routput)
    {
      cout << this->OutputBuffer << endl;
    }
  }

  // generate output
  rcfi->GetTableNames.ResetNameIterator();
  rcfi->GetTreeNames.ResetNameIterator();
  if (this->ProcessOutputDataObject(output) != 0)
  {
    vtkErrorMacro(<<"Filter does not handle output data type");
    return 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputDataSet(vtkDataSet* dsIn)
{
  int ncells;
  int npoints;
  vtkDataSetAttributes* CellinFD = 0;
  vtkDataSetAttributes* PointinFD = 0;

  std::vector<ArrNames>::iterator VectorIterator;
  vtkDataArray* currentArray = 0;

  CellinFD = dsIn->GetCellData();
  PointinFD = dsIn->GetPointData();

  ncells = dsIn->GetNumberOfCells();
  npoints = dsIn->GetNumberOfPoints();

  if( (ncells < 1) && (npoints < 1) )
  {
    vtkErrorMacro(<<"Empty Data Set");
    return(1);
  }

  for(VectorIterator = this->rcfi->PutArrNames.begin();
    VectorIterator != this->rcfi->PutArrNames.end();
    VectorIterator++)
  {
    currentArray = PointinFD->GetArray(VectorIterator->VTKArrName.c_str());

    if(!currentArray)
    {
      currentArray = CellinFD->GetArray(VectorIterator->VTKArrName.c_str());
    }

    if(currentArray)
    {
      this->ri->AssignVTKDataArrayToRVariable(currentArray,
        VectorIterator->RarrName.c_str());
    }
    else
    {
      vtkErrorMacro(<<"Array Name not in Data Set " << VectorIterator->VTKArrName.c_str());
      return(1);
    }
  }
return (1);
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessOutputDataSet(vtkDataSet* dsOut)
{
  vtkDataArray* currentArray = 0;
  vtkDataSetAttributes* CelloutFD = dsOut->GetCellData();
  vtkDataSetAttributes* PointoutFD = dsOut->GetPointData();

  int ncells = dsOut->GetNumberOfCells();
  int npoints = dsOut->GetNumberOfPoints();

  std::vector<ArrNames>::iterator VectorIterator;
  for(VectorIterator = this->rcfi->GetArrNames.begin();
    VectorIterator != this->rcfi->GetArrNames.end();
    VectorIterator++)
  {
    currentArray = this->ri->AssignRVariableToVTKDataArray(VectorIterator->RarrName.c_str());

    if(!currentArray)
    {
      vtkErrorMacro(<<"Failed to get array from R");
      return(1);
    }

    int ntuples = currentArray->GetNumberOfTuples();

    vtkDataSetAttributes* dsa;

    if(ntuples == ncells)
    {
      dsa = CelloutFD;
    }
    else if(ntuples == npoints)
    {
      dsa = PointoutFD;
    }
    else
    {
      vtkErrorMacro(<<"Array returned from R has wrong size");
      currentArray->Delete();
      return(1);
    }

    currentArray->SetName(VectorIterator->VTKArrName.c_str());

    if(dsa->HasArray(VectorIterator->VTKArrName.c_str()))
    {
      dsa->RemoveArray(VectorIterator->VTKArrName.c_str());
    }

    dsa->AddArray(currentArray);
  }

  return(1);
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputGraph(vtkGraph* gIn)
{
  vtkDataSetAttributes* CellinFD = gIn->GetEdgeData();
  vtkDataSetAttributes* PointinFD = gIn->GetVertexData();

  int ncells = gIn->GetNumberOfEdges();
  int npoints = gIn->GetNumberOfVertices();

  if( (npoints < 1) && (ncells < 1) )
  {
    vtkErrorMacro(<<"Empty Data Set");
    return(1);
  }

  std::vector<ArrNames>::iterator VectorIterator;
  vtkDataArray* currentArray = 0;
  for(VectorIterator = this->rcfi->PutArrNames.begin();
    VectorIterator != this->rcfi->PutArrNames.end();
    VectorIterator++)
  {
    currentArray = PointinFD->GetArray(VectorIterator->VTKArrName.c_str());

    if(!currentArray)
    {
      currentArray = CellinFD->GetArray(VectorIterator->VTKArrName.c_str());
    }

    if(currentArray)
    {
      this->ri->AssignVTKDataArrayToRVariable(currentArray,
        VectorIterator->RarrName.c_str());
    }
    else
    {
      vtkErrorMacro(<<"Array Name not in Data Set " << VectorIterator->VTKArrName.c_str());
      return(1);
    }
  }
  return (1);
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessOutputGraph(vtkGraph* gOut)
{
  vtkDataSetAttributes * CelloutFD = gOut->GetEdgeData();
  vtkDataSetAttributes *PointoutFD = gOut->GetVertexData();

  int ncells = gOut->GetNumberOfEdges();
  int npoints = gOut->GetNumberOfVertices();

  vtkDataArray* currentArray = 0;
  std::vector<ArrNames>::iterator VectorIterator;
  for(VectorIterator = this->rcfi->GetArrNames.begin();
    VectorIterator != this->rcfi->GetArrNames.end();
    VectorIterator++)
  {
    currentArray = this->ri->AssignRVariableToVTKDataArray(VectorIterator->RarrName.c_str());

    if(!currentArray)
    {
      vtkErrorMacro(<<"Failed to get array from R");
      return(1);
    }

    int ntuples = currentArray->GetNumberOfTuples();

    vtkDataSetAttributes* dsa;

    if(ntuples == ncells)
    {
      dsa = CelloutFD;
    }
    else if(ntuples == npoints)
    {
      dsa = PointoutFD;
    }
    else
    {
      vtkErrorMacro(<<"Array returned from R has wrong size");
      currentArray->Delete();
      return(1);
    }

    currentArray->SetName(VectorIterator->VTKArrName.c_str());

    if(dsa->HasArray(VectorIterator->VTKArrName.c_str()))
    {
      dsa->RemoveArray(VectorIterator->VTKArrName.c_str());
    }

    dsa->AddArray(currentArray);
  }

  return (1);
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputArrayData(vtkArrayData * adIn)
{

  vtkArray* cArray = 0;
  std::vector<ArrNames>::iterator VectorIterator;
  for(VectorIterator = this->rcfi->PutArrNames.begin();
    VectorIterator != this->rcfi->PutArrNames.end();
    VectorIterator++)
  {
    int index = atoi(VectorIterator->VTKArrName.c_str());

    if( (index < 0) || (index >= adIn->GetNumberOfArrays()) )
    {
      vtkErrorMacro(<<"Array Index out of bounds " << index);
      return(1);
    }

    cArray = adIn->GetArray(index);

    this->ri->AssignVTKArrayToRVariable(cArray,  VectorIterator->RarrName.c_str());
  }
  return (1);
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessOutputArrayData(vtkArrayData * adOut)
{
  vtkArray * cArray = 0;
  std::vector<ArrNames>::iterator VectorIterator;
  for(VectorIterator = this->rcfi->GetArrNames.begin();
    VectorIterator != this->rcfi->GetArrNames.end();
    VectorIterator++)
  {
    cArray = this->ri->AssignRVariableToVTKArray(VectorIterator->RarrName.c_str());

    if(!cArray)
    {
      vtkErrorMacro(<<"Failed to get array from R");
      return(1);
    }

    cArray->SetName(VectorIterator->VTKArrName.c_str());

    adOut->AddArray(cArray);
  }
  return (1);
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputCompositeDataSet(vtkCompositeDataSet* cdsIn)
{
    vtkCompositeDataIterator* iter = cdsIn->NewIterator();

    if(this->BlockInfoOutput)
    {
      if(!this->BlockId)
      {
        this->BlockId = vtkDoubleArray::New();
        this->BlockId->SetNumberOfComponents(1);
        this->BlockId->SetNumberOfTuples(1);
      }

      if(!this->NumBlocks)
      {
        this->NumBlocks = vtkDoubleArray::New();
        this->NumBlocks->SetNumberOfComponents(1);
        this->NumBlocks->SetNumberOfTuples(1);
      }
      int nb = 0;
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        nb++;
      }
      this->NumBlocks->SetValue(0,nb);
      this->ri->AssignVTKDataArrayToRVariable(this->NumBlocks, "VTK_NUMBER_OF_BLOCKS");
    }


    int bid = 1;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if(this->BlockInfoOutput)
      {
        this->BlockId->SetValue(0,bid);
        this->ri->AssignVTKDataArrayToRVariable(this->BlockId, "VTK_BLOCK_ID");
      }
      this->ProcessInputDataObject(iter->GetCurrentDataObject());
      bid++;
    }
    iter->Delete();
    return (1);
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessOutputCompositeDataSet(vtkCompositeDataSet * cdsOut)
{

  vtkCompositeDataIterator* iter = cdsOut->NewIterator();
  iter->InitTraversal();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    this->ProcessOutputDataObject(iter->GetCurrentDataObject());
  }
  iter->Delete();

  return (1);
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputTable(vtkTable* tIn)
{
  if (this->rcfi->PutTableNames.HasName())
  {
    std::string name = this->rcfi->PutTableNames.NextName();
    return this->ProcessInputTable(name, tIn);
  }
  else
  {
    return 0;
  }
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputTable(std::string& name, vtkTable* tIn)
{
  if(name.length() > 0)
  {
    this->ri->AssignVTKTableToRVariable(tIn, name.c_str());
  }
  return (1);

}


//----------------------------------------------------------------------------
vtkTable* vtkRCalculatorFilter::GetOutputTable(std::string& name)
{

  if(name.length() > 0)
  {
    return this->ri->AssignRVariableToVTKTable(name.c_str());
  }

  return NULL;

}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessOutputTable(vtkTable* tOut)
{
  if (rcfi->GetTableNames.HasName())
  {
    std::string name = rcfi->GetTableNames.NextName();
    tOut->ShallowCopy(this->GetOutputTable(name));
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputTree(vtkTree* tIn)
{

  if (this->rcfi->PutTreeNames.HasName())
  {
    std::string name = this->rcfi->PutTreeNames.NextName();
    return this->ProcessInputTree(name, tIn);
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputTree(std::string& name, vtkTree* tIn)
{

  if(name.size() > 0)
  {
    this->ri->AssignVTKTreeToRVariable(tIn, name.c_str());
  }
  return (1);

}

//----------------------------------------------------------------------------
vtkTree* vtkRCalculatorFilter::GetOutputTree(std::string& name)
{

  if(name.length() > 0)
  {
    return this->ri->AssignRVariableToVTKTree(name.c_str());
  }
  return (NULL);

}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessOutputTree(vtkTree* tOut)
{

  if (rcfi->GetTreeNames.HasName())
  {
    std::string name = rcfi->GetTreeNames.NextName();
    tOut->ShallowCopy(this->GetOutputTree(name));
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::SetRscriptFromFile(const char* fname)
{

  FILE *fp;
  long len;
  long rlen;

  if(fname && (strlen(fname) > 0) )
  {
    fp = fopen(fname,"rb");

    if(!fp)
    {
      vtkErrorMacro(<<"Can't open input file named " << fname);
      return(0);
    }

    fseek(fp,0,SEEK_END);
    len = ftell(fp);
    fseek(fp,0,SEEK_SET);

    delete [] this->RfileScript;
    this->RfileScript = new char[len+1];
    rlen = static_cast<long>(fread(this->RfileScript,1,len,fp));
    this->RfileScript[len] = '\0';
    fclose(fp);

    if (rlen != len)
    {
      delete [] this->RfileScript;
      this->RfileScript = 0;
      vtkErrorMacro(<<"Error reading R script");
      return(0);
    }

    this->Modified();

    return(1);
  }
  else if(!fname)
  {
    vtkErrorMacro(<<"Input file name is NULL");
    return(0);
  }
  else
  {
    return(0);
  }

}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::PutArray(const char* NameOfVTKArray,
                                    const char* NameOfMatVar)
{

  if( NameOfVTKArray && (strlen(NameOfVTKArray) > 0) &&
      NameOfMatVar && (strlen(NameOfMatVar) > 0) )
  {
    rcfi->PutArrNames.push_back(ArrNames(NameOfVTKArray, NameOfMatVar));
    this->Modified();
  }

}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::GetArray(const char* NameOfVTKArray,
                                    const char* NameOfMatVar)
{

  if( NameOfVTKArray && (strlen(NameOfVTKArray) > 0) &&
      NameOfMatVar && (strlen(NameOfMatVar) > 0) )
  {
    rcfi->GetArrNames.push_back(ArrNames(NameOfVTKArray, NameOfMatVar));
    this->Modified();
  }

}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::PutTable(const char* NameOfRvar)
{

  if( NameOfRvar && (strlen(NameOfRvar) > 0) )
  {
    rcfi->PutTableNames.SetName(NameOfRvar);
    this->Modified();
  }

}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::GetTable(const char* NameOfRvar)
{

  if( NameOfRvar && (strlen(NameOfRvar) > 0) )
  {
    rcfi->GetTableNames.SetName(NameOfRvar);
    this->Modified();
  }

}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::PutTree(const char* NameOfRvar)
{

  if( NameOfRvar && (strlen(NameOfRvar) > 0) )
  {
    rcfi->PutTreeNames.SetName(NameOfRvar);
    this->Modified();
  }

}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::GetTree(const char* NameOfRvar)
{

  if( NameOfRvar && (strlen(NameOfRvar) > 0) )
  {
    rcfi->GetTreeNames.SetName(NameOfRvar);
    this->Modified();
  }

}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::RemoveAllPutVariables()
{
  rcfi->PutArrNames.clear();
  if (this->HasMultiplePuts())
  {
    rcfi->PutTreeNames.Clear();
    rcfi->PutTableNames.Clear();
  }
  this->Modified();
}


//----------------------------------------------------------------------------
void vtkRCalculatorFilter::RemoveAllGetVariables()
{
  rcfi->GetArrNames.clear();
  if (this->HasMultipleGets())
  {
    rcfi->GetTreeNames.Clear();
    rcfi->GetTableNames.Clear();
  }
  this->Modified();
}


//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessInputDataObject(vtkDataObject *input)
{
  vtkDataSet * dataSetIn = vtkDataSet::SafeDownCast(input);
  if (dataSetIn)
  {
    this->ProcessInputDataSet(dataSetIn);
    return 0;
  }

  vtkTree * treeIn = vtkTree::SafeDownCast(input);
  if (treeIn)
  {
    this->ProcessInputTree(treeIn);
    return 0;
  }

  vtkGraph * graphIn = vtkGraph::SafeDownCast(input);
  if (graphIn)
  {
    this->ProcessInputGraph(graphIn);
    return 0;
  }

  vtkArrayData * arrayDataIn = vtkArrayData::SafeDownCast(input);
  if (arrayDataIn)
  {
    this->ProcessInputArrayData(arrayDataIn);
    return 0;
  }

  vtkCompositeDataSet * compositeDataSetIn =
    vtkCompositeDataSet::SafeDownCast(input);
  if (compositeDataSetIn)
  {
    this->ProcessInputCompositeDataSet(compositeDataSetIn);
    return 0;
  }

  vtkTable * tableIn = vtkTable::SafeDownCast(input);
  if (tableIn)
  {
    this->ProcessInputTable(tableIn);
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkRCalculatorFilter::ProcessOutputDataObject(vtkDataObject *output)
{
  vtkDataSet* dataSetOut = vtkDataSet::SafeDownCast(output);
  if (dataSetOut)
  {
    this->ProcessOutputDataSet(dataSetOut);
    return 0;
  }

  vtkCompositeDataSet* compositeDataSetOut= vtkCompositeDataSet::SafeDownCast(output);
  if (compositeDataSetOut)
  {
    this->ProcessOutputCompositeDataSet(compositeDataSetOut);
    return 0;
  }

  vtkArrayData* arrayDataOut = vtkArrayData::SafeDownCast(output);
  if (arrayDataOut)
  {
    this->ProcessOutputArrayData(arrayDataOut);
    return 0;
  }

  vtkTable* tableOut= vtkTable::SafeDownCast(output);
  if (tableOut)
  {
    this->ProcessOutputTable(tableOut);
    return 0;
  }

  vtkTree* treeOut = vtkTree::SafeDownCast(output);
  if (treeOut)
  {
    this->ProcessOutputTree(treeOut);
    return 0;
  }

  vtkGraph* graphOut = vtkGraph::SafeDownCast(output);
  if (graphOut)
  {
    this->ProcessOutputGraph(graphOut);
    return 0;
  }

  return 1;
}

void vtkRCalculatorFilter::PutTables(vtkStringArray* NamesOfRVars)
{
  rcfi->PutTableNames.SetNames(NamesOfRVars);
  this->Modified();
}

void vtkRCalculatorFilter::GetTables(vtkStringArray* NamesOfRvars)
{
  rcfi->GetTableNames.SetNames(NamesOfRvars);
  this->Modified();
}

void vtkRCalculatorFilter::PutTrees(vtkStringArray* NamesOfRvars)
{
  rcfi->PutTreeNames.SetNames(NamesOfRvars);
  this->Modified();
}

void vtkRCalculatorFilter::GetTrees(vtkStringArray* NamesOfRvars)
{
  rcfi->GetTreeNames.SetNames(NamesOfRvars);
  this->Modified();
}

int vtkRCalculatorFilter::HasMultipleGets()
{
  return rcfi->GetTreeNames.Count() + rcfi->GetTableNames.Count() > 1;

}

int vtkRCalculatorFilter::HasMultiplePuts()
{
  return rcfi->PutTreeNames.Count() + rcfi->PutTableNames.Count() > 1;
}
