/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTypes.h"

#ifndef metaOutput_h
#define metaOutput_h

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#pragma warning ( disable: 4251 )
#pragma warning ( disable: 4511 ) // copy constructor not found
#pragma warning ( disable: 4512 ) // assignment operator not found
#endif

#include "metaCommand.h"
#include <stdio.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class MetaOutputStream
{
  public:

    MetaOutputStream();
    virtual ~MetaOutputStream() {}

    void                     SetName(const char* name);
    METAIO_STL::string       GetName() const;

    void                     Enable();
    bool                     IsEnable() const;
    void                     Disable();

    void                     SetStdStream(METAIO_STREAM::ostream * stream);
    bool                     IsStdStream();
    METAIO_STREAM::ostream * GetStdStream();

    virtual bool             Open();
    virtual bool             Close();

    virtual bool             Write(const char* buffer);

    void                     SetMetaOutput(void* metaOutput);

  protected:

    METAIO_STREAM::ostream * m_StdStream;
    bool                     m_IsStdStream;
    bool                     m_Enable;
    bool                     m_IsOpen;
    METAIO_STL::string       m_Name;
    void*                    m_MetaOutput;

};

class MetaFileOutputStream : public MetaOutputStream
{
  public:

    MetaFileOutputStream(const char* name);
    ~MetaFileOutputStream() MET_OVERRIDE {}

    bool Open() MET_OVERRIDE;
    bool Close() MET_OVERRIDE;

    METAIO_STL::string GetFileName();

  private:

    METAIO_STL::string      m_FileName;
    METAIO_STREAM::ofstream m_FileStream;
};

class METAIO_EXPORT MetaOutput
{
  public:

    typedef enum {INT,FLOAT,CHAR,STRING,LIST,FLAG,BOOL} TypeEnumType;

    struct Field{
      METAIO_STL::string  name;
      METAIO_STL::string  description;
      METAIO_STL::vector<METAIO_STL::string>  value;
      TypeEnumType type;
      METAIO_STL::string  rangeMin;
      METAIO_STL::string  rangeMax;
      };

    typedef METAIO_STL::vector<Field>              FieldVector;
    typedef METAIO_STL::vector<MetaOutputStream*>  StreamVector;
    typedef METAIO_STL::list< METAIO_STL::string > ListType;

    MetaOutput();
    ~MetaOutput();

    /** Add a field */
    bool AddField(METAIO_STL::string name,
                  METAIO_STL::string description,
                  TypeEnumType type,
                  METAIO_STL::string value,
                  METAIO_STL::string rangeMin = "",
                  METAIO_STL::string rangeMax = ""
                  );

    bool AddFloatField(METAIO_STL::string name,
                       METAIO_STL::string description,
                       float value,
                       METAIO_STL::string rangeMin = "",
                       METAIO_STL::string rangeMax = ""
                       );

    bool AddIntField(METAIO_STL::string name,
                     METAIO_STL::string description,
                     int value,
                     METAIO_STL::string rangeMin = "",
                     METAIO_STL::string rangeMax = ""
                     );

    bool AddListField(METAIO_STL::string name,
                      METAIO_STL::string description,
                      ListType list);

    /** Set the metaCommand for parsing */
    void SetMetaCommand(MetaCommand* metaCommand);

    /** Write the output to the connected streams */
    void Write();

    /** Add a standard stream */
    void AddStream(const char* name,METAIO_STREAM::ostream & stream);
    void AddStream(const char* name,MetaOutputStream * stream);

    /** Add a stream file. Helper function */
    void AddStreamFile(const char* name, const char* filename);

    /** Enable or Disable a stream */
    void EnableStream(const char* name);
    void DisableStream(const char* name);

    METAIO_STL::string GetHostname(void);
    METAIO_STL::string GetHostip(void);

  private:

    METAIO_STL::string TypeToString(TypeEnumType type);

    /** Private function to fill in the buffer */
    METAIO_STL::string GenerateXML(const char* filename=NULL);
    METAIO_STL::string GetUsername(void);

    FieldVector   m_FieldVector;
    MetaCommand*  m_MetaCommand;
    StreamVector  m_StreamVector;
    METAIO_STL::string   m_CurrentVersion;

}; // end of class

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
