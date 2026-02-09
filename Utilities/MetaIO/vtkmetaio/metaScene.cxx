/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#if defined(_MSC_VER)
#  pragma warning(disable : 4786)
#  pragma warning(disable : 4702)
#endif

#include "metaUtils.h"
#include "metaObject.h"
#include "metaScene.h"
#include "metaTube.h"
#include "metaContour.h"
#include "metaDTITube.h"
#include "metaVesselTube.h"
#include "metaEllipse.h"
#include "metaGaussian.h"
#include "metaImage.h"
#include "metaBlob.h"
#include "metaLandmark.h"
#include "metaLine.h"
#include "metaGroup.h"
#include "metaSurface.h"
#include "metaMesh.h"
#include "metaArrow.h"
#include "metaTransform.h"
#include "metaTubeGraph.h"
#include "metaFEMObject.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

// MetaScene Constructors
MetaScene::MetaScene()
{
  META_DEBUG_PRINT( "MetaScene()" );
  MetaScene::Clear();
}


MetaScene::MetaScene(const MetaScene * _scene)
{
  META_DEBUG_PRINT( "MetaScene()" );
  MetaScene::Clear();
  MetaScene::CopyInfo(_scene);
}

MetaScene::MetaScene(unsigned int dim)
  : MetaObject(dim)
{
  META_DEBUG_PRINT( "MetaScene()" );
  MetaScene::Clear();
}


/** Destructor */
MetaScene::~MetaScene()
{
  MetaScene::Clear();
  MetaObject::M_Destroy();
}

void
MetaScene::PrintInfo() const
{
  MetaObject::PrintInfo();
  std::cout << "Number of Objects = " << m_NObjects << '\n';
}

void
MetaScene::CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}


void
MetaScene::NObjects(int nobjects)
{
  m_NObjects = nobjects;
}

int
MetaScene::NObjects() const
{
  return m_NObjects;
}

void
MetaScene::AddObject(MetaObject * object)
{
  m_ObjectList.push_back(object);
}

