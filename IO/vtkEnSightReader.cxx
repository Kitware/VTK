/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEnSightReader.h"

#include "vtkDataArrayCollection.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdListCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"

#include <vtkstd/string>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkEnSightReader, "1.56.2.1");

//----------------------------------------------------------------------------
typedef vtkstd::vector< vtkSmartPointer<vtkIdList> > vtkEnSightReaderCellIdsTypeBase;
class vtkEnSightReaderCellIdsType: public vtkEnSightReaderCellIdsTypeBase {};

//----------------------------------------------------------------------------
vtkEnSightReader::vtkEnSightReader()
{
  this->MeasuredFileName = NULL;
  this->MatchFileName = NULL;

  this->IS = NULL;
  
  this->VariableMode = -1;
  
  this->UnstructuredPartIds = vtkIdList::New();
  this->CellIds = NULL;
  
  this->VariableFileNames = NULL;
  this->ComplexVariableFileNames = NULL;
  
  this->VariableDescriptions = NULL;
  this->ComplexVariableDescriptions = NULL;

  this->VariableTimeSetIds = vtkIdList::New();
  this->ComplexVariableTimeSetIds = vtkIdList::New();
  this->VariableFileSetIds = vtkIdList::New();
  this->ComplexVariableFileSetIds = vtkIdList::New();

  this->TimeSetFileNameNumbers = vtkIdListCollection::New();
  this->TimeSetsWithFilenameNumbers = vtkIdList::New();
  this->TimeSets = vtkDataArrayCollection::New();
  this->FileSetFileNameNumbers = vtkIdListCollection::New();
  this->FileSetsWithFilenameNumbers = vtkIdList::New();
  this->FileSetNumberOfSteps = vtkIdListCollection::New();

  this->TimeSetIds = vtkIdList::New();
  this->FileSets = vtkIdList::New();
  
  this->GeometryTimeSet = 1;
  this->GeometryFileSet = 1;
  this->MeasuredTimeSet = 1;
  this->MeasuredFileSet = 1;
  
  this->UseTimeSets = 0;
  this->UseFileSets = 0;

  this->GeometryTimeValue = -1;
  this->MeasuredTimeValue = -1;
  
  this->NumberOfGeometryParts = 0;

  this->NumberOfMeasuredPoints = 0;
  
  this->OutputsAreValid = 1;
  this->InitialRead = 1;
  this->NumberOfNewOutputs = 0;
}

//----------------------------------------------------------------------------
vtkEnSightReader::~vtkEnSightReader()
{
  int i;

  if (this->CellIds)
    {
    delete this->CellIds;
    this->CellIds = NULL;
    }
  
  if (this->MeasuredFileName)
    {
    delete [] this->MeasuredFileName;
    this->MeasuredFileName = NULL;
    }
  if (this->MatchFileName)
    {
    delete [] this->MatchFileName;
    this->MatchFileName = NULL;
    }

  if (this->NumberOfVariables > 0)
    {
    for (i = 0; i < this->NumberOfVariables; i++)
      {
      delete [] this->VariableFileNames[i];
      }
    delete [] this->VariableFileNames;
    this->VariableFileNames = NULL;
    }

  if (this->NumberOfComplexVariables > 0)
    {
    for (i = 0; i < this->NumberOfComplexVariables*2; i++)
      {
      delete [] this->ComplexVariableFileNames[i];
      }
    delete [] this->ComplexVariableFileNames;
    this->ComplexVariableFileNames = NULL;
    }
  
  this->UnstructuredPartIds->Delete();
  this->UnstructuredPartIds = NULL;  
    
  this->VariableTimeSetIds->Delete();
  this->VariableTimeSetIds = NULL;
  this->ComplexVariableTimeSetIds->Delete();
  this->ComplexVariableTimeSetIds = NULL;
  this->VariableFileSetIds->Delete();
  this->VariableFileSetIds = NULL;
  this->ComplexVariableFileSetIds->Delete();
  this->ComplexVariableFileSetIds = NULL;
  
  this->TimeSetFileNameNumbers->Delete();
  this->TimeSetFileNameNumbers = NULL;
  this->TimeSetsWithFilenameNumbers->Delete();
  this->TimeSetsWithFilenameNumbers = NULL;
  this->TimeSets->Delete();
  this->TimeSets = NULL;
  this->FileSetFileNameNumbers->Delete();
  this->FileSetFileNameNumbers = NULL;
  this->FileSetsWithFilenameNumbers->Delete();
  this->FileSetsWithFilenameNumbers = NULL;
  this->FileSetNumberOfSteps->Delete();
  this->FileSetNumberOfSteps = NULL;
  
  this->TimeSetIds->Delete();
  this->TimeSets = NULL;
  this->FileSets->Delete();
  this->FileSets = NULL;
}

