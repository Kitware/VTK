/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMetaImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifdef _MSC_VER
#pragma warning(disable:4018)
#endif

#include "vtkMetaImageWriter.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSetAttributes.h"

#include <string>
#include "vtkmetaio/metaTypes.h"
#include "vtkmetaio/metaUtils.h"
#include "vtkmetaio/metaEvent.h"
#include "vtkmetaio/metaObject.h"
#include "vtkmetaio/metaImageTypes.h"
#include "vtkmetaio/metaImageUtils.h"
#include "vtkmetaio/metaImage.h"

#include <sys/stat.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMetaImageWriter);

//----------------------------------------------------------------------------
vtkMetaImageWriter::vtkMetaImageWriter()
{
  this->MHDFileName = 0;
  this->FileLowerLeft = 1;

  this->MetaImagePtr = new vtkmetaio::MetaImage;
  this->Compress = true;
}

//----------------------------------------------------------------------------
vtkMetaImageWriter::~vtkMetaImageWriter()
{
  this->SetFileName(0);
  delete this->MetaImagePtr;
}

//----------------------------------------------------------------------------
void vtkMetaImageWriter::SetFileName(const char* fname)
{
  this->SetMHDFileName(fname);
  this->Superclass::SetFileName( 0 );
}

//----------------------------------------------------------------------------
void vtkMetaImageWriter::SetRAWFileName(const char* fname)
{
  this->Superclass::SetFileName(fname);
}

//----------------------------------------------------------------------------
char* vtkMetaImageWriter::GetRAWFileName()
{
  return this->Superclass::GetFileName();
}

//----------------------------------------------------------------------------
void vtkMetaImageWriter::Write( )
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkDemandDrivenPipeline::SafeDownCast(
    this->GetInputExecutive(0, 0))->UpdateInformation();

  // Error checking
  if (this->GetInput() == NULL )
  {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return;
  }

  if ( !this->MHDFileName )
  {
    vtkErrorMacro("Output file name not specified");
    return;
  }

  int nDims = 3;
  int * ext = this->GetInputInformation(0, 0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  if ( ext[4] == ext[5] )
  {
    nDims = 2;
    if ( ext[2] == ext[3] )
    {
      nDims = 1;
    }
  }

  this->GetInputAlgorithm()->UpdateExtent(ext);

  double origin[3];
  double spacingDouble[3];
  this->GetInput()->GetOrigin(origin);
  this->GetInput()->GetSpacing(spacingDouble);

  float spacing[3];
  spacing[0] = spacingDouble[0];
  spacing[1] = spacingDouble[1];
  spacing[2] = spacingDouble[2];

  int dimSize[3];
  dimSize[0] = ext[1]-ext[0]+1;
  dimSize[1] = ext[3]-ext[2]+1;
  dimSize[2] = ext[5]-ext[4]+1;

  vtkmetaio::MET_ValueEnumType elementType;

  int scalarType = this->GetInput()->GetScalarType();
  switch ( scalarType )
  {
    case VTK_CHAR:           elementType = vtkmetaio::MET_CHAR; break;
    case VTK_SIGNED_CHAR:    elementType = vtkmetaio::MET_CHAR; break;
    case VTK_UNSIGNED_CHAR:  elementType = vtkmetaio::MET_UCHAR; break;
    case VTK_SHORT:          elementType = vtkmetaio::MET_SHORT; break;
    case VTK_UNSIGNED_SHORT: elementType = vtkmetaio::MET_USHORT; break;
    case VTK_INT:            elementType = vtkmetaio::MET_INT; break;
    case VTK_UNSIGNED_INT:   elementType = vtkmetaio::MET_UINT; break;
    case VTK_LONG:           elementType = vtkmetaio::MET_LONG; break;
    case VTK_UNSIGNED_LONG:  elementType = vtkmetaio::MET_ULONG; break;
    case VTK_FLOAT:          elementType = vtkmetaio::MET_FLOAT; break;
    case VTK_DOUBLE:         elementType = vtkmetaio::MET_DOUBLE; break;
    default:
      vtkErrorMacro("Unknown scalar type." );
      return ;
  }

  origin[0] += ext[0] * spacing[0];
  origin[1] += ext[2] * spacing[1];
  origin[2] += ext[4] * spacing[2];

  int numberOfElements = this->GetInput()->GetNumberOfScalarComponents();

  this->MetaImagePtr->InitializeEssential( nDims,
                                           dimSize,
                                           spacing,
                                           elementType,
                                           numberOfElements,
                                           this->GetInput()
                                               ->GetScalarPointer(ext[0],
                                                                  ext[2],
                                                                  ext[4]),
                                           false );
  this->MetaImagePtr->Position( origin );

  if ( this->GetRAWFileName() )
  {
    this->MetaImagePtr->ElementDataFileName( this->GetRAWFileName() );
  }

  this->SetFileDimensionality(nDims);
  this->MetaImagePtr->CompressedData(Compress);

  this->InvokeEvent(vtkCommand::StartEvent);
  this->UpdateProgress(0.0);
  this->MetaImagePtr->Write(this->MHDFileName);
  this->UpdateProgress(1.0);
  this->InvokeEvent(vtkCommand::EndEvent);
}

//----------------------------------------------------------------------------
void vtkMetaImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MHDFileName: "
               << (this->MHDFileName?this->MHDFileName:"(none)") << endl;
}