bool
MetaScene::Read(const char * _headerName)
{
  META_DEBUG_PRINT( "MetaScene: Read" );

  int  i = 0;
  char suf[METAIO_MAX_WORD_SIZE];
  suf[0] = '\0';
  if (MET_GetFileSuffixPtr(_headerName, &i))
  {
    strcpy(suf, &_headerName[i]);
  }

  MetaObject::M_Destroy();

  Clear();

  M_SetupReadFields();

  if (_headerName != nullptr)
  {
    m_FileName = _headerName;
  }

  META_DEBUG_PRINT( "MetaScene: Read: Opening stream" );

  M_PrepareNewReadStream();

  m_ReadStream->open(m_FileName.c_str(), std::ios::binary | std::ios::in);

  if (!m_ReadStream->rdbuf()->is_open())
  {
    std::cout << "MetaScene: Read: Cannot open file" << '\n';
    return false;
  }

  if (!M_Read())
  {
    std::cout << "MetaScene: Read: Cannot parse file" << '\n';
    m_ReadStream->close();
    return false;
  }

  if (_headerName != nullptr)
  {
    m_FileName = _headerName;
  }

  if (m_Event)
  {
    m_Event->StartReading(static_cast<unsigned int>(m_NObjects));
  }

  /** Objects should be added here */
  for (i = 0; i < m_NObjects; i++)
  {
    META_DEBUG_PRINT( MET_ReadType(*m_ReadStream).c_str() );

    if (m_Event)
    {
      m_Event->SetCurrentIteration(static_cast<unsigned int>(i + 1));
    }

    const std::string objectType = MET_ReadType(*m_ReadStream);
    if (objectType.find("Tube") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "tre")))
    {
    const std::string subtype = MET_ReadSubType(*m_ReadStream);
      if (subtype.find("Vessel") != std::string::npos)
      {
        auto * vesseltube = new MetaVesselTube();
        vesseltube->APIVersion(m_APIVersion);
        vesseltube->SetEvent(m_Event);
        vesseltube->ReadStream(m_NDims, m_ReadStream);
        m_ObjectList.push_back(vesseltube);
      }
      else if (subtype.find("DTI") != std::string::npos)
      {
        auto * dtitube = new MetaDTITube();
        dtitube->APIVersion(m_APIVersion);
        dtitube->SetEvent(m_Event);
        dtitube->ReadStream(m_NDims, m_ReadStream);
        m_ObjectList.push_back(dtitube);
      }
      else
      {
        auto * tube = new MetaTube();
        tube->APIVersion(m_APIVersion);
        tube->SetEvent(m_Event);
        tube->ReadStream(m_NDims, m_ReadStream);
        m_ObjectList.push_back(tube);
      }
    }

    else if (objectType.find("Transform") != std::string::npos)
    {
      auto * transform = new MetaTransform();
      transform->APIVersion(m_APIVersion);
      transform->SetEvent(m_Event);
      transform->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(transform);
    }

    else if (objectType.find("TubeGraph") != std::string::npos)
    {
      auto * tubeGraph = new MetaTubeGraph();
      tubeGraph->APIVersion(m_APIVersion);
      tubeGraph->SetEvent(m_Event);
      tubeGraph->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(tubeGraph);
    }

    else if (objectType.find("Ellipse") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "elp")))
    {
      auto * ellipse = new MetaEllipse();
      ellipse->APIVersion(m_APIVersion);
      ellipse->SetEvent(m_Event);
      ellipse->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(ellipse);
    }

    else if (objectType.find("Contour") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "ctr")))
    {
      auto * contour = new MetaContour();
      contour->APIVersion(m_APIVersion);
      contour->SetEvent(m_Event);
      contour->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(contour);
    }

    else if (objectType.find("Arrow") != std::string::npos)
    {
      auto * arrow = new MetaArrow();
      arrow->APIVersion(m_APIVersion);
      arrow->SetEvent(m_Event);
      arrow->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(arrow);
    }

    else if (objectType.find("Gaussian") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "gau")))
    {
      auto * gaussian = new MetaGaussian();
      gaussian->APIVersion(m_APIVersion);
      gaussian->SetEvent(m_Event);
      gaussian->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(gaussian);
    }

    else if (objectType.find("Image") != std::string::npos ||
             ((objectType.empty()) && (!strcmp(suf, "mhd") || !strcmp(suf, "mha"))))
    {
      auto * image = new MetaImage();
      image->APIVersion(m_APIVersion);
      image->SetEvent(m_Event);
      image->ReadStream(m_NDims, m_ReadStream);
      image->ElementByteOrderFix();
      m_ObjectList.push_back(image);
    }

    else if (objectType.find("Blob") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "blb")))
    {
      auto * blob = new MetaBlob();
      blob->APIVersion(m_APIVersion);
      blob->SetEvent(m_Event);
      blob->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(blob);
    }

    else if (objectType.find("Landmark") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "ldm")))
    {
      auto * landmark = new MetaLandmark();
      landmark->APIVersion(m_APIVersion);
      landmark->SetEvent(m_Event);
      landmark->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(landmark);
    }

    else if (objectType.find("Surface") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "suf")))
    {
      auto * surface = new MetaSurface();
      surface->APIVersion(m_APIVersion);
      surface->SetEvent(m_Event);
      surface->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(surface);
    }

    else if (objectType.find("Line") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "lin")))
    {
      auto * line = new MetaLine();
      line->APIVersion(m_APIVersion);
      line->SetEvent(m_Event);
      line->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(line);
    }

    else if (objectType.find("Group") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "grp")))
    {
      auto * group = new MetaGroup();
      group->APIVersion(m_APIVersion);
      group->SetEvent(m_Event);
      group->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(group);
    }

    else if (objectType.find("AffineTransform") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "trn")))
    {
      auto * group = new MetaGroup();
      group->APIVersion(m_APIVersion);
      group->SetEvent(m_Event);
      group->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(group);
    }
    else if (objectType.find("Mesh") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "msh")))
    {
      auto * mesh = new MetaMesh();
      mesh->APIVersion(m_APIVersion);
      mesh->SetEvent(m_Event);
      mesh->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(mesh);
    }
    else if (objectType.find("FEMObject") != std::string::npos || ((objectType.empty()) && !strcmp(suf, "fem")))
    {
      auto * femobject = new MetaFEMObject();
      femobject->APIVersion(m_APIVersion);
      femobject->SetEvent(m_Event);
      femobject->ReadStream(m_NDims, m_ReadStream);
      m_ObjectList.push_back(femobject);
    }
  }

  if (m_Event)
  {
    m_Event->StopReading();
  }

  m_ReadStream->close();

  return true;
}


