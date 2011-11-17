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
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4702 )
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
#include "metaLandmark.h"
#include "metaMesh.h"
#include "metaArrow.h"
#include "metaTransform.h"
#include "metaTubeGraph.h"
#include "metaFEMObject.h"

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

//
// MetaScene Constructors
//
MetaScene::
MetaScene()
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene()" << METAIO_STREAM::endl;
    }
  Clear();
}


//
MetaScene::
  MetaScene(const MetaScene *_scene)
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene()" << METAIO_STREAM::endl;
    }
  Clear();
  CopyInfo(_scene);
}

//
MetaScene::
  MetaScene(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene()" << METAIO_STREAM::endl;
    }
  Clear();
}


/** Destructor */
MetaScene::
~MetaScene()
{
  Clear();
  M_Destroy();
}

//
void MetaScene::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "Number of Objects = " << m_NObjects << METAIO_STREAM::endl;
}

void MetaScene::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}


void MetaScene::
NObjects(int nobjects)
{
  m_NObjects = nobjects;
}

int MetaScene::
NObjects(void) const
{
  return m_NObjects;
}

void MetaScene::
AddObject(MetaObject* object)
{
  m_ObjectList.push_back(object);
}

bool MetaScene::
Read(const char *_headerName)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene: Read" << METAIO_STREAM::endl;
    }

  int i = 0;
  char suf[80];
  suf[0] = '\0';
  if(MET_GetFileSuffixPtr(_headerName, &i))
    {
    strcpy(suf, &_headerName[i]);
    }

  M_Destroy();

  Clear();

  M_SetupReadFields();

  if(_headerName != NULL)
    {
    strcpy(m_FileName, _headerName);
    }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaScene: Read: Opening stream" << METAIO_STREAM::endl;

  M_PrepareNewReadStream();

#ifdef __sgi
  m_ReadStream->open(m_FileName, METAIO_STREAM::ios::in);
#else
  m_ReadStream->open(m_FileName, METAIO_STREAM::ios::binary
                                 | METAIO_STREAM::ios::in);
#endif

  if(!m_ReadStream->rdbuf()->is_open())
    {
    METAIO_STREAM::cout << "MetaScene: Read: Cannot open file" << METAIO_STREAM::endl;
    return false;
    }

  if(!M_Read())
    {
    METAIO_STREAM::cout << "MetaScene: Read: Cannot parse file" << METAIO_STREAM::endl;
    m_ReadStream->close();
    return false;
    }

  if(_headerName != NULL)
    {
    strcpy(m_FileName, _headerName);
    }

  if(m_Event)
    {
    m_Event->StartReading(m_NObjects);
    }

  /** Objects should be added here */
  for(i=0;i<m_NObjects;i++)
    {
    if(META_DEBUG)
      {
      METAIO_STREAM::cout << MET_ReadType(*m_ReadStream).c_str()
        << METAIO_STREAM::endl;
      }

    if(m_Event)
      {
      m_Event->SetCurrentIteration(i+1);
      }

    if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Tube",4) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "tre")))
      {
      char* subtype = MET_ReadSubType(*m_ReadStream);
      if(!strncmp(subtype,"Vessel",6))
        {
        MetaVesselTube* vesseltube = new MetaVesselTube();
        vesseltube->SetEvent(m_Event);
        vesseltube->ReadStream(m_NDims,m_ReadStream);
        m_ObjectList.push_back(vesseltube);
        }
      else if(!strncmp(subtype,"DTI",3))
        {
        MetaDTITube* dtitube = new MetaDTITube();
        dtitube->SetEvent(m_Event);
        dtitube->ReadStream(m_NDims,m_ReadStream);
        m_ObjectList.push_back(dtitube);
        }
      else
        {
        MetaTube* tube = new MetaTube();
        tube->SetEvent(m_Event);
        tube->ReadStream(m_NDims,m_ReadStream);
        m_ObjectList.push_back(tube);
        }
      delete []subtype;
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Transform",9))
      {
      MetaTransform* transform = new MetaTransform();
      transform->SetEvent(m_Event);
      transform->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(transform);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"TubeGraph",9))
      {
      MetaTubeGraph* tubeGraph = new MetaTubeGraph();
      tubeGraph->SetEvent(m_Event);
      tubeGraph->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(tubeGraph);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Ellipse",7) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "elp")))
      {
      MetaEllipse* ellipse = new MetaEllipse();
      ellipse->SetEvent(m_Event);
      ellipse->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(ellipse);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Contour",7) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "ctr")))
      {
      MetaContour* contour = new MetaContour();
      contour->SetEvent(m_Event);
      contour->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(contour);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Arrow",5))
      {
      MetaArrow* arrow = new MetaArrow();
      arrow->SetEvent(m_Event);
      arrow->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(arrow);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Gaussian",8) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "gau")))
      {
      MetaGaussian* gaussian = new MetaGaussian();
      gaussian->SetEvent(m_Event);
      gaussian->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(gaussian);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Image",5) ||
      ((MET_ReadType(*m_ReadStream).size()==0) &&
       (!strcmp(suf, "mhd") || !strcmp(suf, "mha"))))
      {
      MetaImage* image = new MetaImage();
      image->SetEvent(m_Event);
      image->ReadStream(m_NDims,m_ReadStream);
      image->ElementByteOrderFix();
      m_ObjectList.push_back(image);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Blob",4) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "blb")))
      {
      MetaBlob* blob = new MetaBlob();
      blob->SetEvent(m_Event);
      blob->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(blob);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Landmark",8) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "ldm")))
      {
      MetaLandmark* landmark = new MetaLandmark();
      landmark->SetEvent(m_Event);
      landmark->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(landmark);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Surface",5) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "suf")))
      {
      MetaSurface* surface = new MetaSurface();
      surface->SetEvent(m_Event);
      surface->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(surface);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Line",5) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "lin")))
      {
      MetaLine* line = new MetaLine();
      line->SetEvent(m_Event);
      line->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(line);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Group",5) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "grp")))
      {
      MetaGroup* group = new MetaGroup();
      group->SetEvent(m_Event);
      group->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(group);
      }

    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"AffineTransform",15) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "trn")))
      {
      MetaGroup* group = new MetaGroup();
      group->SetEvent(m_Event);
      group->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(group);
      }
    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"Mesh",4) ||
      ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "msh")))
      {
      MetaMesh* mesh = new MetaMesh();
      mesh->SetEvent(m_Event);
      mesh->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(mesh);
      }
    else if(!strncmp(MET_ReadType(*m_ReadStream).c_str(),"FEMObject",9) ||
            ((MET_ReadType(*m_ReadStream).size()==0) && !strcmp(suf, "fem")))
      {
      MetaFEMObject* femobject = new MetaFEMObject();
      femobject->SetEvent(m_Event);
      femobject->ReadStream(m_NDims,m_ReadStream);
      m_ObjectList.push_back(femobject);
      }
    }

  if(m_Event)
    {
    m_Event->StopReading();
    }

  m_ReadStream->close();

  return true;
}


