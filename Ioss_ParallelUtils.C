// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_CodeTypes.h>
#include <Ioss_ParallelUtils.h>
#include <Ioss_PropertyManager.h>
#include <Ioss_Utils.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fmt/ostream.h>
#include <numeric>
#include <string>
#include <tokenize.h>
#include <vector>

#ifdef SEACAS_HAVE_MPI
#include <Ioss_SerializeIO.h>
#endif

namespace {
#ifdef SEACAS_HAVE_MPI
  MPI_Op which_reduction(Ioss::ParallelUtils::MinMax which)
  {
    MPI_Op oper = MPI_MAX;
    if (which == Ioss::ParallelUtils::DO_MAX) {
      oper = MPI_MAX;
    }
    else if (which == Ioss::ParallelUtils::DO_MIN) {
      oper = MPI_MIN;
    }
    else if (which == Ioss::ParallelUtils::DO_SUM) {
      oper = MPI_SUM;
    }
    return oper;
  }
#endif
} // namespace

Ioss::ParallelUtils::ParallelUtils(MPI_Comm the_communicator) : communicator_(the_communicator) {}

void Ioss::ParallelUtils::add_environment_properties(Ioss::PropertyManager &properties)
{
  static bool do_print = true; // Print the first time called

  std::string env_props;
  if (get_environment("IOSS_PROPERTIES", env_props, parallel_size() > 1)) {
    // env_props string should be of the form
    // "PROP1=VALUE1:PROP2=VALUE2:..."
    std::vector<std::string> prop_val = tokenize(env_props, ":");

    int rank = parallel_rank();
    for (auto &elem : prop_val) {
      std::vector<std::string> property = tokenize(elem, "=");
      if (property.size() != 2) {
        std::ostringstream errmsg;
        fmt::print(
            errmsg,
            "ERROR: Invalid property specification found in IOSS_PROPERTIES environment variable\n"
            "       Found '{}' which is not of the correct PROPERTY=VALUE form",
            elem);
        IOSS_ERROR(errmsg);
      }
      std::string prop      = Utils::uppercase(property[0]);
      std::string value     = property[1];
      std::string up_value  = Utils::uppercase(value);
      bool        all_digit = value.find_first_not_of("0123456789") == std::string::npos;

      if (do_print && rank == 0) {
        fmt::print(Ioss::OUTPUT(), "IOSS: Adding property '{}' with value '{}'\n", prop, value);
      }
      if (all_digit) {
        int int_value = std::stoi(value);
        properties.add(Property(prop, int_value));
      }
      else if (up_value == "TRUE" || up_value == "YES") {
        properties.add(Property(prop, 1));
      }
      else if (up_value == "FALSE" || up_value == "NO") {
        properties.add(Property(prop, 0));
      }
      else {
        properties.add(Property(prop, value));
      }
    }
  }
  do_print = false;
}

bool Ioss::ParallelUtils::get_environment(const std::string &name, std::string &value,
                                          bool sync_parallel) const
{
  PAR_UNUSED(sync_parallel);
#ifdef SEACAS_HAVE_MPI
  char             *result_string = nullptr;
  std::vector<char> broadcast_string;

  int string_length = 0;

  int rank = parallel_rank();
  if (rank == 0) {
    result_string = std::getenv(name.c_str());
    string_length = result_string != nullptr ? static_cast<int>(std::strlen(result_string)) : 0;
  }

  if (sync_parallel && parallel_size() > 1) {
    MPI_Bcast(&string_length, 1, MPI_INT, 0, communicator_);

    if (string_length > 0) {
      broadcast_string.resize(string_length + 1);
      if (rank == 0) {
        Ioss::Utils::copy_string(broadcast_string.data(), result_string,
                                 static_cast<size_t>(string_length) + 1);
      }
      MPI_Bcast(broadcast_string.data(), string_length + 1, MPI_CHAR, 0, communicator_);
      value = std::string(broadcast_string.data());
    }
    else {
      value = std::string("");
    }
  }
  else {
    if (rank == 0) {
      if (string_length > 0) {
        value = std::string(result_string);
      }
      else {
        value = std::string("");
      }
    }
  }
  return string_length > 0;
#else
  char *result_string = std::getenv(name.c_str());
  if (result_string != nullptr) {
    value = std::string(result_string);
  }
  else {
    value = std::string("");
  }
  return (result_string != nullptr);
#endif
}

