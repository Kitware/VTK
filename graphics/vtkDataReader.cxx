/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <ctype.h>
#include "vtkDataReader.h"
#include "vtkRectilinearGrid.h"
#include "vtkPointSet.h"
#include "vtkBitScalars.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkFloatScalars.h"
#include "vtkShortScalars.h"
#include "vtkIntScalars.h"
#include "vtkFloatPoints.h"
#include "vtkIntPoints.h"
#include "vtkFloatNormals.h"
#include "vtkFloatTensors.h"
#include "vtkFloatTCoords.h"
#include "vtkGraymap.h"
#include "vtkAGraymap.h"
#include "vtkPixmap.h"
#include "vtkAPixmap.h"
#include "vtkLookupTable.h"
#include "vtkByteSwap.h"

// Description:
// Construct object.
vtkDataReader::vtkDataReader()
{
  this->Filename = NULL;
  this->ScalarsName = NULL;
  this->VectorsName = NULL;
  this->TensorsName = NULL;
  this->NormalsName = NULL;
  this->TCoordsName = NULL;
  this->LookupTableName = NULL;
  this->ScalarLut = NULL;
  this->InputString = NULL;
  this->InputStringPos = 0;
  this->ReadFromInputString = 0;
  this->IS = NULL;
}  

vtkDataReader::~vtkDataReader()
{
  if (this->Filename) delete [] this->Filename;
  if (this->ScalarsName) delete [] this->ScalarsName;
  if (this->VectorsName) delete [] this->VectorsName;
  if (this->TensorsName) delete [] this->TensorsName;
  if (this->NormalsName) delete [] this->NormalsName;
  if (this->TCoordsName) delete [] this->TCoordsName;
  if (this->LookupTableName) delete [] this->LookupTableName;
  if (this->ScalarLut) delete [] this->ScalarLut;
  if (this->InputString) delete [] this->InputString;
}

void vtkDataReader::SetInputString(char* _arg, int len)
{ 
  if (this->Debug)
    {
    vtkDebugMacro(<< "setting InputString to " << _arg );
    }

  if (this->InputString) delete [] this->InputString; 
  if (_arg) 
    { 
    this->InputString = new char[len]; 
    memcpy(this->InputString,_arg,len); 
    } 
   else 
    { 
    this->InputString = NULL; 
    } 
  this->Modified(); 
} 

// Description:
// Internal function used to consume whitespace when reading in
// an InputString.
void vtkDataReader::EatWhiteSpace()
{
  char c;
  while (this->IS->get(c)) 
    {
    if (c == '\n' || isspace(c)==0)
      {
      this->IS->putback(c);
      break;
      }
    }
}

// Description:
// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int vtkDataReader::ReadLine(char result[256])
{
  this->IS->getline(result,256);
  if (this->IS->eof()) return 0;
  return 1;
}

// Description:
// Internal function to read in a string up to 256 characters.
// Returns zero if there was an error.
int vtkDataReader::ReadString(char result[256])
{
  *this->IS >> result;
  if (this->IS->fail()) return 0;
  return 1;
}