bool
MetaScene::Write(const char * _headName)
{
  META_DEBUG_PRINT( "MetaScene: Write" );

  if (_headName != nullptr)
  {
    FileName(_headName);
  }

  // Set the number of objects based on the net list
  // ObjectListType::const_iterator itNet = m_ObjectList.begin();
  m_NObjects = static_cast<int>(m_ObjectList.size());

  M_SetupWriteFields();

  if (!m_WriteStream)
  {
    m_WriteStream = new METAIO_STREAM::ofstream;
  }

  m_WriteStream->open(m_FileName.c_str(), std::ios::binary | std::ios::out);

  if (!m_WriteStream->rdbuf()->is_open())
  {
    delete m_WriteStream;
    m_WriteStream = nullptr;
    return false;
  }

  M_Write();

  m_WriteStream->close();
  delete m_WriteStream;
  m_WriteStream = nullptr;

  /** Then we write all the objects in the scene */
  auto it = m_ObjectList.begin();
  while (it != m_ObjectList.end())
  {
    (*it)->BinaryData(this->BinaryData());
    (*it)->Append(_headName);
    ++it;
  }

  return true;
}

/** Clear tube information */
void
MetaScene::Clear()
{
  META_DEBUG_PRINT( "MetaScene: Clear" );

  MetaObject::Clear();

  strcpy(m_ObjectTypeName, "Scene");
  // Delete the list of pointers to objects in the scene.
  auto it = m_ObjectList.begin();
  while (it != m_ObjectList.end())
  {
    MetaObject * object = *it;
    ++it;
    delete object;
  }

  m_ObjectList.clear();
}

/** Set Read fields */
void
MetaScene::M_SetupReadFields()
{
  META_DEBUG_PRINT( "MetaScene: M_SetupReadFields" );

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NObjects", MET_INT, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  mF = MET_GetFieldRecord("ElementSpacing", &m_Fields);
  mF->required = false;
}

void
MetaScene::M_SetupWriteFields()
{
  this->ClearFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ObjectType", MET_STRING, strlen(m_ObjectTypeName), m_ObjectTypeName);
  m_Fields.push_back(mF);

  if (m_FileFormatVersion > 0)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "FileFormatVersion", MET_UINT, m_FileFormatVersion);
    m_Fields.push_back(mF);
  }

  if (strlen(m_Comment) > 0)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Comment", MET_STRING, strlen(m_Comment), m_Comment);
    m_Fields.push_back(mF);
  }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NDims", MET_INT, m_NDims);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NObjects", MET_INT, m_NObjects);
  m_Fields.push_back(mF);
}


bool
MetaScene::M_Read()
{
  META_DEBUG_PRINT( "MetaScene: M_Read: Loading Header" );

  if (strncmp(MET_ReadType(*m_ReadStream).c_str(), "Scene", 5) != 0)
  {
    m_NObjects = 1;
    return true;
  }

  if (!MetaObject::M_Read())
  {
    std::cout << "MetaScene: M_Read: Error parsing file" << '\n';
    return false;
  }

  META_DEBUG_PRINT( "MetaScene: M_Read: Parsing Header" );

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("NObjects", &m_Fields);
  if (mF && mF->defined)
  {
    m_NObjects = static_cast<int>(mF->value[0]);
  }

  return true;
}

bool
MetaScene::M_Write()
{
  if (!MetaObject::M_Write())
  {
    std::cout << "MetaScene: M_Write: Error parsing file" << '\n';
    return false;
  }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
