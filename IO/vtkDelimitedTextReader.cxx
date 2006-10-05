/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelimitedTextReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkDelimitedTextReader.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkInformation.h>
#include <vtkStringArray.h>
#include <vtkStdString.h>

#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtkstd/string>
#include "vtkIOStream.h"

vtkCxxRevisionMacro(vtkDelimitedTextReader, "1.1");
vtkStandardNewMacro(vtkDelimitedTextReader);

struct vtkDelimitedTextReaderInternals
{
  ifstream *File;
};

// Forward function reference (definition at bottom :)
static int splitString(const vtkStdString& input, 
       const vtkStdString& delimiter, 
       vtkstd::vector<vtkStdString>& results, 
       bool includeEmpties=true);


vtkDelimitedTextReader::vtkDelimitedTextReader()
{
  this->Internals = new vtkDelimitedTextReaderInternals();

  this->Internals->File = 0;
  this->FileName = 0;
  this->DelimiterString = 0;
  this->SetDelimiterString("\t");
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->ReadBuffer = new char[2048];
}

vtkDelimitedTextReader::~vtkDelimitedTextReader()
{
  this->SetFileName(0);
  this->SetDelimiterString(0);
  delete this->ReadBuffer;
  delete this->Internals;
}

void vtkDelimitedTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " 
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "DelimiterString: " 
     << (this->DelimiterString ? this->DelimiterString : "(none)") << endl;
  os << indent << "HaveHeaders: " 
     << (this->HaveHeaders ? "true" : "false") << endl;
}

void vtkDelimitedTextReader::OpenFile()
{
  // If the file was open close it.
  if (this->Internals->File)
    {
    this->Internals->File->close();
    delete this->Internals->File;
    this->Internals->File = NULL;
    }
  
  // Open the new file.
  vtkDebugMacro(<< "vtkDelimitedTextReader is opening file: " << this->FileName);
  this->Internals->File = new ifstream(this->FileName, ios::in);

  // Check to see if open was successful
  if (! this->Internals->File || this->Internals->File->fail())
    {
    vtkErrorMacro(<< "vtkDelimitedTextReader could not open file " << 
    this->FileName);
    return;
    }
}

int vtkDelimitedTextReader::RequestData(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{

  // Check that the filename has been specified
  if (!this->FileName)
    {
    vtkErrorMacro("A FileName must be specified");
    return 0;
    }
    
  // Open the file
  this->OpenFile();
  
  // Get the headers from the file
  vtkstd::vector<vtkStdString> Headers;
  
  // Go to the top of the file
  this->Internals->File->seekg(0,ios::beg);
  
  // Read the first line of the file and grab headers
  this->Internals->File->getline(this->ReadBuffer,2047);
  
  // Now give buffer to string
  vtkStdString s(this->ReadBuffer);
  
  // Split string on the delimiters
  splitString(s, this->DelimiterString, Headers);

  // Store the text data into a vtkTable
  vtkTable* table = vtkTable::GetData(outputVector);

  // Add arrays for each header (column)
  vtkstd::vector<vtkStdString>::const_iterator I;
  for(I = Headers.begin(); I != Headers.end(); ++I)
    { 
    vtkStringArray* array = vtkStringArray::New();
    array->SetName(I->c_str());
    table->AddColumn(array);
    array->Delete();
    }
  
  // Okay read the file and add the data to the table
  vtkstd::vector<vtkStdString> dataVector;
  while (this->Internals->File->getline(this->ReadBuffer,2047))
    {
    // Now give buffer to string
    vtkStdString s(this->ReadBuffer);
  
    // Split string on the delimiters
    dataVector.resize(0);
    splitString(s, this->DelimiterString, dataVector);
    
    // Add data to the output arrays

    // Convert from vector to variant array
    vtkVariantArray* dataArray = vtkVariantArray::New();
    vtkstd::vector<vtkStdString>::const_iterator I;
    for(I = dataVector.begin(); I != dataVector.end(); ++I)
      {
      dataArray->InsertNextValue(vtkVariant(*I));
      }
    
    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
    }
 
  return 1;
}

static int splitString(const vtkStdString& input, 
       const vtkStdString& delimiter, 
       vtkstd::vector<vtkStdString>& results, 
       bool includeEmpties)
{
    int iPos = 0;
    int newPos = -1;
    int sizeS2 = (int)delimiter.size();
    int isize = (int)input.size();

    if( 
        ( isize == 0 )
        ||
        ( sizeS2 == 0 )
    )
    {
        return 0;
    }

    vtkstd::vector<int> positions;

    newPos = input.find (delimiter, 0);

    if( newPos < 0 )
    { 
        return 0; 
    }

    int numFound = 0;

    while( newPos >= iPos )
    {
        numFound++;
        positions.push_back(newPos);
        iPos = newPos;
        newPos = input.find (delimiter, iPos+sizeS2);
    }

    if( numFound == 0 )
    {
        return 0;
    }

    for( int i=0; i <= (int)positions.size(); ++i )
    {
        vtkStdString s("");
        int offset;
        if( i == 0 ) 
        { 
            s = input.substr( i, positions[i] );
            offset = positions[0] + sizeS2; 
        }
        offset = positions[i-1] + sizeS2;
        if( offset < isize )
        {
            if( i == (int)positions.size() )
            {
                s = input.substr(offset);
            }
            else if( i > 0 )
            {
                s = input.substr( positions[i-1] + sizeS2, 
                      positions[i] - positions[i-1] - sizeS2 );
            }
        }
        if( includeEmpties || ( s.size() > 0 ) )
        {
            results.push_back(s);
        }
    }
    return numFound;
}

