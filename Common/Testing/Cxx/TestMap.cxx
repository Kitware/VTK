#include "vtkString.h"
#include "vtkObject.h"
#include "vtkArrayMap.txx"
#include "vtkHashMap.txx"

template <typename MapType, typename IteratorType>
class TestMapIntToString
{
public:
  static int Test(const char* prefix, char names[][10])
    {
    int error = 0;
    int cc;
    int loopCounter;
    MapType* am = MapType::New();
    for ( cc = 9; cc >= 0; cc -- )
      {
      if ( am->SetItem(cc, names[cc]) != VTK_OK )
        {
        cout << prefix << "Problem adding item to the array map" << endl;
        error = 1;
        }
      }
    for ( cc = 0; cc < 10; cc ++ )
      {
      char *buffer = 0;
      if ( am->GetItem(cc, buffer) != VTK_OK)
        {
        cout << prefix << "Problem retrieving item from the array map" << endl;
        error = 1;
        }
      if ( !buffer || !vtkString::Equals(buffer, names[cc]) )
        {
        cout << prefix << "Retrieved string: " << (buffer?buffer:"") 
             << " is not the same as the"
          " one inserted" << endl;
        error = 1;
        }
      }
    
    // Try the iterator
    IteratorType *nit = am->NewIterator();
    //cout << prefix << "Try iterator" << endl;
    loopCounter = 100;
    nit->GoToFirstItem();
    while ( !nit->IsDoneWithTraversal() )
      {
      char* str = 0;
      int idx = 0;
      if ( nit->GetData(str) != VTK_OK )
        {
        cout << prefix << "Problem accessing data from iterator" << endl;
        error =1;
        }
      if ( nit->GetKey(idx) != VTK_OK )
        {
        cout << prefix << "Problem accessing key from iterator" << endl;
        error =1;     
        }
      //cout << prefix << "Item: " << idx << " = " << str << endl;
      nit->GoToNextItem();
      if(--loopCounter == 0)
        {
        cout << prefix << "Iterator has entered infinite loop." << endl;
        error =1;
        break;
        }
      }
    loopCounter = 100;
    nit->GoToLastItem();
    while ( !nit->IsDoneWithTraversal() )
      {
      char* str = 0;
      int idx = 0;
      if ( nit->GetData(str) != VTK_OK )
        {
        cout << prefix << "Problem accessing data from iterator" << endl;
        error =1;
        }
      if ( nit->GetKey(idx) != VTK_OK )
        {
        cout << prefix << "Problem accessing key from iterator" << endl;
        error =1;     
        }
      //cout << prefix << "Item: " << idx << " = " << str << endl;
      nit->GoToPreviousItem();
      if(--loopCounter == 0)
        {
        cout << prefix << "Iterator has entered infinite loop." << endl;
        error =1;
        break;
        }
      }
    nit->Delete();
    
    // Try printing the map.
    am->Print(cout);
    
    am->Delete();
    return error;
    }
};

template <typename MapType>
class TestMapStringToString
{
public:
  static int Test(const char* prefix, char names[][10])
    {
    int error = 0;
    int cc;  
  
    MapType* sam = MapType::New();
    for ( cc = 9; cc >= 0; cc -- )
      {
      char *nkey = vtkString::Duplicate(names[cc]);
      if ( sam->SetItem(nkey, names[cc]) != VTK_OK )
        {
        cout << prefix << "Problem adding item to the array map" << endl;
        error = 1;
        }
      delete [] nkey;
      }
    //sam->DebugList();
    for ( cc = 0; cc < 10; cc ++ )
      {
      char *buffer = 0;
      if ( sam->GetItem(names[cc], buffer) != VTK_OK)
        {
        cout << prefix << "Problem retrieving item from the array map" << endl;
        error = 1;
        }
      if ( !buffer )
        {
        cout << prefix << "Cannot access key: " << names[cc] << endl;
        }      
      else
        {
        if ( !vtkString::Equals(buffer, names[cc]) )
          {
          cout << prefix << "Retrieved string: " << (buffer?buffer:"") 
               << " is not the same as the"
            " one inserted" << endl;
          error  = 1;
          }
        }
      }
    char *buffer = 0;
    if ( sam->GetItem("Brad", buffer) != VTK_OK)
      {
      cout << prefix << "Problem retrieving item from the array map" << endl;
      error = 1;
      }
    if ( !buffer )
      {
      cout << prefix << "Cannot access key: " << names[cc] << endl;
      error  = 1;
      }      
    else
      {
      if ( !vtkString::Equals(buffer, "Brad") )
        {
        cout << prefix << "Retrieved string: " << (buffer?buffer:"") 
             << " is not the same as the"
             << " one inserted" << endl;
        error  = 1;
        }
      }
 
    // Try printing the map.
    sam->Print(cout);
    
    sam->Delete();
    return error;
    }
};

template <typename MapType>
class TestMapStringToObject
{
public:
  static int Test(const char* prefix, char names[][10])
    {
    int error = 0;
    int cc;  
  
    MapType* soam = MapType::New();
    
    char name[20];
    for( cc = 0; cc < 10; cc ++ )
      {
      sprintf(name, "actor%02d", cc);
      vtkObject* actor = vtkObject::New();
      if ( soam->SetItem(name, actor) != VTK_OK )
        {
        cout << prefix << "Problem inserting item in the map, key: " 
             << name << " data: " << actor << endl;
        error  = 1;
        }

      vtkObject* nactor = 0;
      if ( soam->GetItem(name, nactor) != VTK_OK )
        {
        cout << prefix << "Problem accessing item in the map: " << name
             << endl;
        error  = 1;
        }
      if ( !nactor )
        {
        cout << prefix << "Item: " << name << " should not be null" << endl;
        error  = 1;
        }
      if ( nactor != actor )
        {
        cout << prefix << "Item: " << nactor << " at key: " << name 
             << " is not the same as: " << actor << endl;
        error  = 1;
        }
    
      actor->Delete();
      }
    
    // Try printing the map.
    soam->Print(cout);
    
    soam->Delete();
    return error;
    }
};

int main()
{
  int error = 0;
  char names[][10] = {
    "Andy",
    "Amy",
    "Berk",
    "Bill",
    "Brad",
    "Charles",
    "JoAnne",
    "Ken",
    "Lisa",
    "Sebastien",
    "Will"
  };
  
  {
  typedef vtkArrayMap<int,char*> Map;
  typedef TestMapIntToString< Map, Map::IteratorType > Tester;
  error = Tester::Test("ArrayMapIntToString: ", names) || error;
  }
  
  {
  typedef vtkHashMap<int,char*> Map;
  typedef TestMapIntToString< Map, Map::IteratorType > Tester;
  error = Tester::Test("HashMapIntToString: ", names) || error;
  }
  
  {
  typedef vtkArrayMap<const char*,char*> Map;
  typedef TestMapStringToString< Map > Tester;
  error = Tester::Test("ArrayMapStringToString: ", names) || error;
  }  

  {
  typedef vtkHashMap<const char*,char*> Map;
  typedef TestMapStringToString< Map > Tester;
  error = Tester::Test("HashMapStringToString: ", names) || error;
  }  

  {
  typedef vtkArrayMap<const char*, vtkObject*> Map;
  typedef TestMapStringToObject< Map > Tester;
  error = Tester::Test("ArrayMapStringToObject: ", names) || error;
  }
  
  {
  typedef vtkHashMap<const char*, vtkObject*> Map;
  typedef TestMapStringToObject< Map > Tester;
  error = Tester::Test("HashMapStringToObject: ", names) || error;
  }  
  
  return error;
}

