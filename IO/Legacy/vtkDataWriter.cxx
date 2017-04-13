/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataWriter.h"

#include "vtkBitArray.h"
#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKeyLookup.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkIntArray.h"
#include "vtkLegacyReaderVersion.h"
#include "vtkLongArray.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeTraits.h"
#include "vtkTypeUInt64Array.h"
#include "vtkUnicodeStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVariantArray.h"

#include <cstdio>
#include <sstream>

vtkStandardNewMacro(vtkDataWriter);

// this undef is required on the hp. vtkMutexLock ends up including
// /usr/inclue/dce/cma_ux.h which has the gall to #define write as cma_write

#ifdef write
#undef write
#endif

// Standard trick to use snprintf on MSVC.
#if defined(_MSC_VER) && (_MSC_VER < 1900)
# define snprintf _snprintf
#endif

// Created object with default header, ASCII format, and default names for
// scalars, vectors, tensors, normals, and texture coordinates.
vtkDataWriter::vtkDataWriter()
{
  this->FileName = NULL;

  this->Header = new char[257];
  strcpy(this->Header,"vtk output");
  this->FileType = VTK_ASCII;

  this->ScalarsName = 0;
  this->VectorsName = 0;
  this->TensorsName = 0;
  this->NormalsName = 0;
  this->TCoordsName = 0;
  this->GlobalIdsName = 0;
  this->PedigreeIdsName = 0;
  this->EdgeFlagsName = 0;

  this->LookupTableName = new char[13];
  strcpy(this->LookupTableName,"lookup_table");

  this->FieldDataName = new char[10];
  strcpy(this->FieldDataName,"FieldData");

  this->WriteToOutputString = 0;
  this->OutputString = NULL;
  this->OutputStringLength = 0;
  this->WriteArrayMetaData = true;
}

vtkDataWriter::~vtkDataWriter()
{
  delete [] this->FileName;
  delete [] this->Header;
  delete [] this->ScalarsName;
  delete [] this->VectorsName;
  delete [] this->TensorsName;
  delete [] this->NormalsName;
  delete [] this->TCoordsName;
  delete [] this->GlobalIdsName;
  delete [] this->PedigreeIdsName;
  delete [] this->EdgeFlagsName;
  delete [] this->LookupTableName;
  delete [] this->FieldDataName;

  delete [] this->OutputString;
  this->OutputString = NULL;
  this->OutputStringLength = 0;
}

// Open a vtk data file. Returns NULL if error.
ostream *vtkDataWriter::OpenVTKFile()
{
  ostream *fptr;

  if ((!this->WriteToOutputString) && ( !this->FileName ))
  {
    vtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return NULL;
  }

  vtkDebugMacro(<<"Opening vtk file for writing...");

  if (this->WriteToOutputString)
  {
    // Get rid of any old output string.
    delete [] this->OutputString;
    this->OutputString = NULL;
    this->OutputStringLength = 0;

    // Allocate the new output string. (Note: this will only work with binary).
    if (!this->GetInputExecutive(0, 0))
    {
      vtkErrorMacro(<< "No input! Can't write!");
      return NULL;
    }
    this->GetInputExecutive(0, 0)->Update();
    /// OutputString will be allocated on CloseVTKFile().
    /// this->OutputStringAllocatedLength =
    ///   static_cast<int> (500+ 1024 * input->GetActualMemorySize());
    /// this->OutputString = new char[this->OutputStringAllocatedLength];

    fptr = new std::ostringstream;
  }
  else
  {
    if ( this->FileType == VTK_ASCII )
    {
      fptr = new ofstream(this->FileName, ios::out);
    }
    else
    {
#ifdef _WIN32
      fptr = new ofstream(this->FileName, ios::out | ios::binary);
#else
      fptr = new ofstream(this->FileName, ios::out);
#endif
    }
  }

  if (fptr->fail())
  {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    delete fptr;
    return NULL;
  }

  return fptr;
}

