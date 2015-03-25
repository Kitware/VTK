#include "vtkGeoJSONReader.h"

vtkStandardNewMacro(vtkGeoJSONReader);

//----------------------------------------------------------------------------
vtkGeoJSONReader::vtkGeoJSONReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkGeoJSONReader::~vtkGeoJSONReader()
{
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
int vtkGeoJSONReader::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **vtkNotUsed(request),
                                   vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
 vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));


  // vtkPolyData generated according to the data in the source file
  if(!this->CanReadFile(this->FileName))
    {
    vtkErrorMacro(<< "Unable to Open File " << this->FileName);
    return VTK_ERROR;
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
    return VTK_ERROR;
    }

  // If parsed successfully into Json parser Values and Arrays, then convert it
  // into appropriate vtkPolyData
  if (root.isObject())
    {
    ParseRoot(root, output);
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::ParseRoot(Json::Value root, vtkPolyData *output)
{
  Json::Value rootType = root.get("type", -1);
  Json::Value rootFeatures = root.get("features", -1);

  if(rootFeatures == -1)
    {
    std::cout << "Parse Root :: Features :: -1" << std::endl;
    return;
    }

  //Initialising polyData to which data will be appended
  initialiseOutputData(output);

  if(rootFeatures.isArray())
    {
    // If it is a collection of features
    for(int i = 0; i < rootFeatures.size(); i++)
      {
      //Append extracted geometry to existing outputData
      Json::Value child = rootFeatures[i];
      vtkGeoJSONFeature *feature = new vtkGeoJSONFeature();
      feature->extractGeoJSONFeature(child, output);
      }
    }
  else
    {
    // Single feature in the geoJSON data
    vtkGeoJSONFeature *feature = new vtkGeoJSONFeature();
    feature->extractGeoJSONFeature(rootFeatures, output);
    }
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::initialiseOutputData(vtkPolyData *output)
{
  output->SetPoints(vtkPoints::New());//Initialising containers for points,
  output->SetVerts(vtkCellArray::New());//Vertices,
  output->SetLines(vtkCellArray::New());//Lines and
  output->SetPolys(vtkCellArray::New());//Polygons
}

void vtkGeoJSONReader::PrintSelf(ostream &os, vtkIndent indent)
{

}
