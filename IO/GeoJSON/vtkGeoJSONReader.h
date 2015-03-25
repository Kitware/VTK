#ifndef __vtkGeoJSONReader_h
#define __vtkGeoJSONReader_h

// Local includes
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
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set filename for the geoJSON data source
  void SetFileName(const char *fileName);

protected:
  vtkGeoJSONReader();
  virtual ~vtkGeoJSONReader();

  int RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector);

  ifstream file;
  vtkStdString FileName;

  //Parse the Json Value corresponding to the root of the geoJSON data from the file
  void ParseRoot(Json::Value root, vtkPolyData *output);

  //Verify if file exists and can be read by the parser
  bool CanReadFile(const char *filename);

  void initialiseOutputData(vtkPolyData *output);

private:
  vtkGeoJSONReader(const vtkGeoJSONReader&);  //Not implemented
  void operator=(const vtkGeoJSONReader&);    //Not implemented
};

#endif // __vtkGeoJSONReader_h
