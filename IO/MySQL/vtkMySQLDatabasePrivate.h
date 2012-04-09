#ifndef __vtkMySQLDatabasePrivate_h
#define __vtkMySQLDatabasePrivate_h

#ifdef _WIN32
# include <winsock.h> // mysql.h relies on the typedefs from here
#endif

#include <mysql.h> // needed for MYSQL typedefs

class VTK_IO_EXPORT vtkMySQLDatabasePrivate
{
public:
  vtkMySQLDatabasePrivate() :
    Connection( NULL )
  {
  mysql_init( &this->NullConnection );
  }

  MYSQL NullConnection;
  MYSQL *Connection;
};

#endif // __vtkMySQLDatabasePrivate_h
