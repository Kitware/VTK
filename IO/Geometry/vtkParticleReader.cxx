/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParticleReader.h"

#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkByteSwap.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"

#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

vtkStandardNewMacro(vtkParticleReader);

namespace {
  // .NAME ParseLine - Read a line of four values of type T filtering out comments.
  // .SECTION Description
  // The ParseLine operator scans a string and returns 0 if it finds a
  // comment symbol. Otherwise it returns 1 and four values corresponding
  // to the position of the particle (x,y,z) and a scalar s associated with
  // the particle. It is a good idea to set the values of val to some
  // predefined value before calling the operator.
  template < typename T >
    class ParseLine
  {
  public:
    ParseLine (): LookForEndString(false) {};
    int operator () (std::string &s, T val[4])
    {
      // Skip over comment lines.
      std::string::iterator itr;
      std::string tgt("/*");
      itr = std::search(s.begin(),s.end(),tgt.begin(),tgt.end());
      if ( itr != s.end() )
      {
        LookForEndString = true;
        //continue;
      }
      if ( LookForEndString )
      {
        tgt = "*/";
        itr = std::search(s.begin(),s.end(),tgt.begin(),tgt.end());
        if ( itr != s.end() )
        {
          LookForEndString = false;
        }
        //continue;
        return 0;
      }

      tgt = "//";
      itr = std::search(s.begin(),s.end(),tgt.begin(),tgt.end());
      if ( itr != s.end() )
      {
        return 0;
      }
      tgt = "%";
      itr = std::search(s.begin(),s.end(),tgt.begin(),tgt.end());
      if ( itr != s.end() )
      {
        return 0;
      }
      tgt = "#";
      itr = std::search(s.begin(),s.end(),tgt.begin(),tgt.end());
      if ( itr != s.end() )
      {
        return 0;
      }
      // If comma delimited, replace with tab
      std::replace(s.begin(),s.end(),',','\t');

      // We have data.
      std::stringstream is;
      is << s.c_str();
      is >> val[0] >> val[1] >> val[2] >> val[3];

      return 1;
    }

  private:
    bool LookForEndString;

  };

 // The number of times we output a progress message.
 int const quantum = 20;
 // The ratio of high ASCII characters to low ASCII characeters.
 double hiToLowASCII = 0.1;

}
//----------------------------------------------------------------------------
vtkParticleReader::vtkParticleReader() :
  FileName(NULL)
  , File(NULL)
  , HasScalar(1)
  , FileType(FILE_TYPE_IS_UNKNOWN)
  , DataType(VTK_FLOAT)
  , Alliquot(0)
  , Count(0)
  , SwapBytes(0)
  , NumberOfPoints(0)
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkParticleReader::~vtkParticleReader()
{
  if (this->File)
  {
    this->File->close();
    delete this->File;
    this->File = NULL;
  }

  delete [] this->FileName;
  this->FileName = NULL;
}

//----------------------------------------------------------------------------
void vtkParticleReader::OpenFile()
{
  if (!this->FileName)
  {
    vtkErrorMacro(<<"FileName must be specified.");
    return;
  }

  // If the file was open close it.
  if (this->File)
  {
    this->File->close();
    delete this->File;
    this->File = NULL;
  }

  // Open the new file.
  vtkDebugMacro(<< "Initialize: opening file " << this->FileName);
#ifdef _WIN32
  this->File = new ifstream(this->FileName, ios::in | ios::binary);
#else
  this->File = new ifstream(this->FileName, ios::in);
#endif
  if (! this->File || this->File->fail())
  {
    vtkErrorMacro(<< "Initialize: Could not open file " <<
    this->FileName);
    return;
  }
}

