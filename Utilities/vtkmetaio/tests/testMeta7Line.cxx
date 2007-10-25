#include <stdio.h>
#include <ctype.h>
#include <metaLine.h>

int main(int argc, char **argv)
{

  METAIO_STREAM::cout << "Creating test file ...";
  MetaLine* Line = new MetaLine(3);
  Line->ID(0);
  LinePnt* pnt;

  unsigned int i;
  for(i=0;i<10;i++)
  {
    pnt = new LinePnt(3);
    pnt->m_X[0]=(float)0.2;pnt->m_X[1]=i;pnt->m_X[2]=i;
    pnt->m_V[0][0]=(float)0.3;pnt->m_V[0][1]=i;pnt->m_V[0][2]=i;
    pnt->m_V[1][0]=(float)0.4;pnt->m_V[1][1]=i+1;pnt->m_V[1][2]=i+1;
    Line->GetPoints().push_back(pnt);
  }
  
  METAIO_STREAM::cout << "Writing test file ...";
   
  Line->BinaryData(true);

  Line->Write("myLine.meta");

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Reading test file ...";

  Line->Clear();
  Line->Read("myLine.meta");

  Line->PrintInfo();

  MetaLine::PointListType list =  Line->GetPoints();
  MetaLine::PointListType::const_iterator it = list.begin();
  
  i=0;
  while(it != list.end())
  {
    METAIO_STREAM::cout << "Point #" << i++ << ":" << METAIO_STREAM::endl;
    METAIO_STREAM::cout << "position = ";
    unsigned int d=0;
    for(d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_X[d] << " ";
    }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    METAIO_STREAM::cout << "First normal = ";
    for(d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_V[0][d] << " ";
    }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    METAIO_STREAM::cout << "Second normal = ";
    for(d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_V[1][d] << " ";
    }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    it++;
  }

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  return 1;
}