//
//
//
bool MetaScene::
Write(const char *_headName)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene: Write" << METAIO_STREAM::endl;
    }

  if(_headName != NULL)
    {
    FileName(_headName);
    }

  // Set the number of objects based on the net list
  //ObjectListType::const_iterator itNet = m_ObjectList.begin();
  m_NObjects = static_cast<int>(m_ObjectList.size());

  M_SetupWriteFields();

  if(!m_WriteStream)
    {
    m_WriteStream = new METAIO_STREAM::ofstream;
    }

#ifdef __sgi
  // Create the file. This is required on some older sgi's
    {
    METAIO_STREAM::ofstream tFile(m_FileName, METAIO_STREAM::ios::out);
    tFile.close();
    }
  m_WriteStream->open(m_FileName, METAIO_STREAM::ios::out);
#else
  m_WriteStream->open(m_FileName, METAIO_STREAM::ios::binary
    | METAIO_STREAM::ios::out);
#endif

  if(!m_WriteStream->rdbuf()->is_open())
    {
    delete m_WriteStream;
    m_WriteStream = 0;
    return false;
    }

  M_Write();

  m_WriteStream->close();
  delete m_WriteStream;
  m_WriteStream = 0;

  /** Then we write all the objects in the scene */
  ObjectListType::iterator it = m_ObjectList.begin();
  while(it != m_ObjectList.end())
    {
    (*it)->BinaryData(this->BinaryData());
    (*it)->Append(_headName);
    it++;
    }

  return true;
}

/** Clear tube information */
void MetaScene::
Clear(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene: Clear" << METAIO_STREAM::endl;
    }
  MetaObject::Clear();
  // Delete the list of pointers to objects in the scene.
  ObjectListType::iterator it = m_ObjectList.begin();
  while(it != m_ObjectList.end())
    {
    MetaObject* object = *it;
    it++;
    delete object;
    }

  m_ObjectList.clear();

}

/** Destroy tube information */
void MetaScene::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaScene::
M_SetupReadFields(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene: M_SetupReadFields" << METAIO_STREAM::endl;
    }

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NObjects", MET_INT, false);
  mF->required = true;
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  mF = MET_GetFieldRecord("ElementSpacing", &m_Fields);
  mF->required = false;
}

void MetaScene::
M_SetupWriteFields(void)
{
  this->ClearFields();

  MET_FieldRecordType * mF;

  if(strlen(m_Comment)>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Comment", MET_STRING, strlen(m_Comment), m_Comment);
    m_Fields.push_back(mF);
    }

  strcpy(m_ObjectTypeName,"Scene");
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ObjectType", MET_STRING, strlen(m_ObjectTypeName),
    m_ObjectTypeName);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NDims", MET_INT, m_NDims);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NObjects", MET_INT, m_NObjects);
  m_Fields.push_back(mF);
}



bool MetaScene::
M_Read(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout<<"MetaScene: M_Read: Loading Header"<<METAIO_STREAM::endl;
    }

  if(strncmp(MET_ReadType(*m_ReadStream).c_str(),"Scene",5))
    {
    m_NObjects = 1;
    return true;
    }

  if(!MetaObject::M_Read())
    {
    METAIO_STREAM::cout << "MetaScene: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaScene: M_Read: Parsing Header" << METAIO_STREAM::endl;
    }

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("NObjects", &m_Fields);
  if(mF->defined)
    {
    m_NObjects= (int)mF->value[0];
    }

  return true;
}

bool MetaScene::
M_Write(void)
{
  if(!MetaObject::M_Write())
    {
    METAIO_STREAM::cout << "MetaScene: M_Write: Error parsing file" << METAIO_STREAM::endl;
    return false;
    }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif

