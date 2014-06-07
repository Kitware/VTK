#include "vtkGeoJSONReader.h"

vtkStandardNewMacro(vtkGeoJSONReader);

//----------------------------------------------------------------------------
vtkGeoJSONReader::vtkGeoJSONReader()
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkGeoJSONReader::~vtkGeoJSONReader()
{
  delete[] FileName;
}

//----------------------------------------------------------------------------
int vtkGeoJSONReader::CanParse(const char *filename, Json::Value &root)
{
  ifstream file;
  file.open(filename);

  if (!file.is_open())
    {
    vtkErrorMacro(<< "Unable to Open File " << this->FileName);
    return VTK_ERROR;
    }

  Json::Reader reader;

  //parse the entire geoJSON data into the Json::Value root
  bool parsedSuccess = reader.parse(file, root, false);

  if (!parsedSuccess)
    {
    // Report failures and their locations in the document
    vtkErrorMacro(<<"Failed to parse JSON" << endl << reader.getFormatedErrorMessages());
    return VTK_ERROR;
    }

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkGeoJSONReader::RequestData(vtkInformation* vtkNotUsed(request),
                                   vtkInformationVector** vtkNotUsed(request),
                                   vtkInformationVector* outputVector)
{
  // Get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the ouptut
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  Json::Value root;
  if(CanParse(this->FileName, root) == VTK_ERROR)
    {
    return VTK_ERROR;
    }

  // If parsed successfully into Json parser Values and Arrays, then convert it
  // into appropriate vtkPolyData
  if(root.isObject())
    {
    ParseRoot(root, output);

    //Convert Concave Polygons to convex polygons using triangulation
    vtkTriangleFilter *filter = vtkTriangleFilter::New();
    filter->SetInputData(output);
    filter->Update();

    output->ShallowCopy(filter->GetOutput());
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
    vtkErrorMacro (<<"Parse Root :: Features :: -1");
    return;
    }

  output->SetPoints(vtkPoints::New());//Initialising containers for points,
  output->SetVerts(vtkCellArray::New());//Vertices,
  output->SetLines(vtkCellArray::New());//Lines and
  output->SetPolys(vtkCellArray::New());//Polygons

  if(rootFeatures.isArray())
    {
    // If it is a collection of features
    for(int i = 0; i < rootFeatures.size(); i++)
      {
      // Append extracted geometry to existing outputData
      Json::Value child = rootFeatures[i];
      vtkGeoJSONFeature *feature = vtkGeoJSONFeature::New();
      feature->ExtractGeoJSONFeature(child, output);
      }
    }
  else
    {
    // Single feature in the geoJSON data
    vtkGeoJSONFeature *feature = vtkGeoJSONFeature::New();
    feature->ExtractGeoJSONFeature(rootFeatures, output);
    }
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkGeoJSONReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