//----------------------------------------------------------------------------
void vtkEnSightReader::Execute()
{
  vtkDebugMacro("In execute ");

  int i, timeSet, fileSet, timeStep, timeStepInFile, fileNum;
  vtkDataArray *times;
  vtkIdList *numStepsList, *filenameNumbers;
  float newTime;
  int numSteps;
  char* fileName;
  int filenameNum;
  
  if ( ! this->CaseFileRead)
    {
    vtkErrorMacro("error reading case file");
    return;
    }
  
  this->OutputsAreValid = 1;
  this->NumberOfNewOutputs = 0;
  this->NumberOfGeometryParts = 0;
  if (this->GeometryFileName)
    {
    timeStep = timeStepInFile = 1;
    fileNum = 1;
    fileName = new char[strlen(this->GeometryFileName) + 1];
    strcpy(fileName, this->GeometryFileName);
    
    if (this->UseTimeSets)
      {
      timeSet = this->TimeSetIds->IsId(this->GeometryTimeSet);
      if (timeSet >= 0)
        {
        times = this->TimeSets->GetItem(timeSet);
        this->GeometryTimeValue = times->GetComponent(0, 0);
        for (i = 1; i < times->GetNumberOfTuples(); i++)
          {
          newTime = times->GetComponent(i, 0);
          if (newTime <= this->TimeValue && newTime > this->GeometryTimeValue)
            {
            this->GeometryTimeValue = newTime;
            timeStep++;
            timeStepInFile++;
            }
          }
        if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0)
          {
          int collectionNum = this->TimeSetsWithFilenameNumbers->
            IsId(this->GeometryTimeSet);
          if (collectionNum > -1)
            {
            filenameNumbers =
              this->TimeSetFileNameNumbers->GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(timeStep-1);
            this->ReplaceWildcards(fileName, filenameNum);
            }
          }
      
        // There can only be file sets if there are also time sets.
        if (this->UseFileSets)
          {
          fileSet = this->FileSets->IsId(this->GeometryFileSet);
          numStepsList = (vtkIdList*)this->FileSetNumberOfSteps->
            GetItemAsObject(fileSet);
          
          if (timeStep > numStepsList->GetId(0))
            {
            numSteps = numStepsList->GetId(0);
            timeStepInFile -= numSteps;
            for (i = 1; i < numStepsList->GetNumberOfIds(); i++)
              {
              numSteps += numStepsList->GetId(i);
              if (timeStep > numSteps)
                {
                fileNum++;
                timeStepInFile -= numStepsList->GetId(i);
                }
              }
            }
          if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0)
            {
            int collectionNum = this->FileSetsWithFilenameNumbers->
              IsId(this->GeometryFileSet);
            if (collectionNum > -1)
              {
              filenameNumbers = this->TimeSetFileNameNumbers->
                GetItem(collectionNum);
              filenameNum = filenameNumbers->GetId(fileNum-1);
              this->ReplaceWildcards(fileName, filenameNum);
              }
            }
          }
        }
      }
    
    if (!this->ReadGeometryFile(fileName, timeStepInFile))
      {
      vtkErrorMacro("error reading geometry file");
      delete [] fileName;
      return;
      }
    
    delete [] fileName;
    }
  if (this->MeasuredFileName)
    {
    timeStep = timeStepInFile = 1;
    fileNum = 1;
    fileName = new char[strlen(this->MeasuredFileName) + 1];
    strcpy(fileName, this->MeasuredFileName);
    
    if (this->UseTimeSets)
      {
      timeSet = this->TimeSetIds->IsId(this->MeasuredTimeSet);
      if (timeSet >= 0)
        {
        times = this->TimeSets->GetItem(timeSet);
        this->MeasuredTimeValue = times->GetComponent(0, 0);
        for (i = 1; i < times->GetNumberOfTuples(); i++)
          {
          newTime = times->GetComponent(i, 0);
          if (newTime <= this->TimeValue && newTime > this->MeasuredTimeValue)
            {
            this->MeasuredTimeValue = newTime;
            timeStep++;
            timeStepInFile++;
            }
          }
        if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0)
          {
          int collectionNum = this->TimeSetsWithFilenameNumbers->
            IsId(this->MeasuredTimeSet);
          if (collectionNum > -1)
            {
            filenameNumbers = this->TimeSetFileNameNumbers->
              GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(timeStep-1);
            this->ReplaceWildcards(fileName, filenameNum);
            }
          }
        
        // There can only be file sets if there are also time sets.
        if (this->UseFileSets)
          {
          fileSet = this->FileSets->IsId(this->MeasuredFileSet);
          numStepsList = (vtkIdList*)this->FileSetNumberOfSteps->
            GetItemAsObject(fileSet);
          
          if (timeStep > numStepsList->GetId(0))
            {
            numSteps = numStepsList->GetId(0);
            timeStepInFile -= numSteps;
            for (i = 1; i < numStepsList->GetNumberOfIds(); i++)
              {
              numSteps += numStepsList->GetId(i);
              if (timeStep > numSteps)
                {
                fileNum++;
                timeStepInFile -= numStepsList->GetId(i);
                }
              }
            }
          if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0)
            {
            int collectionNum = this->FileSetsWithFilenameNumbers->
              IsId(this->MeasuredFileSet);
            if (collectionNum > -1)
              {
              filenameNumbers = this->TimeSetFileNameNumbers->
                GetItem(fileSet);
              filenameNum = filenameNumbers->GetId(fileNum-1);
              this->ReplaceWildcards(fileName, filenameNum);
              }
            }
          }
        }
      }
    if (!this->ReadMeasuredGeometryFile(fileName, timeStepInFile))
      {
      vtkErrorMacro("error reading measured geometry file");
      delete [] fileName;
      return;
      }
    delete [] fileName;
    }
  if (!this->CheckOutputConsistency())
    {
    for (i = 0; i < this->NumberOfOutputs; i++)
      {
      this->GetOutput(i)->Initialize();
      }
    return;
    }

  if ((this->NumberOfVariables + this->NumberOfComplexVariables) > 0)
    {
    if (!this->ReadVariableFiles())
      {
      vtkErrorMacro("error reading variable files");
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkEnSightReader::Update()
{
  vtkDebugMacro("In update");;
  int i;
  
  this->UpdateInformation();
  this->UpdateData(0);
  
  for (i = 0; i < this->GetNumberOfOutputs(); i++)
    {
    if ( this->GetOutput(i) )
      {
      this->GetOutput(i)->DataHasBeenGenerated();
      }
    }
}
//----------------------------------------------------------------------------
void vtkEnSightReader::ExecuteInformation()
{
  vtkDebugMacro("In execute information");
  this->CaseFileRead = this->ReadCaseFile();
}

//----------------------------------------------------------------------------
int vtkEnSightReader::ReadCaseFile()
{
  char line[256], formatLine[256];
  char subLine[256], subLine2[256];
  int stringRead;
  int timeSet, fileSet, numTimeSteps, i, filenameNum, increment, lineRead;
  float timeStep;
  
  // Initialize
  //
  if (!this->CaseFileName)
    {
    vtkErrorMacro("A CaseFileName must be specified.");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
      {
      sfilename += "/";
      }
    sfilename += this->CaseFileName;
    vtkDebugMacro("full path to case file: " << sfilename.c_str());
    }
  else
    {
    sfilename = this->CaseFileName;
    }
  
  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
    }

  this->TimeSets->RemoveAllItems();

  for (i = 0; i < this->NumberOfVariables; i++)
    {
    delete [] this->VariableFileNames[i];
    this->VariableFileNames[i] = NULL;
    delete [] this->VariableDescriptions[i];
    this->VariableDescriptions[i] = NULL;
    }
  delete [] this->VariableFileNames;
  this->VariableFileNames = NULL;
  delete [] this->VariableDescriptions;
  this->VariableDescriptions = NULL;
  delete [] this->VariableTypes;
  this->VariableTypes = NULL;
  
  for (i = 0; i < this->NumberOfComplexVariables; i++)
    {
    delete [] this->ComplexVariableFileNames[2*i];
    this->ComplexVariableFileNames[2*i] = NULL;
    delete [] this->ComplexVariableFileNames[2*i+1];
    this->ComplexVariableFileNames[2*i+1] = NULL;
    delete [] this->ComplexVariableDescriptions[i];
    this->ComplexVariableDescriptions[i] = NULL;
    }
  delete [] this->ComplexVariableFileNames;
  this->ComplexVariableFileNames = NULL;
  delete [] this->ComplexVariableDescriptions;
  this->ComplexVariableDescriptions = NULL;
  delete [] this->ComplexVariableTypes;
  this->ComplexVariableTypes = NULL;
  
  this->NumberOfVariables = 0;
  this->NumberOfComplexVariables = 0;
  
  this->ReadNextDataLine(line);
  
  if (strncmp(line, "FORMAT", 6) == 0)
    {
    // found the FORMAT section
    vtkDebugMacro("*** FORMAT section");
    this->ReadNextDataLine(line);
    
    stringRead = sscanf(line, " %*s %*s %s", subLine);
    if (stringRead == 1)
      {
      if (strcmp(subLine, "gold") == 0 &&
          strcmp(this->GetClassName(), "vtkEnSight6Reader") == 0)
        {
        // The class is vtkEnSight6Reader, but the case file says "gold".
        vtkErrorMacro("This is not an EnSight6 file.");
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
      }
    else
      {
      if (strcmp(this->GetClassName(), "vtkEnSightGoldReader") == 0)
        {
        // The class is vtkEnSightGoldReader, but the case file does
        // not say "gold".
        vtkErrorMacro("This is not an EnSight Gold file.");
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
      }
    }
  
  // We know how many lines to read in the FORMAT section, so we haven't read
  // the "GEOMETRY" line yet.
  this->ReadNextDataLine(line);
  if (strncmp(line, "GEOMETRY", 8) == 0)
    {
    // found the GEOMETRY section
    vtkDebugMacro("*** GEOMETRY section");
    
    // There will definitely be a "model" line.  There may also be "measured"
    // and "match" lines.
    while(this->ReadNextDataLine(line) != 0 &&
          strncmp(line, "m", 1) == 0)
      {
      if (strncmp(line, "model:", 6) == 0)
        {
        if (sscanf(line, " %*s %d%*[ \t]%d%*[ \t]%s", &timeSet, &fileSet, subLine) == 3)
          {
          this->GeometryTimeSet = timeSet;
          this->GeometryFileSet = fileSet;
          this->SetGeometryFileName(subLine);
          vtkDebugMacro(<<this->GetGeometryFileName());
          }
        else if (sscanf(line, " %*s %d%*[ \t]%s", &timeSet, subLine) == 2)
          {
          this->GeometryTimeSet = timeSet;
          this->SetGeometryFileName(subLine);
          vtkDebugMacro(<<this->GetGeometryFileName());
          }
        else if (sscanf(line, " %*s %s", subLine) == 1)
          {
          this->SetGeometryFileName(subLine);
          vtkDebugMacro(<<this->GetGeometryFileName());
          }
        }
      else if (strncmp(line, "measured:", 9) == 0)
        {
        if (sscanf(line, " %*s %d%*[ \t]%d%*[ \t]%s", &timeSet, &fileSet, subLine) == 3)
          {
          this->MeasuredTimeSet = timeSet;
          this->MeasuredFileSet = fileSet;
          this->SetMeasuredFileName(subLine);
          vtkDebugMacro(<< this->GetMeasuredFileName());
          }
        else if (sscanf(line, " %*s %d%*[ \t]%s", &timeSet, subLine) == 2)
          {
          this->MeasuredTimeSet = timeSet;
          this->SetMeasuredFileName(subLine);
          vtkDebugMacro(<< this->GetMeasuredFileName());
          }
        else if (sscanf(line, " %*s %s", subLine) == 1)
          {
          this->SetMeasuredFileName(subLine);
          vtkDebugMacro(<< this->GetMeasuredFileName());
          }
        }
      else if (strncmp(line, "match:", 6) == 0)
        {
        sscanf(line, " %*s %s", subLine);
        this->SetMatchFileName(subLine);
        vtkDebugMacro(<< this->GetMatchFileName());
        }
      }
    }
  
  if (strncmp(line, "VARIABLE", 8) == 0)
    {
    // found the VARIABLE section
    vtkDebugMacro(<< "*** VARIABLE section");

    this->NumberOfScalarsPerNode = 0;
    this->NumberOfVectorsPerNode = 0;
    this->NumberOfTensorsSymmPerNode = 0;
    this->NumberOfScalarsPerElement = 0;
    this->NumberOfVectorsPerElement = 0;
    this->NumberOfTensorsSymmPerElement = 0;
    this->NumberOfScalarsPerMeasuredNode = 0;
    this->NumberOfVectorsPerMeasuredNode = 0;
    this->NumberOfComplexScalarsPerNode = 0;
    this->NumberOfComplexVectorsPerNode = 0;
    this->NumberOfComplexScalarsPerElement = 0;
    this->NumberOfComplexVectorsPerElement = 0;
      
    while(this->ReadNextDataLine(line) != 0 &&
          strncmp(line, "TIME", 4) != 0 &&
          strncmp(line, "FILE", 4) != 0)
      {
      if (strncmp(line, "constant", 8) == 0)
        {
        vtkDebugMacro(<< line);
        }
      else if (strncmp(line, "scalar", 6) == 0)
        {
        sscanf(line, " %*s %*s %s", subLine);
        if (strcmp(subLine, "node:") == 0)
          {
          vtkDebugMacro("scalar per node");
          this->VariableMode = vtkEnSightReader::SCALAR_PER_NODE;
          if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfScalarsPerNode++;
          }
        else if (strcmp(subLine, "element:") == 0)
          {
          vtkDebugMacro("scalar per element");
          this->VariableMode = vtkEnSightReader::SCALAR_PER_ELEMENT;
          if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfScalarsPerElement++;
          }
        else if (strcmp(subLine, "measured") == 0)
          {
          vtkDebugMacro("scalar per measured node");
          this->VariableMode = vtkEnSightReader::SCALAR_PER_MEASURED_NODE;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                          subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfScalarsPerMeasuredNode++;
          }
        this->AddVariableFileName(subLine);
        this->NumberOfVariables++;
        }
      else if (strncmp(line, "vector", 6) == 0)
        {
        sscanf(line, " %*s %*s %s", subLine);
        if (strcmp(subLine, "node:") == 0)
          {
          vtkDebugMacro("vector per node");
          this->VariableMode = vtkEnSightReader::VECTOR_PER_NODE;
          if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfVectorsPerNode++;
          }
        else if (strcmp(subLine, "element:") == 0)
          {
          vtkDebugMacro("vector per element");
          this->VariableMode = vtkEnSightReader::VECTOR_PER_ELEMENT;
          if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfVectorsPerElement++;
          }
        else if (strcmp(subLine, "measured") == 0)
          {
          vtkDebugMacro("vector per measured node");
          this->VariableMode = vtkEnSightReader::VECTOR_PER_MEASURED_NODE;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                          subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfVectorsPerMeasuredNode++;
          }
        this->AddVariableFileName(subLine);
        this->NumberOfVariables++;
        }
      else if (strncmp(line, "tensor", 6) == 0)
        {
        // According to EnSight documentation tensor entry should be of the form:
        // tensor symm per node/element
        // but it seems like you can also find:
        // tensor per node/element
        // Let handle this case here:
        char symm[10];
        char per[10];
        if( sscanf(line, " %*s %s %s %s", symm, per, subLine) != 3 )
          {
          vtkErrorMacro( "Error while reading: " << line );
          }
        if (!(strcmp(symm, "symm") == 0 && strcmp(per, "per") == 0))
          {
          if( sscanf(line, " %*s %s %s", per, subLine) != 2 )
            {
            vtkErrorMacro( "Error while reading: " << line );
            }
          if (strcmp(per, "per") == 0)
            {
            //Not valid file but seems alright, only 'symm' is missing
            vtkWarningMacro( "Looks almost like a valid case file, continuing" );
            }
          else
            {
            vtkErrorMacro("Trouble reading: " << line );
            }
          }
        if (strcmp(subLine, "node:") == 0)
          {
          vtkDebugMacro("tensor symm per node");
          this->VariableMode = vtkEnSightReader::TENSOR_SYMM_PER_NODE;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                          subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfTensorsSymmPerNode++;
          }
        else if (strcmp(subLine, "element:") == 0)
          {
          vtkDebugMacro("tensor symm per element");
          this->VariableMode = vtkEnSightReader::TENSOR_SYMM_PER_ELEMENT;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                     subLine) == 3)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->VariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                          subLine) == 2)
            {
            this->VariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->VariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            }
          this->AddVariableType();
          this->NumberOfTensorsSymmPerElement++;
          }
        else
          {
          vtkErrorMacro("Unknow type, faulty line was:" << line );
          }
        this->AddVariableFileName(subLine);
        this->NumberOfVariables++;
        }
      else if (strncmp(line, "complex", 6) == 0)
        {
        sscanf(line, " %*s %s", subLine);
        if (strcmp(subLine, "scalar") == 0)
          {
          sscanf(line, " %*s %*s %*s %s", subLine);
          if (strcmp(subLine, "node:") == 0)
            {
            vtkDebugMacro("complex scalar per node");
            this->VariableMode = vtkEnSightReader::COMPLEX_SCALAR_PER_NODE;
            if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                       subLine) == 3)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->ComplexVariableFileSetIds->InsertNextId(fileSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                            subLine) == 2)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(1);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              }
            this->AddVariableType();
            this->NumberOfComplexScalarsPerNode++;
            }
          else if (strcmp(subLine, "element:") == 0)
            {
            vtkDebugMacro("complex scalar per element");
            this->VariableMode = vtkEnSightReader::COMPLEX_SCALAR_PER_ELEMENT;
            if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                       subLine) == 3)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->ComplexVariableFileSetIds->InsertNextId(fileSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                            subLine) == 2)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(1);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              }
            this->AddVariableType();
            this->NumberOfComplexScalarsPerElement++;
            }
          }
        else if (strcmp(subLine, "vector") == 0)
          {
          sscanf(line, " %*s %*s %*s %s", subLine);
          if (strcmp(subLine, "node:") == 0)
            {
            vtkDebugMacro("complex vector per node");
            this->VariableMode = vtkEnSightReader::COMPLEX_VECTOR_PER_NODE;
            if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                       subLine) == 3)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->ComplexVariableFileSetIds->InsertNextId(fileSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                            subLine) == 2)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(1);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              }
            this->AddVariableType();
            this->NumberOfComplexVectorsPerNode++;
            }
          else if (strcmp(subLine, "element:") == 0)
            {
            vtkDebugMacro("complex vector per element");
            this->VariableMode = vtkEnSightReader::COMPLEX_VECTOR_PER_ELEMENT;
            if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet,
                       subLine) == 3)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->ComplexVariableFileSetIds->InsertNextId(fileSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet,
                            subLine) == 2)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->ComplexVariableTimeSetIds->InsertNextId(1);
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              }
            this->AddVariableType();
            this->NumberOfComplexVectorsPerElement++;
            }
          }
        this->AddVariableFileName(subLine, subLine2);
        this->NumberOfComplexVariables++;
        }
      else
        {
        vtkErrorMacro("invalid VARIABLE line: " << line);
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
      }
    }
  
  if (strncmp(line, "TIME", 4) == 0)
    {
    // found TIME section
    int firstTimeStep = 1;
    
    this->UseTimeSetsOn();
    while(this->ReadNextDataLine(line) != 0 &&
          strncmp(line, "FILE", 4) != 0)
      {
      sscanf(line, "%*s %*s %d", &timeSet);
      this->TimeSetIds->InsertNextId(timeSet);
      this->ReadNextDataLine(line);
      sscanf(line, "%*s %*s %*s %d", &numTimeSteps);
      this->ReadNextDataLine(line);
      if (strncmp(line, "filename", 8) == 0)
        {
        vtkIdList *filenameNumbers = vtkIdList::New();
        this->TimeSetsWithFilenameNumbers->InsertNextId(timeSet);
        sscanf(line, "%*s %s", subLine);
        if (strncmp(subLine, "numbers", 7) == 0)
          {
          strcpy(formatLine, "%*s %*s");
          strcpy(subLine, "%*s %*s");
          for (i = 0; i < numTimeSteps; i++)
            {
            strcat(formatLine, " %d");
            sscanf(line, formatLine, &filenameNum);
            filenameNumbers->InsertNextId(filenameNum);
            strcat(subLine, " %*d");
            strcpy(formatLine, subLine);
            }
          }
        else
          {
          sscanf(line, "%*s %*s %*s %d", &filenameNum);
          this->ReadNextDataLine(line);
          sscanf(line, "%*s %*s %d", &increment);
          for (i = 0; i < numTimeSteps; i++)
            {
            filenameNumbers->InsertNextId(filenameNum + i*increment);
            }
          }
        this->TimeSetFileNameNumbers->AddItem(filenameNumbers);
        filenameNumbers->Delete();
        this->ReadLine(line);
        }
      vtkFloatArray *timeValues = vtkFloatArray::New();
      timeValues->SetNumberOfComponents(1);
      timeValues->SetNumberOfTuples(numTimeSteps);
      strcpy(formatLine, "%*s %*s");
      strcpy(subLine, "%*s %*s");
      for (i = 0; i < numTimeSteps; i++)
        {
        strcat(formatLine, " %f");
        if (sscanf(line, formatLine, &timeStep) != 1)
          {
          this->ReadNextDataLine(line);
          strcpy(formatLine, " %f");
          strcpy(subLine, "");
          sscanf(line, formatLine, &timeStep);
          }
        if (firstTimeStep)
          {
          this->MinimumTimeValue = timeStep;
          this->MaximumTimeValue = timeStep;
          firstTimeStep = 0;
          // Set this as default TimeValue.
          if(!this->TimeValueInitialized)
            {
            this->SetTimeValue(timeStep);
            }
          }
        else
          {
          if (timeStep < this->MinimumTimeValue)
            {
            this->MinimumTimeValue = timeStep;
            }
          else if (timeStep > this->MaximumTimeValue)
            {
            this->MaximumTimeValue = timeStep;
            }
          }
        timeValues->SetComponent(i, 0, timeStep);
        strcat(subLine, " %*f");
        strcpy(formatLine, subLine);
        }
      this->TimeSets->AddItem(timeValues);
      timeValues->Delete();
      }
    }
  
  if (strncmp(line, "FILE", 4) == 0)
    {
    // found FILE section
    this->UseFileSetsOn();
    lineRead = this->ReadNextDataLine(line);
    while (lineRead != 0)
      {
      vtkIdList *filenameNums = vtkIdList::New();
      vtkIdList *numSteps = vtkIdList::New();
      sscanf(line, "%*s %*s %d", &fileSet);
      this->FileSets->InsertNextId(fileSet);
      lineRead = this->ReadNextDataLine(line);
      if (strncmp(line, "filename", 8) == 0)
        {
        this->FileSetsWithFilenameNumbers->InsertNextId(fileSet);
        while (lineRead != 0 && strncmp(line, "filename", 8) == 0)
          {
          sscanf(line, "%*s %*s %d", &filenameNum);
          filenameNums->InsertNextId(filenameNum);
          this->ReadNextDataLine(line);
          sscanf(line, "%*s %*s %*s %d", &numTimeSteps);
          numSteps->InsertNextId(numTimeSteps);
          lineRead = this->ReadNextDataLine(line);
          }
        this->FileSetFileNameNumbers->AddItem(filenameNums);
        }
      else
        {
        sscanf(line, "%*s %*s %*s %d", &numTimeSteps);
        numSteps->InsertNextId(numTimeSteps);
        lineRead = this->ReadNextDataLine(line);
        }
      
      this->FileSetNumberOfSteps->AddItem(numSteps);
      
      filenameNums->Delete();
      numSteps->Delete();
      }
    }

  delete this->IS;
  this->IS = NULL;
  
  // Fill data array selection objects with these arrays.
  this->SetDataArraySelectionSetsFromVariables();
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightReader::ReadVariableFiles()
{
  int i, j;
  char description[256];
  int timeSet, fileSet, timeStep, timeStepInFile, numSteps;
  vtkDataArray *times;
  float newTime;
  vtkIdList *numStepsList, *filenameNumbers;
  //int fileNum;
  int validTime, filenameNum;
  char* fileName, *fileName2;
  
  for (i = 0; i < this->NumberOfVariables; i++)
    {
    switch (this->VariableTypes[i])
      {
      case SCALAR_PER_NODE:
      case VECTOR_PER_NODE:
      case TENSOR_SYMM_PER_NODE:
      case SCALAR_PER_MEASURED_NODE:
      case VECTOR_PER_MEASURED_NODE:
        if(!this->GetPointArrayStatus(this->VariableDescriptions[i]))
          {
          continue;
          }
        break;
      case SCALAR_PER_ELEMENT:
      case VECTOR_PER_ELEMENT:
      case TENSOR_SYMM_PER_ELEMENT:
        if(!this->GetCellArrayStatus(this->VariableDescriptions[i]))
          {
          continue;
          }
        break;
      }
    
    timeStep = 0;
    timeStepInFile = 1;
    //fileNum = 1;
    validTime = 1;
    fileName = new char[strlen(this->VariableFileNames[i]) + 1];
    strcpy(fileName, this->VariableFileNames[i]);
    if (this->UseTimeSets)
      {
      validTime = 0;
      timeSet = this->VariableTimeSetIds->GetId(i);
      times = this->TimeSets->GetItem(this->TimeSetIds->IsId(timeSet));
      for (j = 0; j < times->GetNumberOfTuples(); j++)
        {
        newTime = times->GetComponent(j, 0);
        if (newTime <= this->TimeValue)
          {
          timeStep++;
          if (this->VariableTypes[i] == SCALAR_PER_MEASURED_NODE ||
               this->VariableTypes[i] == VECTOR_PER_MEASURED_NODE)
            {
            if (newTime >= this->MeasuredTimeValue ||
                this->MeasuredTimeSet == -1)
              {
              validTime = 1;
              }
            }
          else if (newTime >= this->GeometryTimeValue ||
                   this->GeometryTimeSet == -1)
            {
            validTime = 1;
            }
          }
        }
      if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0 &&
          validTime)
        {
        int collectionNum = this->TimeSetsWithFilenameNumbers->IsId(timeSet);
        if (collectionNum > -1)
          {
          filenameNumbers = this->TimeSetFileNameNumbers->
            GetItem(collectionNum);
          filenameNum = filenameNumbers->GetId(timeStep-1);
          this->ReplaceWildcards(fileName, filenameNum);
          }
        }
      
      // There can only be file sets if there are also time sets.
      if (this->UseFileSets)
        {
        timeStepInFile = timeStep;
        fileSet = this->VariableFileSetIds->GetId(i);
        numStepsList = (vtkIdList*)this->FileSetNumberOfSteps->
          GetItemAsObject(this->FileSets->IsId(fileSet));

        if (timeStep > numStepsList->GetId(0))
          {
          numSteps = numStepsList->GetId(0);
          timeStepInFile -= numSteps;
          for (i = 1; i < numStepsList->GetNumberOfIds(); i++)
            {
            numSteps += numStepsList->GetId(i);
            if (timeStep > numSteps)
              {
              //fileNum++;
              timeStepInFile -= numStepsList->GetId(i);
              }
            }
          }
        if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0 &&
            validTime)
          {
          int collectionNum = this->FileSetsWithFilenameNumbers->IsId(fileSet);
          if (collectionNum > -1)
            {
            filenameNumbers = this->TimeSetFileNameNumbers->
              GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(timeStep-1);
            this->ReplaceWildcards(fileName, filenameNum);
            }
          }
        }
      }

    if (validTime)
      {
      switch (this->VariableTypes[i])
        {
        case vtkEnSightReader::SCALAR_PER_NODE:
          this->ReadScalarsPerNode(fileName, this->VariableDescriptions[i],
                                   timeStepInFile);
          break;
        case vtkEnSightReader::SCALAR_PER_MEASURED_NODE:
          this->ReadScalarsPerNode(fileName, this->VariableDescriptions[i], 
                                   timeStepInFile, 1);
          break;
        case vtkEnSightReader::VECTOR_PER_NODE:
          this->ReadVectorsPerNode(fileName, this->VariableDescriptions[i],
                                   timeStepInFile);
          break;
        case vtkEnSightReader::VECTOR_PER_MEASURED_NODE:
          this->ReadVectorsPerNode(fileName, this->VariableDescriptions[i],
                                   timeStepInFile, 1);
          break;
        case vtkEnSightReader::TENSOR_SYMM_PER_NODE:
          this->ReadTensorsPerNode(fileName, this->VariableDescriptions[i],
                                   timeStepInFile);
          break;
        case vtkEnSightReader::SCALAR_PER_ELEMENT:
          this->ReadScalarsPerElement(fileName, this->VariableDescriptions[i],
                                      timeStepInFile);
          break;
        case vtkEnSightReader::VECTOR_PER_ELEMENT:
          this->ReadVectorsPerElement(fileName, this->VariableDescriptions[i],
                                      timeStepInFile);
          break;
        case vtkEnSightReader::TENSOR_SYMM_PER_ELEMENT:
          this->ReadTensorsPerElement(fileName, this->VariableDescriptions[i],
                                      timeStepInFile);
          break;
        }
      }
    delete [] fileName;
    }
  for (i = 0; i < this->NumberOfComplexVariables; i++)
    {
    switch(this->ComplexVariableTypes[i])
      {
      case COMPLEX_SCALAR_PER_NODE:
      case COMPLEX_VECTOR_PER_NODE:
        if(!this->GetPointArrayStatus(this->ComplexVariableDescriptions[i]))
          {
          continue;
          }
        break;
      case COMPLEX_SCALAR_PER_ELEMENT:
      case COMPLEX_VECTOR_PER_ELEMENT:
        if(!this->GetCellArrayStatus(this->ComplexVariableDescriptions[i]))
          {
          continue;
          }
        break;
      }
    timeStep = 0;
    timeStepInFile = 1;
    //fileNum = 1;
    validTime = 1;
    fileName = new char[strlen(this->ComplexVariableFileNames[2*i]) + 1];
    strcpy(fileName, this->ComplexVariableFileNames[2*i]);
    fileName2 = new char[strlen(this->ComplexVariableFileNames[2*i+1]) + 1];
    strcpy(fileName2, this->ComplexVariableFileNames[2*i+1]);
    if (this->UseTimeSets)
      {
      validTime = 0;
      timeSet = this->VariableTimeSetIds->GetId(i);
      times = this->TimeSets->GetItem(this->TimeSetIds->IsId(timeSet));
      for (j = 0; j < times->GetNumberOfTuples(); j++)
        {
        newTime = times->GetComponent(j, 0);
        if (newTime <= this->TimeValue)
          {
          timeStep++;
          if (this->VariableTypes[i] == SCALAR_PER_MEASURED_NODE ||
               this->VariableTypes[i] == VECTOR_PER_MEASURED_NODE)
            {
            if (newTime >= this->MeasuredTimeValue)
              {
              validTime = 1;
              }
            }
          else if (newTime >= this->GeometryTimeValue)
            {
            validTime = 1;
            }
          }
        }
      if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0 &&
          validTime)
        {
        int collectionNum = this->TimeSetsWithFilenameNumbers->IsId(timeSet);
        if (collectionNum > -1)
          {
          filenameNumbers = this->TimeSetFileNameNumbers->
            GetItem(collectionNum);
          filenameNum = filenameNumbers->GetId(timeStep-1);
          this->ReplaceWildcards(fileName, filenameNum);
          this->ReplaceWildcards(fileName2, filenameNum);
          }
        }
      
      // There can only be file sets if there are also time sets.
      if (this->UseFileSets)
        {
        timeStepInFile = timeStep;
        fileSet = this->VariableFileSetIds->GetId(i);
        numStepsList = (vtkIdList*)this->FileSetNumberOfSteps->
          GetItemAsObject(this->FileSets->IsId(fileSet));

        if (timeStep > numStepsList->GetId(0))
          {
          numSteps = numStepsList->GetId(0);
          timeStepInFile -= numSteps;
          for (i = 1; i < numStepsList->GetNumberOfIds(); i++)
            {
            numSteps += numStepsList->GetId(i);
            if (timeStep > numSteps)
              {
              //fileNum++;
              timeStepInFile -= numStepsList->GetId(i);
              }
            }
          }
        if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0 &&
            validTime)
          {
          int collectionNum = this->FileSetsWithFilenameNumbers->IsId(fileSet);
          if (collectionNum > -1)
            {
            filenameNumbers = this->FileSetFileNameNumbers->
              GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(timeStep-1);
            this->ReplaceWildcards(fileName, filenameNum);
            this->ReplaceWildcards(fileName2, filenameNum);
            }
          }
        }
      }

    if (validTime)
      {
      switch (this->ComplexVariableTypes[i])
        {
        case vtkEnSightReader::COMPLEX_SCALAR_PER_NODE:
          this->ReadScalarsPerNode(fileName,
                                   this->ComplexVariableDescriptions[i],
                                   timeStepInFile, 0, 2);
          this->ReadScalarsPerNode(fileName2,
                                   this->ComplexVariableDescriptions[i],
                                   timeStepInFile, 0, 2, 1);
          break;
        case vtkEnSightReader::COMPLEX_VECTOR_PER_NODE:
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_r");
          this->ReadVectorsPerNode(fileName, description, timeStepInFile);
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_i");
          this->ReadVectorsPerNode(fileName2, description, timeStepInFile);
          break;
        case vtkEnSightReader::COMPLEX_SCALAR_PER_ELEMENT:
          this->ReadScalarsPerElement(fileName,
                                      this->ComplexVariableDescriptions[i],
                                      timeStepInFile, 2);
          this->ReadScalarsPerElement(fileName2,
                                      this->ComplexVariableDescriptions[i],
                                      timeStepInFile, 2, 1);
          break;
        case vtkEnSightReader::COMPLEX_VECTOR_PER_ELEMENT:
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_r");
          this->ReadVectorsPerElement(fileName, description, timeStepInFile);
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_i");
          this->ReadVectorsPerElement(fileName2, description, timeStepInFile);
          break;
        }
      }
    delete [] fileName;
    delete [] fileName2;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkEnSightReader::AddVariableFileName(const char* fileName1,
                                           const char* fileName2)
{
  int size;
  int i;
  
  if (this->VariableMode < 8)
    {
    size = this->NumberOfVariables;

    char** newFileNameList = new char *[size]; // temporary array
  
    // copy file names to temporary array
    for (i = 0; i < size; i++)
      {
      newFileNameList[i] = new char[strlen(this->VariableFileNames[i]) + 1];
      strcpy(newFileNameList[i], this->VariableFileNames[i]);
      delete [] this->VariableFileNames[i];
      }
    delete [] this->VariableFileNames;
    
    // make room for new file name
    this->VariableFileNames = new char *[size+1];
    
    // copy existing file names back to first array
    for (i = 0; i < size; i++)
      {
      this->VariableFileNames[i] = new char[strlen(newFileNameList[i]) + 1];
      strcpy(this->VariableFileNames[i], newFileNameList[i]);
      delete [] newFileNameList[i];
      }
    delete [] newFileNameList;
    
    // add new file name at end of first array
    this->VariableFileNames[size] = new char[strlen(fileName1) + 1];
    strcpy(this->VariableFileNames[size], fileName1);
    vtkDebugMacro( << "file name: " << this->VariableFileNames[size]);
    }
  else
    {
    size = this->NumberOfComplexVariables;
    
    char** newFileNameList = new char *[2 * size]; // temporary array
    
    // copy file names to temporary array
    for (i = 0; i < 2*size; i++)
      {
      newFileNameList[i] =
        new char[strlen(this->ComplexVariableFileNames[i]) + 1];
      strcpy(newFileNameList[i], this->ComplexVariableFileNames[i]);
      delete [] this->ComplexVariableFileNames[i];
      }
    delete [] this->ComplexVariableFileNames;

    // make room for new file name
    this->ComplexVariableFileNames = new char *[2*(size+1)];
    
    // copy existing file names back to first array
    for (i = 0; i < 2*size; i++)
      {
      this->ComplexVariableFileNames[i] =
        new char[strlen(newFileNameList[i]) + 1];
      strcpy(this->ComplexVariableFileNames[i], newFileNameList[i]);
      delete [] newFileNameList[i];
      }
    delete [] newFileNameList;
    
    // add new file name at end of first array
    this->ComplexVariableFileNames[2*size] = new char[strlen(fileName1) + 1];
    strcpy(this->ComplexVariableFileNames[2*size], fileName1);
    vtkDebugMacro("real file name: "
                  << this->ComplexVariableFileNames[2*size]);
    this->ComplexVariableFileNames[2*size+1] = new char[strlen(fileName2) + 1];
    strcpy(this->ComplexVariableFileNames[2*size+1], fileName2);
    vtkDebugMacro("imag. file name: "
                  << this->ComplexVariableFileNames[2*size+1]);
    }
}