//----------------------------------------------------------------------------
int vtkParticleReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro(<<"FileName must be specified.");
    return 0;
  }

  this->OpenFile();
  if (!this->File || this->File->fail())
  {
    return 0;
  }
  int ft = this->FileType;
  if ( ft == FILE_TYPE_IS_UNKNOWN )
  {
    ft = DetermineFileType();
    if ( ft == FILE_TYPE_IS_UNKNOWN )
    {
      vtkErrorMacro(<< "File type cannot be determined.");
      return 0;
    }
  }
  this->File->close();
  delete this->File;
  this->File = NULL;


  if (ft == FILE_TYPE_IS_BINARY)
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(),
                 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkParticleReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro(<<"FileName must be specified.");
    return 0;
  }

  this->OpenFile();
  int ft = this->FileType;
  if ( ft == FILE_TYPE_IS_UNKNOWN )
  {
      ft = DetermineFileType();
      if ( ft == FILE_TYPE_IS_UNKNOWN )
      {
        vtkErrorMacro(<< "File type cannot be determined.");
        return 0;
      }
  }

  switch ( ft )
  {
  case FILE_TYPE_IS_TEXT:
    switch ( this->DataType )
    {
    case VTK_FLOAT:
      return ProduceOutputFromTextFileFloat(outputVector);
    case VTK_DOUBLE:
      return ProduceOutputFromTextFileDouble(outputVector);
    default:
    {
      vtkErrorMacro(<<"Only float or double data can be processed.");
      return 0;
    }
    }
  case FILE_TYPE_IS_BINARY:
    switch ( this->DataType )
    {
    case VTK_FLOAT:
      return ProduceOutputFromBinaryFileFloat(outputVector);
    case VTK_DOUBLE:
      return ProduceOutputFromBinaryFileDouble(outputVector);
    default:
    {
      vtkErrorMacro(<<"Only float or double data can be processed.");
      return 0;
    }
    }
  default:
  {
    vtkErrorMacro(<<"The file type was not able to be determined.");
    return 0;
  }
  }
}

//----------------------------------------------------------------------------
int vtkParticleReader::DetermineFileType()
{
  // This function assumes that the file has been opened.

  this->File->seekg(0,ios::end);
  if (this->File->fail())
  {
    vtkErrorMacro("Could not seek to end of file.");
    return FILE_TYPE_IS_UNKNOWN;
  }
  size_t fileLength = this->File->tellg();
  if ( fileLength == 0 )
  {
    vtkErrorMacro("File is empty.");
    return FILE_TYPE_IS_UNKNOWN;
  }

  this->File->seekg(0,ios::beg);
  if (this->File->fail())
  {
    vtkErrorMacro("Could not seek to start of file.");
    return FILE_TYPE_IS_UNKNOWN;
  }

  size_t sampleSize = fileLength < 5000 ? fileLength: 5000;
  // cout << "File length: " << fileLength << " Sample size: " << sampleSize << endl;
  std::vector <unsigned char> s;
  for ( size_t i = 0; i < sampleSize; ++i )
  {
    char c;
    this->File->read(&c,sizeof(char));
    s.push_back(c);
  }
  // If read terminated prematurely then it may have detected
  // a premature EOF character in the data.
  // Assume that the file type is undetermined in this case.
  if ( s.size() != sampleSize )
  {
    // cout << "Premature termination" << endl;
    return FILE_TYPE_IS_UNKNOWN;
  }

  size_t zero = 0;
  size_t conventionalASCII = 0;
  size_t extendedASCII = 0;
  size_t controlASCII = 0;
  size_t otherASCII = 0;
  for ( size_t j = 0; j < s.size(); ++j )
  {
    if ( s[j] == '\0' )
    {
      zero++;
      continue;
    }
    // Conventional ASCII characters.
    if ( s[j] > 0x1f && s[j] < 0x80 )
    {
     conventionalASCII++;
     continue;
    }
    // Extended ASCII characters may have been used.
    if ( s[j] > 0x7f )
    {
      extendedASCII++;
      continue;
    }
    // Control characters.
    if ( s[j] == '\n' || s[j] == '\r' || s[j] == '\t' || s[j] == '\f' )
    {
      controlASCII++;
      continue;
    }
    otherASCII++;
  }

  // NULL shouldn't ever appear in a text file.
  if ( zero != 0 || otherASCII > 0 || conventionalASCII == 0 )
  {
    return FILE_TYPE_IS_BINARY;
  }
  if ( (double)extendedASCII / (double) conventionalASCII < hiToLowASCII )
  {
    return FILE_TYPE_IS_TEXT;
  }

  return FILE_TYPE_IS_BINARY;
}

