#ifndef __vtkGeoJSONReader_h
#define __vtkGeoJSONReader_h

#include "vtkGeoJSONFeature.h"

// VTK Includes
#include <vtkPolyDataAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStdString.h>
#include <vtk_jsoncpp.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

// C++ includes
#include <fstream>
#include <iostream>

class vtkGeoJSONReader: public vtkPolyDataAlgorithm
{
public:
  static vtkGeoJSONReader* New();
  vtkTypeMacro(vtkGeoJSONReader,vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Decription:
  // Accessor for name of the file that will be opened on WriteData
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkGeoJSONReader();
  virtual ~vtkGeoJSONReader();

  // Decription:
  // Core implementation of the
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector);

  // Decription:
  // Parse the Json Value corresponding to the root of the geoJSON data from the file
  void ParseRoot(Json::Value root, vtkPolyData *output);

  // Decription:
  // Verify if file exists and can be read by the parser
  // If exists, parse into Jsoncpp data structure
  int CanParse(const char *filename, Json::Value &root);

  char *FileName;

private:
  vtkGeoJSONReader(const vtkGeoJSONReader&);  // Not implemented
  void operator=(const vtkGeoJSONReader&);    // Not implemented
};

#endif // __vtkGeoJSONReader_h