//----------------------------------------------------------------------------
void vtkEnSightReader::AddVariableDescription(const char* description)
{
  int size;
  int i;
  
  if (this->VariableMode < 8)
    {
    size = this->NumberOfVariables;

    char ** newDescriptionList = new char *[size]; // temporary array    
    
    // copy descriptions to temporary array
    for (i = 0; i < size; i++)
      {
      newDescriptionList[i] =
        new char[strlen(this->VariableDescriptions[i]) + 1];
      strcpy(newDescriptionList[i], this->VariableDescriptions[i]);
      delete [] this->VariableDescriptions[i];
      }
    delete [] this->VariableDescriptions;
    
    // make room for new description
    this->VariableDescriptions = new char *[size+1];
    
    // copy existing descriptions back to first array
    for (i = 0; i < size; i++)
      {
      this->VariableDescriptions[i] =
        new char[strlen(newDescriptionList[i]) + 1];
      strcpy(this->VariableDescriptions[i], newDescriptionList[i]);
      delete [] newDescriptionList[i];
      }
    delete [] newDescriptionList;
    
    // add new description at end of first array
    this->VariableDescriptions[size] = new char[strlen(description) + 1];
    strcpy(this->VariableDescriptions[size], description);
    vtkDebugMacro("description: " << this->VariableDescriptions[size]);
    }
  else
    {
    size = this->NumberOfComplexVariables;
    
    char ** newDescriptionList = new char *[size]; // temporary array    
    
    // copy descriptions to temporary array
    for (i = 0; i < size; i++)
      {
      newDescriptionList[i] =
        new char[strlen(this->ComplexVariableDescriptions[i]) + 1];
      strcpy(newDescriptionList[i], this->ComplexVariableDescriptions[i]);
      delete [] this->ComplexVariableDescriptions[i];
      }
    delete [] this->ComplexVariableDescriptions;
    
    // make room for new description
    this->ComplexVariableDescriptions = new char *[size+1];
    
    // copy existing descriptions back to first array
    for (i = 0; i < size; i++)
      {
      this->ComplexVariableDescriptions[i] =
        new char[strlen(newDescriptionList[i]) + 1];
      strcpy(this->ComplexVariableDescriptions[i], newDescriptionList[i]);
      delete [] newDescriptionList[i];
      }
    delete [] newDescriptionList;
    
    // add new description at end of first array
    this->ComplexVariableDescriptions[size] =
      new char[strlen(description) + 1];
    strcpy(this->ComplexVariableDescriptions[size], description);
    vtkDebugMacro("description: "
                  << this->ComplexVariableDescriptions[size]);
    }
}