// Write the header of a vtk data file. Returns 0 if error.
int vtkDataWriter::WriteHeader(ostream *fp)
{
  vtkDebugMacro(<<"Writing header...");

  *fp << "# vtk DataFile Version " << vtkLegacyReaderMajorVersion << "."
      << vtkLegacyReaderMinorVersion << "\n";
  *fp << this->Header << "\n";

  if ( this->FileType == VTK_ASCII )
  {
    *fp << "ASCII\n";
  }
  else
  {
    *fp << "BINARY\n";
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

// Write the cell data (e.g., scalars, vectors, ...) of a vtk dataset.
// Returns 0 if error.
int vtkDataWriter::WriteCellData(ostream *fp, vtkDataSet *ds)
{
  vtkIdType numCells;
  vtkDataArray *scalars;
  vtkDataArray *vectors;
  vtkDataArray *normals;
  vtkDataArray *tcoords;
  vtkDataArray *tensors;
  vtkDataArray *globalIds;
  vtkAbstractArray *pedigreeIds;
  vtkFieldData *field;
  vtkCellData *cd=ds->GetCellData();

  vtkDebugMacro(<<"Writing cell data...");

  numCells = ds->GetNumberOfCells();
  if(numCells <= 0)
  {
    vtkDebugMacro(<<"No cell data to write!");
    return 1;
  }

  scalars = cd->GetScalars();
  if(scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = 0;

  vectors = cd->GetVectors();
  if(vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = 0;

  normals = cd->GetNormals();
  if(normals && normals->GetNumberOfTuples() <= 0)
    normals = 0;

  tcoords = cd->GetTCoords();
  if(tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = 0;

  tensors = cd->GetTensors();
  if(tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = 0;

  globalIds = cd->GetGlobalIds();
  if(globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = 0;

  pedigreeIds = cd->GetPedigreeIds();
  if(pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = 0;

  field = cd;
  if(field && field->GetNumberOfTuples() <= 0)
    field = 0;

  if(!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    vtkDebugMacro(<<"No cell data to write!");
    return 1;
  }

  *fp << "CELL_DATA " << numCells << "\n";
  //
  // Write scalar data
  //
  if( scalars )
  {
    if ( ! this->WriteScalarData(fp, scalars, numCells) )
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if( vectors )
  {
    if ( ! this->WriteVectorData(fp, vectors, numCells) )
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if ( normals )
  {
    if ( ! this->WriteNormalData(fp, normals, numCells) )
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if ( tcoords )
  {
    if ( ! this->WriteTCoordData(fp, tcoords, numCells) )
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if ( tensors )
  {
    if ( ! this->WriteTensorData(fp, tensors, numCells) )
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if ( globalIds )
  {
    if ( ! this->WriteGlobalIdData(fp, globalIds, numCells) )
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if ( pedigreeIds )
  {
    if ( ! this->WritePedigreeIdData(fp, pedigreeIds, numCells) )
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if ( field )
  {
    if ( ! this->WriteFieldData(fp, field) )
    {
      return 0;
    }
  }

  return 1;
}

// Write the point data (e.g., scalars, vectors, ...) of a vtk dataset.
// Returns 0 if error.
int vtkDataWriter::WritePointData(ostream *fp, vtkDataSet *ds)
{
  vtkIdType numPts;
  vtkDataArray *scalars;
  vtkDataArray *vectors;
  vtkDataArray *normals;
  vtkDataArray *tcoords;
  vtkDataArray *tensors;
  vtkDataArray *globalIds;
  vtkAbstractArray *pedigreeIds;
  vtkDataArray *edgeFlags;
  vtkFieldData *field;
  vtkPointData *pd=ds->GetPointData();

  vtkDebugMacro(<<"Writing point data...");

  numPts = ds->GetNumberOfPoints();
  if(numPts <= 0)
  {
    vtkDebugMacro(<<"No point data to write!");
    return 1;
  }

  scalars = pd->GetScalars();
  if(scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = 0;

  vectors = pd->GetVectors();
  if(vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = 0;

  normals = pd->GetNormals();
  if(normals && normals->GetNumberOfTuples() <= 0)
    normals = 0;

  tcoords = pd->GetTCoords();
  if(tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = 0;

  tensors = pd->GetTensors();
  if(tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = 0;

  globalIds = pd->GetGlobalIds();
  if(globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = 0;

  pedigreeIds = pd->GetPedigreeIds();
  if(pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = 0;

  edgeFlags = pd->GetAttribute(vtkDataSetAttributes::EDGEFLAG);
  if(edgeFlags && edgeFlags->GetNumberOfTuples() <= 0)
    edgeFlags = 0;

  field = pd;
  if(field && field->GetNumberOfTuples() <= 0)
    field = 0;

  if(!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || edgeFlags || field))
  {
    vtkDebugMacro(<<"No point data to write!");
    return 1;
  }

  *fp << "POINT_DATA " << numPts << "\n";
  //
  // Write scalar data
  //
  if ( scalars )
  {
    if ( ! this->WriteScalarData(fp, scalars, numPts) )
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if ( vectors )
  {
    if ( ! this->WriteVectorData(fp, vectors, numPts) )
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if ( normals )
  {
    if ( ! this->WriteNormalData(fp, normals, numPts) )
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if ( tcoords )
  {
    if ( ! this->WriteTCoordData(fp, tcoords, numPts) )
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if ( tensors )
  {
    if ( ! this->WriteTensorData(fp, tensors, numPts) )
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if ( globalIds )
  {
    if ( ! this->WriteGlobalIdData(fp, globalIds, numPts) )
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if ( pedigreeIds )
  {
    if ( ! this->WritePedigreeIdData(fp, pedigreeIds, numPts) )
    {
      return 0;
    }
  }
  //
  // Write edge flags
  //
  if ( edgeFlags )
  {
    if ( ! this->WriteEdgeFlagsData(fp, edgeFlags, numPts) )
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if ( field )
  {
    if ( ! this->WriteFieldData(fp, field) )
    {
      return 0;
    }
  }

  return 1;
}

// Write the vertex data (e.g., scalars, vectors, ...) of a vtk graph.
// Returns 0 if error.
int vtkDataWriter::WriteVertexData(ostream *fp, vtkGraph *ds)
{
  vtkIdType numVertices;
  vtkDataArray *scalars;
  vtkDataArray *vectors;
  vtkDataArray *normals;
  vtkDataArray *tcoords;
  vtkDataArray *tensors;
  vtkDataArray *globalIds;
  vtkAbstractArray *pedigreeIds;
  vtkFieldData *field;
  vtkDataSetAttributes *cd=ds->GetVertexData();

  vtkDebugMacro(<<"Writing vertex data...");

  numVertices = ds->GetNumberOfVertices();
  if(numVertices <= 0)
  {
    vtkDebugMacro(<<"No vertex data to write!");
    return 1;
  }

  scalars = cd->GetScalars();
  if(scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = 0;

  vectors = cd->GetVectors();
  if(vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = 0;

  normals = cd->GetNormals();
  if(normals && normals->GetNumberOfTuples() <= 0)
    normals = 0;

  tcoords = cd->GetTCoords();
  if(tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = 0;

  tensors = cd->GetTensors();
  if(tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = 0;

  globalIds = cd->GetGlobalIds();
  if(globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = 0;

  pedigreeIds = cd->GetPedigreeIds();
  if(pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = 0;

  field = cd;
  if(field && field->GetNumberOfTuples() <= 0)
    field = 0;

  if(!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    vtkDebugMacro(<<"No cell data to write!");
    return 1;
  }

  *fp << "VERTEX_DATA " << numVertices << "\n";
  //
  // Write scalar data
  //
  if( scalars )
  {
    if ( ! this->WriteScalarData(fp, scalars, numVertices) )
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if( vectors )
  {
    if ( ! this->WriteVectorData(fp, vectors, numVertices) )
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if ( normals )
  {
    if ( ! this->WriteNormalData(fp, normals, numVertices) )
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if ( tcoords )
  {
    if ( ! this->WriteTCoordData(fp, tcoords, numVertices) )
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if ( tensors )
  {
    if ( ! this->WriteTensorData(fp, tensors, numVertices) )
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if ( globalIds )
  {
    if ( ! this->WriteGlobalIdData(fp, globalIds, numVertices) )
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if ( pedigreeIds )
  {
    if ( ! this->WritePedigreeIdData(fp, pedigreeIds, numVertices) )
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if ( field )
  {
    if ( ! this->WriteFieldData(fp, field) )
    {
      return 0;
    }
  }

  return 1;
}

// Write the edge data (e.g., scalars, vectors, ...) of a vtk graph.
// Returns 0 if error.
int vtkDataWriter::WriteEdgeData(ostream *fp, vtkGraph *g)
{
  vtkIdType numEdges;
  vtkDataArray *scalars;
  vtkDataArray *vectors;
  vtkDataArray *normals;
  vtkDataArray *tcoords;
  vtkDataArray *tensors;
  vtkDataArray *globalIds;
  vtkAbstractArray *pedigreeIds;
  vtkFieldData *field;
  vtkDataSetAttributes *cd=g->GetEdgeData();

  vtkDebugMacro(<<"Writing edge data...");

  numEdges = g->GetNumberOfEdges();
  if(numEdges <= 0)
  {
    vtkDebugMacro(<<"No edge data to write!");
    return 1;
  }

  scalars = cd->GetScalars();
  if(scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = 0;

  vectors = cd->GetVectors();
  if(vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = 0;

  normals = cd->GetNormals();
  if(normals && normals->GetNumberOfTuples() <= 0)
    normals = 0;

  tcoords = cd->GetTCoords();
  if(tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = 0;

  tensors = cd->GetTensors();
  if(tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = 0;

  globalIds = cd->GetGlobalIds();
  if(globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = 0;

  pedigreeIds = cd->GetPedigreeIds();
  if(pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = 0;

  field = cd;
  if(field && field->GetNumberOfTuples() <= 0)
    field = 0;

  if(!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    vtkDebugMacro(<<"No edge data to write!");
    return 1;
  }

  *fp << "EDGE_DATA " << numEdges << "\n";
  //
  // Write scalar data
  //
  if( scalars )
  {
    if ( ! this->WriteScalarData(fp, scalars, numEdges) )
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if( vectors )
  {
    if ( ! this->WriteVectorData(fp, vectors, numEdges) )
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if ( normals )
  {
    if ( ! this->WriteNormalData(fp, normals, numEdges) )
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if ( tcoords )
  {
    if ( ! this->WriteTCoordData(fp, tcoords, numEdges) )
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if ( tensors )
  {
    if ( ! this->WriteTensorData(fp, tensors, numEdges) )
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if ( globalIds )
  {
    if ( ! this->WriteGlobalIdData(fp, globalIds, numEdges) )
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if ( pedigreeIds )
  {
    if ( ! this->WritePedigreeIdData(fp, pedigreeIds, numEdges) )
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if ( field )
  {
    if ( ! this->WriteFieldData(fp, field) )
    {
      return 0;
    }
  }

  return 1;
}

// Write the row data (e.g., scalars, vectors, ...) of a vtk table.
// Returns 0 if error.
int vtkDataWriter::WriteRowData(ostream *fp, vtkTable *t)
{
  vtkIdType numRows;
  vtkDataArray *scalars;
  vtkDataArray *vectors;
  vtkDataArray *normals;
  vtkDataArray *tcoords;
  vtkDataArray *tensors;
  vtkDataArray *globalIds;
  vtkAbstractArray *pedigreeIds;
  vtkFieldData *field;
  vtkDataSetAttributes *cd=t->GetRowData();

  numRows = t->GetNumberOfRows();

  vtkDebugMacro(<<"Writing row data...");

  scalars = cd->GetScalars();
  if(scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = 0;

  vectors = cd->GetVectors();
  if(vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = 0;

  normals = cd->GetNormals();
  if(normals && normals->GetNumberOfTuples() <= 0)
    normals = 0;

  tcoords = cd->GetTCoords();
  if(tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = 0;

  tensors = cd->GetTensors();
  if(tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = 0;

  globalIds = cd->GetGlobalIds();
  if(globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = 0;

  pedigreeIds = cd->GetPedigreeIds();
  if(pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = 0;

  field = cd;
  if(field && field->GetNumberOfTuples() <= 0)
    field = 0;

  if(!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    vtkDebugMacro(<<"No row data to write!");
    return 1;
  }

  *fp << "ROW_DATA " << numRows << "\n";
  //
  // Write scalar data
  //
  if( scalars )
  {
    if ( ! this->WriteScalarData(fp, scalars, numRows) )
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if( vectors )
  {
    if ( ! this->WriteVectorData(fp, vectors, numRows) )
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if ( normals )
  {
    if ( ! this->WriteNormalData(fp, normals, numRows) )
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if ( tcoords )
  {
    if ( ! this->WriteTCoordData(fp, tcoords, numRows) )
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if ( tensors )
  {
    if ( ! this->WriteTensorData(fp, tensors, numRows) )
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if ( globalIds )
  {
    if ( ! this->WriteGlobalIdData(fp, globalIds, numRows) )
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if ( pedigreeIds )
  {
    if ( ! this->WritePedigreeIdData(fp, pedigreeIds, numRows) )
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if ( field )
  {
    if ( ! this->WriteFieldData(fp, field) )
    {
      return 0;
    }
  }

  return 1;
}

namespace
{
// Template to handle writing data in ascii or binary
// We could change the format into C++ io standard ...
template <class T>
void vtkWriteDataArray(ostream *fp, T *data, int fileType,
                       const char *format, vtkIdType num, vtkIdType numComp)
{
  vtkIdType i, j, idx, sizeT;
  char str[1024];

  sizeT = sizeof(T);

  if ( fileType == VTK_ASCII )
  {
    for (j=0; j<num; j++)
    {
      for (i=0; i<numComp; i++)
      {
        idx = i + j*numComp;
        snprintf (str, sizeof(str), format, *data++); *fp << str;
        if ( !((idx+1)%9) )
        {
          *fp << "\n";
        }
      }
    }
  }
  else
  {
    if (num*numComp > 0)
    {
      // need to byteswap ??
      switch (sizeT)
      {
        case 2:
          // no typecast needed here; method call takes void* data
          vtkByteSwap::SwapWrite2BERange(data, num*numComp, fp);
          break;
        case 4:
          // no typecast needed here; method call takes void* data
          vtkByteSwap::SwapWrite4BERange(data, num*numComp, fp);
          break;
        case 8:
          // no typecast needed here; method call takes void* data
          vtkByteSwap::SwapWrite8BERange(data, num*numComp, fp);
          break;
        default:
          fp->write(reinterpret_cast<char*>(data), sizeof(T) * (num*numComp));
          break;
      }
    }
  }
  *fp << "\n";
}

// Returns a pointer to the data ordered in original VTK style ordering
// of the data. If this is an SOA array it has to allocate the memory
// for that in which case the calling function must delete it.
template <class T>
T* GetArrayRawPointer(vtkAbstractArray* array, T* ptr, int isAOSArray)
{
  if (isAOSArray)
  {
    return ptr;
  }
  T* data = new T[array->GetNumberOfComponents()*array->GetNumberOfTuples()];
  vtkSOADataArrayTemplate<T>* typedArray =
    vtkSOADataArrayTemplate<T>::SafeDownCast(array);
  typedArray->ExportToVoidPointer(data);
  return data;
}

} // end anonymous namespace

// Write out data to file specified.
int vtkDataWriter::WriteArray(ostream *fp, int dataType, vtkAbstractArray *data,
                              const char *format, vtkIdType num, vtkIdType numComp)
{
  vtkIdType i, j, idx;
  char str[1024];

  bool isAOSArray = data->HasStandardMemoryLayout();

  char* outputFormat = new char[10];
  switch (dataType)
  {
    case VTK_BIT:
    { // assume that bit array is always in original AOS ordering
      snprintf (str, sizeof(str), format, "bit"); *fp << str;
      if ( this->FileType == VTK_ASCII )
      {
        int s;
        for (j=0; j<num; j++)
        {
          for (i=0; i<numComp; i++)
          {
            idx = i + j*numComp;
            s = static_cast<vtkBitArray *>(data)->GetValue(idx);
            *fp << (s!=0.0?1:0);
            if ( !((idx+1)%8) )
            {
              *fp << "\n";
            }
            else
            {
              *fp << " ";
            }
          }
        }
      }
      else
      {
        unsigned char *cptr=
          static_cast<vtkUnsignedCharArray *>(data)->GetPointer(0);
        fp->write(reinterpret_cast<char *>(cptr),
                  (sizeof(unsigned char))*((num-1)/8+1));

      }
      *fp << "\n";
    }
    break;

    case VTK_CHAR:
    {
      snprintf (str, sizeof(str), format, "char"); *fp << str;
      char *s=GetArrayRawPointer(
        data, static_cast<vtkCharArray *>(data)->GetPointer(0), isAOSArray);
#if VTK_TYPE_CHAR_IS_SIGNED
      vtkWriteDataArray(fp, s, this->FileType, "%hhd ", num, numComp);
#else
      vtkWriteDataArray(fp, s, this->FileType, "%hhu ", num, numComp);
#endif
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_SIGNED_CHAR:
    {
      snprintf (str, sizeof(str), format, "signed_char"); *fp << str;
      signed char *s=GetArrayRawPointer(
        data, static_cast<vtkSignedCharArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%hhd ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_UNSIGNED_CHAR:
    {
      snprintf (str, sizeof(str), format, "unsigned_char"); *fp << str;
      unsigned char *s=GetArrayRawPointer(
        data, static_cast<vtkUnsignedCharArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%hhu ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_SHORT:
    {
      snprintf (str, sizeof(str), format, "short"); *fp << str;
      short *s=GetArrayRawPointer(
        data, static_cast<vtkShortArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%hd ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_UNSIGNED_SHORT:
    {
      snprintf (str, sizeof(str), format, "unsigned_short"); *fp << str;
      unsigned short *s=GetArrayRawPointer(
        data, static_cast<vtkUnsignedShortArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%hu ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_INT:
    {
      snprintf (str, sizeof(str), format, "int"); *fp << str;
      int *s=GetArrayRawPointer(
        data, static_cast<vtkIntArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%d ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_UNSIGNED_INT:
    {
      snprintf (str, sizeof(str), format, "unsigned_int"); *fp << str;
      unsigned int *s=GetArrayRawPointer(
        data, static_cast<vtkUnsignedIntArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%u ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_LONG:
    {
      snprintf (str, sizeof(str), format, "long"); *fp << str;
      long *s=GetArrayRawPointer(
        data, static_cast<vtkLongArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%ld ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_UNSIGNED_LONG:
    {
      snprintf (str, sizeof(str), format, "unsigned_long"); *fp << str;
      unsigned long *s=GetArrayRawPointer(
        data, static_cast<vtkUnsignedLongArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%lu ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_LONG_LONG:
    {
      snprintf (str, sizeof(str), format, "vtktypeint64"); *fp << str;
      long long *s=GetArrayRawPointer(
        data, static_cast<vtkTypeInt64Array *>(data)->GetPointer(0), isAOSArray);
      strcpy(outputFormat, vtkTypeTraits<long long>::ParseFormat());
      strcat(outputFormat, " ");
      vtkWriteDataArray(fp, s, this->FileType, outputFormat, num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_UNSIGNED_LONG_LONG:
    {
      snprintf (str, sizeof(str), format, "vtktypeuint64"); *fp << str;
      unsigned long long *s=GetArrayRawPointer(
        data, static_cast<vtkTypeUInt64Array *>(data)->GetPointer(0), isAOSArray);
      strcpy(outputFormat, vtkTypeTraits<unsigned long long>::ParseFormat());
      strcat(outputFormat, " ");
      vtkWriteDataArray(fp, s, this->FileType, outputFormat, num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_FLOAT:
    {
      snprintf (str, sizeof(str), format, "float"); *fp << str;
      float *s=GetArrayRawPointer(
        data, static_cast<vtkFloatArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%g ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_DOUBLE:
    {
      snprintf (str, sizeof(str), format, "double"); *fp << str;
      double *s=GetArrayRawPointer(
        data, static_cast<vtkDoubleArray *>(data)->GetPointer(0), isAOSArray);
      vtkWriteDataArray(fp, s, this->FileType, "%.11lg ", num, numComp);
      if (!isAOSArray)
      {
        delete [] s;
      }
    }
    break;

    case VTK_ID_TYPE:
    {
      // currently writing vtkIdType as int.
      vtkIdType size = data->GetNumberOfTuples();
      std::vector<int> intArray(size*numComp);
      snprintf (str, sizeof(str), format, "vtkIdType"); *fp << str;
      if (isAOSArray)
      {
        vtkIdType *s=static_cast<vtkIdTypeArray *>(data)->GetPointer(0);
        for (vtkIdType jj = 0; jj < size*numComp; jj++)
        {
          intArray[jj] = s[jj];
        }
      }
      else
      {
        vtkSOADataArrayTemplate<vtkIdType>* data2=
          static_cast<vtkSOADataArrayTemplate<vtkIdType> *>(data);
        std::vector<vtkIdType> vals(numComp);
        for (vtkIdType jj = 0; jj < size; jj++)
        {
          data2->GetTypedTuple(jj, &vals[0]);
          for (i = 0; i < numComp; i++)
          {
            intArray[jj*numComp+i] = vals[i];
          }
        }
      }
      vtkWriteDataArray(fp, &intArray[0], this->FileType, "%d ", num, numComp);
    }
    break;

    case VTK_STRING:
    {
      snprintf (str, sizeof(str), format, "string"); *fp << str;
      if ( this->FileType == VTK_ASCII )
      {
        vtkStdString s;
        for (j=0; j<num; j++)
        {
          for (i=0; i<numComp; i++)
          {
            idx = i + j*numComp;
            s = static_cast<vtkStringArray *>(data)->GetValue(idx);
            this->EncodeWriteString(fp, s.c_str(), false);
            *fp << "\n";
          }
        }
      }
      else
      {
        vtkStdString s;
        for (j=0; j<num; j++)
        {
          for (i=0; i<numComp; i++)
          {
            idx = i + j*numComp;
            s = static_cast<vtkStringArray *>(data)->GetValue(idx);
            vtkTypeUInt64 length = s.length();
            if (length < (static_cast<vtkTypeUInt64>(1) << 6))
            {
              vtkTypeUInt8 len = (static_cast<vtkTypeUInt8>(3) << 6)
                | static_cast<vtkTypeUInt8>(length);
              fp->write(reinterpret_cast<char*>(&len), 1);
            }
            else if (length < (static_cast<vtkTypeUInt64>(1) << 14))
            {
              vtkTypeUInt16 len = (static_cast<vtkTypeUInt16>(2) << 14)
                | static_cast<vtkTypeUInt16>(length);
              vtkByteSwap::SwapWrite2BERange(&len, 1, fp);
            }
            else if (length < (static_cast<vtkTypeUInt64>(1) << 30))
            {
              vtkTypeUInt32 len = (static_cast<vtkTypeUInt32>(1) << 30)
                | static_cast<vtkTypeUInt32>(length);
              vtkByteSwap::SwapWrite4BERange(&len, 1, fp);
            }
            else
            {
              vtkByteSwap::SwapWrite8BERange(&length, 1, fp);
            }
            fp->write(s.c_str(), length);
          }
        }
      }
      *fp << "\n";
    }
    break;

    case VTK_UNICODE_STRING:
    {
      snprintf (str, sizeof(str), format, "utf8_string"); *fp << str;
      if ( this->FileType == VTK_ASCII )
      {
        vtkStdString s;
        for (j=0; j<num; j++)
        {
          for (i=0; i<numComp; i++)
          {
            idx = i + j*numComp;
            s = static_cast<vtkUnicodeStringArray *>(data)->GetValue(idx).utf8_str();
            this->EncodeWriteString(fp, s.c_str(), false);
            *fp << "\n";
          }
        }
      }
      else
      {
        vtkStdString s;
        for (j=0; j<num; j++)
        {
          for (i=0; i<numComp; i++)
          {
            idx = i + j*numComp;
            s = static_cast<vtkUnicodeStringArray *>(data)->GetValue(idx).utf8_str();
            vtkTypeUInt64 length = s.length();
            if (length < (static_cast<vtkTypeUInt64>(1) << 6))
            {
              vtkTypeUInt8 len = (static_cast<vtkTypeUInt8>(3) << 6)
                | static_cast<vtkTypeUInt8>(length);
              fp->write(reinterpret_cast<char*>(&len), 1);
            }
            else if (length < (static_cast<vtkTypeUInt64>(1) << 14))
            {
              vtkTypeUInt16 len = (static_cast<vtkTypeUInt16>(2) << 14)
                | static_cast<vtkTypeUInt16>(length);
              vtkByteSwap::SwapWrite2BERange(&len, 1, fp);
            }
            else if (length < (static_cast<vtkTypeUInt64>(1) << 30))
            {
              vtkTypeUInt32 len = (static_cast<vtkTypeUInt32>(1) << 30)
                | static_cast<vtkTypeUInt32>(length);
              vtkByteSwap::SwapWrite4BERange(&len, 1, fp);
            }
            else
            {
              vtkByteSwap::SwapWrite8BERange(&length, 1, fp);
            }
            fp->write(s.c_str(), length);
          }
        }
      }
      *fp << "\n";
    }
    break;

    case VTK_VARIANT:
    {
      snprintf (str, sizeof(str), format, "variant"); *fp << str;
      vtkVariant *v=static_cast<vtkVariantArray *>(data)->GetPointer(0);
      for (j = 0; j < num*numComp; j++)
      {
        *fp << v[j].GetType() << " ";
        this->EncodeWriteString(fp, v[j].ToString().c_str(), false);
        *fp << endl;
      }
    }
    break;

    default:
    {
      vtkErrorMacro(<<"Type currently not supported");
      *fp << "NULL_ARRAY" << endl;
      delete[] outputFormat;
      return 0;
    }
  }
  delete[] outputFormat;

  // Write out metadata if it exists:
  vtkInformation *info = data->GetInformation();
  bool hasComponentNames = data->HasAComponentName();
  bool hasInformation = info && info->GetNumberOfKeys() > 0;
  bool hasMetaData = hasComponentNames || hasInformation;
  if (this->WriteArrayMetaData && hasMetaData)
  {
    *fp << "METADATA" << endl;

    if (hasComponentNames)
    {
      *fp << "COMPONENT_NAMES" << endl;
      for (i = 0; i < numComp; ++i)
      {
        const char *compName = data->GetComponentName(i);
        this->EncodeWriteString(fp, compName, false);
        *fp << endl;
      }
    }

    if (hasInformation)
    {
      this->WriteInformation(fp, info);
    }

    *fp << endl;
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

int vtkDataWriter::WritePoints(ostream *fp, vtkPoints *points)
{
  vtkIdType numPts;

  if (points == NULL)
  {
    *fp << "POINTS 0 float\n";
    return 1;
  }

  numPts=points->GetNumberOfPoints();
  *fp << "POINTS " << numPts << " ";
  return this->WriteArray(fp, points->GetDataType(), points->GetData(), "%s\n", numPts, 3);
}

// Write out coordinates for rectilinear grids.
int vtkDataWriter::WriteCoordinates(ostream *fp, vtkDataArray *coords,
                                    int axes)
{
  int ncoords=(coords?coords->GetNumberOfTuples():0);

  if ( axes == 0 )
  {
    *fp << "X_COORDINATES " << ncoords << " ";
  }
  else if ( axes == 1)
  {
    *fp << "Y_COORDINATES " << ncoords << " ";
  }
  else
  {
    *fp << "Z_COORDINATES " << ncoords << " ";
  }

  if (coords)
  {
    return
      this->WriteArray(fp, coords->GetDataType(), coords, "%s\n", ncoords, 1);
  }
  *fp << "float\n";
  return 1;
}

// Write out scalar data.
int vtkDataWriter::WriteScalarData(ostream *fp, vtkDataArray *scalars, vtkIdType num)
{
  vtkIdType i, j, size=0;
  const char *name;
  vtkLookupTable *lut;
  int dataType = scalars->GetDataType();
  int numComp = scalars->GetNumberOfComponents();

  if ( (lut=scalars->GetLookupTable()) == NULL ||
       (size = lut->GetNumberOfColors()) <= 0 )
  {
    name = "default";
  }
  else
  {
    name = this->LookupTableName;
  }

  char* scalarsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->ScalarsName)
  {
    if (scalars->GetName() && strlen(scalars->GetName()))
    {
      scalarsName = new char[ strlen(scalars->GetName()) * 4 + 1];
      this->EncodeString(scalarsName, scalars->GetName(), true);
    }
    else
    {
      scalarsName = new char[ strlen("scalars") + 1];
      strcpy(scalarsName, "scalars");
    }
  }
  else
  {
    scalarsName = new char[ strlen(this->ScalarsName) * 4 + 1];
    this->EncodeString(scalarsName, this->ScalarsName, true);
  }

  if ( dataType != VTK_UNSIGNED_CHAR )
  {
    char format[1024];
    *fp << "SCALARS ";

    if (numComp == 1)
    {
      snprintf(format, sizeof(format), "%s %%s\nLOOKUP_TABLE %s\n", scalarsName, name);
    }
    else
    {
      snprintf(format, sizeof(format), "%s %%s %d\nLOOKUP_TABLE %s\n",
              scalarsName, numComp, name);
    }
    delete[] scalarsName;
    if (this->WriteArray(fp, scalars->GetDataType(), scalars, format,
                         num, numComp) == 0)
    {
      return 0;
    }
  }

  else //color scalars
  {
    int nvs = scalars->GetNumberOfComponents();
    unsigned char *data=
      static_cast<vtkUnsignedCharArray *>(scalars)->GetPointer(0);
    *fp << "COLOR_SCALARS " << scalarsName << " " << nvs << "\n";

    if ( this->FileType == VTK_ASCII )
    {
      for (i=0; i<num; i++)
      {
        for (j=0; j<nvs; j++)
        {
          *fp << (static_cast<float>(data[nvs*i+j])/255.0) << " ";
        }
        if ( i != 0 && !(i%2) )
        {
          *fp << "\n";
        }
      }
    }
    else // binary type
    {
      fp->write(reinterpret_cast<char *>(data),
                (sizeof(unsigned char))*(nvs*num));
    }

    *fp << "\n";
    delete[] scalarsName;

  }

  //if lookup table, write it out
  if ( lut && size > 0 )
  {
    *fp << "LOOKUP_TABLE " << this->LookupTableName << " " << size << "\n";
    if ( this->FileType == VTK_ASCII )
    {
      double *c;
      for (i=0; i<size; i++)
      {
        c = lut->GetTableValue(i);
        *fp << c[0] << " " << c[1] << " " << c[2] << " " << c[3] << "\n";
      }
    }
    else
    {
      unsigned char *colors=lut->GetPointer(0);
      fp->write(reinterpret_cast<char *>(colors),
                (sizeof(unsigned char)*4*size));
    }
    *fp << "\n";
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

int vtkDataWriter::WriteVectorData(ostream *fp, vtkDataArray *vectors, vtkIdType num)
{
  *fp << "VECTORS ";

  char* vectorsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->VectorsName)
  {
    if (vectors->GetName() && strlen(vectors->GetName()))
    {
      vectorsName = new char[ strlen(vectors->GetName()) * 4 + 1];
      this->EncodeString(vectorsName, vectors->GetName(), true);
    }
    else
    {
      vectorsName = new char[ strlen("vectors") + 1];
      strcpy(vectorsName, "vectors");
    }
  }
  else
  {
    vectorsName = new char[ strlen(this->VectorsName) * 4 + 1];
    this->EncodeString(vectorsName, this->VectorsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", vectorsName, "%s");
  delete[] vectorsName;

  return this->WriteArray(fp, vectors->GetDataType(), vectors, format, num, 3);
}

int vtkDataWriter::WriteNormalData(ostream *fp, vtkDataArray *normals, vtkIdType num)
{
  char* normalsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->NormalsName)
  {
    if (normals->GetName() && strlen(normals->GetName()) )
    {
      normalsName = new char[ strlen(normals->GetName()) * 4 + 1];
      this->EncodeString(normalsName, normals->GetName(), true);
    }
    else
    {
      normalsName = new char[ strlen("normals") + 1];
      strcpy(normalsName, "normals");
    }
  }
  else
  {
    normalsName = new char[ strlen(this->NormalsName) * 4 + 1];
    this->EncodeString(normalsName, this->NormalsName, true);
  }

  *fp << "NORMALS ";
  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", normalsName, "%s");
  delete[] normalsName;

  return this->WriteArray(fp, normals->GetDataType(), normals, format, num, 3);
}

int vtkDataWriter::WriteTCoordData(ostream *fp, vtkDataArray *tcoords, vtkIdType num)
{
  int dim=tcoords->GetNumberOfComponents();

  char* tcoordsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->TCoordsName)
  {
    if (tcoords->GetName() && strlen(tcoords->GetName()))
    {
      tcoordsName = new char[ strlen(tcoords->GetName()) * 4 + 1];
      this->EncodeString(tcoordsName, tcoords->GetName(), true);
    }
    else
    {
      tcoordsName = new char[ strlen("tcoords") + 1];
      strcpy(tcoordsName, "tcoords");
    }
  }
  else
  {
    tcoordsName = new char[ strlen(this->TCoordsName) * 4 + 1];
    this->EncodeString(tcoordsName, this->TCoordsName, true);
  }

  *fp << "TEXTURE_COORDINATES ";
  char format[1024];
  snprintf(format, sizeof(format), "%s %d %s\n", tcoordsName, dim, "%s");
  delete[] tcoordsName;

  return this->WriteArray(fp, tcoords->GetDataType(), tcoords, format, num,
                          dim);
}

int vtkDataWriter::WriteTensorData(ostream *fp, vtkDataArray *tensors, vtkIdType num)
{
  char* tensorsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->TensorsName)
  {
    if (tensors->GetName() && strlen(tensors->GetName()))
    {
      tensorsName = new char[ strlen(tensors->GetName()) * 4 + 1];
      this->EncodeString(tensorsName, tensors->GetName(), true);
    }
    else
    {
      tensorsName = new char[ strlen("tensors") + 1];
      strcpy(tensorsName, "tensors");
    }
  }
  else
  {
    tensorsName = new char[ strlen(this->TensorsName) * 4 + 1];
    this->EncodeString(tensorsName, this->TensorsName, true);
  }

  *fp << "TENSORS";
  int numComp = 9;
  if (tensors->GetNumberOfComponents() == 6)
    {
    *fp << "6";
    numComp = 6;
    }
  *fp << " ";
  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", tensorsName, "%s");
  delete[] tensorsName;

  return this->WriteArray(fp, tensors->GetDataType(), tensors, format, num, numComp);
}

int vtkDataWriter::WriteGlobalIdData(ostream *fp, vtkDataArray *globalIds, vtkIdType num)
{
  *fp << "GLOBAL_IDS ";

  char* globalIdsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->GlobalIdsName)
  {
    if (globalIds->GetName() && strlen(globalIds->GetName()))
    {
      globalIdsName = new char[ strlen(globalIds->GetName()) * 4 + 1];
      this->EncodeString(globalIdsName, globalIds->GetName(), true);
    }
    else
    {
      globalIdsName = new char[ strlen("global_ids") + 1];
      strcpy(globalIdsName, "global_ids");
    }
  }
  else
  {
    globalIdsName = new char[ strlen(this->GlobalIdsName) * 4 + 1];
    this->EncodeString(globalIdsName, this->GlobalIdsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", globalIdsName, "%s");
  delete[] globalIdsName;

  return this->WriteArray(fp, globalIds->GetDataType(), globalIds, format, num, 1);
}

int vtkDataWriter::WritePedigreeIdData(ostream *fp, vtkAbstractArray *pedigreeIds, vtkIdType num)
{
  *fp << "PEDIGREE_IDS ";

  char* pedigreeIdsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->PedigreeIdsName)
  {
    if (pedigreeIds->GetName() && strlen(pedigreeIds->GetName()))
    {
      pedigreeIdsName = new char[ strlen(pedigreeIds->GetName()) * 4 + 1];
      this->EncodeString(pedigreeIdsName, pedigreeIds->GetName(), true);
    }
    else
    {
      pedigreeIdsName = new char[ strlen("pedigree_ids") + 1];
      strcpy(pedigreeIdsName, "pedigree_ids");
    }
  }
  else
  {
    pedigreeIdsName = new char[ strlen(this->PedigreeIdsName) * 4 + 1];
    this->EncodeString(pedigreeIdsName, this->PedigreeIdsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", pedigreeIdsName, "%s");
  delete[] pedigreeIdsName;

  return this->WriteArray(fp, pedigreeIds->GetDataType(), pedigreeIds, format, num, 1);
}

int vtkDataWriter::WriteEdgeFlagsData(ostream *fp, vtkDataArray *edgeFlags, vtkIdType num)
{
  *fp << "EDGE_FLAGS ";

  char* edgeFlagsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->EdgeFlagsName)
  {
    if (edgeFlags->GetName() && strlen(edgeFlags->GetName()))
    {
      edgeFlagsName = new char[ strlen(edgeFlags->GetName()) * 4 + 1];
      this->EncodeString(edgeFlagsName, edgeFlags->GetName(), true);
    }
    else
    {
      edgeFlagsName = new char[ strlen("edge_flags") + 1];
      strcpy(edgeFlagsName, "edge_flags");
    }
  }
  else
  {
    edgeFlagsName = new char[ strlen(this->EdgeFlagsName) * 4 + 1];
    this->EncodeString(edgeFlagsName, this->EdgeFlagsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", edgeFlagsName, "%s");
  delete[] edgeFlagsName;

  return this->WriteArray(fp, edgeFlags->GetDataType(), edgeFlags, format, num, 1);
}

bool vtkDataWriter::CanWriteInformationKey(vtkInformation *info,
                                           vtkInformationKey *key)
{
  vtkInformationDoubleKey *dKey = NULL;
  vtkInformationDoubleVectorKey *dvKey = NULL;
  if ((dKey = vtkInformationDoubleKey::SafeDownCast(key)))
  {
    // Skip keys with NaNs/infs
    double value = info->Get(dKey);
    if (!vtkMath::IsFinite(value))
    {
      vtkWarningMacro("Skipping key '" << key->GetLocation() << "::"
                      << key->GetName() << "': bad value: " << value);
      return false;
    }
    return true;
  }
  else if ((dvKey = vtkInformationDoubleVectorKey::SafeDownCast(key)))
  {
    // Skip keys with NaNs/infs
    int length = dvKey->Length(info);
    bool valid = true;
    for (int i = 0; i < length; ++i)
    {
      double value = info->Get(dvKey, i);
      if (!vtkMath::IsFinite(value))
      {
        vtkWarningMacro("Skipping key '" << key->GetLocation() << "::"
                        << key->GetName() << "': bad value: " << value);
        valid = false;
        break;
      }
    }
    return valid;
  }
  else if (vtkInformationIdTypeKey::SafeDownCast(key) ||
           vtkInformationIntegerKey::SafeDownCast(key) ||
           vtkInformationIntegerVectorKey::SafeDownCast(key) ||
           vtkInformationStringKey::SafeDownCast(key) ||
           vtkInformationStringVectorKey::SafeDownCast(key) ||
           vtkInformationUnsignedLongKey::SafeDownCast(key))
  {
    return true;
  }
  vtkDebugMacro("Could not serialize information with key "
                << key->GetLocation() << "::" << key->GetName() << ": "
                "Unsupported data type '" << key->GetClassName() << "'.");
  return false;
}

namespace {
void writeInfoHeader(std::ostream *fp, vtkInformationKey *key)
{
  *fp << "NAME " << key->GetName() << " LOCATION " << key->GetLocation() << "\n"
      << "DATA ";
}
} // end anon namespace

int vtkDataWriter::WriteInformation(std::ostream *fp, vtkInformation *info)
{
  // This will contain the serializable keys:
  vtkNew<vtkInformation> keys;
  vtkInformationKey *key = NULL;
  vtkNew<vtkInformationIterator> iter;
  iter->SetInformationWeak(info);
  for (iter->InitTraversal(); (key = iter->GetCurrentKey());
       iter->GoToNextItem())
  {
    if (this->CanWriteInformationKey(info, key))
    {
      keys->CopyEntry(info, key);
    }
  }

  *fp << "INFORMATION " << keys->GetNumberOfKeys() << "\n";

  iter->SetInformationWeak(keys.Get());
  char buffer[1024];
  for (iter->InitTraversal(); (key = iter->GetCurrentKey());
       iter->GoToNextItem())
  {
    vtkInformationDoubleKey *dKey = NULL;
    vtkInformationDoubleVectorKey *dvKey = NULL;
    vtkInformationIdTypeKey *idKey = NULL;
    vtkInformationIntegerKey *iKey = NULL;
    vtkInformationIntegerVectorKey *ivKey = NULL;
    vtkInformationStringKey *sKey = NULL;
    vtkInformationStringVectorKey *svKey = NULL;
    vtkInformationUnsignedLongKey *ulKey = NULL;
    if ((dKey = vtkInformationDoubleKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      // "%lg" is used to write double array data in ascii, using the same
      // precision here.
      snprintf(buffer, sizeof(buffer), "%lg", dKey->Get(info));
      *fp << buffer << "\n";
    }
    else if ((dvKey = vtkInformationDoubleVectorKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);

      // Size first:
      int length = dvKey->Length(info);
      snprintf(buffer, sizeof(buffer), "%d", length);
      *fp << buffer << " ";

      double *data = dvKey->Get(info);
      for (int i = 0; i < length; ++i)
      {
        // "%lg" is used to write double array data in ascii, using the same
        // precision here.
        snprintf(buffer, sizeof(buffer), "%lg", data[i]);
        *fp << buffer << " ";
      }
      *fp << "\n";
    }
    else if ((idKey = vtkInformationIdTypeKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      snprintf(buffer, sizeof(buffer), vtkTypeTraits<vtkIdType>::ParseFormat(),
               idKey->Get(info));
      *fp << buffer << "\n";
    }
    else if ((iKey = vtkInformationIntegerKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      snprintf(buffer, sizeof(buffer), vtkTypeTraits<int>::ParseFormat(),
               iKey->Get(info));
      *fp << buffer << "\n";
    }
    else if ((ivKey = vtkInformationIntegerVectorKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);

      // Size first:
      int length = ivKey->Length(info);
      snprintf(buffer, sizeof(buffer), "%d", length);
      *fp << buffer << " ";

      int *data = ivKey->Get(info);
      for (int i = 0; i < length; ++i)
      {
        snprintf(buffer, sizeof(buffer), vtkTypeTraits<int>::ParseFormat(), data[i]);
        *fp << buffer << " ";
      }
      *fp << "\n";
    }
    else if ((sKey = vtkInformationStringKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      this->EncodeWriteString(fp, sKey->Get(info), false);
      *fp << "\n";
    }
    else if ((svKey = vtkInformationStringVectorKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);

      // Size first:
      int length = svKey->Length(info);
      snprintf(buffer, sizeof(buffer), "%d", length);
      *fp << buffer << "\n";

      for (int i = 0; i < length; ++i)
      {
        this->EncodeWriteString(fp, svKey->Get(info, i), false);
        *fp << "\n";
      }
    }
    else if ((ulKey = vtkInformationUnsignedLongKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      snprintf(buffer, sizeof(buffer), vtkTypeTraits<unsigned long>::ParseFormat(),
               ulKey->Get(info));
      *fp << buffer << "\n";
    }
    else
    {
      vtkDebugMacro("Could not serialize information with key "
                    << key->GetLocation() << "::" << key->GetName() << ": "
                    "Unsupported data type '" << key->GetClassName() << "'.");
    }
  }
  return 1;
}

static int vtkIsInTheList(int index, int* list, vtkIdType numElem)
{
  for(vtkIdType i=0; i<numElem; i++)
  {
    if (index == list[i])
    {
      return 1;
    }
  }
  return 0;
}

int vtkDataWriter::WriteFieldData(ostream *fp, vtkFieldData *f)
{
  char format[1024];
  int i, numArrays=f->GetNumberOfArrays(), actNumArrays=0;
  vtkIdType numComp, numTuples;
  int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
  vtkAbstractArray *array;

  for(i=0; i<vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    attributeIndices[i] = -1;
  }
  vtkDataSetAttributes* dsa;
  if ((dsa=vtkDataSetAttributes::SafeDownCast(f)))
  {
    dsa->GetAttributeIndices(attributeIndices);
  }

  for (i=0; i < numArrays; i++)
  {
    if (!vtkIsInTheList(i, attributeIndices,
                        vtkDataSetAttributes::NUM_ATTRIBUTES))
    {
      actNumArrays++;
    }
  }
  if ( actNumArrays < 1 )
  {
    return 1;
  }
  *fp << "FIELD " << this->FieldDataName << " " << actNumArrays << "\n";

  for (i=0; i < numArrays; i++)
  {
    if (!vtkIsInTheList(i, attributeIndices,
                        vtkDataSetAttributes::NUM_ATTRIBUTES))
    {
      array = f->GetAbstractArray(i);
      if ( array != NULL )
      {
        numComp = array->GetNumberOfComponents();
        numTuples = array->GetNumberOfTuples();

        // Buffer size is size of array name times four because
        // in theory there could be array name consisting of only
        // weird symbols.
        char* buffer;
        if( !array->GetName() || strlen(array->GetName()) == 0)
        {
          buffer = strcpy(new char[strlen("unknown")+1], "unknown");
        }
        else
        {
          buffer = new char[ strlen(array->GetName()) * 4 + 1];
          this->EncodeString(buffer, array->GetName(), true);
        }
        snprintf(format, sizeof(format), "%s %" VTK_ID_TYPE_PRId " %" VTK_ID_TYPE_PRId " %s\n", buffer, numComp, numTuples,
                "%s");
        this->WriteArray(fp, array->GetDataType(), array, format, numTuples,
                         numComp);
        delete [] buffer;
      }
      else
      {
        *fp << "NULL_ARRAY" << endl;
      }
    }
  }

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  return 1;
}

int vtkDataWriter::WriteCells(ostream *fp, vtkCellArray *cells, const char *label)
{
  if ( ! cells )
  {
    return 1;
  }

  vtkIdType ncells=cells->GetNumberOfCells();
  vtkIdType size=cells->GetNumberOfConnectivityEntries();

  if ( ncells < 1 )
  {
    return 1;
  }

  *fp << label << " " << ncells << " " << size << "\n";

  if ( this->FileType == VTK_ASCII )
  {
    vtkIdType j;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
    {
      // currently writing vtkIdType as int
      *fp << static_cast<int>(npts) << " ";
      for (j=0; j<npts; j++)
      {
        // currently writing vtkIdType as int
        *fp << static_cast<int>(pts[j]) << " ";
      }
      *fp << "\n";
    }
  }
  else
  {
    // swap the bytes if necc
    // currently writing vtkIdType as int
    vtkIdType *tempArray = cells->GetPointer();
    int arraySize = cells->GetNumberOfConnectivityEntries();
    int *intArray = new int[arraySize];
    int i;

    for (i = 0; i < arraySize; i++)
    {
      intArray[i] = tempArray[i];
    }

    vtkByteSwap::SwapWrite4BERange(intArray,size,fp);
    delete [] intArray;
  }

  *fp << "\n";

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

void vtkDataWriter::WriteData()
{
  vtkErrorMacro(<<"WriteData() should be implemented in concrete subclass");
}

// Close a vtk file.
void vtkDataWriter::CloseVTKFile(ostream *fp)
{
  vtkDebugMacro(<<"Closing vtk file\n");

  if ( fp != NULL )
  {
    if (this->WriteToOutputString)
    {
      std::ostringstream *ostr =
        static_cast<std::ostringstream*>(fp);

      delete [] this->OutputString;
      this->OutputStringLength = static_cast<int>(ostr->str().size());
      this->OutputString = new char[this->OutputStringLength+1];
      memcpy(this->OutputString, ostr->str().c_str(),
        this->OutputStringLength+1);
    }
    delete fp;
  }
}

char *vtkDataWriter::RegisterAndGetOutputString()
{
  char *tmp = this->OutputString;

  this->OutputString = NULL;
  this->OutputStringLength = 0;

  return tmp;
}

vtkStdString vtkDataWriter::GetOutputStdString()
{
  return vtkStdString(this->OutputString, this->OutputStringLength);
}

void vtkDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

  if ( this->FileType == VTK_BINARY )
  {
    os << indent << "File Type: BINARY\n";
  }
  else
  {
    os << indent << "File Type: ASCII\n";
  }

  if ( this->Header )
  {
    os << indent << "Header: " << this->Header << "\n";
  }
  else
  {
    os << indent << "Header: (None)\n";
  }

  os << indent << "Output String Length: " << this->OutputStringLength << "\n";
  os << indent << "Output String (addr): " << static_cast<void *>(this->OutputString) << "\n";
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "On\n" : "Off\n");

  if ( this->ScalarsName )
  {
    os << indent << "Scalars Name: " << this->ScalarsName << "\n";
  }
  else
  {
    os << indent << "Scalars Name: (None)\n";
  }

  if ( this->VectorsName )
  {
    os << indent << "Vectors Name: " << this->VectorsName << "\n";
  }
  else
  {
    os << indent << "Vectors Name: (None)\n";
  }

  if ( this->NormalsName )
  {
    os << indent << "Normals Name: " << this->NormalsName << "\n";
  }
  else
  {
    os << indent << "Normals Name: (None)\n";
  }

  if ( this->TensorsName )
  {
    os << indent << "Tensors Name: " << this->TensorsName << "\n";
  }
  else
  {
    os << indent << "Tensors Name: (None)\n";
  }

  if ( this->TCoordsName )
  {
    os << indent << "Texture Coords Name: " << this->TCoordsName << "\n";
  }
  else
  {
    os << indent << "Texture Coordinates Name: (None)\n";
  }

  if ( this->GlobalIdsName )
  {
    os << indent << "Global Ids Name: " << this->GlobalIdsName << "\n";
  }
  else
  {
    os << indent << "Global Ids Name: (None)\n";
  }

  if ( this->PedigreeIdsName )
  {
    os << indent << "Pedigree Ids Name: " << this->PedigreeIdsName << "\n";
  }
  else
  {
    os << indent << "Pedigree Ids Name: (None)\n";
  }

  if ( this->EdgeFlagsName )
  {
    os << indent << "Edge Flags Name: " << this->EdgeFlagsName << "\n";
  }
  else
  {
    os << indent << "Edge Flags Name: (None)\n";
  }

  if ( this->LookupTableName )
  {
    os << indent << "Lookup Table Name: " << this->LookupTableName << "\n";
  }
  else
  {
    os << indent << "Lookup Table Name: (None)\n";
  }

  if ( this->FieldDataName )
  {
    os << indent << "Field Data Name: " << this->FieldDataName << "\n";
  }
  else
  {
    os << indent << "Field Data Name: (None)\n";
  }

}

int vtkDataWriter::WriteDataSetData(ostream *fp, vtkDataSet *ds)
{
  vtkFieldData* field = ds->GetFieldData();
  if (field && field->GetNumberOfTuples() > 0)
  {
    if ( !this->WriteFieldData(fp, field) )
    {
      return 0; // we tried to write field data, but we couldn't
    }
  }
  return 1;
}
