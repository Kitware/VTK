/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericEnSightReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericEnSightReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArrayCollection.h"
#include "vtkDataArraySelection.h"
#include "vtkEnSight6BinaryReader.h"
#include "vtkEnSight6Reader.h"
#include "vtkEnSightGoldBinaryReader.h"
#include "vtkEnSightGoldReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkIdListCollection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <vtkstd/string>
#include <vtkstd/map>
#include <assert.h>
#include <ctype.h> /* isspace */

vtkStandardNewMacro(vtkGenericEnSightReader);

vtkCxxSetObjectMacro(vtkGenericEnSightReader,TimeSets,
                     vtkDataArrayCollection);

class TranslationTableType
{
public:
  vtkstd::map<int,int> PartIdMap;
};


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

  this->TimeValueInitialized = 0;

  this->TimeSets = NULL;

  this->ReadAllVariables = 1;

  this->ByteOrder = FILE_UNKNOWN_ENDIAN;

  this->ParticleCoordinatesByIndex = 0;

  this->EnSightVersion = -1;

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkGenericEnSightReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);
  this->SelectionModifiedDoNotCallModified = 0;
  this->TranslationTable = new TranslationTableType;

  this->SetNumberOfInputPorts(0);
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

  this->SetTimeSets(0);
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
  delete this->TranslationTable;
}