// Description:
// Internal function to read in an integer value.
// Returns zero if there was an error.
int vtkDataReader::ReadInt(int *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::ReadShort(short *result)
{
  int i, ret;
  ret = this->ReadInt(&i);
  *result = (short)i;
  return ret;
}

int vtkDataReader::ReadUnsignedShort(unsigned short *result)
{
  int i, ret;
  ret = this->ReadInt(&i);
  *result = (unsigned short)i;
  return ret;
}

int vtkDataReader::ReadUChar(unsigned char *result)
{
  int i, ret;
  ret = this->ReadInt(&i);
  *result = (unsigned char)i;
  return ret;
}

// Description:
// Internal function to read in a float value.
// Returns zero if there was an error.
int vtkDataReader::ReadFloat(float *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

// Description:
// Open a vtk data file. Returns zero if error.
int vtkDataReader::OpenVTKFile()
{
  if (this->ReadFromInputString)
    {
    if (this->InputString)
      {
      vtkDebugMacro(<< "Reading from InputString");
      this->IS = new istrstream(this->InputString,strlen(this->InputString));
      return 1;
      }
    }
  else
    {
    vtkDebugMacro(<< "Opening vtk file");

    if ( !this->Filename )
      {
      vtkErrorMacro(<< "No file specified!");
      return 0;
      }
    this->IS = new ifstream(this->Filename, ios::in);
    if (this->IS->fail())
      {
      vtkErrorMacro(<< "Unable to open file: "<< this->Filename);
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    return 1;
    }

  return 0;
}

// Description:
// Read the header of a vtk data file. Returns 0 if error.
int vtkDataReader::ReadHeader()
{
  char line[256];

  vtkDebugMacro(<< "Reading vtk file header");
  //
  // read header
  //
  if (!this->ReadLine(line))
    {
    vtkErrorMacro(<<"Premature EOF reading first line! " << " for file: " 
                  << this->Filename);
    return 0;
    }
  if ( strncmp ("# vtk DataFile Version", line, 20) )
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                  << this->Filename);
    return 0;
    }
  //
  // read title
  //
  if (!this->ReadLine(line))
    {
    vtkErrorMacro(<<"Premature EOF reading title! " << " for file: " 
                  << this->Filename);
    return 0;
    }
  vtkDebugMacro(<< "Reading vtk file entitled: " << line);
  //
  // read type
  //
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Premature EOF reading file type!" << " for file: " 
                  << this->Filename);
    return 0;
    }

  if ( !strncmp(this->LowerCase(line),"ascii",5) ) this->FileType = VTK_ASCII;
  else if ( !strncmp(line,"binary",6) ) this->FileType = VTK_BINARY;
  else
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                  << this->Filename);
    this->FileType = 0;
    return 0;
    }

  // if this is a binary file we need to make sure that we opened it 
  // as a binary file.
  if (this->FileType == VTK_BINARY)
    {
    vtkDebugMacro(<< "Opening vtk file as binary");
    delete this->IS;
#ifdef _WIN32
    this->IS = new ifstream(this->Filename, ios::in | ios::binary);
#else
    this->IS = new ifstream(this->Filename, ios::in);
#endif
    if (this->IS->fail())
      {
      vtkErrorMacro(<< "Unable to open file: "<< this->Filename);
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    // read up to the same point in the file
    this->ReadLine(line);
    this->ReadLine(line);
    this->ReadString(line);
    }
  return 1;
}

// Description:
// Read the point data of a vtk data file. The number of points (from the 
// dataset) must match the number of points defined in point attributes (unless
// no geometry was defined).
int vtkDataReader::ReadPointData(vtkDataSet *ds, int numPts)
{
  char line[256];
  
  vtkDebugMacro(<< "Reading vtk point data");
  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
    {
    //
    // read scalar data
    //
    if ( ! strncmp(this->LowerCase(line), "scalars", 7) )
      {
      if ( ! this->ReadScalarData(ds, numPts) ) return 0;
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(ds, numPts) ) return 0;
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(ds, numPts) ) return 0;
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {
      if ( ! this->ReadNormalData(ds, numPts) ) return 0;
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(ds, numPts) ) return 0;
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(ds, numPts) ) return 0;
      }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(line, "lookup_table", 12) )
      {
      if ( ! this->ReadLutData(ds) ) return 0;
      }

    else
      {
      vtkErrorMacro(<< "Unsupported point attribute type: " << line 
                    << " for file: " << this->Filename);
      return 0;
      }
    }

  return 1;
}