//----------------------------------------------------------------------------
void vtkEnSightReader::AddVariableType()
{
  int size;
  int i;
  int *types = NULL;
  
  // Figure out what the size of the variable type array is.
  if (this->VariableMode < 8)
    {
    size = this->NumberOfVariables;
  
    types = new int[size];
    
    for (i = 0; i < size; i++)
      {
      types[i] = this->VariableTypes[i];
      }
    delete [] this->VariableTypes;
    
    this->VariableTypes = new int[size+1];
    for (i = 0; i < size; i++)
      {
      this->VariableTypes[i] = types[i];
      }
    delete [] types;
    this->VariableTypes[size] = this->VariableMode;
    vtkDebugMacro("variable type: " << this->VariableTypes[size]);
    }
  else
    {
    size = this->NumberOfComplexVariables;

    if (size > 0)
      {
      types = new int[size];
      for (i = 0; i < size; i++)
        {
        types[i] = this->ComplexVariableTypes[i];
        }
      delete [] this->ComplexVariableTypes;
      }
    
    this->ComplexVariableTypes = new int[size+1];
    for (i = 0; i < size; i++)
      {
      this->ComplexVariableTypes[i] = types[i];
      }
    
    if (size > 0)
      {
      delete [] types;
      }
    this->ComplexVariableTypes[size] = this->VariableMode;
    vtkDebugMacro("complex variable type: "
                  << this->ComplexVariableTypes[size]);
    }
}

