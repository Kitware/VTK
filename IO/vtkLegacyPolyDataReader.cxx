/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLegacyPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataReader.h"
#include "vtkLegacyPolyDataReader.h"

#include "vtkObjectFactory.h"
#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkErrorCode.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkIncrementalPointLocator.h"

#include <ctype.h>
#include <vtksys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkLegacyPolyDataReader, "1.2");
vtkStandardNewMacro(vtkLegacyPolyDataReader);

// Construct object with merging set to true.
vtkLegacyPolyDataReader::vtkLegacyPolyDataReader()
{
  this->PolyDataReaderPointer = vtkSmartPointer<vtkPolyDataReader>::New();
  this->SetNumberOfInputPorts(0); 
}

vtkLegacyPolyDataReader::~vtkLegacyPolyDataReader()
{
}

vtkPolyData *
vtkLegacyPolyDataReader::
GetOutput(void)
{
   return this->PolyDataReaderPointer->GetOutput();
}

vtkPolyData *
vtkLegacyPolyDataReader::
GetOutput(int idx)
{
  return this->PolyDataReaderPointer->GetOutput(idx);
}

void
vtkLegacyPolyDataReader::
SetFileName(const char *filename)
{
  this->PolyDataReaderPointer->SetFileName( filename );
}

char* 
vtkLegacyPolyDataReader::
GetFileName(void)
{
  return this->PolyDataReaderPointer->GetFileName();
}

int vtkLegacyPolyDataReader::CanReadFile(const char *filename)
{
  char line[256];
  int npts, ncells;
  vtkDebugMacro(<<"Testing ability to read vtk polygonal data...");

  //Update filename if wrong, then check for readability.
  if (( this->PolyDataReaderPointer->GetFileName() == NULL ) ||
      (strcmp(this->PolyDataReaderPointer->GetFileName(), filename)!=0))
    {
    this->PolyDataReaderPointer->SetFileName( filename );
    }
  
  if ( !(this->PolyDataReaderPointer->OpenVTKFile()) ||
       !this->PolyDataReaderPointer->ReadHeader())
    {
    return 0;
    }
  //
  // Read polygonal data specific stuff
  //
  if (!this->PolyDataReaderPointer->ReadString(line))
    {
    this->PolyDataReaderPointer->CloseVTKFile();
    return 0;
    }

  if ( !strncmp(this->PolyDataReaderPointer->LowerCase(line),"dataset",(unsigned long)7) )
    {
    //
    // Make sure we're reading right type of geometry
    //
    if (!this->PolyDataReaderPointer->ReadString(line))
      {
      this->PolyDataReaderPointer->CloseVTKFile();
      return 0;
      } 

    if ( strncmp(this->PolyDataReaderPointer->LowerCase(line),"polydata",8) )
      {
      this->PolyDataReaderPointer->CloseVTKFile();
      return 0;
      }
    }
  else if ( ! strncmp(line, "cell_data", 9) )
    {
    if (!this->PolyDataReaderPointer->Read(&ncells))
      {
      this->PolyDataReaderPointer->CloseVTKFile ();
      return 0;
      }
    }
  else if ( ! strncmp(line, "point_data", 10) )
    {
    if (!this->PolyDataReaderPointer->Read(&npts))
      {
        this->PolyDataReaderPointer->CloseVTKFile ();
        return 0;
      }
    }
  else
    {
    this->PolyDataReaderPointer->CloseVTKFile ();
    return 0;
    }
  
    return 1; 
}

int
vtkLegacyPolyDataReader::
RequestData( vtkInformation *request, vtkInformationVector **inputVector,
             vtkInformationVector *outputVector)
{
   return this->PolyDataReaderPointer->RequestData(
      request, inputVector, outputVector); 
}

int
vtkLegacyPolyDataReader::
RequestUpdateExtent( vtkInformation *request, vtkInformationVector **inputVector,
                     vtkInformationVector *outputVector)
{
   return this->PolyDataReaderPointer->RequestUpdateExtent(
      request, inputVector, outputVector);
}
  
int
vtkLegacyPolyDataReader::
FillOutputPortInformation(int port, vtkInformation *output)
{ 
   return this->PolyDataReaderPointer->FillOutputPortInformation( port, output);
}


void vtkLegacyPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";

  this->PolyDataReaderPointer->PrintSelf( os, indent );
}