// Description:
// Read point coordinates. Return 0 if error.
int vtkDataReader::ReadPoints(vtkPointSet *ps, int numPts)
{
  int i;
  char line[256];

  if (!this->ReadString(line)) 
    {
    vtkErrorMacro(<<"Cannot read points type!" << " for file: " << this->Filename);
    return 0;
    }

  if ( ! strncmp(this->LowerCase(line), "int", 3) )
    {
    vtkIntPoints *points = vtkIntPoints::New();
    int *ptr = points->WritePointer(0,numPts);
    if (this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(int)*3*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary points!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,3*numPts);
      }
    else // ascii
      {
      int p[3];
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadInt(p) && this->ReadInt(p+1) && this->ReadInt(p+2)))
          {
          vtkErrorMacro(<<"Error reading points!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        points->SetPoint(i,p);
        }
      }
    ps->SetPoints(points);
    points->Delete();
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vtkFloatPoints *points = vtkFloatPoints::New();
    float *ptr = points->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(float)*3*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary points!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,3*numPts);
      }
    else // ascii
      {
      float p[3];
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(p) && this->ReadFloat(p+1) && 
	      this->ReadFloat(p+2)))
          {
          vtkErrorMacro(<<"Error reading points!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        points->SetPoint(i,p);
        }
      }
    ps->SetPoints(points);
    points->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported points type: " << line << " for file: " 
                  << this->Filename);
    return 0;
    }

  vtkDebugMacro(<<"Read " << ps->GetNumberOfPoints() << " points");
  return 1;
}

// Description:
// Read the coordinates for a rectilinear grid. The axes parameter specifies
// which coordinate axes (0,1,2) is being read.
int vtkDataReader::ReadCoordinates(vtkRectilinearGrid *rg, int axes, 
                                   int numCoords)
{
  int i;
  char line[256];
  vtkScalars *coords;

  if (!this->ReadString(line)) 
    {
    vtkErrorMacro(<<"Cannot read coordinates type!" << " for file: " 
                  << this->Filename);
    return 0;
    }

  if ( ! strncmp(this->LowerCase(line), "int", 3) )
    {
    vtkIntScalars *intCoords = vtkIntScalars::New();
    coords = (vtkScalars *)intCoords;
    int *ptr = intCoords->WritePointer(0,numCoords);

    if (this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(int)*numCoords);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary coordinates!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,numCoords);
      }
    else // ascii
      {
      int p[1];
      for (i=0; i<numCoords; i++)
        {
        if ( ! this->ReadInt(p) )
          {
          vtkErrorMacro(<<"Error reading coordinates!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        coords->SetScalar(i,p[0]);
        }
      }
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vtkFloatScalars *floatCoords = vtkFloatScalars::New();
    coords = (vtkScalars *)floatCoords;
    float *ptr = floatCoords->WritePointer(0,numCoords);

    if (this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(float)*numCoords);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary coordinates!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,numCoords);
      }
    else // ascii
      {
      float p[1];
      for (i=0; i<numCoords; i++)
        {
        if ( ! this->ReadFloat(p) )
          {
          vtkErrorMacro(<<"Error reading coordinates!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        coords->SetScalar(i,p[0]);
        }
      }
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported coordinates type: " << line 
                  << " for file: " << this->Filename);
    return 0;
    }

  if ( axes == 0 ) rg->SetXCoordinates(coords);
  else if ( axes == 1 ) rg->SetYCoordinates(coords);
  else rg->SetZCoordinates(coords);

  coords->Delete();

  vtkDebugMacro(<<"Read " << coords->GetNumberOfScalars() << " coordinates");
  return 1;
}

