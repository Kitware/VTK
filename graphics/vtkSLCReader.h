
// .NAME vtkSLCReader - read an SLC volume file.
// .SECTION Description
// vtkSLCReader reads an SLC file and creates a structured point dataset.
// The size of the volume and the aspect ratio is set from the SLC file
// header.

#ifndef __vtkSLCReader_h
#define __vtkSLCReader_h

#include <stdio.h>
#include "vtkStructuredPointsSource.h"
#include "vtkBitScalars.h"

class vtkSLCReader : public vtkStructuredPointsSource 
{
public:
  vtkSLCReader();
  ~vtkSLCReader() {if (this->Filename) delete [] this->Filename;};
  char *GetClassName() {return "vtkSLCReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the name of the file to read.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

protected:
  // Description:
  // Stores the filename of the SLC file to read.
  char *Filename;

  // Description:
  // Reads the filename and builds a vtkStructuredPoints dataset
  void Execute();
  
  // Description:
  // Decodes an array of eight bit run-length encoded data.
  unsigned char *Decode_8bit_data( unsigned char *in_ptr, int size );
};

#endif