bool Ioss::ParallelUtils::get_environment(const std::string &name, int &value,
                                          bool sync_parallel) const
{
  std::string str_value;
  bool        success = get_environment(name, str_value, sync_parallel);
  if (success) {
    value = std::stoi(str_value);
  }
  return success;
}

bool Ioss::ParallelUtils::get_environment(const std::string &name, bool sync_parallel) const
{
  // Return true if 'name' defined, no matter what the value.
  // Return false if 'name' not defined.
  PAR_UNUSED(sync_parallel);
#ifdef SEACAS_HAVE_MPI
  char *result_string = nullptr;
  int   string_length = 0;

  int rank = Ioss::ParallelUtils::parallel_rank();
  if (rank == 0) {
    result_string = std::getenv(name.c_str());
    string_length = result_string != nullptr ? static_cast<int>(std::strlen(result_string)) : 0;
  }

  if (sync_parallel && parallel_size() > 1) {
    MPI_Bcast(&string_length, 1, MPI_INT, 0, communicator_);
  }

  return string_length > 0;
#else
  char *result_string = std::getenv(name.c_str());
  return (result_string != nullptr);
#endif
}

std::string Ioss::ParallelUtils::decode_filename(const std::string &filename,
                                                 bool               is_parallel) const
{
  std::string decoded_filename(filename);

  if (is_parallel) {
    // Running in parallel, assume nemesis and decode what the filename
    // should be for this processor.
    int processor      = parallel_rank();
    int num_processors = parallel_size();

    decoded_filename = Ioss::Utils::decode_filename(filename, processor, num_processors);
  }
  return decoded_filename;
}

int Ioss::ParallelUtils::parallel_size() const
{
  int my_size = 1;
#ifdef SEACAS_HAVE_MPI
  if (communicator_ != MPI_COMM_NULL) {
    MPI_Comm_size(communicator_, &my_size);
  }
#endif
  return my_size;
}

int Ioss::ParallelUtils::parallel_rank() const
{
  int my_rank = 0;
#ifdef SEACAS_HAVE_MPI
  if (communicator_ != MPI_COMM_NULL) {
    MPI_Comm_rank(communicator_, &my_rank);
  }
#endif
  return my_rank;
}

void Ioss::ParallelUtils::memory_stats(int64_t &min, int64_t &max, int64_t &avg) const
{
  int64_t my_memory = Ioss::Utils::get_memory_info();
  min = max = avg = my_memory;
#ifdef SEACAS_HAVE_MPI
  if (parallel_size() > 1) {
    min = global_minmax(my_memory, DO_MIN);
    max = global_minmax(my_memory, DO_MAX);
    avg = global_minmax(my_memory, DO_SUM);
    avg /= parallel_size(); // Integer truncation probably ok...
  }
#endif
}

void Ioss::ParallelUtils::hwm_memory_stats(int64_t &min, int64_t &max, int64_t &avg) const
{
  int64_t my_memory = Ioss::Utils::get_hwm_memory_info();
  min = max = avg = my_memory;
#ifdef SEACAS_HAVE_MPI
  if (parallel_size() > 1) {
    min = global_minmax(my_memory, DO_MIN);
    max = global_minmax(my_memory, DO_MAX);
    avg = global_minmax(my_memory, DO_SUM);
    avg /= parallel_size(); // Integer truncation probably ok...
  }
#endif
}