// Description:
// Read scalar point attributes. Return 0 if error.
int vtkDataReader::ReadScalarData(vtkDataSet *ds, int numPts)
{
  char line[256], name[256], key[256], tableName[256];
  int i, skipScalar=0;

  if (!(this->ReadString(name) && this->ReadString(line) && 
	this->ReadString(key) && this->ReadString(tableName)))
    {
    vtkErrorMacro(<<"Cannot read scalar header!" << " for file: " 
                  << this->Filename);
    return 0;
    }

  if (strcmp(this->LowerCase(key), "lookup_table"))
    {
    vtkErrorMacro(<<"Lookup table must be specified with scalar.\n" <<
    "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
    }
  //
  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetScalars() != NULL || 
  (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    skipScalar = 1;
  else
    this->SetScalarLut(tableName); //may be "default"


  if (!strncmp(this->LowerCase(line), "bit", 3))
    {
    vtkBitScalars *scalars = vtkBitScalars::New();
    unsigned char *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*(numPts+7)/8);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary bit scalars!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      }
    else // ascii
      {
      int iv;
      for (i=0; i<numPts; i++)
        {
        if (!this->ReadInt(&iv))
          {
          vtkErrorMacro(<<"Error reading bit scalars!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        scalars->SetScalar(i,iv);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if (!strncmp(line, "unsigned_char", 13))
    {
    vtkUnsignedCharScalars *scalars = vtkUnsignedCharScalars::New();
    unsigned char *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary char scalars!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      }
    else // ascii
      {
      unsigned char c;
      for (i=0; i<numPts; i++)
        {
        if (!this->ReadUChar(&c))
          {
          vtkErrorMacro(<<"Error reading char scalars!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        scalars->SetScalar(i,c);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "short", 5) )
    {
    vtkShortScalars *scalars = vtkShortScalars::New();
    short *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(short)*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary short scalars!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap2BERange(ptr,numPts);
      }
    else // ascii
      {
      short s;
      for (i=0; i<numPts; i++)
        {
        if (!this->ReadShort(&s))
          {
          vtkErrorMacro(<<"Error reading short scalars!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        scalars->SetScalar(i,s);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "unsigned_short", 14) )
    {
    vtkUnsignedShortScalars *scalars = vtkUnsignedShortScalars::New();
    unsigned short *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned short)*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary unsigned short scalars!" 
                      << " for file: " << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap2BERange((short *)ptr,numPts);
      }
    else // ascii
      {
      unsigned short s;
      for (i=0; i<numPts; i++)
        {
        if (!this->ReadUnsignedShort(&s))
          {
          vtkErrorMacro(<<"Error reading unsigned short scalars!" 
                        << " for file: " << this->Filename);
          return 0;
          }
        scalars->SetScalar(i,s);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "int", 3) )
    {
    vtkIntScalars *scalars = vtkIntScalars::New();
    int *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(int)*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary int scalars!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,numPts);
      }
    else // ascii
      {
      int iv;
      for (i=0; i<numPts; i++)
        {
        if (!this->ReadInt(&iv))
          {
          vtkErrorMacro(<<"Error reading int scalars!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        scalars->SetScalar(i,iv);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vtkFloatScalars *scalars = vtkFloatScalars::New();
    float *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(float)*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary float scalars!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,numPts);
      }
    else // ascii
      {
      float f;
      for (i=0; i<numPts; i++)
        {
        if (!this->ReadFloat(&f))
          {
          vtkErrorMacro(<<"Error reading float scalars!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        scalars->SetScalar(i,f);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported scalar data type: " << line << " for file: " 
                  << this->Filename);
    return 0;
    }

  return 1;
}

// Description:
// Read vector point attributes. Return 0 if error.
int vtkDataReader::ReadVectorData(vtkDataSet *ds, int numPts)
{
  int i, skipVector=0;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read vector data!" << " for file: " << this->Filename);
    return 0;
    }

  //
  // See whether vector has been already read or vector name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetVectors() != NULL || 
  (this->VectorsName && strcmp(name,this->VectorsName)) )
    {
    skipVector = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vtkFloatVectors *vectors = vtkFloatVectors::New();
    float *ptr = vectors->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(float)*3*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary vectors!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,3*numPts);
      }
    else // ascii
      {
      float v[3];
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(v) && this->ReadFloat(v+1) && 
	      this->ReadFloat(v+2)))
          {
          vtkErrorMacro(<<"Error reading vectors!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        vectors->SetVector(i,v);
        }
      }
    if ( ! skipVector ) ds->GetPointData()->SetVectors(vectors);
    vectors->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported vector type: " << line << " for file: " 
                  << this->Filename);
    return 0;
    }

  return 1;
}

// Description:
// Read normal point attributes. Return 0 if error.
int vtkDataReader::ReadNormalData(vtkDataSet *ds, int numPts)
{
  int i, skipNormal=0;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read normal data!" << " for file: " << this->Filename);
    return 0;
    }
  //
  // See whether normal has been already read or normal name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetNormals() != NULL || 
  (this->NormalsName && strcmp(name,this->NormalsName)) )
    {
    skipNormal = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vtkFloatNormals *normals = vtkFloatNormals::New();
    float *ptr = normals->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(float)*3*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary normals!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,3*numPts);
      }
    else // ascii
      {
      float n[3];
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(n) && this->ReadFloat(n+1) && 
	      this->ReadFloat(n+2)))
          {
          vtkErrorMacro(<<"Error reading normals!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        normals->SetNormal(i,n);
        }
      }
    if ( ! skipNormal ) ds->GetPointData()->SetNormals(normals);
    normals->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported normals type: " << line << " for file: " 
                  << this->Filename);
    return 0;
    }

  return 1;
}

// Description:
// Read tensor point attributes. Return 0 if error.
int vtkDataReader::ReadTensorData(vtkDataSet *ds, int numPts)
{
  int i, skipTensor=0;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read tensor data!" << " for file: " << this->Filename);
    return 0;
    }
  //
  // See whether tensor has been already read or tensor name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetTensors() != NULL || 
  (this->TensorsName && strcmp(name,this->TensorsName)) )
    {
    skipTensor = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vtkFloatTensors *tensors = vtkFloatTensors::New();
    tensors->SetDimension(3);
    float *ptr = tensors->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(float)*9*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary tensors!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,3*numPts);
      }
    else // ascii
      {
      vtkTensor tensor;
      float t[9];
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(t) && this->ReadFloat(t+1) && 
	      this->ReadFloat(t+2) && this->ReadFloat(t+3) && 
	      this->ReadFloat(t+4) && this->ReadFloat(t+5) && 
	      this->ReadFloat(t+6) && this->ReadFloat(t+7) && 
	      this->ReadFloat(t+8)))
	  {
          vtkErrorMacro(<<"Error reading tensors!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        tensor = t;
        tensors->SetTensor(i,&tensor);
        }
      }
    if ( ! skipTensor ) ds->GetPointData()->SetTensors(tensors);
    tensors->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported tensors type: " << line << " for file: " 
                  << this->Filename);
    return 0;
    }

  return 1;
}