//----------------------------------------------------------------------------
int vtkEnSightReader::GetSectionType(const char *line)
{
  if (strncmp(line, "coordinates", 5) == 0)
    {
    return vtkEnSightReader::COORDINATES;
    }
  else if (strncmp(line, "block", 4) == 0)
    {
    return vtkEnSightReader::BLOCK;
    }
  else if (this->GetElementType(line) != -1)
    {
    return vtkEnSightReader::ELEMENT;
    }
  else
    {
    return -1;
    }
}

//----------------------------------------------------------------------------
int vtkEnSightReader::GetElementType(const char* line)
{
  if (strncmp(line, "point", 5) == 0)
    {
    return vtkEnSightReader::POINT;
    }
  else if (strncmp(line, "bar2", 4) == 0)
    {
    return vtkEnSightReader::BAR2;
    }
  else if (strncmp(line, "bar3", 4) == 0)
    {
    return vtkEnSightReader::BAR3;
    }
  else if (strncmp(line, "nsided", 6) == 0)
    {
    return vtkEnSightReader::NSIDED;
    }
  else if (strncmp(line, "tria3", 5) == 0)
    {
    return vtkEnSightReader::TRIA3;
    }
  else if (strncmp(line, "tria6", 5) == 0)
    {
    return vtkEnSightReader::TRIA6;
    }
  else if (strncmp(line, "quad4", 5) == 0)
    {
    return vtkEnSightReader::QUAD4;
    }
  else if (strncmp(line, "quad8", 5) == 0)
    {
    return vtkEnSightReader::QUAD8;
    }
  else if (strncmp(line, "tetra4", 6) == 0)
    {
    return vtkEnSightReader::TETRA4;
    }
  else if (strncmp(line, "tetra10", 7) == 0)
    {
    return vtkEnSightReader::TETRA10;
    }
  else if (strncmp(line, "pyramid5", 8) == 0)
    {
    return vtkEnSightReader::PYRAMID5;
    }
  else if (strncmp(line, "pyramid13", 9) == 0)
    {
    return vtkEnSightReader::PYRAMID13;
    }
  else if (strncmp(line, "hexa8", 5) == 0)
    {
    return vtkEnSightReader::HEXA8;
    }
  else if (strncmp(line, "hexa20", 6) == 0)
    {
    return vtkEnSightReader::HEXA20;
    }
  else if (strncmp(line, "penta6", 6) == 0)
    {
    return vtkEnSightReader::PENTA6;
    }
  else if (strncmp(line, "penta15", 7) == 0)
    {
    return vtkEnSightReader::PENTA15;
    }
  else
    {
    return -1;
    }
}

