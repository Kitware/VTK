/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatlabEngineFilter.cxx

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
#include "vtkMatlabEngineFilter.h"
#include "vtkMatlabEngineInterface.h"
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

#include <cstdlib>
#include <string>
#include <vector>
#include <vtksys/ios/sstream>
#include <sys/stat.h>

#define BUFFER_SIZE 32768

vtkStandardNewMacro(vtkMatlabEngineFilter);

class ArrNames
{
public:
  ArrNames(const char* Vname, const char* MatName)
    {
    this->VTKArrName = Vname;
    this->MatArrName = MatName;
    };

  std::string VTKArrName;
  std::string MatArrName;
};

class vtkMatlabEngineFilterInternals
{
public:
  std::vector<ArrNames> PutArrNames;
  std::vector<ArrNames> GetArrNames;
};

vtkMatlabEngineFilter::vtkMatlabEngineFilter()
{

  this->mengi = 0;
  this->MatlabScript = 0;
  this->MatlabFileScript = 0;
  this->ScriptFname = 0;
  this->EngineVisible = 1;
  this->TimeOutput = 1;
  this->EngineOutput = 1;
  this->BlockInfoOutput = 1;
  this->ScriptFname = 0;
  this->CurrentTime = 0;
  this->TimeRange = 0;
  this->TimeSteps = 0;
  this->BlockId = 0;
  this->NumBlocks = 0;

  this->OutputBuffer = new char[BUFFER_SIZE];

  this->mefi = new vtkMatlabEngineFilterInternals;

}

