/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericEnSightReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkGenericEnSightReader.h"
#include "vtkEnSight6Reader.h"
#include "vtkEnSight6BinaryReader.h"
#include "vtkEnSightGoldReader.h"
#include "vtkEnSightGoldBinaryReader.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkGenericEnSightReader* vtkGenericEnSightReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGenericEnSightReader");
  if(ret)
    {
    return (vtkGenericEnSightReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGenericEnSightReader;
}

//----------------------------------------------------------------------------
vtkGenericEnSightReader::vtkGenericEnSightReader()
{
  this->Reader = NULL;
  this->IS = NULL;
  this->IFile = NULL;
  
  this->CaseFileName = NULL;
  this->GeometryFileName = NULL;
  this->FilePath = NULL;
  
  this->VariableTypes = NULL;
  this->ComplexVariableTypes = NULL;
  
  this->VariableDescriptions = NULL;
  this->ComplexVariableDescriptions = NULL;
  
  this->NumberOfVariables = 0;
  this->NumberOfComplexVariables = 0;
  
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
  
  this->TimeValue = 0;
  this->MinimumTimeValue = 0;
  this->MaximumTimeValue = 0;
}

//----------------------------------------------------------------------------
vtkGenericEnSightReader::~vtkGenericEnSightReader()
{
  int i;
  
  if (this->Reader)
    {
    this->Reader->Delete();
    this->Reader = NULL;
    }
  if (this->IS)
    {
    delete this->IS;
    this->IS = NULL;
    }
  if (this->CaseFileName)
    {
    delete [] this->CaseFileName;
    this->CaseFileName = NULL;
    }
  if (this->GeometryFileName)
    {
    delete [] this->GeometryFileName;
    this->GeometryFileName = NULL;
    }
  if (this->FilePath)
    {
    delete [] this->FilePath;
    this->FilePath = NULL;
    }
  if (this->NumberOfVariables > 0)
    {
    for (i = 0; i < this->NumberOfVariables; i++)
      {
      delete [] this->VariableDescriptions[i];
      }
    delete [] this->VariableDescriptions;
    delete [] this->VariableTypes;
    this->VariableDescriptions = NULL;
    this->VariableTypes = NULL;
    }
  if (this->NumberOfComplexVariables > 0)
    {
    for (i = 0; i < this->NumberOfComplexVariables; i++)
      {
      delete [] this->ComplexVariableDescriptions[i];
      }
    delete [] this->ComplexVariableDescriptions;
    delete [] this->ComplexVariableTypes;
    this->ComplexVariableDescriptions = NULL;
    this->ComplexVariableTypes = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::Execute()
{
  int i;
  int version = this->DetermineEnSightVersion();
  
  if (version == VTK_ENSIGHT_6)
    {
    vtkDebugMacro("EnSight6");
    this->Reader = vtkEnSight6Reader::New();
    }
  else if (version == VTK_ENSIGHT_6_BINARY)
    {
    vtkDebugMacro("EnSight6 binary");
    this->Reader = vtkEnSight6BinaryReader::New();
    }
  else if (version == VTK_ENSIGHT_GOLD)
    {
    vtkDebugMacro("EnSightGold");
    this->Reader = vtkEnSightGoldReader::New();
    }
  else if (version == VTK_ENSIGHT_GOLD_BINARY)
    {
    vtkDebugMacro("EnSightGold binary");
    this->Reader = vtkEnSightGoldBinaryReader::New();
    }
  else
    {
    vtkErrorMacro("Error determining EnSightVersion");
    return;
    }
  
  this->Reader->SetCaseFileName(this->GetCaseFileName());
  this->Reader->SetFilePath(this->GetFilePath());
  this->Reader->SetTimeValue(this->GetTimeValue());
  this->Reader->Update();

  this->MinimumTimeValue = this->Reader->GetMinimumTimeValue();
  this->MaximumTimeValue = this->Reader->GetMaximumTimeValue();
  this->NumberOfScalarsPerNode = this->Reader->GetNumberOfScalarsPerNode();
  this->NumberOfVectorsPerNode = this->Reader->GetNumberOfVectorsPerNode();
  this->NumberOfTensorsSymmPerNode = this->Reader->GetNumberOfTensorsSymmPerNode();
  this->NumberOfScalarsPerElement = this->Reader->GetNumberOfScalarsPerElement();
  this->NumberOfVectorsPerElement = this->Reader->GetNumberOfVectorsPerElement();
  this->NumberOfTensorsSymmPerElement = this->Reader->GetNumberOfTensorsSymmPerElement();
  this->NumberOfScalarsPerMeasuredNode = this->Reader->GetNumberOfScalarsPerMeasuredNode();
  this->NumberOfVectorsPerMeasuredNode = this->Reader->GetNumberOfVectorsPerMeasuredNode();
  this->NumberOfComplexScalarsPerNode = this->Reader->GetNumberOfComplexScalarsPerNode();
  this->NumberOfComplexVectorsPerNode = this->Reader->GetNumberOfComplexVectorsPerNode();
  this->NumberOfComplexScalarsPerElement =
    this->Reader->GetNumberOfComplexScalarsPerElement();
  this->NumberOfComplexVectorsPerElement =
    this->Reader->GetNumberOfComplexScalarsPerElement();
  
  for (i = 0; i < this->Reader->GetNumberOfOutputs(); i++)
    {
    this->SetNthOutput(i, this->Reader->GetOutput(i));
    }
  for (i = 0; i < this->Reader->GetNumberOfVariables(); i++)
    {
    this->AddVariableDescription(this->Reader->GetDescription(i));
    this->AddVariableType(this->Reader->GetVariableType(i));
    this->NumberOfVariables++;
    }
  for (i = 0; i < this->Reader->GetNumberOfComplexVariables(); i++)
    {
    this->AddComplexVariableDescription(this->Reader->GetComplexDescription(i));
    this->AddComplexVariableType(this->Reader->GetComplexVariableType(i));
    this->NumberOfComplexVariables++;
    }

  this->Reader->Delete();
  this->Reader = NULL;
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::DetermineEnSightVersion()
{
  char line[256], subLine[256], binaryLine[80];
  int stringRead;
  int timeSet = 0, fileSet = 0;
  char *fileName = NULL;
  
  if (!this->CaseFileName)
    {
    vtkErrorMacro("A case file name must be specified.");
    return -1;
    }
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, this->CaseFileName);
    vtkDebugMacro("full path to case file: " << line);
    }
  else
    {
    strcpy(line, this->CaseFileName);
    }
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
    return -1;
    }

  this->ReadNextDataLine(line);
  
  if (strncmp(line, "FORMAT", 6) == 0)
    {
    // found the FORMAT section
    vtkDebugMacro("*** FORMAT section");
    this->ReadNextDataLine(line);
    
    stringRead = sscanf(line, " %*s %*s %s", subLine);
    if (stringRead == 1)
      {
      if (strcmp(subLine, "gold") == 0)
        {
        this->ReadNextDataLine(line);
        if (strncmp(line, "GEOMETRY", 8) == 0)
          {
          // found the GEOMETRY section
          vtkDebugMacro("*** GEOMETRY section");
          
          this->ReadNextDataLine(line);
          if (strncmp(line, "model:", 6) == 0)
            {
            if (sscanf(line, " %*s %d %d %s", &timeSet, &fileSet,
		       subLine) == 3)
              {
              this->SetGeometryFileName(subLine);
              }
            else if (sscanf(line, " %*s %d %s", &timeSet, subLine) == 2)
              {
              this->SetGeometryFileName(subLine);
              }
            else if (sscanf(line, " %*s %s", subLine) == 1)
              {
              this->SetGeometryFileName(subLine);
              }
            } // geometry file name set
          delete this->IS;
          this->IS = NULL;
	  
	  fileName = new char[strlen(this->GeometryFileName) + 1];
	  strcpy(fileName, this->GeometryFileName);
	  
          if (!fileName)
            {
            vtkErrorMacro("A GeometryFileName must be specified in the case file.");
            return 0;
            }
          if (strrchr(fileName, '*') != NULL)
            {
	    // reopen case file; find right time set and fill in wildcards from
	    // there if possible; if not, then find right file set and fill in
	    // wildcards from there.
	    this->ReplaceWildcards(fileName, timeSet, fileSet);
            }
          if (this->FilePath)
            {
            strcpy(line, this->FilePath);
            strcat(line, fileName);
            vtkDebugMacro("full path to geometry file: " << line);
            }
          else
            {
            strcpy(line, fileName);
            } // got full path to geometry file
          
          this->IFile = fopen(line, "rb");
          if (this->IFile == NULL)
            {
            vtkErrorMacro("Unable to open file: " << line);
            fclose(this->IFile);
            this->IFile = NULL;
	    delete [] fileName;
            return 0;
            } // end if IFile == NULL
          
          this->ReadBinaryLine(binaryLine);
          sscanf(binaryLine, " %*s %s", subLine);
          if (strcmp(subLine, "Binary") == 0)
            {
            fclose(this->IFile);
            this->IFile = NULL;
	    delete [] fileName;
            return VTK_ENSIGHT_GOLD_BINARY;
            } //end if binary
          
          fclose(this->IFile);
          this->IFile = NULL;
	  delete [] fileName;
          return VTK_ENSIGHT_GOLD;
          } // if we found the geometry section in the case file
        }
      }
    else
      {
      this->ReadNextDataLine(line);
      if (strncmp(line, "GEOMETRY", 8) == 0)
        {
        // found the GEOMETRY section
        vtkDebugMacro("*** GEOMETRY section");
        
        this->ReadNextDataLine(line);
        if (strncmp(line, "model:", 6) == 0)
          {
          if (sscanf(line, " %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
            {
            this->SetGeometryFileName(subLine);
            }
          else if (sscanf(line, " %*s %d %s", &timeSet, subLine) == 2)
            {
            this->SetGeometryFileName(subLine);
            }
          else if (sscanf(line, " %*s %s", subLine) == 1)
            {
            this->SetGeometryFileName(subLine);
            }
          } // geometry file name set
	
	fileName = new char[strlen(this->GeometryFileName) + 1];
	strcpy(fileName, this->GeometryFileName);
	
        delete this->IS;
        this->IS = NULL;
        if (!fileName)
          {
          vtkErrorMacro("A GeometryFileName must be specified in the case file.");
          return 0;
          }
        if (strrchr(fileName, '*') != NULL)
          {
	  // reopen case file; find right time set and fill in wildcards from
	  // there if possible; if not, then find right file set and fill in
	  // wildcards from there.
	  this->ReplaceWildcards(fileName, timeSet, fileSet);
          }
        if (this->FilePath)
          {
          strcpy(line, this->FilePath);
          strcat(line, fileName);
          vtkDebugMacro("full path to geometry file: " << line);
          }
        else
          {
          strcpy(line, fileName);
          } // got full path to geometry file
        
        this->IFile = fopen(line, "rb");
        if (this->IFile == NULL)
          {
          vtkErrorMacro("Unable to open file: " << line);
          fclose(this->IFile);
          this->IFile = NULL;
	  delete [] fileName;
          return 0;
          } // end if IFile == NULL
        
        this->ReadBinaryLine(binaryLine);
        sscanf(binaryLine, " %*s %s", subLine);
        if (strcmp(subLine, "Binary") == 0)
          {
          fclose(this->IFile);
          this->IFile = NULL;
	  delete [] fileName;
          return VTK_ENSIGHT_6_BINARY;
          } //end if binary
        
        fclose(this->IFile);
        this->IFile = NULL;
	delete [] fileName;
        return VTK_ENSIGHT_6;
        } // if we found the geometry section in the case file
      } // not ensight gold
    } // if we found the format section in the case file
  
  if (fileName)
    {
    delete [] fileName;
    }
  
  return -1;
}

void vtkGenericEnSightReader::SetCaseFileName(char* fileName)
{
  char *endingSlash = NULL;
  char *path, *newFileName;
  int position, numChars;
  
  if ( this->CaseFileName && fileName && (!strcmp(this->CaseFileName, fileName)))
    {
    return;
    }
  if (this->CaseFileName)
    {
    delete [] this->CaseFileName;
    }
  if (fileName)
    {
    this->CaseFileName = new char[strlen(fileName)+1];
    strcpy(this->CaseFileName, fileName);
    }
   else
    {
    this->CaseFileName = NULL;
    }
  
  // strip off the path and save it as FilePath if it was included in the filename
  if ((endingSlash = strrchr(this->CaseFileName, '/')))
    {
    position = endingSlash - this->CaseFileName + 1;
    path = new char[position + 1];
    numChars = strlen(this->CaseFileName);
    newFileName = new char[numChars - position + 1];
    strcpy(path, "");
    strncat(path, this->CaseFileName, position);
    this->SetFilePath(path);
    strcpy(newFileName, this->CaseFileName + position);
    strcpy(this->CaseFileName, newFileName);
    delete [] path;
    delete [] newFileName;
    }
      
  this->Modified();
}

// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int vtkGenericEnSightReader::ReadLine(char result[256])
{
  this->IS->getline(result,256);
  if (this->IS->eof()) 
    {
    return 0;
    }
  
  return 1;
}

// Internal function to read in a line (from a binary file) up
// to 80 characters.  Returns zero if there was an error.
int vtkGenericEnSightReader::ReadBinaryLine(char result[80])
{
  fread(result, sizeof(char), 80, this->IFile);

  if (feof(this->IFile) || ferror(this->IFile))
    {
    return 0;
    }
  
  return 1;
}

// Internal function that skips blank lines and comment lines
// and reads the next line it finds (up to 256 characters).
// Returns 0 is there was an error.
int vtkGenericEnSightReader::ReadNextDataLine(char result[256])
{
  int value, sublineFound;
  char subline[256];
  
  value = this->ReadLine(result);
  sublineFound = sscanf(result, " %s", subline);
  
  while((strcmp(result, "") == 0 || subline[0] == '#' || sublineFound < 1) &&
        value != 0)
    {
    value = this->ReadLine(result);
    sublineFound = sscanf(result, " %s", subline);
    }
  return value;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::Update()
{
  int i;
  
  this->Execute();
  
  for (i = 0; i < this->GetNumberOfOutputs(); i++)
    {
    if ( this->GetOutput(i) )
      {
      this->GetOutput(i)->DataHasBeenGenerated();
      }
    }
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::AddVariableDescription(char* description)
{
  int size = this->NumberOfVariables;
  int i;
  

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

void vtkGenericEnSightReader::AddComplexVariableDescription(char* description)
{
  int i;
  int size = this->NumberOfComplexVariables;
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

int vtkGenericEnSightReader::GetNumberOfVariables(int type)
{
  switch (type)
    {
    case VTK_SCALAR_PER_NODE:
      return this->GetNumberOfScalarsPerNode();
    case VTK_VECTOR_PER_NODE:
      return this->GetNumberOfVectorsPerNode();
    case VTK_TENSOR_SYMM_PER_NODE:
      return this->GetNumberOfTensorsSymmPerNode();
    case VTK_SCALAR_PER_ELEMENT:
      return this->GetNumberOfScalarsPerElement();
    case VTK_VECTOR_PER_ELEMENT:
      return this->GetNumberOfVectorsPerElement();
    case VTK_TENSOR_SYMM_PER_ELEMENT:
      return this->GetNumberOfTensorsSymmPerElement();
    case VTK_SCALAR_PER_MEASURED_NODE:
      return this->GetNumberOfScalarsPerMeasuredNode();
    case VTK_VECTOR_PER_MEASURED_NODE:
      return this->GetNumberOfVectorsPerMeasuredNode();
    case VTK_COMPLEX_SCALAR_PER_NODE:
      return this->GetNumberOfComplexScalarsPerNode();
    case VTK_COMPLEX_VECTOR_PER_NODE:
      return this->GetNumberOfComplexVectorsPerNode();
    case VTK_COMPLEX_SCALAR_PER_ELEMENT:
      return this->GetNumberOfComplexScalarsPerElement();
    case VTK_COMPLEX_VECTOR_PER_ELEMENT:
      return this->GetNumberOfComplexVectorsPerElement();
    default:
      vtkWarningMacro("unknow variable type");
      return -1;
    }
}

char* vtkGenericEnSightReader::GetDescription(int n)
{
  if (n < this->NumberOfVariables)
    {
    return this->VariableDescriptions[n];
    }
  return NULL;
}

char* vtkGenericEnSightReader::GetComplexDescription(int n)
{
  if (n < this->NumberOfComplexVariables)
    {
    return this->ComplexVariableDescriptions[n];
    }
  return NULL;
}

char* vtkGenericEnSightReader::GetDescription(int n, int type)
{
  int i, numMatches = 0;
  
  if (type < 8)
    {
    for (i = 0; i < this->NumberOfVariables; i++)
      {
      if (this->VariableTypes[i] == type)
        {
        if (numMatches == n)
          {
          return this->VariableDescriptions[i];
          }
        else
          {
          numMatches++;
          }
        }
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfVariables; i++)
      {
      if (this->ComplexVariableTypes[i] == type)
        {
        if (numMatches == n)
          {
          return this->ComplexVariableDescriptions[i];
          }
        else
          {
          numMatches++;
          }
        }
      }
    }
  
  return NULL;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::AddVariableType(int variableType)
{
  int size;
  int i;
  int *types;
  
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
  this->VariableTypes[size] = variableType;
  vtkDebugMacro("variable type: " << this->VariableTypes[size]);
}

void vtkGenericEnSightReader::AddComplexVariableType(int variableType)
{
  int i;
  int* types = NULL;
  int size = this->NumberOfComplexVariables;
  
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
  this->ComplexVariableTypes[size] = variableType;
  vtkDebugMacro("complex variable type: "
		<< this->ComplexVariableTypes[size]);
}

int vtkGenericEnSightReader::GetVariableType(int n)
{
  if (n < this->NumberOfVariables)
    {
    return this->VariableTypes[n];
    }
  return -1;
}

int vtkGenericEnSightReader::GetComplexVariableType(int n)
{
  if (n < this->NumberOfComplexVariables)
    {
    return this->ComplexVariableTypes[n];
    }
  return -1;
}

void vtkGenericEnSightReader::ReplaceWildcards(char* fileName, int timeSet,
					       int fileSet)
{
  char line[256], subLine[256];
  int cmpTimeSet, cmpFileSet, fileNameNum;
  
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, this->CaseFileName);
    vtkDebugMacro("full path to case file: " << line);
    }
  else
    {
    strcpy(line, this->CaseFileName);
    }
  
  this->IS = new ifstream(line, ios::in);
  // We already know we have a valid case file if we've gotten to this point.
  
  this->ReadLine(line);
  while (strncmp(line, "TIME", 4) != 0)
    {
    this->ReadLine(line);
    }
  
  this->ReadNextDataLine(line);
  sscanf(line, " %*s %*s %d", &cmpTimeSet);
  while (cmpTimeSet != timeSet)
    {
    this->ReadNextDataLine(line);
    this->ReadNextDataLine(line);
    sscanf(line, " %s", subLine);
    if (strncmp(subLine, "filename", 8) == 0)
      {
      this->ReadNextDataLine(line);
      }
    if (strncmp(subLine, "filename", 8) == 0)
      {
      this->ReadNextDataLine(line);
      }
    sscanf(line, " %*s %*s %d", &cmpTimeSet);
    }
  
  this->ReadNextDataLine(line); // number of timesteps
  this->ReadNextDataLine(line);
  sscanf(line, " %s", subLine);
  if (strncmp(subLine, "filename", 8) == 0)
    {
    sscanf(line, " %*s %s", subLine);
    if (strncmp(subLine, "start", 5) == 0)
      {
      sscanf(line, " %*s %*s %*s %d", &fileNameNum);
      }
    else
      {
      sscanf(line, " %*s %*s %d", &fileNameNum);
      }
    this->ReplaceWildcardsHelper(fileName, fileNameNum);
    }
  else
    {
    while (strncmp(line, "FILE", 4) != 0)
      {
      this->ReadLine(line);
      }
    this->ReadNextDataLine(line);
    sscanf(line, " %*s %*s %d", &cmpFileSet);
    while (cmpFileSet != fileSet)
      {
      this->ReadNextDataLine(line);
      this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);
      if (strncmp(subLine, "filename", 8) == 0)
	{
	this->ReadNextDataLine(line);
	}
      sscanf(line, " %*s %*s %d", &cmpFileSet);
      }
    this->ReadNextDataLine(line);
    sscanf(line, " %*s %*s %d", &fileNameNum);
    this->ReplaceWildcardsHelper(fileName, fileNameNum);
    }
}

void vtkGenericEnSightReader::ReplaceWildcardsHelper(char* fileName, int num)
{
  int wildcardPos, numWildcards, numDigits = 1, i;
  int tmpNum = num, multTen = 1;
  char newChar;
  int newNum;
  
  wildcardPos = strcspn(fileName, "*");
  numWildcards = strspn(fileName + wildcardPos, "*");
  
  tmpNum /= 10;
  while (tmpNum >= 1)
    {
    numDigits++;
    multTen *= 10;
    tmpNum /= 10;
    }
  
  for (i = 0; i < numWildcards - numDigits; i++)
    {
    fileName[i + wildcardPos] = '0';
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
    
    fileName[i + wildcardPos] = newChar;
    tmpNum -= multTen * newNum;
    multTen /= 10;
    }
}

void vtkGenericEnSightReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetSource::PrintSelf(os,indent);

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
}

