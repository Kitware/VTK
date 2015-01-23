#ifndef vtkMySQLDatabasePrivate_h
#define vtkMySQLDatabasePrivate_h

#ifdef _WIN32
# include <winsock.h> // mysql.h relies on the typedefs from here
#endif

#include "vtkIOMySQLModule.h" // For export macro
#include <mysql.h> // needed for MYSQL typedefs

class VTKIOMYSQL_EXPORT vtkMySQLDatabasePrivate
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

#endif // vtkMySQLDatabasePrivate_h
// VTK-HeaderTest-Exclude: vtkMySQLDatabasePrivate.h
