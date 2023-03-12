#include <iostream>
#include <set>
#include <sstream>

#include "vtk_libproj.h"

int TestLibProj(int, char*[])
{
  try
  {
    int epsgCode = 3978;
    std::set<std::string> expectedProjStrings = {
      "+proj=lcc +lat_0=49 +lon_0=-95 +lat_1=49 +lat_2=77 +x_0=0 "
      "+y_0=0 +datum=NAD83 +units=m +no_defs +type=crs",

      "+proj=lcc +lat_0=49 +lon_0=-95 +lat_1=49 +lat_2=77 +x_0=0 +y_0=0 +ellps=GRS80 "
      "+towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"
    };
    namespace projio = osgeo::proj::io;
    namespace projutil = osgeo::proj::util;
    namespace projcrs = osgeo::proj::crs;
    namespace projoperation = osgeo::proj::operation;
    namespace projcast = dropbox::oxygen;
    projio::DatabaseContextPtr dbContext;
    dbContext = projio::DatabaseContext::create("", std::vector<std::string>()).as_nullable();
    std::ostringstream ostr;
    ostr << epsgCode;
    std::string code = ostr.str();
    if (!dbContext)
    {
      throw std::runtime_error("no database context specified");
    }
    projio::DatabaseContextNNPtr dbContextNNPtr(NN_NO_CHECK(dbContext));
    auto factory = projio::AuthorityFactory::create(dbContextNNPtr, "EPSG");
    projutil::BaseObjectNNPtr obj = factory->createCoordinateReferenceSystem(code);
    auto projStringExportable =
      projcast::nn_dynamic_pointer_cast<projio::IPROJStringExportable>(obj);
    auto crs = projcast::nn_dynamic_pointer_cast<projcrs::CRS>(obj);

    // export to a proj string
    std::shared_ptr<projio::IPROJStringExportable> objToExport;
    if (crs)
    {
      auto allowUseIntermediateCRS =
        projoperation::CoordinateOperationContext::IntermediateCRSUse::NEVER;
      objToExport = projcast::nn_dynamic_pointer_cast<projio::IPROJStringExportable>(
        crs->createBoundCRSToWGS84IfPossible(dbContext, allowUseIntermediateCRS));
    }
    if (!objToExport)
    {
      objToExport = projStringExportable;
    }
    auto formatter = projio::PROJStringFormatter::create(
      projio::PROJStringFormatter::Convention::PROJ_5, dbContext);
    formatter->setMultiLine(false);
    std::string projString = objToExport->exportToPROJString(formatter.get());
    if (expectedProjStrings.find(projString) == expectedProjStrings.end())
    {
      std::cerr << "Error: Proj string " << projString << " not expected:" << std::endl;
      for (const std::string& s : expectedProjStrings)
      {
        std::cerr << s << std::endl;
      }
      return 1;
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
