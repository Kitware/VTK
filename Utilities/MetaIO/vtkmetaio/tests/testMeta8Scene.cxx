#include <stdio.h>
#include <ctype.h>
#include <metaScene.h>
#include <metaGroup.h>
#include <metaEllipse.h>

int main(int argc, char **argv)
{

  METAIO_STREAM::cout << "Creating test scene ..." << METAIO_STREAM::endl;
  MetaScene * s = new MetaScene(3);

  MetaEllipse * e1 = new MetaEllipse(3);
  e1->ID(0);
  e1->Radius(3);

  MetaEllipse * e2 = new MetaEllipse(3);
  e2->ID(1);
  e2->Radius(4);

  MetaGroup * g1 = new MetaGroup(3);
  g1->ID(2);

  e1->ParentID(2);
  e2->ParentID(2);

  s->AddObject(g1);
  s->AddObject(e1);
  s->AddObject(e2);

  METAIO_STREAM::cout << "...[ok]" << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "Writing test file ..." << METAIO_STREAM::endl;

  s->Write("scene.scn");

  METAIO_STREAM::cout << "...[ok]" << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "Clearing the scene..." << METAIO_STREAM::endl;
  s->Clear();
  METAIO_STREAM::cout << "...[ok]" << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "Reading test file ..." << METAIO_STREAM::endl;

  s->Read("scene.scn");

  if(s->NObjects() != 3)
    {
    METAIO_STREAM::cout << "Number of obejcts: " << s->NObjects()
              << " != 3...[FAILED]" << METAIO_STREAM::endl;
    return 0;
    }

  METAIO_STREAM::cout << "...[ok]" << METAIO_STREAM::endl;

  s->Clear();

  METAIO_STREAM::cout << "Writing single object..." << METAIO_STREAM::endl;

  e1 = new MetaEllipse(3);
  e1->ID(0);
  e1->Radius(3);
  e1->Write("ellipse.elp");

  METAIO_STREAM::cout << "[OK]" << METAIO_STREAM::endl;

  s->Clear();

  METAIO_STREAM::cout << "Reading test file ..." << METAIO_STREAM::endl;

  s->Read("ellipse.elp");

  if(s->NObjects() != 1)
    {
    METAIO_STREAM::cout << "Number of obejcts: " << s->NObjects()
              << " != 1...[FAILED]" << METAIO_STREAM::endl;
    return 0;
    }

  METAIO_STREAM::cout << "[OK]" << METAIO_STREAM::endl;

  // (*(s->GetObjectList()->begin()))->PrintInfo();

  return 1;
}