void vtkEnSightReader::ReplaceWildcards(char* filename, int num)
{
  int wildcardPos, numWildcards, numDigits = 1, i;
  int tmpNum = num, multTen = 1;
  char newChar;
  int newNum;
  
  wildcardPos = static_cast<int>(strcspn(filename, "*"));
  numWildcards = static_cast<int>(strspn(filename + wildcardPos, "*"));
  
  if (numWildcards == 0)
    {
    return;
    }
  
  tmpNum /= 10;
  while (tmpNum >= 1)
    {
    numDigits++;
    multTen *= 10;
    tmpNum /= 10;
    }
  
  for (i = 0; i < numWildcards - numDigits; i++)
    {
    filename[i + wildcardPos] = '0';
    }
  
  tmpNum = num;
  for (i = numWildcards - numDigits; i < numWildcards; i++)
    {
    newNum = tmpNum / multTen;
    switch (newNum)
      {
      case 0:
        newChar = '0';
        break;
      case 1:
        newChar = '1';
        break;
      case 2:
        newChar = '2';
        break;
      case 3:
        newChar = '3';
        break;
      case 4:
        newChar = '4';
        break;
      case 5:
        newChar = '5';
        break;
      case 6:
        newChar = '6';
        break;
      case 7:
        newChar = '7';
        break;
      case 8:
        newChar = '8';
        break;
      case 9:
        newChar = '9';
        break;
      default:
        // This case should never be reached.
        return;
      }
    
    filename[i + wildcardPos] = newChar;
    tmpNum -= multTen * newNum;
    multTen /= 10;
    }
}