// Generate a "globally unique id" which is unique over all entities
// of a specific type over all processors.
// Used by some applications for uniquely identifying an entity.
int64_t Ioss::ParallelUtils::generate_guid(size_t id, int rank) const
{
  PAR_UNUSED(rank);
#ifdef SEACAS_HAVE_MPI
  static size_t lpow2 = 0;
  if (lpow2 == 0) {
    lpow2 = Ioss::Utils::log_power_2(parallel_size());
  }
  if (rank == -1) {
    rank = parallel_rank();
  }
  return (id << lpow2) + rank;
#else
  return id;
#endif
}

void Ioss::ParallelUtils::attribute_reduction(const int length, char buffer[]) const
{
  PAR_UNUSED(length);
  PAR_UNUSED(buffer);
#ifdef SEACAS_HAVE_MPI
  if (1 < parallel_size()) {
    static_assert(sizeof(char) == 1, "");

    std::vector<char> recv_buf(length);
    const int         success =
        MPI_Allreduce(buffer, recv_buf.data(), length, MPI_BYTE, MPI_BOR, communicator_);
    if (MPI_SUCCESS != success) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Allreduce failed", __func__);
      IOSS_ERROR(errmsg);
    }

    std::memcpy(buffer, recv_buf.data(), length);
  }
#endif
}

void Ioss::ParallelUtils::barrier() const
{
#ifdef SEACAS_HAVE_MPI
  MPI_Barrier(communicator_);
#endif
}

void Ioss::ParallelUtils::global_count(const IntVector &local_counts,
                                       IntVector       &global_counts) const
{
  // Vector 'local_counts' contains the number of objects
  // local to this processor.  On exit, global_counts
  // contains the total number of objects on all processors.
  // Assumes that ordering is the same on all processors
  global_counts.resize(local_counts.size());
#ifdef SEACAS_HAVE_MPI
  if (!local_counts.empty() && parallel_size() > 1) {
    if (Ioss::SerializeIO::isEnabled() && Ioss::SerializeIO::inBarrier()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - Attempting mpi while in barrier owned by {}", __func__,
                 Ioss::SerializeIO::getOwner());
      IOSS_ERROR(errmsg);
    }
    const int success =
        MPI_Allreduce((void *)local_counts.data(), global_counts.data(),
                      static_cast<int>(local_counts.size()), MPI_INT, MPI_SUM, communicator_);
    if (success != MPI_SUCCESS) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Allreduce failed", __func__);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    // Serial run, just copy local to global...
    std::copy(local_counts.begin(), local_counts.end(), global_counts.begin());
  }
#else
  std::copy(local_counts.begin(), local_counts.end(), global_counts.begin());
#endif
}

void Ioss::ParallelUtils::global_count(const Int64Vector &local_counts,
                                       Int64Vector       &global_counts) const
{
  // Vector 'local_counts' contains the number of objects
  // local to this processor.  On exit, global_counts
  // contains the total number of objects on all processors.
  // Assumes that ordering is the same on all processors
  global_counts.resize(local_counts.size());
#ifdef SEACAS_HAVE_MPI
  if (!local_counts.empty() && parallel_size() > 1) {
    if (Ioss::SerializeIO::isEnabled() && Ioss::SerializeIO::inBarrier()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - Attempting mpi while in barrier owned by {}", __func__,
                 Ioss::SerializeIO::getOwner());
      IOSS_ERROR(errmsg);
    }
    const int success = MPI_Allreduce((void *)local_counts.data(), global_counts.data(),
                                      static_cast<int>(local_counts.size()), MPI_LONG_LONG_INT,
                                      MPI_SUM, communicator_);
    if (success != MPI_SUCCESS) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Allreduce failed", __func__);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    // Serial run, just copy local to global...
    std::copy(local_counts.begin(), local_counts.end(), global_counts.begin());
  }
#else
  std::copy(local_counts.begin(), local_counts.end(), global_counts.begin());
#endif
}

