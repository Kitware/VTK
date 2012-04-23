#include <ncconfig.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <netcdf.h>

using namespace std;

#ifdef EXTRA_TESTS
#define MEGABYTE 1048576
void
get_mem_used2(int *mem_used)
{
   char buf[30];
   FILE *pf;
   size_t page_size = 4092; /* For spock... */
   unsigned size; /*       total program size */
   unsigned resident;/*   resident set size */
   unsigned share;/*      shared pages */
   unsigned text;/*       text (code) */
   unsigned lib;/*        library */
   unsigned data;/*       data/stack */
   /*unsigned dt;          dirty pages (unused in Linux 2.6)*/

   snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
   if ((pf = fopen(buf, "r")))
   {
      fscanf(pf, "%u %u %u %u %u %u", &size, &resident, &share,
             &text, &lib, &data);
      *mem_used = (data * page_size) / MEGABYTE;
   }
   else
      *mem_used = -1;
  fclose(pf);
}
#endif /* EXTRA_TESTS */

//Exception class
class NcErrorException : public exception
{
public:
   NcErrorException(const string& descr) throw(): exception(), _descr(descr)  {};
   ~NcErrorException() throw() {};

   const char* what() const throw() { ostringstream err; err << "NcErrorException: " << _descr;  return err.str().c_str();   };


private:
   string _descr;
};

void handle_error(int status) {
   if (status != NC_NOERR) {
      throw NcErrorException(nc_strerror(status));
   }
};

/******MAIN********/
int main(int argc, char** argv)
{
   int NUMVARS = 1;
   size_t NUMREC=10000;
   int fileId, dimId, varId[1];
   string filename("tst_many_writes.nc");

   cout << "\n*** Testing netCDF-4 file with user-provided test (thanks Marica!)\n";

   try{
      //create the netcdf-4 file
      handle_error(nc_create(filename.c_str(), NC_NETCDF4, &fileId));

      //define the unlimited dimension "rec"
      handle_error(nc_def_dim(fileId, "rec", NC_UNLIMITED, &dimId)); //--> Segmentation Fault
      //handle_error ( nc_def_dim(fileId, "rec", NUMREC, &dimId) );  //--> Good!!

      int dimids[1] = {dimId};

      //define NUMVARS variables named field_%i using a loop
      for (int v = 0; v < NUMVARS; v++)
      {
         size_t chunkSize[1] = {100000};
         ostringstream varName; varName << "field_" << v;
         handle_error(nc_def_var(fileId, varName.str().c_str(), NC_DOUBLE, 1, dimids , &varId[v]));
         handle_error(nc_def_var_chunking(fileId, varId[v], NC_CHUNKED, chunkSize));
      }
      handle_error (nc_enddef(fileId));

      //write data to the NUMVARS variables using nc_put_var1_double
      double data = 100;
      size_t start[1];
      size_t count[1] = {1};
      char charName[NC_MAX_NAME+1];

      for (int v = 0; v < NUMVARS; v++)
      {
         handle_error(nc_inq_varname(fileId, varId[v], charName));
         cout << "var " << v << "\n";
         for (size_t start = 0; start < NUMREC; start++)
         {
#ifdef EXTRA_TESTS
            if (start % 1000 == 0)
            {
               int mem_used;
               get_mem_used2(&mem_used);
               cout << mem_used << "\n";
            }
#endif /* EXTRA_TESTS */
            handle_error(nc_put_vara_double(fileId, varId[v], &start, count, &data));
         }
      }

      //close file
      handle_error(nc_close(fileId));
      cout << "*** nctst SUCCESS!\n";
   }
   catch(exception &ex) //exception handling
   {
      cerr << "Exception caught: " << ex.what() << endl;
      cout << "*** nctst FAILURE!\n";
      return -1;
   }
}
