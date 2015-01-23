/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoJSONWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeoJSONWriter - Convert vtkPolyData to Geo JSON format.
// .SECTION Description
// Outputs a Geo JSON (http://www.geojson.org) description of the input
// polydata data set.

#ifndef vtkGeoJSONWriter_h
#define vtkGeoJSONWriter_h

#include "vtkIOGeoJSONModule.h" // For export macro
#include "vtkWriter.h"

class vtkLookupTable;

class VTKIOGEOJSON_EXPORT vtkGeoJSONWriter : public vtkWriter
{
public:
  static vtkGeoJSONWriter* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkGeoJSONWriter,vtkWriter);

  //Decription:
  // Accessor for name of the file that will be opened on WriteData
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Enable writing to an OutputString instead of the default, a file.
  vtkSetMacro(WriteToOutputString,bool);
  vtkGetMacro(WriteToOutputString,bool);
  vtkBooleanMacro(WriteToOutputString,bool);

  // Description:
  // When WriteToOutputString in on, then a string is allocated, written to,
  // and can be retrieved with these methods.  The string is deleted during
  // the next call to write ...
  vtkGetMacro(OutputStringLength, int);
  vtkGetStringMacro(OutputString);
  unsigned char *GetBinaryOutputString()
    {
    return reinterpret_cast<unsigned char *>(this->OutputString);
    }

  // Description:
  // Controls how data attributes are written out.
  // When 0, data attributes are ignored and not written at all.
  // When 1, values are mapped through a lookup table and colors are written to the output.
  // When 2, which is the default, the values are written directly.
  vtkSetMacro(ScalarFormat,int);
  vtkGetMacro(ScalarFormat,int);

  // Description:
  // Controls the lookup table to use when ValueMode is set to map colors;
  void SetLookupTable(vtkLookupTable *lut);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);

  // Description:
  // When WriteToOutputString is on, this method returns a copy of the
  // output string in a vtkStdString.
  vtkStdString GetOutputStdString();

  // Description:
  // This convenience method returns the string, sets the IVAR to NULL,
  // so that the user is responsible for deleting the string.
  // I am not sure what the name should be, so it may change in the future.
  char *RegisterAndGetOutputString();

protected:
  vtkGeoJSONWriter();
  virtual ~vtkGeoJSONWriter();

  // Only accepts vtkPolyData
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // Implementation of Write()
  void WriteData();

  // Helper for Write that writes attributes out
  void WriteScalar(vtkDataArray *da, vtkIdType ptId);
  vtkLookupTable *LookupTable;

  bool WriteToOutputString;
  char *OutputString;
  int OutputStringLength;

  int ScalarFormat;

  // Internal helpers
  ostream *OpenFile();
  void ConditionalComma(vtkIdType, vtkIdType);
  void CloseFile(ostream *);
  class Internals;
  Internals *WriterHelper;
  char* FileName;

private:
  vtkGeoJSONWriter(const vtkGeoJSONWriter&);  // Not implemented.
  void operator=(const vtkGeoJSONWriter&);       // Not implemented.
};

#endif // vtkGeoJSONWriter_h
