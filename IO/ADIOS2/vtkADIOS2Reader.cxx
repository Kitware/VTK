/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * vtkADIOS2ReaderDriver.cxx
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "vtkADIOS2Reader.h"

#include <iostream>
#include <memory>

#include "ADIOS2Helper.h"
#include "ADIOS2Schema.h"
#include "xml_vtk/ADIOS2xmlVTI.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtk_pugixml.h>
#include <vtksys/SystemTools.hxx>

#include <adios2.h>

// Impl
class vtkADIOS2Reader::Impl
{
public:
  double m_Time = 0.;
  size_t m_Step = 0;
  std::unique_ptr<adios2vtk::ADIOS2Schema> m_Reader;

  Impl();
  ~Impl() = default;

  void Update(const std::string& streamName, const size_t step = 0,
    const std::string& schemaName = "vtk.xml");
  void Fill(vtkMultiBlockDataSet* multiblock, const size_t step = 0);

private:
  std::string m_StreamName;
  std::unique_ptr<adios2::ADIOS> m_ADIOS;
  adios2::IO m_IO;
  adios2::Engine m_Engine;

  std::string m_SchemaName;

  static const std::set<std::string> m_SupportedTypes;

  void InitReader();

  // we can extend this to add more schemas
  bool InitReaderXMLVTK();
};

vtkStandardNewMacro(vtkADIOS2Reader);

vtkADIOS2Reader::vtkADIOS2Reader()
  : FileName(nullptr)
  , m_Impl(new Impl)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

int vtkADIOS2Reader::RequestInformation(vtkInformation* vtkNotUsed(inputVector),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  m_Impl->Update(FileName); // check if FileName changed

  std::vector<double> vTimes;
  vTimes.reserve(m_Impl->m_Reader->m_Times.size());
  for (const auto& timePair : m_Impl->m_Reader->m_Times)
  {
    vTimes.push_back(timePair.first);
  }

  // set time info
  std::cout << "Setting time info\n";
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(), vTimes.data(), static_cast<int>(vTimes.size()));

  const std::vector<double> timeRange = { vTimes.front(), vTimes.back() };
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange.data(),
    static_cast<int>(timeRange.size()));

  return 1;
}

int vtkADIOS2Reader::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  const double newTime = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  std::cout << "Requested New Time: " << newTime << " Old Step: " << m_Impl->m_Time << "\n";
  m_Impl->m_Step = m_Impl->m_Reader->m_Times[newTime];
  m_Impl->m_Time = newTime;
  return 1;
}

int vtkADIOS2Reader::RequestData(vtkInformation* vtkNotUsed(inputVector),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(output);

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), m_Impl->m_Time);
  m_Impl->Fill(multiBlock, m_Impl->m_Step);
  return 1;
}

void vtkADIOS2Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

// Impl Starts
vtkADIOS2Reader::Impl::Impl() {}

void vtkADIOS2Reader::Impl::Update(
  const std::string& streamName, const size_t step, const std::string& schemaName)
{
  if (!m_ADIOS)
  {
    std::cout << "Initializing ADIOS\n";
    m_ADIOS.reset(new adios2::ADIOS(adios2vtk::helper::MPIGetComm()));
  }

  if (!m_IO && !m_Engine)
  {
    m_StreamName = streamName;
    if (m_ADIOS)
    {
      std::cout << "Opening " << m_StreamName << "\n";
      m_IO = m_ADIOS->DeclareIO(m_StreamName);
      std::cout << "DeclaredIO\n";

      m_Engine = m_IO.Open(m_StreamName, adios2::Mode::Read);
      std::cout << "Opened Engine\n";
      m_SchemaName = schemaName;
    }
    InitReader();
  }
  else
  {
    // TODO: check if variables changed
  }
}

void vtkADIOS2Reader::Impl::Fill(vtkMultiBlockDataSet* multiBlock, const size_t step)
{
  m_Reader->Fill(multiBlock, step);
}

const std::set<std::string> vtkADIOS2Reader::Impl::m_SupportedTypes = { "ImageData",
  "StructuredGrid", "UnstructuredGrid" };

void vtkADIOS2Reader::Impl::InitReader()
{
  if (InitReaderXMLVTK())
  {
    return;
  }
  // else if( InitReaderOther() ) {}
  // here we can make it extensible by trying to find other schema types for
  // now we stick with VTK XML schemas
}

bool vtkADIOS2Reader::Impl::InitReaderXMLVTK()
{
  pugi::xml_document xmlDocument;
  std::string xmlContents;

  bool isSchemaFile = false;

  // auto lf_GetXMLDoc = [&]() {
  // check if it's file, not optimizing with MPI_Bcast
  std::string xmlFileName;
  if (vtksys::SystemTools::FileIsDirectory(m_Engine.Name()))
  {
    xmlFileName = m_Engine.Name() + "/" + m_SchemaName;
  }
  else if (vtksys::SystemTools::FileIsDirectory(m_Engine.Name() + ".dir"))
  {
    xmlFileName = m_Engine.Name() + ".dir/" + m_SchemaName;
  }

  if (!xmlFileName.empty())
  {
    if (vtksys::SystemTools::FileExists(xmlFileName))
    {
      xmlContents = adios2vtk::helper::FileToString(xmlFileName);
      xmlDocument =
        adios2vtk::helper::XMLDocument(xmlContents, true, "when reading " + m_SchemaName + " file");
      isSchemaFile = true;
    }
  }

  if (!isSchemaFile)
  {
    const adios2::Attribute<std::string> vtkXMLAttribute =
      m_IO.InquireAttribute<std::string>(m_SchemaName);
    const std::vector<std::string> vtkAttributes = vtkXMLAttribute.Data();
    if (vtkAttributes.empty())
    {
      throw std::runtime_error("ERROR: neither " + m_SchemaName +
        " file or bp attribute was found in " + m_Engine.Name() + "\n");
    }

    xmlContents = vtkAttributes.front();
    xmlDocument = adios2vtk::helper::XMLDocument(
      xmlContents, true, "when reading " + m_SchemaName + " attribute");
  }

  // auto lf_InitReader = [&]() {
  constexpr bool isDebug = true;
  constexpr bool isMandatory = true;
  constexpr bool isUnique = true;

  const pugi::xml_node vtkXMLFileNode = adios2vtk::helper::XMLNode("VTKFile", xmlDocument, isDebug,
    "when reading VTKFile node in " + m_Engine.Name(), isMandatory, isUnique);

  const pugi::xml_attribute typeXML = adios2vtk::helper::XMLAttribute("type", vtkXMLFileNode, true,
    "when reading type xml attribute in vtk.xml " + m_Engine.Name(), isMandatory);

  const std::string type = std::string(typeXML.value());

  if (m_SupportedTypes.count(type) == 0)
  {
    throw std::runtime_error(
      "ERROR: ADIOS2Reader only supports types= " + adios2vtk::helper::SetToCSV(m_SupportedTypes) +
      " when reading type xml attribute in " + m_SchemaName + " from " + m_Engine.Name() + "\n");
  }

  if (type == "ImageData")
  {
    m_Reader.reset(new adios2vtk::xml::ADIOS2xmlVTI(xmlContents, &m_IO, &m_Engine));
  }

  const bool success = m_Reader ? true : false;
  return success;
}