//----------------------------------------------------------------------------
// Called by constructor to set up output array.
void vtkEnSightReader::SetNumberOfOutputsInternal(int num)
{
  int idx;
  vtkDataObject **outputs;

  // in case nothing has changed.
  if (num == this->NumberOfOutputs)
    {
    return;
    }
  
  // Allocate new arrays.
  outputs = new vtkDataObject *[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    outputs[idx] = NULL;
    }

  // Copy old outputs
  for (idx = 0; idx < num && idx < this->NumberOfOutputs; ++idx)
    {
    outputs[idx] = this->Outputs[idx];
    }
  
  // delete the previous arrays
  if (this->Outputs)
    {
    delete [] this->Outputs;
    this->Outputs = NULL;
    this->NumberOfOutputs = 0;
    }
  
  // Set the new arrays
  this->Outputs = outputs;
  
  this->NumberOfOutputs = num;
}

//----------------------------------------------------------------------------
void vtkEnSightReader::ReplaceNthOutput(int idx, vtkDataObject* newOutput)
{
  vtkDataObject *oldOutput;
  
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthOutput: " << idx << ", cannot set output. ");
    return;
    }
  // Expand array if necessary.
  if (idx >= this->NumberOfOutputs)
    {
    this->SetNumberOfOutputsInternal(idx + 1);
    }
  
  // does this change anything?
  oldOutput = this->Outputs[idx];
  if (newOutput == oldOutput)
    {
    return;
    }

  if ( !newOutput->IsA(oldOutput->GetClassName()) )
    {
    vtkErrorMacro("Can not replace the output with a different type.");
    return;
    }
  
  if (newOutput)
    {
    vtkSource *newOutputOldSource = newOutput->GetSource();
    if (newOutputOldSource)
      {
      vtkErrorMacro("The new output can not have a source.");
      return;
      }
    }

  // disconnect first existing source-output relationship.
  if (oldOutput)
    {
    oldOutput->SetSource(NULL);
    oldOutput->UnRegister(this);
    this->Outputs[idx] = NULL;
    }
  
  if (newOutput)
    {
    // Register the newOutput so it does not get deleted.
    // Don't set the link yet until previous links is disconnected.
    newOutput->Register(this);
    newOutput->SetSource(this);
    }

  // now actually make the link that was registered previously.
  this->Outputs[idx] = newOutput;

}