template int Ioss::ParallelUtils::global_minmax(int, Ioss::ParallelUtils::MinMax which) const;
template unsigned int Ioss::ParallelUtils::global_minmax(unsigned int,
                                                         Ioss::ParallelUtils::MinMax which) const;
template int64_t      Ioss::ParallelUtils::global_minmax(int64_t,
                                                         Ioss::ParallelUtils::MinMax which) const;
template double Ioss::ParallelUtils::global_minmax(double, Ioss::ParallelUtils::MinMax which) const;

template <typename T>
T Ioss::ParallelUtils::global_minmax(T local_minmax, Ioss::ParallelUtils::MinMax which) const
{
  PAR_UNUSED(which);
  T minmax = local_minmax;

#ifdef SEACAS_HAVE_MPI
  if (parallel_size() > 1) {
    if (Ioss::SerializeIO::isEnabled() && Ioss::SerializeIO::inBarrier()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - Attempting mpi while in barrier owned by {}", __func__,
                 Ioss::SerializeIO::getOwner());
      IOSS_ERROR(errmsg);
    }
    static T inbuf[1], outbuf[1];
    inbuf[0] = local_minmax;

    MPI_Op oper = which_reduction(which);

    const int success =
        MPI_Allreduce((void *)&inbuf[0], &outbuf[0], 1, mpi_type(T()), oper, communicator_);
    if (success != MPI_SUCCESS) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Allreduce failed", __func__);
      IOSS_ERROR(errmsg);
    }
    minmax = outbuf[0];
  }
#endif
  return minmax;
}

/// \relates Ioss::ParallelUtils::gather
template void Ioss::ParallelUtils::gather(double, std::vector<double> &) const;
/// \relates Ioss::ParallelUtils::gather
template void Ioss::ParallelUtils::gather(int, std::vector<int> &) const;
/// \relates Ioss::ParallelUtils::gather
template void Ioss::ParallelUtils::gather(int64_t, std::vector<int64_t> &) const;
/// \relates Ioss::ParallelUtils::all_gather
template void Ioss::ParallelUtils::all_gather(int, std::vector<int> &) const;
/// \relates Ioss::ParallelUtils::all_gather
template void Ioss::ParallelUtils::all_gather(int64_t, std::vector<int64_t> &) const;
/// \relates Ioss::ParallelUtils::all_gather
template void Ioss::ParallelUtils::all_gather(std::vector<int> &, std::vector<int> &) const;
/// \relates Ioss::ParallelUtils::all_gather
template void Ioss::ParallelUtils::all_gather(std::vector<int64_t> &, std::vector<int64_t> &) const;

template <typename T> void Ioss::ParallelUtils::gather(T my_value, std::vector<T> &result) const
{
  if (parallel_rank() == 0) {
    result.resize(parallel_size());
  }
#ifdef SEACAS_HAVE_MPI
  if (parallel_size() > 1) {
    const int success = MPI_Gather((void *)&my_value, 1, mpi_type(T()), (void *)result.data(), 1,
                                   mpi_type(T()), 0, communicator_);
    if (success != MPI_SUCCESS) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Gather failed", __func__);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    result[0] = my_value;
  }
#else
  result[0] = my_value;
#endif
}

template <typename T> void Ioss::ParallelUtils::all_gather(T my_value, std::vector<T> &result) const
{
  result.resize(parallel_size());
#ifdef SEACAS_HAVE_MPI
  if (parallel_size() > 1) {
    const int success = MPI_Allgather((void *)&my_value, 1, mpi_type(T()), (void *)result.data(), 1,
                                      mpi_type(T()), communicator_);
    if (success != MPI_SUCCESS) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Allgather failed", __func__);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    result[0] = my_value;
  }
#else
  result[0] = my_value;
#endif
}

