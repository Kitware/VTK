/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataReader.h"

#include "vtkDataObjectTypes.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGenericDataObjectReader.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkCompositeDataReader);
//----------------------------------------------------------------------------
vtkCompositeDataReader::vtkCompositeDataReader()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataReader::~vtkCompositeDataReader()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataReader::GetOutput(int idx)
{
  return vtkCompositeDataSet::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkCompositeDataReader::SetOutput(vtkCompositeDataSet *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::RequestUpdateExtent(
  vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevel;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return 1;
    }

  if (ghostLevel < 0)
    {
    return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::ProcessRequest(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::RequestDataObject(vtkInformation *,
  vtkInformationVector **, vtkInformationVector *outputVector)
{
  int output_type = this->ReadOutputType();
  if (output_type < 0)
    {
    vtkErrorMacro("Failed to read data-type.");
    return 0;
    }

  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  if (!output ||
    !output->IsA(vtkDataObjectTypes::GetClassNameFromTypeId(output_type)))
    {
    output = vtkDataObjectTypes::NewDataObject(output_type);
    output->SetPipelineInformation(outputVector->GetInformationObject(0));
    output->FastDelete();
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::ReadOutputType()
{
  char line[256];
  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return -1;
    }

  // Determine dataset type
  //
  if (!this->ReadString(line))
    {
    vtkDebugMacro(<< "Premature EOF reading dataset keyword");
    return -1;
    }

  if (strncmp(this->LowerCase(line),"dataset",static_cast<unsigned long>(7)) == 0)
    {
    // See iftype is recognized.
    //
    if (!this->ReadString(line))
      {
      vtkDebugMacro(<< "Premature EOF reading type");
      this->CloseVTKFile ();
      return -1;
      }
    this->CloseVTKFile();

    if (strncmp(this->LowerCase(line), "multiblock", strlen("multiblock")) == 0)
      {
      return VTK_MULTIBLOCK_DATA_SET;
      }
    if (strncmp(this->LowerCase(line), "multipiece", strlen("multipiece")) == 0)
      {
      return VTK_MULTIPIECE_DATA_SET;
      }
    if (strncmp(this->LowerCase(line), "hierarchical_box",
        strlen("hierarchical_box")) == 0)
      {
      return VTK_HIERARCHICAL_BOX_DATA_SET;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::RequestData(vtkInformation *, vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  if (!(this->OpenVTKFile()) || !this->ReadHeader())
    {
    return 0;
    }

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::GetData(outputVector, 0);
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::GetData(outputVector, 0);
  vtkHierarchicalBoxDataSet* hb = vtkHierarchicalBoxDataSet::GetData(outputVector, 0);

  // Read the data-type description line which was already read in
  // RequestDataObject() so we just skip it here without any additional
  // validation.
  char line[256];
  if (!this->ReadString(line) || !this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return 0;
    }

  if (mb)
    {
    this->ReadCompositeData(mb);
    }
  else if (mp)
    {
    this->ReadCompositeData(mp);
    }
  else if (hb)
    {
    this->ReadCompositeData(hb);
    }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkMultiBlockDataSet* mb)
{
  char line[256];
  if (!this->ReadString(line))
    {
    vtkErrorMacro("Failed to read block-count");
    return false;
    }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
    {
    vtkErrorMacro("Failed to read CHILDREN.");
    return false;
    }

  unsigned int num_blocks = 0;
  if (!this->Read(&num_blocks))
    {
    vtkErrorMacro("Failed to read number of blocks");
    return false;
    }

  mb->SetNumberOfBlocks(num_blocks);
  for (unsigned int cc=0; cc < num_blocks; cc++)
    {
    if (!this->ReadString(line))
      {
      vtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
      }

    int type;
    if (!this->Read(&type))
      {
      vtkErrorMacro("Failed to read child type.");
      return false;
      }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);

    if (type != -1)
      {
      vtkDataObject* child = this->ReadChild();
      if (!child)
        {
        vtkErrorMacro("Failed to read child.");
        return false;
        }
      mb->SetBlock(cc, child);
      child->FastDelete();
      }
    else
      {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkHierarchicalBoxDataSet* hb)
{
  (void)hb;
  vtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkMultiPieceDataSet* mp)
{
  char line[256];
  if (!this->ReadString(line))
    {
    vtkErrorMacro("Failed to read block-count");
    return false;
    }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
    {
    vtkErrorMacro("Failed to read CHILDREN.");
    return false;
    }

  unsigned int num_pieces = 0;
  if (!this->Read(&num_pieces))
    {
    vtkErrorMacro("Failed to read number of pieces.");
    return false;
    }

  mp->SetNumberOfPieces(num_pieces);
  for (unsigned int cc=0; cc < num_pieces; cc++)
    {
    if (!this->ReadString(line))
      {
      vtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
      }

    int type;
    if (!this->Read(&type))
      {
      vtkErrorMacro("Failed to read child type.");
      return false;
      }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);

    if (type != -1)
      {
      vtkDataObject* child = this->ReadChild();
      if (!child)
        {
        vtkErrorMacro("Failed to read child.");
        return false;
        }
      mp->SetPiece(cc, child);
      child->FastDelete();
      }
    else
      {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
      }
    }

  return true;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataReader::ReadChild()
{
  // This is tricky. Simplistically speaking, we need to read the string for the
  // child and then pass it to a vtkGenericDataObjectReader and get it read.
  // Identifying where the child ends can be tricky since the child itself may
  // be a composite-dataset. Also, we need to avoid copy memory if we are
  // already reading from a string.

  // We can optimize this further when reading in from a string buffer to avoid
  // copying. But we'll do that later.

  unsigned int child_stack_depth = 1;

  vtksys_ios::ostringstream child_data;
  char line[512];
  while (child_stack_depth > 0)
    // read until ENDCHILD (passing over any nested CHILD-ENDCHILD correctly).
    {
    bool new_line = true;
    while (true)
      // read a full line until "\n". This maybe longer than 512 and hence this
      // extra loop.
      {
      this->IS->get(line, 512);
      if (this->IS->fail())
        {
        if (this->IS->eof())
          {
          vtkErrorMacro("Premature EOF.");
          return NULL;
          }
        else
          {
          // else it's possible that we read in an empty line. istream still marks
          // that as fail().
          this->IS->clear();
          }
        }

      if (new_line)
        {
        // these comparisons need to happen only when a new line is
        // started, hence this "if".
        if (strncmp(line, "ENDCHILD", strlen("ENDCHILD")) == 0)
          {
          child_stack_depth--;
          }
        else if (strncmp(line, "CHILD", strlen("CHILD")) == 0)
          {
          // should not match CHILDREN :(
          if (strncmp(line, "CHILDREN", strlen("CHILDREN")) != 0)
            {
            child_stack_depth++;
            }
          }
        }

      if (child_stack_depth > 0)
        {
        // except the last ENDCHILD, all the read content is to passed to the
        // child-reader.
        child_data.write(line, this->IS->gcount());
        }
      new_line = false;
      if (this->IS->peek() == '\n')
        {
        this->IS->ignore(VTK_INT_MAX, '\n');
        // end of line reached.
        child_data << '\n';
        break;
        }
      } // while (true);
    } // while (child_stack_depth > 0);

  vtkGenericDataObjectReader* reader = vtkGenericDataObjectReader::New();
  reader->SetBinaryInputString(child_data.str().c_str(),
    static_cast<int>(child_data.str().size()));
  reader->ReadFromInputStringOn();
  reader->Update();
  vtkDataObject* child = reader->GetOutput(0)->NewInstance();
  child->ShallowCopy(reader->GetOutput(0));
  reader->Delete();
  return child;
}

//----------------------------------------------------------------------------
void vtkCompositeDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
