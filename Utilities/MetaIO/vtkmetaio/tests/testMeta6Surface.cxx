#include <stdio.h>
#include <ctype.h>
#include <metaSurface.h>

int main(int argc, char **argv)
{

  METAIO_STREAM::cout << "Creating test file ...";
  MetaSurface* surface = new MetaSurface(3);
  surface->ID(0);
  SurfacePnt* pnt;

  unsigned int i;
  for(i=0;i<10;i++)
  {
    pnt = new SurfacePnt(3);
    pnt->m_X[0]=(float)0.2;
    pnt->m_X[1]=i;
    pnt->m_X[2]=i;
    pnt->m_V[0]=(float)0.8;
    pnt->m_V[1]=i;
    pnt->m_V[2]=i;
    surface->GetPoints().push_back(pnt);
  }


  METAIO_STREAM::cout << "Writing ASCII test file ...";

  surface->Write("mySurface.meta");

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Reading ASCII test file ...";

  surface->Clear();
  surface->Read("mySurface.meta");
  surface->PrintInfo();

  MetaSurface::PointListType list =  surface->GetPoints();
  MetaSurface::PointListType::const_iterator it = list.begin();

  unsigned int d=0;
  while(it != list.end())
  {

    for(d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_X[d] << " ";
    }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    for(d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_V[d] << " ";
    }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    it++;
  }

  METAIO_STREAM::cout << "Writing Binary test file ...";
  surface->BinaryData(true);
  surface->ElementType(MET_FLOAT);
  surface->Write("mySurface.meta");

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Reading Binary test file ...";

  surface->Clear();
  surface->Read("mySurface.meta");
  surface->PrintInfo();

  MetaSurface::PointListType list2 =  surface->GetPoints();
  it = list2.begin();

  while(it != list2.end())
  {
    for(d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_X[d] << " ";
    }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    for(d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_V[d] << " ";
    }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    it++;
  }

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  return 1;
}
