#include "vtkGeoJSONReader.h"

//----------------------------------------------------------------------------
vtkGeoJSONReader::vtkGeoJSONReader()
{
  this->OutputData = NULL;
}

//----------------------------------------------------------------------------
bool vtkGeoJSONReader::CanReadFile(const char *filename)
{
  // Check if file can be opened for reading
  file.open(filename);

  if (!file.is_open())
    {
    std::cout << "File Not Opened!";
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::SetFileName(const char* fileName)
{
  // Set filename for the source of the geoJSON data
  this->FileName = vtkStdString(fileName);
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::Update()
{
  // Read data from the geoJSON file and update outputData with appropriate
  // vtkPolyData generated according to the data in the source file

  if(!this->CanReadFile(this->FileName))
    {
    vtkErrorMacro(<< "Unable to Open File in parseFile " << this->FileName);
    }

  Json::Value root;
  Json::Reader reader;

  //parse the entire geoJSON data into the Json::Value root
  bool parsedSuccess = reader.parse(file, root, false);

  if (!parsedSuccess)
    {
    // Report failures and their locations in the document
    vtkErrorMacro(<<"Failed to parse JSON" << endl <<
                  reader.getFormatedErrorMessages());
    return;
    }

  // If parsed successfully into Json parser Values and Arrays, then convert it
  // into appropriate vtkPolyData
  if (root.isObject())
    {
    ParseRoot(root);
    }
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::ParseRoot(Json::Value root)
{
  Json::Value rootType = root.get("type", -1);
  Json::Value rootFeatures = root.get("features", -1);

  if(rootFeatures == -1)
    {
    std::cout << "Parse Root :: Features :: -1" << std::endl;
    return;
    }

  //Initialising polyData to which data will be appended
  initialiseOutputData();

  if(rootFeatures.isArray())
    {
    // If it is a collection of features
    for(int i = 0; i < rootFeatures.size(); i++)
      {
      //Append extracted geometry to existing outputData
      Json::Value child = rootFeatures[i];
      vtkGeoJSONFeature *feature = new vtkGeoJSONFeature();
      feature->extractGeoJSONFeature(child, this->OutputData);
      }
    }
  else
    {
    // Single feature in the geoJSON data
    vtkGeoJSONFeature *feature = new vtkGeoJSONFeature();
    feature->extractGeoJSONFeature(rootFeatures, this->OutputData);
    }
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::initialiseOutputData()
{
  if (this->OutputData != NULL)
    {
    // Cleaning previous data
    this->OutputData->Delete();
    }

  this->OutputData = vtkPolyData::New();

  this->OutputData->SetPoints(vtkPoints::New());//Initialising containers for points,
  this->OutputData->SetVerts(vtkCellArray::New());//Vertices,
  this->OutputData->SetLines(vtkCellArray::New());//Lines and
  this->OutputData->SetPolys(vtkCellArray::New());//Polygons
}

//----------------------------------------------------------------------------
vtkGeoJSONReader::~vtkGeoJSONReader()
{
  // ToDo.
}

//----------------------------------------------------------------------------
vtkPolyData* vtkGeoJSONReader::GetOutput()
{
  // Return the outputData generated after parsing the data from the
  // geoJSON source file
  return this->OutputData;
}