// Description:
// Read color scalar point attributes. Return 0 if error.
int vtkDataReader::ReadCoScalarData(vtkDataSet *ds, int numPts)
{
  int i, nValues, skipScalar=0;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->ReadInt(&nValues)))
    {
    vtkErrorMacro(<<"Cannot read color scalar data!" << " for file: " 
                  << this->Filename);
    return 0;
    }
  //
  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetScalars() != NULL || 
  (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    {
    skipScalar = 1;
    }

  if ( nValues == 1 )
    {
    vtkGraymap *scalars = vtkGraymap::New();
    unsigned char *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary graymap!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      }
    else // ascii
      {
      float f;
      unsigned char g;
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(&f)))
          {
          vtkErrorMacro(<<"Error reading graymap!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        g = (unsigned char)((float)f*255.0);
        scalars->SetGrayValue(i,g);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( nValues == 2 )
    {
    vtkAGraymap *scalars = vtkAGraymap::New();
    unsigned char *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*2*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary alpha-graymap!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      }
    else // ascii
      {
      float f[2];
      unsigned char ga[2];
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(f) && this->ReadFloat(f+1)))
          {
          vtkErrorMacro(<<"Error reading alpha-graymap!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        ga[0] = (unsigned char)((float)f[0]*255.0);
        ga[1] = (unsigned char)((float)f[1]*255.0);
        scalars->SetAGrayValue(i,ga);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( nValues == 3 )
    {
    vtkPixmap *scalars = vtkPixmap::New();
    unsigned char *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*3*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary pixmap!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      }
    else // ascii
      {
      float f[3];
      unsigned char rgba[4];
      rgba[3] = 0;
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(f) && this->ReadFloat(f+1) && 
	      this->ReadFloat(f+2)))
          {
          vtkErrorMacro(<<"Error reading pixmap!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[1] = (unsigned char)((float)f[1]*255.0);
        rgba[2] = (unsigned char)((float)f[2]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( nValues == 4 )
    {
    vtkAPixmap *scalars = vtkAPixmap::New();
    unsigned char *ptr = scalars->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*4*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary alpha-pixmap!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      }
    else // ascii
      {
      float f[4];
      unsigned char rgba[4];
      for (i=0; i<numPts; i++)
        {
        if (!(this->ReadFloat(f) && this->ReadFloat(f+1) &&
	      this->ReadFloat(f+2) && this->ReadFloat(f+3)))
          {
          vtkErrorMacro(<<"Error reading alpha-pixmap!" << " for file: " 
                        << this->Filename);
          return 0;
          }
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[1] = (unsigned char)((float)f[1]*255.0);
        rgba[2] = (unsigned char)((float)f[2]*255.0);
        rgba[3] = (unsigned char)((float)f[3]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else
    {
    vtkErrorMacro(<< "Unsupported number values per scalar: " << nValues 
                  << " for file: " << this->Filename);
    return 0;
    }

  return 1;
}

// Description:
// Read texture coordinates point attributes. Return 0 if error.
int vtkDataReader::ReadTCoordsData(vtkDataSet *ds, int numPts)
{
  int i, dim;
  int skipTCoord = 0;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->ReadInt(&dim) && 
	this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read texture data!" << " for file: " << this->Filename);
    return 0;
    }

  if ( dim < 1 || dim > 3 )
    {
    vtkErrorMacro(<< "Unsupported texture coordinates dimension: " << dim 
                  << " for file: " << this->Filename);
    return 0;
    }

  //
  // See whether texture coords have been already read or texture coords name
  // (if specified) matches name in file. 
  //
  if ( ds->GetPointData()->GetTCoords() != NULL || 
  (this->TCoordsName && strcmp(name,this->TCoordsName)) )
    {
    skipTCoord = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vtkFloatTCoords *tcoords = vtkFloatTCoords::New();
    tcoords->SetDimension(dim);
    float *ptr = tcoords->WritePointer(0,numPts);
    if ( this->FileType == VTK_BINARY)
      {
      // suck up newline
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(float)*dim*numPts);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary tensors!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      vtkByteSwap::Swap4BERange(ptr,dim*numPts);
      }
    else // ascii
      {
      float tc[3];
      int j;
      for (i=0; i<numPts; i++)
        {
        for (j=0; j<dim; j++)
          {
          if (!this->ReadFloat(tc+j))
            {
            vtkErrorMacro(<<"Error reading texture coordinates!" << " for file: " 
                          << this->Filename);
            return 0;
            }
          }
        tcoords->SetTCoord(i,tc);
        }
      }
    if ( ! skipTCoord ) ds->GetPointData()->SetTCoords(tcoords);
    tcoords->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported texture coordinates data type: " << line 
                  << " for file: " << this->Filename);
    return 0;
  }

  return 1;
}

// Description:
// Read lookup table. Return 0 if error.
int vtkDataReader::ReadLutData(vtkDataSet *ds)
{
  int i, size, skipTable=0;
  vtkLookupTable *lut;
  unsigned char *ptr;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->ReadInt(&size)))
    {
    vtkErrorMacro(<<"Cannot read lookup table data!" << " for file: " 
                  << this->Filename);
    return 0;
    }

  if ( ds->GetPointData()->GetScalars() == NULL ||
  (this->LookupTableName && strcmp(name,this->LookupTableName)) ||
  (this->ScalarLut && strcmp(name,this->ScalarLut)) )
    {
    skipTable = 1;
    }

  lut = vtkLookupTable::New();
  lut->Allocate(size);
  ptr = lut->WritePointer(0,size);

  if ( this->FileType == VTK_BINARY)
    {
    // suck up newline
    this->IS->getline(line,256);
    this->IS->read((char *)ptr,sizeof(unsigned char)*4*size);
    if (this->IS->eof())
      {
      vtkErrorMacro(<<"Error reading binary lookup table!" << " for file: " 
                    << this->Filename);
      return 0;
      }
    }
  else // ascii
    {
    float rgba[4];
    for (i=0; i<size; i++)
      {
      if (!(this->ReadFloat(rgba) && this->ReadFloat(rgba+1) &&
	    this->ReadFloat(rgba+2) && this->ReadFloat(rgba+3)))
        {
        vtkErrorMacro(<<"Error reading lookup table!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      lut->SetTableValue(i, rgba[0], rgba[1], rgba[2], rgba[3]);
      }
    }

  if ( ! skipTable ) ds->GetPointData()->GetScalars()->SetLookupTable(lut);
  lut->Delete();

  return 1;
}


// Description:
// Read lookup table. Return 0 if error.
int vtkDataReader::ReadCells(int size, int *data)
{
  char line[256];
  int i;

  if ( this->FileType == VTK_BINARY)
    {
    // suck up newline
    this->IS->getline(line,256);
    this->IS->read((char *)data,sizeof(int)*size);
    if (this->IS->eof())
      {
      vtkErrorMacro(<<"Error reading binary cell data!" << " for file: " 
                    << this->Filename);
      return 0;
      }
    vtkByteSwap::Swap4BERange(data,size);
    }
  else // ascii
    {
    for (i=0; i<size; i++)
      {
      if (!this->ReadInt(data+i))
        {
        vtkErrorMacro(<<"Error reading ascii cell data!" << " for file: " 
                      << this->Filename);
        return 0;
        }
      }
    }

  return 1;
}

char *vtkDataReader::LowerCase(char *str)
{
  int i;
  char *s;

  for ( i=0, s=str; *s != '\0' && i<256; s++,i++) 
    *s = tolower(*s);
  return str;
}

// Description:
// Close a vtk file.
void vtkDataReader::CloseVTKFile()
{
  vtkDebugMacro(<<"Closing vtk file");
  if ( this->IS != NULL ) delete this->IS;
  this->IS = NULL;
}

void vtkDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";

  if ( this->FileType == VTK_BINARY )
    os << indent << "File Type: BINARY\n";
  else
    os << indent << "File Type: ASCII\n";

  if ( this->ScalarsName )
    os << indent << "Scalars Name: " << this->ScalarsName << "\n";
  else
    os << indent << "Scalars Name: (None)\n";

  if ( this->VectorsName )
    os << indent << "Vectors Name: " << this->VectorsName << "\n";
  else
    os << indent << "Vectors Name: (None)\n";

  if ( this->NormalsName )
    os << indent << "Normals Name: " << this->NormalsName << "\n";
  else
    os << indent << "Normals Name: (None)\n";

  if ( this->TensorsName )
    os << indent << "Tensors Name: " << this->TensorsName << "\n";
  else
    os << indent << "Tensors Name: (None)\n";

  if ( this->TCoordsName )
    os << indent << "Texture Coords Name: " << this->TCoordsName << "\n";
  else
    os << indent << "Texture Coordinates Name: (None)\n";

  if ( this->LookupTableName )
    os << indent << "Lookup Table Name: " << this->LookupTableName << "\n";
  else
    os << indent << "Lookup Table Name: (None)\n";

}