int vtkEnSightReader::CheckOutputConsistency()
{
  if (this->NumberOfOutputs > this->NumberOfNewOutputs &&
      ! this->InitialRead)
    {
    vtkErrorMacro("Cannot decrease number of outputs after initial read");
    this->OutputsAreValid = 0;
    }
  
  if (this->InitialRead)
    {
    this->InitialRead = 0;
    }
  
  return this->OutputsAreValid;
}

//----------------------------------------------------------------------------
vtkIdList* vtkEnSightReader::GetCellIds(int index, int cellType)
{
  // Check argument range.
  if(cellType < 0 || cellType > 15)
    {
    vtkErrorMacro("Cell type " << cellType
                  << " out of range.  Only 16 types exist.");
    return 0;
    }
  if(index < 0 || index > this->UnstructuredPartIds->GetNumberOfIds())
    {
    vtkErrorMacro("Index " << index << " out of range.  Only "
                  << this->UnstructuredPartIds->GetNumberOfIds()
                  << " IDs exist.");
    return 0;
    }
  
  // Create the container if necessary.
  if(!this->CellIds)
    {
    this->CellIds = new vtkEnSightReaderCellIdsType;
    }
  
  // Get the index of the actual vtkIdList requested.
  unsigned int cellIdsIndex = index*16 + cellType;
  
  // Make sure the container is large enough for this index.
  if(cellIdsIndex+1 > this->CellIds->size())
    {
    this->CellIds->resize(cellIdsIndex+1);
    }
  
  // Make sure this vtkIdList exists.
  if(!(*this->CellIds)[cellIdsIndex].GetPointer())
    {
    vtkIdList* nl = vtkIdList::New();
    (*this->CellIds)[cellIdsIndex] = nl;
    nl->Delete();
    }
  
  // Return the requested vtkIdList.
  return (*this->CellIds)[cellIdsIndex].GetPointer();
}

//----------------------------------------------------------------------------
void vtkEnSightReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CaseFileName: "
     << (this->CaseFileName ? this->CaseFileName : "(none)") << endl;
  os << indent << "FilePath: "
     << (this->FilePath ? this->FilePath : "(none)") << endl;
  os << indent << "NumberOfComplexScalarsPerNode: "
     << this->NumberOfComplexScalarsPerNode << endl;
  os << indent << "NumberOfVectorsPerElement :"
     << this->NumberOfVectorsPerElement << endl;
  os << indent << "NumberOfTensorsSymmPerElement: "
     << this->NumberOfTensorsSymmPerElement << endl;
  os << indent << "NumberOfComplexVectorsPerNode: "
     << this->NumberOfComplexVectorsPerNode << endl;
  os << indent << "NumberOfScalarsPerElement: "
     << this->NumberOfScalarsPerElement << endl;
  os << indent << "NumberOfComplexVectorsPerElement: "
     << this->NumberOfComplexVectorsPerElement << endl;
  os << indent << "NumberOfComplexScalarsPerElement: "
     << this->NumberOfComplexScalarsPerElement << endl;
  os << indent << "NumberOfTensorsSymmPerNode: "
     << this->NumberOfTensorsSymmPerNode << endl;
  os << indent << "NumberOfScalarsPerMeasuredNode: "
     << this->NumberOfScalarsPerMeasuredNode << endl;
  os << indent << "NumberOfVectorsPerMeasuredNode: "
     << this->NumberOfVectorsPerMeasuredNode << endl;
  os << indent << "NumberOfScalarsPerNode: "
     << this->NumberOfScalarsPerNode << endl;
  os << indent << "NumberOfVectorsPerNode: "
     << this->NumberOfVectorsPerNode << endl;
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "MinimumTimeValue: " << this->MinimumTimeValue << endl;
  os << indent << "MaximumTimeValue: " << this->MaximumTimeValue << endl;
  os << indent << "TimeSets: " << this->TimeSets << endl;
  os << indent << "OutputsAreValid: " << this->OutputsAreValid << endl;
}
