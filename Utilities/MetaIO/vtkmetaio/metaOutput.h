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
    std::string       GetName() const;

    void                     Enable();
    bool                     IsEnable() const;
    void                     Disable();

    void                     SetStdStream(std::ostream * stream);
    bool                     IsStdStream();
    std::ostream * GetStdStream();

    virtual bool             Open();
    virtual bool             Close();

    virtual bool             Write(const char* buffer);

    void                     SetMetaOutput(void* metaOutput);

  protected:

    std::ostream * m_StdStream;
    bool                     m_IsStdStream;
    bool                     m_Enable;
    bool                     m_IsOpen;
    std::string       m_Name;
    void*                    m_MetaOutput;

};

class MetaFileOutputStream : public MetaOutputStream
{
  public:

    MetaFileOutputStream(const char* name);
    ~MetaFileOutputStream() override {}

    bool Open() override;
    bool Close() override;

    std::string GetFileName();

  private:

    std::string      m_FileName;
    std::ofstream m_FileStream;
};

class METAIO_EXPORT MetaOutput
{
  public:

    typedef enum {INT,FLOAT,CHAR,STRING,LIST,FLAG,BOOL} TypeEnumType;

    struct Field{
      std::string  name;
      std::string  description;
      std::vector<std::string>  value;
      TypeEnumType type;
      std::string  rangeMin;
      std::string  rangeMax;
      };

    typedef std::vector<Field>              FieldVector;
    typedef std::vector<MetaOutputStream*>  StreamVector;
    typedef std::list< std::string > ListType;

    MetaOutput();
    ~MetaOutput();

    /** Add a field */
    bool AddField(std::string name,
                  std::string description,
                  TypeEnumType type,
                  std::string value,
                  std::string rangeMin = "",
                  std::string rangeMax = ""
                  );

    bool AddFloatField(std::string name,
                       std::string description,
                       float value,
                       std::string rangeMin = "",
                       std::string rangeMax = ""
                       );

    bool AddIntField(std::string name,
                     std::string description,
                     int value,
                     std::string rangeMin = "",
                     std::string rangeMax = ""
                     );

    bool AddListField(std::string name,
                      std::string description,
                      ListType list);

    /** Set the metaCommand for parsing */
    void SetMetaCommand(MetaCommand* metaCommand);

    /** Write the output to the connected streams */
    void Write();

    /** Add a standard stream */
    void AddStream(const char* name,std::ostream & stream);
    void AddStream(const char* name,MetaOutputStream * stream);

    /** Add a stream file. Helper function */
    void AddStreamFile(const char* name, const char* filename);

    /** Enable or Disable a stream */
    void EnableStream(const char* name);
    void DisableStream(const char* name);

    std::string GetHostname(void);
    std::string GetHostip(void);

  private:

    std::string TypeToString(TypeEnumType type);

    /** Private function to fill in the buffer */
    std::string GenerateXML(const char* filename=nullptr);
    std::string GetUsername(void);

    FieldVector   m_FieldVector;
    MetaCommand*  m_MetaCommand;
    StreamVector  m_StreamVector;
    std::string   m_CurrentVersion;

}; // end of class

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
