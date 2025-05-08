// Wrap the mpi types in structs as they can be macros in some implementations,
// causing `check_type_size` to fail.
struct Wrapped_MPI_Comm     { MPI_Comm     obj; };
struct Wrapped_MPI_Datatype { MPI_Datatype obj; };
struct Wrapped_MPI_Status   { MPI_Status   obj; };
struct Wrapped_MPI_Request  { MPI_Request  obj; };
struct Wrapped_MPI_Op       { MPI_Op       obj; };
struct Wrapped_MPI_File     { MPI_File     obj; };
struct Wrapped_MPI_Win      { MPI_Win      obj; };