//-----------------------------------------------------------------------------
int vtkGenericEnSightReader::CanReadFile(const char *casefilename)
{
  vtkGenericEnSightReader *reader = vtkGenericEnSightReader::New();
  reader->SetCaseFileName(casefilename);
  int type = reader->DetermineEnSightVersion(1);
  reader->Delete();
  return (type != -1);
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int i;

  if ( !this->Reader )
    {
    return 0;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Set the real reader's data array selections from ours.
  this->SetReaderDataArraySelectionSetsFromSelf();

  this->Reader->SetTimeValue(this->GetTimeValue());
  this->Reader->UpdateInformation();
  vtkInformation* tmpOutInfo =
    this->Reader->GetExecutive()->GetOutputInformation(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    tmpOutInfo->CopyEntry(
      outInfo,
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    }

  // GHOST LEVEL
  // uncomment these lines below if you want to active
  // the ghost level system
  /*
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    tmpOutInfo->CopyEntry(
      outInfo,
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    }
   */

  this->Reader->Update();

  this->NumberOfScalarsPerNode = this->Reader->GetNumberOfScalarsPerNode();
  this->NumberOfVectorsPerNode = this->Reader->GetNumberOfVectorsPerNode();
  this->NumberOfTensorsSymmPerNode =
    this->Reader->GetNumberOfTensorsSymmPerNode();
  this->NumberOfScalarsPerElement =
    this->Reader->GetNumberOfScalarsPerElement();
  this->NumberOfVectorsPerElement =
    this->Reader->GetNumberOfVectorsPerElement();
  this->NumberOfTensorsSymmPerElement =
    this->Reader->GetNumberOfTensorsSymmPerElement();
  this->NumberOfScalarsPerMeasuredNode =
    this->Reader->GetNumberOfScalarsPerMeasuredNode();
  this->NumberOfVectorsPerMeasuredNode =
    this->Reader->GetNumberOfVectorsPerMeasuredNode();
  this->NumberOfComplexScalarsPerNode =
    this->Reader->GetNumberOfComplexScalarsPerNode();
  this->NumberOfComplexVectorsPerNode =
    this->Reader->GetNumberOfComplexVectorsPerNode();
  this->NumberOfComplexScalarsPerElement =
    this->Reader->GetNumberOfComplexScalarsPerElement();
  this->NumberOfComplexVectorsPerElement =
    this->Reader->GetNumberOfComplexScalarsPerElement();

  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(this->Reader->GetOutput());

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
    this->NumberOfVariables = 0;
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
    this->NumberOfComplexVariables = 0;
    }

  for (i = 0; i < this->Reader->GetNumberOfVariables(); i++)
    {
    this->AddVariableDescription(this->Reader->GetDescription(i));
    this->AddVariableType(this->Reader->GetVariableType(i));
    this->NumberOfVariables++;
    }
  for (i = 0; i < this->Reader->GetNumberOfComplexVariables(); i++)
    {
    this->AddComplexVariableDescription(
      this->Reader->GetComplexDescription(i));
    this->AddComplexVariableType(this->Reader->GetComplexVariableType(i));
    this->NumberOfComplexVariables++;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetTimeValue(float value)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TimeValue to " << value);
  if (this->TimeValue != value)
    {
    this->TimeValue = value;
    this->Modified();
    }
  this->TimeValueInitialized = 1;
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::DetermineEnSightVersion(int quiet)
{
  char line[256], subLine[256], subLine1[256], subLine2[256], binaryLine[81];
  char *binaryLinePtr;
  int stringRead;
  int timeSet = 1, fileSet = 1;
  int xtimeSet= 1, xfileSet= 1;
  char *fileName = NULL;
  int lineRead;
  if (!this->CaseFileName)
    {
    if (!quiet) vtkErrorMacro("A case file name must be specified.");
    return -1;
    }
  vtkstd::string sfilename = "";
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
      {
      sfilename += "/";
      }
    sfilename += this->CaseFileName;
    vtkDebugMacro("full path to case file: "
                  << sfilename.c_str());
    }
  else
    {
    sfilename = this->CaseFileName;
    }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
    {
    if (!quiet) vtkErrorMacro("Unable to open file: " << sfilename.c_str());
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
      sscanf(line, " %*s %s %s", subLine1, subLine2);
      if (strncmp(subLine1,"ensight",7) == 0)
        {
        if (strncmp(subLine2,"gold",4) == 0)
          {
          lineRead = this->ReadNextDataLine(line);
          while (strncmp(line, "GEOMETRY", 8) != 0 && lineRead != 0)
            {
            lineRead = this->ReadNextDataLine(line);
            }
          if (lineRead == 0)
            {
            return -1;
            }
          if (strncmp(line, "GEOMETRY", 8) == 0)
            {
            // found the GEOMETRY section
            vtkDebugMacro("*** GEOMETRY section");

            this->ReadNextDataLine(line);
            if (strncmp(line, "model:", 6) == 0)
              {
              if (sscanf(line, " %*s %d %d %s", &xtimeSet, &fileSet, subLine) == 3)
                {
                timeSet = xtimeSet;
                fileSet = xfileSet;
                this->SetGeometryFileName(subLine);
                }
              else if (sscanf(line, " %*s %d%*[ \t]%s", &xtimeSet, subLine) == 2)
                {
                timeSet = xtimeSet;
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
              if (!quiet)
                {
                vtkErrorMacro(
                      "A GeometryFileName must be specified in the case file.");
                }
              return 0;
              }
            if (strrchr(fileName, '*') != NULL)
              {
              // RE-open case file; find right time set and fill in
              // wildcards from there if possible; if not, then find right
              // file set and fill in wildcards from there.
              if ( this->ReplaceWildcards(fileName, timeSet, fileSet) == 0 )
                {
                if (!quiet)
                  {
                  vtkErrorMacro(
                    "upon DetermineEnSightVersion()'s call to ReplaceWildCards()");
                  }
                return -1;
                }
              }
            sfilename = "";
            if (this->FilePath)
              {
              sfilename = this->FilePath;
              if (sfilename.at(sfilename.length()-1) != '/')
                {
                sfilename += "/";
                }
              sfilename += fileName;
              vtkDebugMacro("full path to geometry file: "
                            << sfilename.c_str());
              }
            else
              {
              sfilename = fileName;
              }

            // got full path to geometry file

            this->IFile = fopen(sfilename.c_str(), "rb");
            if (this->IFile == NULL)
              {
              if (!quiet)
                {
                vtkErrorMacro("Unable to open file: " << sfilename.c_str());
                vtkWarningMacro("Assuming binary file.");
                }
              this->IFile = NULL;
              delete [] fileName;
              return vtkGenericEnSightReader::ENSIGHT_GOLD_BINARY;
              } // end if IFile == NULL

            this->ReadBinaryLine(binaryLine);
            binaryLine[80] = '\0';
            // because fortran stores 4 length bytes at the start,
            // if the strlen is less than 4, skip the first 4
            // and jump to the start of the actual string
            binaryLinePtr = &binaryLine[0];
            if (strlen(binaryLine)<4)
              {
              binaryLinePtr = &binaryLine[4];
              }
            sscanf(binaryLinePtr, " %*s %s", subLine);
            // If the file is ascii, there might not be a null
            // terminator. This leads to a UMR in sscanf
            if (strncmp(subLine,"Binary",6) == 0 ||
                strncmp(subLine,"binary",6) == 0)
              {
              fclose(this->IFile);
              this->IFile = NULL;
              delete [] fileName;
              return vtkGenericEnSightReader::ENSIGHT_GOLD_BINARY;
              } //end if binary

            fclose(this->IFile);
            this->IFile = NULL;
            delete [] fileName;
            return vtkGenericEnSightReader::ENSIGHT_GOLD;
            } // if we found the geometry section in the case file
          } // if ensight gold file
        } // if regular ensight file (not master_server)
      else if (strncmp(subLine1,"master_server",13) == 0)
        {
        return vtkGenericEnSightReader::ENSIGHT_MASTER_SERVER;
        }
      } // if the type line is like "type: xxxx xxxx"
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
          if (sscanf(line, " %*s %d %d %s", &xtimeSet, &fileSet, subLine) == 3)
            {
            timeSet = xtimeSet;
            fileSet = xfileSet;
            this->SetGeometryFileName(subLine);
            }
          else if (sscanf(line, " %*s %d%*[ \t]%s", &xtimeSet, subLine) == 2)
            {
            timeSet = xtimeSet;
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
          if (!quiet)
            {
            vtkErrorMacro(
                      "A GeometryFileName must be specified in the case file.");
            }
          return 0;
          }
        if (strrchr(fileName, '*') != NULL)
          {
          // reopen case file; find right time set and fill in wildcards from
          // there if possible; if not, then find right file set and fill in
          // wildcards from there.
          this->ReplaceWildcards(fileName, timeSet, fileSet);
          }
        sfilename = "";
        if (this->FilePath)
          {
          sfilename = this->FilePath;
          if (sfilename.at(sfilename.length()-1) != '/')
            {
            sfilename += "/";
            }
          sfilename += fileName;
          vtkDebugMacro("full path to geometry file: "
                        << sfilename.c_str());
          }
        else
          {
          sfilename = fileName;
          }

        // got full path to geometry file

        this->IFile = fopen(sfilename.c_str(), "rb");
        if (this->IFile == NULL)
          {
          if (!quiet)
            {
            vtkErrorMacro("Unable to open file: " << sfilename.c_str());
            vtkWarningMacro("Assuming binary file.");
            }
          this->IFile = NULL;
          delete [] fileName;
          return vtkGenericEnSightReader::ENSIGHT_6_BINARY;
          } // end if IFile == NULL

        this->ReadBinaryLine(binaryLine);
        // If the file is ascii, there might not be a null
        // terminator. This leads to a UMR in sscanf
        binaryLine[80] = '\0';
        sscanf(binaryLine, " %*s %s", subLine);
        if (strncmp(subLine,"Binary",6) == 0)
          {
          fclose(this->IFile);
          this->IFile = NULL;
          delete [] fileName;
          return vtkGenericEnSightReader::ENSIGHT_6_BINARY;
          } //end if binary

        fclose(this->IFile);
        this->IFile = NULL;
        delete [] fileName;
        return vtkGenericEnSightReader::ENSIGHT_6;
        } // if we found the geometry section in the case file
      } // not ensight gold
    } // if we found the format section in the case file

  if (fileName)
    {
    delete [] fileName;
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetCaseFileName(const char* fileName)
{
  char *endingSlash;
  char *path, *newFileName;
  int position, numChars;

  if ( this->CaseFileName && fileName &&
       (!strcmp(this->CaseFileName, fileName)))
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

  this->Modified();
  if (!this->CaseFileName)
    {
    return;
    }

  // strip off the path and save it as FilePath if it was included in the
  // filename
  endingSlash = strrchr(this->CaseFileName, '/');
  if(endingSlash == NULL)
    {
    // check Windows directory separator
    endingSlash = strrchr(this->CaseFileName, '\\');
    }

  if (endingSlash)
    {
    position = endingSlash - this->CaseFileName + 1;
    path = new char[position + 1];
    numChars = static_cast<int>(strlen(this->CaseFileName));
    newFileName = new char[numChars - position + 1];
    strcpy(path, "");
    strncat(path, this->CaseFileName, position);
    this->SetFilePath(path);
    strcpy(newFileName, this->CaseFileName + position);
    strcpy(this->CaseFileName, newFileName);
    delete [] path;
    delete [] newFileName;
    }
}

//----------------------------------------------------------------------------
// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int vtkGenericEnSightReader::ReadLine(char result[256])
{
  this->IS->getline(result,256);
//  if (this->IS->eof())
  if (this->IS->fail())
      {
      // Reset the error flag before returning. This way, we can keep working
      // if we handle the error downstream.
      this->IS->clear();
      return 0;
      }

  return 1;
}

//----------------------------------------------------------------------------
// Internal function to read in a line (from a binary file) up
// to 80 characters.  Returns zero if there was an error.
int vtkGenericEnSightReader::ReadBinaryLine(char result[80])
{
  int n = static_cast<int>(fread(result, sizeof(char), 80, this->IFile));

  if ((n<80) || feof(this->IFile) || ferror(this->IFile))
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
// Internal function that skips blank lines and comment lines
// and reads the next line it finds (up to 256 characters).
// Returns 0 is there was an error.
int vtkGenericEnSightReader::ReadNextDataLine(char result[256])
{
  int isComment = 1;
  int value = 1;

  while( isComment && value )
    {
    value = this->ReadLine(result);
    if( *result && result[0] != '#' )
      {
      size_t len = strlen( result );
      unsigned int i = 0;
      while( i < len && (static_cast<unsigned int>(result[i]) <= 255) && isspace(result[i]) )
        {
        ++i;
        }
      // If there was only space characters this is a comment, thus skip it
      if( i != len )
        {
        // The line was not empty, not begining by '#' and not composed
        // of only white space, this is not a comment
        isComment = 0;
        }
      }
    }

  return value;
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
  {
  int version = this->DetermineEnSightVersion();
  int createReader = 1;
  if (version == vtkGenericEnSightReader::ENSIGHT_6)
    {
    vtkDebugMacro("EnSight6");
    if (this->Reader)
      {
      if (strcmp(this->Reader->GetClassName(), "vtkEnSight6Reader") == 0)
        {
        createReader = 0;
        }
      else
        {
        this->Reader->Delete();
        }
      }
    if (createReader)
      {
      this->Reader = vtkEnSight6Reader::New();
      }
    }
  else if (version == vtkGenericEnSightReader::ENSIGHT_6_BINARY)
    {
    vtkDebugMacro("EnSight6 binary");
    if (this->Reader)
      {
      if (strcmp(this->Reader->GetClassName(), "vtkEnSight6BinaryReader") == 0)
        {
        createReader = 0;
        }
      else
        {
        this->Reader->Delete();
        }
      }
    if (createReader)
      {
      this->Reader = vtkEnSight6BinaryReader::New();
      }
    }
  else if (version == vtkGenericEnSightReader::ENSIGHT_GOLD)
    {
    vtkDebugMacro("EnSightGold");
    if (this->Reader)
      {
      if (strcmp(this->Reader->GetClassName(), "vtkEnSightGoldReader") == 0)
        {
        createReader = 0;
        }
      else
        {
        this->Reader->Delete();
        }
      }
    if (createReader)
      {
      this->Reader = vtkEnSightGoldReader::New();
      }
    }
  else if (version == vtkGenericEnSightReader::ENSIGHT_GOLD_BINARY)
    {
    vtkDebugMacro("EnSightGold binary");
    if (this->Reader)
      {
      if (strcmp(this->Reader->GetClassName(),
        "vtkEnSightGoldBinaryReader") == 0)
        {
        createReader = 0;
        }
      else
        {
        this->Reader->Delete();
        }
      }
    if (createReader)
      {
      this->Reader = vtkEnSightGoldBinaryReader::New();
      }
    }
  else
    {
    vtkErrorMacro("Error determining EnSightVersion");
    this->EnSightVersion = -1;
    return 0;
    }
  this->EnSightVersion = version;

  // Copy current array selections to internal reader.
  this->SetReaderDataArraySelectionSetsFromSelf();
  this->Reader->SetReadAllVariables(this->ReadAllVariables);
  this->Reader->SetCaseFileName(this->GetCaseFileName());
  this->Reader->SetFilePath(this->GetFilePath());

  // The following line, explicitly initializing this->ByteOrder to
  // FILE_UNKNOWN_ENDIAN,  MUST !!NOT!! be removed as it is used to
  // force vtkEnSightGoldBinaryReader::ReadPartId(...) to determine
  // the actual endian type. Otherwise the endian type, the default
  // value from combobox 'Byte Order' of the user interface -------
  // FILE_BIG_ENDIAN unless the user manually toggles the combobox,
  // would be forwarded to  this->Reader->ByteOrder through the next
  // line and therefore would prevent vtkEnSightGoldBinaryReader::
  // ReadPartId(...) from automatically checking the endian type. As
  // a consequence, little-endian files such as the one mentioned in
  // bug #0008237 would not be loadable. The following line might be
  // removed ONLY WHEN the combobox is removed through
  // ParaViews\Servers\ServerManager\Resources\readers.xml.
  // Thus it is highly suggested that the following line be retained
  // to guarantee the fix to bug #0007424 -- automatic determination
  // of the endian type.
  this->ByteOrder = FILE_UNKNOWN_ENDIAN;

  this->Reader->SetByteOrder(this->ByteOrder);
  this->Reader->RequestInformation(request, inputVector, outputVector);
  this->Reader->SetParticleCoordinatesByIndex(this->ParticleCoordinatesByIndex);

  this->SetTimeSets(this->Reader->GetTimeSets());
  if(!this->TimeValueInitialized)
    {
    this->SetTimeValue(this->Reader->GetTimeValue());
    }
  this->MinimumTimeValue = this->Reader->GetMinimumTimeValue();
  this->MaximumTimeValue = this->Reader->GetMaximumTimeValue();

  // Copy new data array selections from internal reader.
  this->SetDataArraySelectionSetsFromReader();

  return 1;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::AddVariableDescription(const char* description)
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
  if (this->VariableDescriptions)
    {
    delete [] this->VariableDescriptions;
    }

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

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::AddComplexVariableDescription(const char* description)
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

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::GetNumberOfVariables(int type)
{
  switch (type)
    {
    case vtkEnSightReader::SCALAR_PER_NODE:
      return this->GetNumberOfScalarsPerNode();
    case vtkEnSightReader::VECTOR_PER_NODE:
      return this->GetNumberOfVectorsPerNode();
    case vtkEnSightReader::TENSOR_SYMM_PER_NODE:
      return this->GetNumberOfTensorsSymmPerNode();
    case vtkEnSightReader::SCALAR_PER_ELEMENT:
      return this->GetNumberOfScalarsPerElement();
    case vtkEnSightReader::VECTOR_PER_ELEMENT:
      return this->GetNumberOfVectorsPerElement();
    case vtkEnSightReader::TENSOR_SYMM_PER_ELEMENT:
      return this->GetNumberOfTensorsSymmPerElement();
    case vtkEnSightReader::SCALAR_PER_MEASURED_NODE:
      return this->GetNumberOfScalarsPerMeasuredNode();
    case vtkEnSightReader::VECTOR_PER_MEASURED_NODE:
      return this->GetNumberOfVectorsPerMeasuredNode();
    case vtkEnSightReader::COMPLEX_SCALAR_PER_NODE:
      return this->GetNumberOfComplexScalarsPerNode();
    case vtkEnSightReader::COMPLEX_VECTOR_PER_NODE:
      return this->GetNumberOfComplexVectorsPerNode();
    case vtkEnSightReader::COMPLEX_SCALAR_PER_ELEMENT:
      return this->GetNumberOfComplexScalarsPerElement();
    case vtkEnSightReader::COMPLEX_VECTOR_PER_ELEMENT:
      return this->GetNumberOfComplexVectorsPerElement();
    default:
      vtkWarningMacro("unknow variable type");
      return -1;
    }
}

//----------------------------------------------------------------------------
const char* vtkGenericEnSightReader::GetDescription(int n)
{
  if (n < this->NumberOfVariables)
    {
    return this->VariableDescriptions[n];
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkGenericEnSightReader::GetComplexDescription(int n)
{
  if (n < this->NumberOfComplexVariables)
    {
    return this->ComplexVariableDescriptions[n];
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkGenericEnSightReader::GetDescription(int n, int type)
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::GetVariableType(int n)
{
  if (n < this->NumberOfVariables)
    {
    return this->VariableTypes[n];
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::GetComplexVariableType(int n)
{
  if (n < this->NumberOfComplexVariables)
    {
    return this->ComplexVariableTypes[n];
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::ReplaceWildcards(char* fileName, int timeSet,
                                              int fileSet)
{
  char line[256],  subLine[256];
  int  cmpTimeSet, cmpFileSet, fileNameNum, lineReadResult, lineScanResult;

  vtkstd::string sfilename;
  if ( this->FilePath )
    {
    sfilename = this->FilePath;
    if (  sfilename.at( sfilename.length() - 1 ) != '/'  )
      {
      sfilename += "/";
      }
    sfilename += this->CaseFileName;
    vtkDebugMacro( "full path to case file: " << sfilename.c_str() );
    }
  else
    {
    sfilename = this->CaseFileName;
    }

  // We have got a valid CASE file name
  this->IS = new ifstream( sfilename.c_str(), ios::in );

  // Below is a revamped version of the code in support of inline & non-inline
  // file name numbers, in a CASE file, of which the first one is obtained to
  // make a geometry file name, through wildcards replacement, used to determine
  // the specific EnSight version.

  // Locate the 'TIME' section
  do
    {
    if ( this->ReadNextDataLine(line) == 0 )
      {
      vtkErrorMacro(
        "ReplaceWildCards() failed to find the 'TIME' section!" );
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    } while ( strncmp(line, "TIME", 4) != 0 );


  // Locate the very 'time set' entry by the index
  cmpTimeSet = -10000;
  do
    {
    if ( this->ReadNextDataLine(line) == 0 )
      {
      vtkErrorMacro(
        "ReplaceWildCards() failed to find the target 'time set' entry!" );
      delete this->IS;
      this->IS = NULL;
      return 0;
      }

      // 'time set: <int>' --- where to obtain cmpTimeSet, a time set index
      lineScanResult = sscanf(line, "%*s %s %d", subLine, &cmpTimeSet);
    } while ( lineScanResult != 2 || strncmp(line, "time", 4) != 0 ||
              strncmp(subLine, "set", 3) != 0 || cmpTimeSet != timeSet );

  // Skip 'time set: <int>' and 'number of steps: <int>' to go to
  // 'filename xxx: ...' --- where to obtain the actual file name number(s)
  for ( int  i = 0;  i < 2;  i ++ )
    {
    lineReadResult = this->ReadNextDataLine(line);
    if ( lineReadResult == 0 ||
         // check 'filename xxx: ...' upon the second line (i = 1)
         ( i == 1 &&
           ( strncmp(line, "filename", 8)    != 0 ||
             sscanf(line, "%*s %s", subLine) != 1
           )
         )
       )
      {
      vtkErrorMacro(
        "ReplaceWildCards() failed to find the target 'filename ...: ...' entry!" );
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    }

  fileNameNum = -10000;

  // 'filename numbers: ...'
  if ( strncmp(subLine, "numbers", 7) == 0 )
    {
    // The filename number(s) may be provided on the line(s) following
    // 'filename numbers:', as is usually the case --- not "inline". Thus we
    // need to go to the FIRST line that indeed contains the filename number(s).
    // Note that we only need to obtain the FIRST file name number since a
    // single geometry file allows us to determine the EnSight version. This is
    // based on the reasonable assumption that all geometry files referenced by
    // a CASE file have the same EnSight version.

    // not "inline"
    if ( sscanf(line, "%*s %*s %d", &fileNameNum) != 1 )
      {
      // let's go to the next VALID line that might be several empty lines apart
      if ( this->ReadNextDataLine(line) == 0 )
        {
        vtkErrorMacro(
          "ReplaceWildCards() failed to obtain any non-inline file name number!" );
        delete this->IS;
        this->IS = NULL;
        return 0;
        }

      // obtain the first file name number from the next valid line
      sscanf(line, "%d", &fileNameNum);
      }
    }
  // 'filename start number: ...' --- followed by 'filename increment: ...'
  else
    {
    char  subSubLine[256];
    lineScanResult = sscanf( line, "%*s %s %s %d",
                             subLine, subSubLine, &fileNameNum );
    if ( lineScanResult != 3 ||
         strncmp(subLine,    "start",  5) != 0 ||
         strncmp(subSubLine, "number", 6) != 0
       )
      {
      vtkErrorMacro(
        "ReplaceWildCards() failed to find 'filename start number: <int>'!" );
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    }

  // Let's resort to the 'FILE' section, just in case of a failure so far
  if ( fileNameNum == -10000 )
    {
    // Locate the 'FILE' section
    do
      {
      if ( this->ReadNextDataLine(line) == 0 )
        {
        vtkErrorMacro(
          "ReplaceWildCards() failed to find the optional 'FILE' section!" );
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
      } while ( strncmp(line, "FILE", 4) != 0 );

    // Locate the very 'file set' entry by the index
    cmpFileSet = -10000;
    do
      {
      if ( this->ReadNextDataLine(line) == 0 )
        {
        vtkErrorMacro(
          "ReplaceWildCards() failed to find the target 'file set' entry!" );
        delete this->IS;
        this->IS = NULL;
        return 0;
        }

      // 'file set: <int>' --- to obtain cmpFileSet, a file set index
      lineScanResult = sscanf(line, "%*s %s %d", subLine, &cmpFileSet);
      } while ( lineScanResult != 2 || strncmp(line, "file", 4) != 0 ||
                strncmp(subLine, "set", 3) != 0 || cmpFileSet != fileSet );

    // Skip 'file set: <int>' to go to
    // 'filename index: <int>' --- where to obtain ONE actual file name
    // Note that we here do NOT allow any non-'inline' scenarios since
    // there is ONE AND ONLY ONE integer value, within a 'filename index: <int>'
    // entry, that is used to specify a file name index. Thus any violation
    // of this reasonable assumption is considered to use an invalid EnSight
    // format that needs to be corrected by the EnSight CASE file user.
    lineReadResult = this->ReadNextDataLine(line);
    lineScanResult = sscanf( line, "%*s %s %d", subLine, &fileNameNum );
    if ( lineReadResult == 0               || lineScanResult != 2 ||
         strncmp(line, "filename", 8) != 0 || strncmp(subLine, "index", 5) != 0
       )
      {
      vtkErrorMacro(
        "ReplaceWildCards() failed to find 'filename index: <int>'!" );
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    }

  // So far we have got a file name index
  this->ReplaceWildcardsHelper(fileName, fileNameNum);
  delete this->IS;
  this->IS = NULL;

  return  1;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::ReplaceWildcardsHelper(char* fileName, int num)
{
  int wildcardPos, numWildcards, numDigits = 1, i;
  int tmpNum = num, multTen = 1;
  char newChar;
  int newNum;

  wildcardPos = static_cast<int>(strcspn(fileName, "*"));
  numWildcards = static_cast<int>(strspn(fileName + wildcardPos, "*"));

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
    assert( newChar == ('0' + newNum) );

    fileName[i + wildcardPos] = newChar;
    tmpNum -= multTen * newNum;
    multTen /= 10;
    }
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

//----------------------------------------------------------------------------
const char *vtkGenericEnSightReader::GetByteOrderAsString()
{
  if ( this->ByteOrder ==  FILE_LITTLE_ENDIAN)
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CaseFileName: "
     << (this->CaseFileName ? this->CaseFileName : "(none)") << endl;
  os << indent << "FilePath: "
     << (this->FilePath ? this->FilePath : "(none)") << endl;
  os << indent << "EnSight Version: "
     <<  this->EnSightVersion << endl;
  os << indent << "NumberOfComplexVariables: "
     << this->NumberOfComplexVariables << endl;
  os << indent << "NumberOfVariables: "
     << this->NumberOfVariables << endl;
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
  os << indent << "ReadAllVariables: " << this->ReadAllVariables << endl;
  os << indent << "ByteOrder: " << this->ByteOrder << endl;
  os << indent << "ParticleCoordinatesByIndex: " << this->ParticleCoordinatesByIndex << endl;
  os << indent << "CellDataArraySelection: " << this->CellDataArraySelection
     << endl;
  os << indent << "PointDataArraySelection: " << this->PointDataArraySelection
     << endl;
  os << indent << "GeometryFileName: " <<
     (this->GeometryFileName ? this->GeometryFileName : "(none)") << endl;
}

//----------------------------------------------------------------------------
char** vtkGenericEnSightReader::CreateStringArray(int numStrings)
{
  char** strings = new char*[numStrings];
  int i;
  for(i=0; i < numStrings; ++i)
    {
    strings[i] = 0;
    }
  return strings;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::DestroyStringArray(int numStrings,
                                                 char** strings)
{
  int i;
  for(i=0; i < numStrings; ++i)
    {
    if(strings[i])
      {
      delete [] strings[i];
      }
    }
  delete[] strings;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetDataArraySelectionSetsFromVariables()
{
  int numPointArrays = (this->NumberOfScalarsPerNode +
                        this->NumberOfVectorsPerNode +
                        this->NumberOfTensorsSymmPerNode +
                        this->NumberOfScalarsPerMeasuredNode +
                        this->NumberOfVectorsPerMeasuredNode +
                        this->NumberOfComplexScalarsPerNode +
                        this->NumberOfComplexVectorsPerNode);
  int numCellArrays = (this->NumberOfScalarsPerElement +
                       this->NumberOfVectorsPerElement +
                       this->NumberOfTensorsSymmPerElement +
                       this->NumberOfComplexScalarsPerElement +
                       this->NumberOfComplexVectorsPerElement);

  char** pointNames = this->CreateStringArray(numPointArrays);
  char** cellNames = this->CreateStringArray(numCellArrays);
  int pointArrayCount = 0;
  int cellArrayCount = 0;

  int i;
  for(i=0; i < this->NumberOfVariables; ++i)
    {
    switch (this->VariableTypes[i])
      {
      case vtkEnSightReader::SCALAR_PER_NODE:
      case vtkEnSightReader::VECTOR_PER_NODE:
      case vtkEnSightReader::TENSOR_SYMM_PER_NODE:
      case vtkEnSightReader::SCALAR_PER_MEASURED_NODE:
      case vtkEnSightReader::VECTOR_PER_MEASURED_NODE:
        pointNames[pointArrayCount] =
          new char[strlen(this->VariableDescriptions[i])+1];
        strcpy(pointNames[pointArrayCount], this->VariableDescriptions[i]);
        ++pointArrayCount;
        break;
      case vtkEnSightReader::SCALAR_PER_ELEMENT:
      case vtkEnSightReader::VECTOR_PER_ELEMENT:
      case vtkEnSightReader::TENSOR_SYMM_PER_ELEMENT:
        cellNames[cellArrayCount] =
          new char[strlen(this->VariableDescriptions[i])+1];
        strcpy(cellNames[cellArrayCount], this->VariableDescriptions[i]);
        ++cellArrayCount;
        break;
      }
    }
  for(i=0; i < this->NumberOfComplexVariables; ++i)
    {
    switch(this->ComplexVariableTypes[i])
      {
      case vtkEnSightReader::COMPLEX_SCALAR_PER_NODE:
      case vtkEnSightReader::COMPLEX_VECTOR_PER_NODE:
        pointNames[pointArrayCount] =
          new char[strlen(this->ComplexVariableDescriptions[i])+1];
        strcpy(pointNames[pointArrayCount],
               this->ComplexVariableDescriptions[i]);
        ++pointArrayCount;
        break;
      case vtkEnSightReader::COMPLEX_SCALAR_PER_ELEMENT:
      case vtkEnSightReader::COMPLEX_VECTOR_PER_ELEMENT:
        cellNames[cellArrayCount] =
          new char[strlen(this->ComplexVariableDescriptions[i])+1];
        strcpy(cellNames[cellArrayCount],
               this->ComplexVariableDescriptions[i]);
        ++cellArrayCount;
        break;
      }
    }

  this->PointDataArraySelection->SetArraysWithDefault(pointNames,
                                                      numPointArrays,
                                                      this->ReadAllVariables);
  this->CellDataArraySelection->SetArraysWithDefault(cellNames,
                                                     numCellArrays,
                                                     this->ReadAllVariables);
  this->DestroyStringArray(numPointArrays, pointNames);
  this->DestroyStringArray(numCellArrays, cellNames);
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetDataArraySelectionSetsFromReader()
{
  this->SelectionModifiedDoNotCallModified = 1;
  this->PointDataArraySelection->CopySelections(
    this->Reader->GetPointDataArraySelection());
  this->CellDataArraySelection->CopySelections(
    this->Reader->GetCellDataArraySelection());
  this->SelectionModifiedDoNotCallModified = 0;
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetReaderDataArraySelectionSetsFromSelf()
{
  // Set the real reader's data array selections from ours.
  this->Reader->GetPointDataArraySelection()->CopySelections(
    this->PointDataArraySelection);
  this->Reader->GetCellDataArraySelection()->CopySelections(
    this->CellDataArraySelection);
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SelectionModifiedCallback(vtkObject*,
                                                        unsigned long,
                                                        void* clientdata,
                                                        void*)
{
  static_cast<vtkGenericEnSightReader*>(clientdata)->SelectionModified();
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SelectionModified()
{
  if(!this->SelectionModifiedDoNotCallModified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkGenericEnSightReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetPointArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkGenericEnSightReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkGenericEnSightReader::SetCellArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->CellDataArraySelection->EnableArray(name);
    }
  else
    {
    this->CellDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::InsertNewPartId(int partId)
{
  int lastId = static_cast<int>(this->TranslationTable->PartIdMap.size());
  this->TranslationTable->PartIdMap.insert(
    vtkstd::map<int,int>::value_type(partId, lastId));
  lastId = this->TranslationTable->PartIdMap[partId];
  //assert( lastId == this->PartIdTranslationTable[partId] );
  return lastId;
}

//----------------------------------------------------------------------------
int vtkGenericEnSightReader::FillOutputPortInformation(int vtkNotUsed(port),
                                                       vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}
