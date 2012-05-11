/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterXMLWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Kristian Sons
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkX3DExporterXMLWriter.h"

#include "vtkX3D.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkMath.h"

#include <vtksys/ios/sstream>
#include <vtksys/ios/fstream>
#include <string>
#include <cassert>

using namespace vtkX3D;

struct XMLInfo {

XMLInfo(int _elementId)
{
  this->elementId = _elementId;
  this->endTagWritten = false;
}
  int elementId;
  bool endTagWritten;
};

typedef std::vector<XMLInfo> vtkX3DExporterXMLNodeInfoStackBase;
class vtkX3DExporterXMLNodeInfoStack: public vtkX3DExporterXMLNodeInfoStackBase
{};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkX3DExporterXMLWriter);
//-----------------------------------------------------------------------------
vtkX3DExporterXMLWriter::~vtkX3DExporterXMLWriter()
{
  delete this->InfoStack;
  if(this->OutputStream)
    {
    delete this->OutputStream;
    this->OutputStream = NULL;
    }
}

//-----------------------------------------------------------------------------
vtkX3DExporterXMLWriter::vtkX3DExporterXMLWriter()
{
  this->OutputStream = NULL;
  this->InfoStack = new vtkX3DExporterXMLNodeInfoStack();
  this->Depth = 0;
  this->ActTab = "";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkX3DExporterXMLWriter::OpenFile(const char* file)
{
  this->CloseFile();
  this->WriteToOutputString = 0;
  vtksys_ios::fstream* fileStream = new vtksys_ios::fstream();
  fileStream->open (file, ios::out);
  if(fileStream->fail())
    {
    delete fileStream;
    return 0;
    }
  else
    {
    this->OutputStream = fileStream;
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkX3DExporterXMLWriter::OpenStream()
{
  this->CloseFile();

  this->WriteToOutputString = 1;
  this->OutputStream = new vtksys_ios::ostringstream();
  return 1;
}

//----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::CloseFile()
{
  if(this->OutputStream != NULL)
    {
    if(this->WriteToOutputString)
      {
      vtksys_ios::ostringstream *ostr =
        static_cast<vtksys_ios::ostringstream*>(this->OutputStream);

      if(this->OutputString)
        {
        delete [] this->OutputString;
        }
      this->OutputStringLength = static_cast<int>(ostr->str().size());
      this->OutputString = new char[ostr->str().size()];
      memcpy(this->OutputString, ostr->str().c_str(),
             this->OutputStringLength);
      }

    delete this->OutputStream;
    this->OutputStream = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::StartDocument()
{
  this->Depth = 0;
  *this->OutputStream << "<?xml version=\"1.0\" encoding =\"UTF-8\"?>"
    << endl << endl;
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::EndDocument()
{
  assert(this->Depth == 0);
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::StartNode(int elementID)
{
  // End last tag, if this is the first child
  if (!this->InfoStack->empty())
    {
    if (!this->InfoStack->back().endTagWritten)
      {
      *this->OutputStream << ">" << this->GetNewline();
      this->InfoStack->back().endTagWritten = true;
      }
    }

  this->InfoStack->push_back(XMLInfo(elementID));
  *this->OutputStream << this->ActTab << "<" << x3dElementString[elementID];
  this->AddDepth();
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::EndNode()
{
  assert(!this->InfoStack->empty());
  this->SubDepth();
  int elementID = this->InfoStack->back().elementId;

  // There were no childs
  if (!this->InfoStack->back().endTagWritten)
    {
    *this->OutputStream << "/>" << this->GetNewline();
    }
  else
    {
    *this->OutputStream << this->ActTab << "</" << x3dElementString[elementID]
      << ">" << this->GetNewline();
    }

  this->InfoStack->pop_back();
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, int type, const double* d)
{
  *this->OutputStream << " " << x3dAttributeString[attributeID] << "=\"";
  switch (type)
    {
    case(SFVEC3F):
    case(SFCOLOR):
      *this->OutputStream << d[0]
                         << " "
                         << d[1]
                         << " "
                         << d[2];
      break;
    case(SFROTATION):
      *this->OutputStream << d[1]
                         << " "
                         << d[2]
                         << " "
                         << d[3]
                         << " "
                         << vtkMath::RadiansFromDegrees( -d[0] );
      break;
    default:
      *this->OutputStream << "UNKNOWN DATATYPE";
    }
  *this->OutputStream << "\"";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, int type, vtkDataArray* a)
{
  *this->OutputStream << " " << x3dAttributeString[attributeID] << "=\"" << this->GetNewline();
  switch (type)
    {
    case(MFVEC3F):
      for (vtkIdType i = 0; i < a->GetNumberOfTuples(); i++)
        {
        double* d = a->GetTuple(i);
        *this->OutputStream << this->ActTab << d[0] << " " << d[1] << " " << d[2] << "," << this->GetNewline();
        }
      break;
    case(MFVEC2F):
      for (vtkIdType i = 0; i < a->GetNumberOfTuples(); i++)
        {
        double* d = a->GetTuple(i);
        *this->OutputStream << this->ActTab << d[0] << " " << d[1] << "," << this->GetNewline();
        }
      break;
    default:
      *this->OutputStream << "UNKNOWN DATATYPE";
    }
    *this->OutputStream << this->ActTab << "\"";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, const double* values, size_t size)
{
  *this->OutputStream << " " << x3dAttributeString[attributeID] << "=\"" << this->GetNewline() << this->ActTab;

  unsigned int i = 0;
  while (i < size)
    {
    *this->OutputStream << values[i];
    if ((i+1)%3)
      {
      *this->OutputStream << " ";
      }
    else
      {
      *this->OutputStream << "," << this->GetNewline() << this->ActTab;
      }
    i++;
    }
  *this->OutputStream  << "\"";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, const int* values, size_t size, bool image)
{
  *this->OutputStream << " " << x3dAttributeString[attributeID] << "=\"" << this->GetNewline() << this->ActTab;

  unsigned int i = 0;
  if (image)
    {
    assert(size > 2);
    char buffer[20];
    *this->OutputStream << values[0] << " "; // width
    *this->OutputStream << values[1] << " "; // height
    int bpp = values[2]; *this->OutputStream << bpp << "\n"; // bpp

    i = 3;
    unsigned int j = 0;

    while (i < size)
      {
      sprintf(buffer,"0x%.8x",values[i]);
      *this->OutputStream << buffer;

      if (j%(8*bpp))
        {
        *this->OutputStream << " ";
        }
      else
        {
        *this->OutputStream << this->GetNewline();// << this->ActTab;
        }
      i++; j+=bpp;
      }
    *this->OutputStream << dec;
    }
  else
    while (i < size)
    {
    *this->OutputStream << values[i] << " ";
    if (values[i] == -1)
      {
      *this->OutputStream << this->GetNewline() << this->ActTab;
      }
    i++;
    }
  *this->OutputStream  << "\"";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, int value)
{
  *this->OutputStream << " " << x3dAttributeString[attributeID] << "=\"" << value << "\"";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, float value)
{
  *this->OutputStream << " " << x3dAttributeString[attributeID] << "=\"" << value << "\"";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, double vtkNotUsed(value))
{
  *this->OutputStream << " " << x3dAttributeString[attributeID] << "=\"" << "WHY DOUBLE?" << "\"";
  assert(false);
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, bool value)
{
  *this->OutputStream << " "
    << x3dAttributeString[attributeID] << "=\"" << (value  ? "true" : "false" ) << "\"";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SetField(int attributeID, const char* value, bool mfstring)
{
  if (mfstring)
    {
    *this->OutputStream << " "
      << x3dAttributeString[attributeID] << "='" << value << "'";
    }
  else
    {
    *this->OutputStream
      << " " << x3dAttributeString[attributeID] << "=\"" << value << "\"";
    }
}


//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::Flush()
{
  this->OutputStream->flush();
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::AddDepth()
{
  this->ActTab += "  ";
}

//-----------------------------------------------------------------------------
void vtkX3DExporterXMLWriter::SubDepth()
{
  this->ActTab.erase(0, 2);
}

