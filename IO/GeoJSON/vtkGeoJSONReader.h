#ifndef VTKGEOJSONREADER_H
#define VTKGEOJSONREADER_H

// Local includes
#include "vtkGeoJSONFeature.h"

// VTK Includes
#include <vtkPolyDataAlgorithm.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStdString.h>
#include <vtk_jsoncpp.h>

// C++ includes
#include <fstream>
#include <iostream>

class vtkGeoJSONReader: public vtkPolyDataAlgorithm
{
public:
    vtkGeoJSONReader();

    // Description:
    // Set filename for the geoJSON data source
    void SetFileName(const char *fileName);

    // Description:
    // Parse the data and update the output data according to the
    // geoJSON data in the source file
    void Update();

    // Description:
    // Get the outputData generated after Update() i.e.
    // outputData containing vtkPolyData corresponding to the data in geoJSON fileName
    vtkPolyData* GetOutput();

protected:
    ifstream file;
    vtkStdString FileName;
    vtkPolyData* OutputData;

    //Parse the Json Value corresponding to the root of the geoJSON data from the file
    void ParseRoot(Json::Value root);

    //Verify if file exists and can be read by the parser
    bool CanReadFile(const char *filename);

    void initialiseOutputData();

    //Destructor for the reader. To Do.
    ~vtkGeoJSONReader();

};

#endif // VTKGEOJSONREADER_H