vtkMatlabEngineFilter::~vtkMatlabEngineFilter()
{

  delete this->mefi;

  if(this->mengi)
    {
    this->mengi->Delete();
    }

  delete [] this->MatlabScript;
  delete [] this->MatlabFileScript;
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

void vtkMatlabEngineFilter::PrintSelf(ostream& os, vtkIndent indent)
{

  this->Superclass::PrintSelf(os,indent);

  os << indent << "MatlabScript: "
     << (this->MatlabScript ? this->MatlabScript : "(none)") << endl;

  os << indent << "MatlabFileScript: "
     << (this->MatlabFileScript ? this->MatlabFileScript : "(none)") << endl;

  os << indent << "ScriptFname: "
     << (this->ScriptFname ? this->ScriptFname : "(none)") << endl;

  os << indent << "OutputBuffer: "
     << (this->OutputBuffer ? this->OutputBuffer : "(none)") << endl;

  os << indent << "EngineVisible: "
     << (this->EngineVisible ? "On" : "Off") << endl;

  os << indent << "EngineOutput: "
     << (this->EngineOutput ? "On" : "Off") << endl;

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
int vtkMatlabEngineFilter::ProcessRequest(
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
int vtkMatlabEngineFilter::RequestDataObject(
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
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->CopyInformationFromPipeline(info);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}



int vtkMatlabEngineFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{

  int ncells;
  int npoints;
  int result;
  std::vector<ArrNames>::iterator VectorIterator;
  vtkInformation* inpinfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outinfo = outputVector->GetInformationObject(0);
  vtkDataSetAttributes* CellinFD = 0;
  vtkDataSetAttributes* PointinFD = 0;

  vtkDataObject* input = inpinfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outinfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  if(!this->mengi)
    {
    this->mengi = vtkMatlabEngineInterface::New();
    }

  if(!this->mengi->EngineOpen())
    {
    vtkErrorMacro(<<"Matlab Engine not Open");
    return(1);
    }

  if(this->EngineVisible)
    {
    this->mengi->SetVisibleOn();
    }

  if(this->EngineOutput)
    {
    this->mengi->OutputBuffer(this->OutputBuffer, BUFFER_SIZE);
    }

  if(this->ScriptFname)
    {
    this->SetMatlabScriptFromFile(this->ScriptFname);
    }

  if( (!this->MatlabScript) && (!this->MatlabFileScript) )
    {
    return(1);
    }

  vtkDataArray* currentArray = 0;
  vtkArray* cArray = 0;

  vtkDataSetAttributes* CelloutFD = 0;
  vtkDataSetAttributes* PointoutFD = 0;

  vtkDataSet* dsinp = vtkDataSet::SafeDownCast(input);
  vtkDataSet* dsout = vtkDataSet::SafeDownCast(output);

  vtkGraph* graphinp = vtkGraph::SafeDownCast(input);
  vtkGraph* graphout = vtkGraph::SafeDownCast(output);

  vtkArrayData* adinp = vtkArrayData::SafeDownCast(input);
  vtkArrayData* adout = vtkArrayData::SafeDownCast(output);

  vtkCompositeDataSet* cdsinp = vtkCompositeDataSet::SafeDownCast(input);
  vtkCompositeDataSet* cdsout = vtkCompositeDataSet::SafeDownCast(output);

  vtkTable* tableinp = vtkTable::SafeDownCast(input);
  vtkTable* tableout = vtkTable::SafeDownCast(output);

  if(this->TimeOutput)
    {
    if( inpinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
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

      this->mengi->PutVtkDataArray("VTK_TIME_STEPS", this->TimeSteps);

      }

    if ( inpinfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()) )
      {
      if(!this->TimeRange)
        {
        this->TimeRange = vtkDoubleArray::New();
        this->TimeRange->SetNumberOfComponents(1);
        this->TimeRange->SetNumberOfTuples(2);
        }

      this->TimeRange->InsertValue(0,
            inpinfo->Get( vtkStreamingDemandDrivenPipeline::TIME_RANGE() )[0] );

      this->TimeRange->InsertValue(1,
            inpinfo->Get( vtkStreamingDemandDrivenPipeline::TIME_RANGE() )[1] );

      this->mengi->PutVtkDataArray("VTK_TIME_RANGE", this->TimeRange);

      }

    if ( input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()) )
      {
      if(!this->CurrentTime)
        {
        this->CurrentTime = vtkDoubleArray::New();
        this->CurrentTime->SetNumberOfComponents(1);
        this->CurrentTime->SetNumberOfTuples(1);
        }

      this->CurrentTime->InsertValue(0,
            input->GetInformation()->Get( vtkDataObject::DATA_TIME_STEP()));

      this->mengi->PutVtkDataArray("VTK_CURRENT_TIME", this->CurrentTime);
      }
    }

  if( dsinp  ) /* Data Set */
    {
    this->ProcessDataSet(dsinp, dsout);
    }
  else if( tableinp ) /* vtkTable input */
    {

    if(!tableinp->GetNumberOfColumns())
      {
      vtkErrorMacro(<<"Empty Input Table");
      return(1);
      }

    for(VectorIterator = this->mefi->PutArrNames.begin();
        VectorIterator != this->mefi->PutArrNames.end();
        VectorIterator++)
      {
      currentArray = 0;

      if(tableinp->GetColumnByName(VectorIterator->VTKArrName.c_str()))
        {
        currentArray = vtkDataArray::SafeDownCast(tableinp->GetColumnByName(VectorIterator->VTKArrName.c_str()));
        }

      if(currentArray)
        {
        result = this->mengi->PutVtkDataArray(VectorIterator->MatArrName.c_str(),
                                              currentArray );

        if(result)
          {
          vtkErrorMacro(<<"Cannot copy array to Matlab Engine");
          return(1);
          }

        }
      else
        {
        vtkErrorMacro(<<"Array Name not in Table " << VectorIterator->VTKArrName.c_str());
        return(1);
        }
      }

    if(this->MatlabScript)
      {
      result = this->mengi->EvalString(this->MatlabScript);

      if(result)
        {
        vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
        return(1);
        }

      if(this->EngineOutput)
        {
        cout << this->OutputBuffer << endl;
        }
      }

    if(this->MatlabFileScript)
      {
      result = this->mengi->EvalString(this->MatlabFileScript);

      if(result)
        {
        vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
        return(1);
        }

      if(this->EngineOutput)
        {
        cout << this->OutputBuffer << endl;
        }
      }

    for(VectorIterator = this->mefi->GetArrNames.begin();
        VectorIterator != this->mefi->GetArrNames.end();
        VectorIterator++)
      {
      currentArray = this->mengi->GetVtkDataArray(VectorIterator->MatArrName.c_str());

      if(!currentArray)
        {
        vtkErrorMacro(<<"Failed to get array from Matlab Engine");
        return(1);
        }

      int ntuples = currentArray->GetNumberOfTuples();

      if(ntuples != tableout->GetNumberOfRows())
        {
        vtkErrorMacro(<<"Array returned from Matlab Engine has wrong size");
        currentArray->Delete();
        return(1);
        }

      currentArray->SetName(VectorIterator->VTKArrName.c_str());

      tableout->AddColumn(currentArray);
      }

    }
  else if( cdsinp ) /* Composite Data Set input */
    {
    vtkCompositeDataIterator* iter = cdsinp->NewIterator();
    vtkCompositeDataIterator* oiter = cdsout->NewIterator();

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
      mengi->PutVtkDataArray("VTK_NUMBER_OF_BLOCKS", this->NumBlocks);
      }

    oiter->InitTraversal();
    int bid = 1;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      if(this->BlockInfoOutput)
        {
        this->BlockId->SetValue(0,bid);
        mengi->PutVtkDataArray("VTK_BLOCK_ID", this->BlockId);
        }

      vtkDataSet* inputDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      vtkDataSet* outputDS = vtkDataSet::SafeDownCast(oiter->GetCurrentDataObject());
      this->ProcessDataSet(inputDS, outputDS);
      oiter->GoToNextItem();
      bid++;
      }

    iter->Delete();
    }
  else if( graphinp ) /* Graph input */
    {
    CellinFD = graphinp->GetEdgeData();
    PointinFD = graphinp->GetVertexData();

    CelloutFD = graphout->GetEdgeData();
    PointoutFD = graphout->GetVertexData();

    ncells = graphinp->GetNumberOfEdges();
    npoints = graphinp->GetNumberOfVertices();

    if( (npoints < 1) && (ncells < 1) )
      {
      vtkErrorMacro(<<"Empty Data Set");
      return(1);
      }

    for(VectorIterator = this->mefi->PutArrNames.begin();
        VectorIterator != this->mefi->PutArrNames.end();
        VectorIterator++)
      {
      currentArray = PointinFD->GetArray(VectorIterator->VTKArrName.c_str());

      if(!currentArray)
        {
        currentArray = CellinFD->GetArray(VectorIterator->VTKArrName.c_str());
        }

      if(currentArray)
        {
        result = this->mengi->PutVtkDataArray(VectorIterator->MatArrName.c_str(),
                                              currentArray );

        if(result)
          {
          vtkErrorMacro(<<"Cannot copy array to Matlab Engine");
          return(1);
          }

        }
      else
        {
        vtkErrorMacro(<<"Array Name not in Data Set " << VectorIterator->VTKArrName.c_str());
        return(1);
        }
      }

    if(this->MatlabScript)
      {
      result = this->mengi->EvalString(this->MatlabScript);

      if(result)
        {
        vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
        return(1);
        }

      if(this->EngineOutput)
        {
        cout << this->OutputBuffer << endl;
        }
      }

    if(this->MatlabFileScript)
      {
      result = this->mengi->EvalString(this->MatlabFileScript);

      if(result)
        {
        vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
        return(1);
        }

      if(this->EngineOutput)
        {
        cout << this->OutputBuffer << endl;
        }
      }


    for(VectorIterator = this->mefi->GetArrNames.begin();
        VectorIterator != this->mefi->GetArrNames.end();
        VectorIterator++){
      currentArray = this->mengi->GetVtkDataArray(VectorIterator->MatArrName.c_str());

      if(!currentArray)
        {
        vtkErrorMacro(<<"Failed to get array from Matlab Engine");
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
        vtkErrorMacro(<<"Array returned from Matlab Engine has wrong size");
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

    }
  else if(adinp) /* Array Data */
    {
    for(VectorIterator = this->mefi->PutArrNames.begin();
        VectorIterator != this->mefi->PutArrNames.end();
        VectorIterator++)
      {
      int index = atoi(VectorIterator->VTKArrName.c_str());

      if( (index < 0) || (index >= adinp->GetNumberOfArrays()) )
        {
        vtkErrorMacro(<<"Array Index out of bounds " << index);
        return(1);
        }
        cArray = adinp->GetArray(index);

        result = this->mengi->PutVtkArray(VectorIterator->MatArrName.c_str(),
                                          cArray );

      if(result)
        {
        vtkErrorMacro(<<"Cannot copy array to Matlab Engine");
        return(1);
        }
      }

      if(this->MatlabScript)
        {
        result = this->mengi->EvalString(this->MatlabScript);

        if(result)
          {
          vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
          return(1);
          }

        if(this->EngineOutput)
          {
          cout << this->OutputBuffer << endl;
          }
        }

      if(this->MatlabFileScript)
        {
        result = this->mengi->EvalString(this->MatlabFileScript);

        if(result)
          {
          vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
          return(1);
          }

        if(this->EngineOutput)
          {
          cout << this->OutputBuffer << endl;
          }
        }

      for(VectorIterator = this->mefi->GetArrNames.begin();
          VectorIterator != this->mefi->GetArrNames.end();
          VectorIterator++)
        {
        cArray = this->mengi->GetVtkArray(VectorIterator->MatArrName.c_str());

        if(!cArray)
          {
          vtkErrorMacro(<<"Failed to get array from Matlab Engine");
          return(1);
          }

        adout->AddArray(cArray);
        }

    }
  else
    {
    vtkErrorMacro(<<"Filter does not handle input data type");
    return(1);
    }

  return(1);

}

int vtkMatlabEngineFilter::ProcessDataSet(vtkDataSet* dsinp, vtkDataSet* dsout)
{

  int ncells;
  int npoints;
  int result;
  vtkDataSetAttributes* CellinFD = 0;
  vtkDataSetAttributes* PointinFD = 0;
  vtkDataSetAttributes* CelloutFD = 0;
  vtkDataSetAttributes* PointoutFD = 0;
  std::vector<ArrNames>::iterator VectorIterator;
  vtkDataArray* currentArray = 0;

  CellinFD = dsinp->GetCellData();
  PointinFD = dsinp->GetPointData();

  CelloutFD = dsout->GetCellData();
  PointoutFD = dsout->GetPointData();

  ncells = dsinp->GetNumberOfCells();
  npoints = dsinp->GetNumberOfPoints();

  if( (ncells < 1) && (npoints < 1) )
    {
    vtkErrorMacro(<<"Empty Data Set");
    return(1);
    }

  for(VectorIterator = this->mefi->PutArrNames.begin();
      VectorIterator != this->mefi->PutArrNames.end();
      VectorIterator++)
    {
    currentArray = PointinFD->GetArray(VectorIterator->VTKArrName.c_str());

    if(!currentArray)
      {
      currentArray = CellinFD->GetArray(VectorIterator->VTKArrName.c_str());
      }

    if(currentArray)
      {
      result = this->mengi->PutVtkDataArray(VectorIterator->MatArrName.c_str(),
                                            currentArray );

      if(result)
        {
        vtkErrorMacro(<<"Cannot copy array to Matlab Engine");
        return(1);
        }

      }
    else
      {
      vtkErrorMacro(<<"Array Name not in Data Set " << VectorIterator->VTKArrName.c_str());
      return(1);
      }
    }

    if(this->MatlabScript)
      {
      result = this->mengi->EvalString(this->MatlabScript);

      if(result)
        {
        vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
        return(1);
        }

      if(this->EngineOutput)
        {
        cout << this->OutputBuffer << endl;
        }
      }

    if(this->MatlabFileScript)
      {
      result = this->mengi->EvalString(this->MatlabFileScript);

      if(result)
        {
        vtkErrorMacro(<<"Failed to evaluate command string on Matlab Engine");
        return(1);
        }

        if(this->EngineOutput)
        {
        cout << this->OutputBuffer << endl;
        }
      }

    for(VectorIterator = this->mefi->GetArrNames.begin();
        VectorIterator != this->mefi->GetArrNames.end();
        VectorIterator++)
      {
      currentArray = this->mengi->GetVtkDataArray(VectorIterator->MatArrName.c_str());

      if(!currentArray)
        {
        vtkErrorMacro(<<"Failed to get array from Matlab Engine");
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
        vtkErrorMacro(<<"Array returned from Matlab Engine has wrong size");
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

int vtkMatlabEngineFilter::SetMatlabScriptFromFile(const char* fname)
{

  FILE *fp;
  long len;

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

    if(this->MatlabFileScript)
      {
      delete [] this->MatlabFileScript;
      this->MatlabFileScript = 0;
      }

    this->MatlabFileScript = new char[len+1];
    fread(this->MatlabFileScript,len,1,fp);
    this->MatlabFileScript[len] = '\0';
    fclose(fp);

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

void vtkMatlabEngineFilter::PutArray(const char* NameOfVTKArray, const char* NameOfMatVar)
{

  if( NameOfVTKArray && (strlen(NameOfVTKArray) > 0) && NameOfMatVar && (strlen(NameOfMatVar) > 0) )
    {
    mefi->PutArrNames.push_back(ArrNames(NameOfVTKArray, NameOfMatVar));
    this->Modified();
    }

}

void vtkMatlabEngineFilter::GetArray(const char* NameOfVTKArray, const char* NameOfMatVar)
{

  if( NameOfVTKArray && (strlen(NameOfVTKArray) > 0) && NameOfMatVar && (strlen(NameOfMatVar) > 0) )
    {
    mefi->GetArrNames.push_back(ArrNames(NameOfVTKArray, NameOfMatVar));
    this->Modified();
    }

}

void vtkMatlabEngineFilter::RemoveAllPutVariables()
{

  mefi->PutArrNames.clear();
  this->Modified();

}

void vtkMatlabEngineFilter::RemoveAllGetVariables()
{

  mefi->GetArrNames.clear();
  this->Modified();

}


