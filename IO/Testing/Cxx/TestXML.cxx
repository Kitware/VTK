#include "vtkXMLParser.h"
#include "vtkOutputWindow.h"
#include "vtkObjectFactory.h"


class vtkMyXML : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkMyXML, vtkXMLParser);
  static vtkMyXML* New();

protected:  
  vtkMyXML() {} 
  void StartElement(const char*, const char**) {}
  void EndElement(const char*) {}

private:
  vtkMyXML(const vtkMyXML&); // Not implemented
  void operator=(const vtkMyXML&); // Not implemented    
};

vtkStandardNewMacro(vtkMyXML);

int TestXML(int argc, char *argv[])
{
  int res = 0;
  vtkOutputWindow::GetInstance()->PromptUserOn();
  if ( argc <= 1 )
    {
    cout << "Usage: " << argv[0] << " <xml file>" << endl;
    return 1;
    }

  vtkMyXML *parser = vtkMyXML::New();
  parser->SetFileName(argv[1]);
  if ( ! parser->Parse() )
    {
    cout << "Cannot parse the file: " << argv[1] << endl;
    res = 1;
    }
  parser->SetFileName(0);

  if( !parser->Parse("<xml>This is an XML file</xml>") )
    {
    cout << "Cannot parse message" << endl;
    res = 1;
    }
  
  parser->Delete();

  return res;
}