//----------------------------------------------------------------------------
int vtkParticleReader::ProduceOutputFromTextFileDouble(vtkInformationVector *outputVector)
{
  // Get the size of the file.
  this->File->seekg(0,ios::end);
  if (this->File->fail())
  {
    vtkErrorMacro("Could not seek to end of file.");
    return 0;
  }

  size_t fileLength = (unsigned long)this->File->tellg();
  size_t bytesRead = 0;

  std::string s;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->Reset();

  vtkSmartPointer<vtkDoubleArray> scalars = vtkSmartPointer<vtkDoubleArray>::New();
  scalars->Reset();
  scalars->SetName("Scalar");

  this->File->seekg(0,ios::beg);

  this->Alliquot = fileLength / quantum;
  this->Count = 1;
  ParseLine<double> pl;
  char buffer[256];
  while ( this->File->getline(buffer,256,'\n') )
  {
    s = buffer;
    if ( s.size() != 0 )
    {
      bytesRead += s.size();
      this->DoProgressUpdate( bytesRead, fileLength );
      double val[4];
      val[0]=val[1]=val[2]=val[3]=0;
      if ( pl(s,val) )
      {
        points->InsertNextPoint(val[0], val[1], val[2]);
        if ( this->HasScalar)
        {
          scalars->InsertNextValue(val[3]);
        }
      }
    }
  }

  vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
  vertices->Reset();

  this->NumberOfPoints = points->GetNumberOfPoints();
  for( vtkIdType j = 0; j < (vtkIdType)this->NumberOfPoints; ++j )
  {
    vertices->InsertNextCell( 1 );
    vertices->InsertCellPoint( j );
  }

   // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->SetPoints(points);
  output->SetVerts(vertices);
  if ( this->HasScalar )
    output->GetPointData()->SetScalars(scalars);

  output->Modified();

  return 1;
}

//----------------------------------------------------------------------------
int vtkParticleReader::ProduceOutputFromTextFileFloat(vtkInformationVector *outputVector)
{
  // Get the size of the file.
  this->File->seekg(0,ios::end);
  if (this->File->fail())
  {
    vtkErrorMacro("Could not seek to end of file.");
    return 0;
  }

  size_t fileLength = (unsigned long)this->File->tellg();
  size_t bytesRead = 0;

  std::string s;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToFloat();
  points->Reset();

  vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
  scalars->Reset();
  scalars->SetName("Scalar");

  this->File->seekg(0,ios::beg);

  this->Alliquot = fileLength / quantum;
  this->Count = 1;
  ParseLine<float> pl;
  char buffer[256];
  while ( this->File->getline(buffer,256,'\n') )
  {
    s = buffer;
    if ( s.size() != 0 )
    {
      bytesRead += s.size();
      this->DoProgressUpdate( bytesRead, fileLength );

      float val[4];
      val[0]=val[1]=val[2]=val[3]=0;
      if ( pl(s,val) )
      {
        points->InsertNextPoint(val[0], val[1], val[2]);
        if ( this->HasScalar)
        {
          scalars->InsertNextValue(val[3]);
        }
      }
    }
  }

  vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
  vertices->Reset();

  this->NumberOfPoints = points->GetNumberOfPoints();
  for( vtkIdType j = 0; j < (vtkIdType)this->NumberOfPoints; ++j )
  {
    vertices->InsertNextCell( 1 );
    vertices->InsertCellPoint( j );
  }

   // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->SetPoints(points);
  output->SetVerts(vertices);
  if ( this->HasScalar )
  {
    output->GetPointData()->SetScalars(scalars);
  }

  output->Modified();

  return 1;
}



