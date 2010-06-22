#include "vtkThresholdGraph.h"

#include "vtkDoubleArray.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkThresholdGraph);

//-----------------------------------------------------------------------------
vtkThresholdGraph::vtkThresholdGraph() : vtkGraphAlgorithm(),
  LowerThreshold(0.0),
  UpperThreshold(0.0)
{
}


//-----------------------------------------------------------------------------
vtkThresholdGraph::~vtkThresholdGraph()
{
  // Do nothing.
}

//-----------------------------------------------------------------------------
void  vtkThresholdGraph::PrintSelf(ostream &os, vtkIndent indent)
{
  // Base class print.
  vtkGraphAlgorithm::PrintSelf(os, indent);

  os << indent << "LowerThreshold: "  << this->LowerThreshold << endl;
  os << indent << "UpperThreshold: "  << this->UpperThreshold << endl;
}

//-----------------------------------------------------------------------------
int vtkThresholdGraph::RequestData(vtkInformation* vtkNotUsed(request),
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{
  if(!inputVector[0])
    {
    vtkErrorMacro("Error: NULL or invalid input vtkInformationVector.");
    return 1;
    }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if(!inInfo)
    {
    vtkErrorMacro("Error: NULL or invalid input vtkInformation.");
    return 1;
    }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!inDataObj)
    {
    vtkErrorMacro("Error: NULL or invalid input data object.");
    return 1;
    }

  if(!outputVector)
    {
    vtkErrorMacro("Error: NULL or invalid output vtkInformationVector.");
    return 1;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if(!outInfo)
    {
    vtkErrorMacro("Error: NULL of invalid output vtkInformation.")
    }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!outDataObj)
    {
    vtkErrorMacro("Error: NULL or invalid output data object.");
    return 1;
    }

  vtkSmartPointer<vtkExtractSelectedGraph> extractThreshold (
    vtkSmartPointer<vtkExtractSelectedGraph>::New());
  vtkSmartPointer<vtkSelection> threshold (vtkSmartPointer<vtkSelection>::New());
  vtkSmartPointer<vtkSelectionNode> thresholdNode (
    vtkSmartPointer<vtkSelectionNode>::New());
  vtkSmartPointer<vtkDoubleArray> thresholdArr (
    vtkSmartPointer<vtkDoubleArray>::New());

  vtkInformationVector* inArrayVec =
    this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());

  if (!inArrayVec)
    {
    vtkErrorMacro("Problem finding array to process");
    return 1;
    }
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(0);
  if (!inArrayInfo)
    {
    vtkErrorMacro("Problem getting name of array to process.");
    return 1;
    }
  if ( ! inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
    {
    vtkErrorMacro("Missing field name.");
    return 1;
    }
  const char* arrayNameToProcess =
    inArrayInfo->Get(vtkDataObject::FIELD_NAME());

  if(!arrayNameToProcess)
    {
    vtkErrorMacro("Unable to find valid array.");
    return 1;
    }

  int fieldAssociation(-1);
  if(!inArrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
    {
    vtkErrorMacro("Unable to query field association for the scalar.");
    return 1;
    }
  fieldAssociation = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

  thresholdArr->SetName(arrayNameToProcess);
  thresholdArr->InsertNextValue(this->LowerThreshold);
  thresholdArr->InsertNextValue(this->UpperThreshold);

  thresholdNode->SetSelectionList(thresholdArr);
  thresholdNode->SetContentType(vtkSelectionNode::THRESHOLDS);

  if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_EDGES)
    {
    thresholdNode->SetFieldType(vtkSelectionNode::EDGE);
    }
  else if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
    {
    thresholdNode->SetFieldType(vtkSelectionNode::VERTEX);
    }
  else
    {
    vtkErrorMacro("Array selected should be associated with vertex or " <<
                  "edge data.");
    return 1;
    }

  threshold->AddNode(thresholdNode);

  vtkSmartPointer<vtkDataObject> inputClone;
  inputClone.TakeReference(inDataObj->NewInstance());
  inputClone->ShallowCopy(inDataObj);

  extractThreshold->SetInput(0, inputClone);
  extractThreshold->SetInput(1, threshold);

  extractThreshold->Update();

  vtkDataObject* output = extractThreshold->GetOutputDataObject(0);

  if(!output)
    {
    vtkErrorMacro("NULL or invalid output.");
    return 1;
    }

  outDataObj->ShallowCopy(output);

  return 1;
}