template <typename T>
void Ioss::ParallelUtils::all_gather(std::vector<T> &my_values, std::vector<T> &result) const
{
  result.resize(parallel_size() * my_values.size());
#ifdef SEACAS_HAVE_MPI
  if (parallel_size() > 1) {
    const int success =
        MPI_Allgather(my_values.data(), my_values.size(), mpi_type(T()), (void *)result.data(),
                      my_values.size(), mpi_type(T()), communicator_);
    if (success != MPI_SUCCESS) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Allgather failed", __func__);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    result = my_values;
  }
#else
  result    = my_values;
#endif
}

void Ioss::ParallelUtils::progress(const std::string &output) const
{
  static double begin = Utils::timer();

  int64_t MiB = 1024 * 1024;
  int64_t min = 0, max = 0, avg = 0;
  memory_stats(min, max, avg);

  if (parallel_rank() == 0) {
    double diff = Utils::timer() - begin;
    fmt::print(Ioss::DEBUG(), "  [{:.3f}] ({}MiB  {}MiB  {}MiB)\t{}\n", diff, min / MiB, max / MiB,
               avg / MiB, output);
  }
}

/// \relates Ioss::ParallelUtils::gather
template void Ioss::ParallelUtils::gather(std::vector<int> &my_values,
                                          std::vector<int> &result) const;
/// \relates Ioss::ParallelUtils::gather
template void Ioss::ParallelUtils::gather(std::vector<int64_t> &my_values,
                                          std::vector<int64_t> &result) const;
template <typename T>
void Ioss::ParallelUtils::gather(std::vector<T> &my_values, std::vector<T> &result) const
{
  size_t count = my_values.size();
  if (parallel_rank() == 0) {
    result.resize(count * parallel_size());
  }
#ifdef SEACAS_HAVE_MPI
  if (parallel_size() > 1) {
    const int success = MPI_Gather((void *)my_values.data(), count, mpi_type(T()),
                                   (void *)result.data(), count, mpi_type(T()), 0, communicator_);
    if (success != MPI_SUCCESS) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "{} - MPI_Gather failed", __func__);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    std::copy(my_values.begin(), my_values.end(), result.begin());
  }
#else
  std::copy(my_values.begin(), my_values.end(), result.begin());
#endif
}

/// \relates Ioss::ParallelUtils::gather
template int Ioss::ParallelUtils::gather(int num_vals, int size_per_val,
                                         std::vector<int> &my_values,
                                         std::vector<int> &result) const;
/// \relates Ioss::ParallelUtils::gather
template int Ioss::ParallelUtils::gather(int num_vals, int size_per_val,
                                         std::vector<char> &my_values,
                                         std::vector<char> &result) const;
template <typename T>
int Ioss::ParallelUtils::gather(int num_vals, int size_per_val, std::vector<T> &my_values,
                                std::vector<T> &result) const
{
  PAR_UNUSED(size_per_val);
#ifdef SEACAS_HAVE_MPI
  std::vector<int> vals_per_proc;
  gather(num_vals, vals_per_proc);

  int tot_vals = std::accumulate(vals_per_proc.begin(), vals_per_proc.end(), 0);

  std::vector<int> vals_offset(vals_per_proc);
  std::vector<int> vals_index(vals_per_proc);

  int rank = parallel_rank();
  assert(my_values.size() % size_per_val == 0);

  if (rank == 0) {
    Ioss::Utils::generate_index(vals_offset);
    for (size_t i = 0; i < vals_per_proc.size(); i++) {
      vals_index[i] *= size_per_val;
      vals_offset[i] *= size_per_val;
    }
    result.resize(tot_vals * size_per_val);
  }

  MPI_Gatherv(my_values.data(), (int)my_values.size(), mpi_type(T{}), result.data(),
              vals_index.data(), vals_offset.data(), mpi_type(T{}), 0, communicator());
#else
  int tot_vals = num_vals;
  result.resize(num_vals);
  std::copy(my_values.begin(), my_values.end(), result.begin());
#endif
  return tot_vals; // NOTE: Only valid on processor 0
}