//----------------------------------------------------------------------------
int vtkParticleReader::ProduceOutputFromBinaryFileDouble(vtkInformationVector *outputVector)
{

  unsigned long fileLength, start, next, length, ptIdx, cellPtIdx;
  unsigned long cellLength;
  int piece, numPieces;
  double *data, *ptr;

  if (!this->FileName)
  {
    vtkErrorMacro(<<"FileName must be specified.");
    return 0;
  }

  this->OpenFile();

  // Get the size of the header from the size of the image
  this->File->seekg(0,ios::end);
  if (this->File->fail())
  {
    vtkErrorMacro("Could not seek to end of file.");
    return 0;
  }

  fileLength = (unsigned long)this->File->tellg();
  if ( this->HasScalar )
  {
    this->NumberOfPoints = fileLength / (4 * sizeof(double));
  }
  else
  {
    this->NumberOfPoints = fileLength / (3 * sizeof(double));
  }

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if ((unsigned long)numPieces > this->NumberOfPoints)
  {
    numPieces = (int)(this->NumberOfPoints);
  }
  if (numPieces <= 0 || piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  start = static_cast<unsigned long>(piece * this->NumberOfPoints / numPieces);
  next = static_cast<unsigned long>((piece+1) * this->NumberOfPoints / numPieces);

  length = next - start;

  if ( this->HasScalar )
  {
    data = new double[length * 4];
  }
  else
  {
    data = new double[length * 3];
  }

  // Seek to the first point in the file.
  if ( this->HasScalar )
  {
    this->File->seekg(start*4*sizeof(double), ios::beg);
  }
  else
  {
    this->File->seekg(start*3*sizeof(double), ios::beg);
  }
  if (this->File->fail())
  {
    vtkErrorMacro(<< "File operation failed: Seeking to " << start*4);
    delete [] data;
    return 0;
  }

  // Read the data.
  if ( this->HasScalar )
  {
    if ( ! this->File->read((char *)data, length*4*sizeof(double)))
    {
      vtkErrorMacro("Could not read points: " << start
             << " to " << next-1);
      delete [] data;
      return 0;
    }
  }
  else
  {
    if ( ! this->File->read((char *)data, length*3*sizeof(double)))
    {
      vtkErrorMacro("Could not read points: " << start
             << " to " << next-1);
      delete [] data;
      return 0;
    }
  }

  // Swap bytes if necessary.
  if (this->GetSwapBytes())
  {
    if ( this->HasScalar )
    {
      vtkByteSwap::SwapVoidRange(data, length*4, sizeof(double));
    }
    else
    {
      vtkByteSwap::SwapVoidRange(data, length*3, sizeof(double));
    }
  }

  this->UpdateProgress(0.5);

  ptr = data;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(length);
  vtkSmartPointer<vtkDoubleArray> array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetName("Scalar");
  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();

  // Each cell will have 1000 points.  Leave a little extra space just in case.
  // We break up the cell this way so that the render will check for aborts
  // at a reasonable rate.
  verts->Allocate((int)((double)length * 1.002));
  // Keep adding cells until we run out of points.
  ptIdx = 0;
  int cnt = 1;
  double len = length;
  while (length > 0)
  {
    if ( cnt % 10 == 0 )
    {
     this->UpdateProgress(0.5+((cnt * 1000.0)/len)/2.0);
    }
    cnt++;
    cellLength = 1000;
    if (cellLength > length)
    {
      cellLength = length;
    }
    length = length - cellLength;
    verts->InsertNextCell((int)cellLength);
    for (cellPtIdx = 0; cellPtIdx < cellLength; ++cellPtIdx)
    {
      points->SetPoint(ptIdx, ptr[0], ptr[1], ptr[2]);
      if ( this->HasScalar )
      {
        array->InsertNextValue(ptr[3]);
        ptr += 4;
      }
      else
      {
        ptr += 3;
      }
      verts->InsertCellPoint(ptIdx);
      ++ptIdx;
    }
  }
  delete [] data;

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->SetPoints(points);
  output->SetVerts(verts);
  if ( this->HasScalar )
  {
    output->GetPointData()->SetScalars(array);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkParticleReader::ProduceOutputFromBinaryFileFloat(vtkInformationVector *outputVector)
{

  unsigned long fileLength, start, next, length, ptIdx, cellPtIdx;
  unsigned long cellLength;
  int piece, numPieces;
  float *data, *ptr;

  if (!this->FileName)
  {
    vtkErrorMacro(<<"FileName must be specified.");
    return 0;
  }

  this->OpenFile();

  // Get the size of the header from the size of the image
  this->File->seekg(0,ios::end);
  if (this->File->fail())
  {
    vtkErrorMacro("Could not seek to end of file.");
    return 0;
  }

  fileLength = (unsigned long)this->File->tellg();
  if ( this->HasScalar )
  {
    this->NumberOfPoints = fileLength / (4 * sizeof(float));
  }
  else
  {
    this->NumberOfPoints = fileLength / (3 * sizeof(float));
  }


  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if ((unsigned long)numPieces > this->NumberOfPoints)
  {
    numPieces = (int)(this->NumberOfPoints);
  }
  if (numPieces <= 0 || piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  start = static_cast<unsigned long>(piece * this->NumberOfPoints / numPieces);
  next = static_cast<unsigned long>((piece+1) * this->NumberOfPoints / numPieces);

  length = next - start;

  if ( this->HasScalar )
  {
    data = new float[length * 4];
  }
  else
  {
    data = new float[length * 3];
  }


  // Seek to the first point in the file.
  if ( this->HasScalar )
  {
    this->File->seekg(start*4*sizeof(float), ios::beg);
  }
  else
  {
    this->File->seekg(start*3*sizeof(float), ios::beg);
  }
  if (this->File->fail())
  {
    vtkErrorMacro(<< "File operation failed: Seeking to " << start*4);
    delete [] data;
    return 0;
  }

  // Read the data.
  if ( this->HasScalar )
  {
    if ( ! this->File->read((char *)data, length*4*sizeof(float)))
    {
      vtkErrorMacro("Could not read points: " << start
             << " to " << next-1);
      delete [] data;
      return 0;
    }
  }
  else
  {
    if ( ! this->File->read((char *)data, length*3*sizeof(float)))
    {
      vtkErrorMacro("Could not read points: " << start
             << " to " << next-1);
      delete [] data;
      return 0;
    }
  }

  // Swap bytes if necessary.
  if (this->GetSwapBytes())
  {
    if ( this->HasScalar )
    {
      vtkByteSwap::SwapVoidRange(data, length*4, sizeof(float));
    }
    else
    {
      vtkByteSwap::SwapVoidRange(data, length*3, sizeof(float));
    }
  }

  this->UpdateProgress(0.5);

  ptr = data;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(length);
  vtkSmartPointer<vtkFloatArray> array = vtkSmartPointer<vtkFloatArray>::New();
  array->SetName("Scalar");
  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();

  // Each cell will have 1000 points.  Leave a little extra space just in case.
  // We break up the cell this way so that the render will check for aborts
  // at a reasonable rate.
  verts->Allocate((int)((float)length * 1.002));
  // Keep adding cells until we run out of points.
  ptIdx = 0;
  int cnt = 1;
  double len = length;
  while (length > 0)
  {
    if ( cnt % 10 == 0 )
    {
     this->UpdateProgress(0.5+((cnt * 1000.0)/len)/2.0);
    }
    cnt++;
    cellLength = 1000;
    if (cellLength > length)
    {
      cellLength = length;
    }
    length = length - cellLength;
    verts->InsertNextCell((int)cellLength);
    for (cellPtIdx = 0; cellPtIdx < cellLength; ++cellPtIdx)
    {
      points->SetPoint(ptIdx, ptr[0], ptr[1], ptr[2]);
      if ( this->HasScalar )
      {
        array->InsertNextValue(ptr[3]);
        ptr += 4;
      }
      else
      {
        ptr += 3;
      }
      verts->InsertCellPoint(ptIdx);
      ++ptIdx;
    }
  }
  delete [] data;

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->SetPoints(points);
  output->SetVerts(verts);
  if ( this->HasScalar )
  {
    output->GetPointData()->SetScalars(array);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkParticleReader::DoProgressUpdate( size_t & bytesRead, size_t & fileLength )
{
  if ( bytesRead > this->Alliquot )
  {
    this->UpdateProgress( bytesRead/(double)fileLength );
    this->Count++;
    this->Alliquot = fileLength / quantum * this->Count;
  }
}

//----------------------------------------------------------------------------
void vtkParticleReader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void vtkParticleReader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void vtkParticleReader::SetDataByteOrder(int byteOrder)
{
  if ( byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN )
  {
    this->SetDataByteOrderToBigEndian();
  }
  else
  {
    this->SetDataByteOrderToLittleEndian();
  }
}

//----------------------------------------------------------------------------
int vtkParticleReader::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
  {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
  else
  {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
#else
  if ( this->SwapBytes )
  {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
  else
  {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
#endif
}

//----------------------------------------------------------------------------
const char *vtkParticleReader::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
  {
    return "LittleEndian";
  }
  else
  {
    return "BigEndian";
  }
#else
  if ( this->SwapBytes )
  {
    return "BigEndian";
  }
  else
  {
    return "LittleEndian";
  }
#endif
}

//----------------------------------------------------------------------------
void vtkParticleReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Swap Bytes: " << (this->SwapBytes ? "On\n" : "Off\n");
  os << indent << "Has Scalar: " << (this->HasScalar ? "On\n" : "Off\n");
  switch ( this->FileType )
  {
  case FILE_TYPE_IS_UNKNOWN:
    os << indent << "File type is unknown (The class automatically determines the file type).\n";
    break;
  case FILE_TYPE_IS_TEXT:
    os << indent << "File type is text.\n";
    break;
  case FILE_TYPE_IS_BINARY:
    os << indent << "File type is binary.\n";
    break;
  default:
    os << indent << "File type should never have this value: " << this->FileType << "\n";
    break;
  }
  switch ( this->DataType )
  {
  case VTK_FLOAT:
    os << indent << "Data type is float.\n";
    break;
  case VTK_DOUBLE:
    os << indent << "Data type is double.\n";
    break;
  default:
    os << indent << "Data type should never have this value: " << this->DataType << "\n";
    break;
  }
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << "\n";
  os << indent << "Alliquot: " << (unsigned int)this->Alliquot << "\n";
  os << indent << "Count: " << (unsigned int)this->Count << "\n";

}
