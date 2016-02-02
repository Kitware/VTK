#ifndef DIY_STORAGE_HPP
#define DIY_STORAGE_HPP

#include <string>
#include <map>
#include <fstream>

#include <unistd.h>     // mkstemp() on Mac
#include <cstdlib>      // mkstemp() on Linux
#include <cstdio>       // remove()
#include <fcntl.h>

#include "serialization.hpp"
#include "thread.hpp"

namespace diy
{
  namespace detail
  {
    typedef       void  (*Save)(const void*, BinaryBuffer& buf);
    typedef       void  (*Load)(void*,       BinaryBuffer& buf);

    struct FileBuffer: public BinaryBuffer
    {
                          FileBuffer(FILE* file_): file(file_), head(0), tail(0)    {}

      // TODO: add error checking
      virtual inline void save_binary(const char* x, size_t count)    { fwrite(x, 1, count, file); head += count; }
      virtual inline void load_binary(char* x, size_t count)          { fread(x, 1, count, file); }
      virtual inline void load_binary_back(char* x, size_t count)     { fseek(file, tail, SEEK_END); fread(x, 1, count, file); tail += count; fseek(file, head, SEEK_SET); }

      size_t              size() const                                { return head; }

      FILE*  file;
      size_t head, tail;  // tail is used to support reading from the back;
                          // the mechanism is a little awkward and unused, but should work if needed
    };
  }

  class ExternalStorage
  {
    public:
      virtual int   put(MemoryBuffer& bb)                               =0;
      virtual int   put(const void* x, detail::Save save)               =0;
      virtual void  get(int i, MemoryBuffer& bb, size_t extra = 0)      =0;
      virtual void  get(int i, void* x, detail::Load load)              =0;
      virtual void  destroy(int i)                                      =0;
  };

  class FileStorage: public ExternalStorage
  {
    private:
      struct FileRecord
      {
        size_t          size;
        std::string     name;
      };

    public:
                    FileStorage(const std::string& filename_template = "/tmp/DIY.XXXXXX"):
                      filename_templates_(1, filename_template),
                      count_(0), current_size_(0), max_size_(0)         {}

                    FileStorage(const std::vector<std::string>& filename_templates):
                      filename_templates_(filename_templates),
                      count_(0), current_size_(0), max_size_(0)         {}

      virtual int   put(MemoryBuffer& bb)
      {
        std::string     filename;
        int fh = open_random(filename);

        //fprintf(stdout, "FileStorage::put(): %s; buffer size: %lu\n", filename.c_str(), bb.size());

        size_t sz = bb.buffer.size();
        size_t written = write(fh, &bb.buffer[0], sz);
        if (written < sz || written == (size_t)-1)
          fprintf(stderr, "Warning: could not write the full buffer to %s: written = %lu; size = %lu\n", filename.c_str(), written, sz);
        fsync(fh);
        close(fh);
        bb.wipe();

#if 0       // double-check the written file size: only for extreme debugging
        FILE* fp = fopen(filename.c_str(), "r");
        fseek(fp, 0L, SEEK_END);
        int fsz = ftell(fp);
        if (fsz != sz)
            fprintf(stderr, "Warning: file size doesn't match the buffer size, %d vs %d\n", fsz, sz);
        fclose(fp);
#endif

        return make_file_record(filename, sz);
      }

      virtual int    put(const void* x, detail::Save save)
      {
        std::string     filename;
        int fh = open_random(filename);

        detail::FileBuffer fb(fdopen(fh, "w"));
        save(x, fb);
        size_t sz = fb.size();
        fclose(fb.file);
        fsync(fh);

        return make_file_record(filename, sz);
      }

      virtual void   get(int i, MemoryBuffer& bb, size_t extra)
      {
        FileRecord fr = extract_file_record(i);

        //fprintf(stdout, "FileStorage::get(): %s\n", fr.name.c_str());

        bb.buffer.reserve(fr.size + extra);
        bb.buffer.resize(fr.size);
        int fh = open(fr.name.c_str(), O_RDONLY | O_SYNC, 0600);
        read(fh, &bb.buffer[0], fr.size);
        close(fh);

        remove_file(fr);
      }

      virtual void   get(int i, void* x, detail::Load load)
      {
        FileRecord fr = extract_file_record(i);

        //int fh = open(fr.name.c_str(), O_RDONLY | O_SYNC, 0600);
        int fh = open(fr.name.c_str(), O_RDONLY, 0600);
        detail::FileBuffer fb(fdopen(fh, "r"));
        load(x, fb);
        fclose(fb.file);

        remove_file(fr);
      }

      virtual void  destroy(int i)
      {
        FileRecord      fr;
        {
          CriticalMapAccessor accessor = filenames_.access();
          fr = (*accessor)[i];
          accessor->erase(i);
        }
        remove(fr.name.c_str());
        (*current_size_.access()) -= fr.size;
      }

      int           count() const               { return (*count_.const_access()); }
      size_t        current_size() const        { return (*current_size_.const_access()); }
      size_t        max_size() const            { return (*max_size_.const_access()); }

                    ~FileStorage()
      {
        for (FileRecordMap::const_iterator it =  filenames_.const_access()->begin();
                                           it != filenames_.const_access()->end();
                                         ++it)
        {
          remove(it->second.name.c_str());
        }
      }

    private:
      int           open_random(std::string& filename) const
      {
        if (filename_templates_.size() == 1)
            filename = filename_templates_[0].c_str();
        else
        {
            // pick a template at random (very basic load balancing mechanism)
            filename  = filename_templates_[std::rand() % filename_templates_.size()].c_str();
        }
#ifdef __MACH__
        // TODO: figure out how to open with O_SYNC
        int fh = mkstemp(const_cast<char*>(filename.c_str()));
#else
        int fh = mkostemp(const_cast<char*>(filename.c_str()), O_WRONLY | O_SYNC);
#endif

        return fh;
      }

      int           make_file_record(const std::string& filename, size_t sz)
      {
        int res = (*count_.access())++;
        FileRecord  fr = { sz, filename };
        (*filenames_.access())[res] = fr;

        // keep track of sizes
        critical_resource<size_t>::accessor     cur = current_size_.access();
        *cur += sz;
        critical_resource<size_t>::accessor     max = max_size_.access();
        if (*cur > *max)
            *max = *cur;

        return res;
      }

      FileRecord    extract_file_record(int i)
      {
        CriticalMapAccessor accessor = filenames_.access();
        FileRecord fr = (*accessor)[i];
        accessor->erase(i);
        return fr;
      }

      void          remove_file(const FileRecord& fr)
      {
        remove(fr.name.c_str());
        (*current_size_.access()) -= fr.size;
      }

    private:
      typedef           std::map<int, FileRecord>                   FileRecordMap;
      typedef           critical_resource<FileRecordMap>            CriticalMap;
      typedef           CriticalMap::accessor                       CriticalMapAccessor;

    private:
      std::vector<std::string>      filename_templates_;
      CriticalMap                   filenames_;
      critical_resource<int>        count_;
      critical_resource<size_t>     current_size_, max_size_;
  };
}

#endif
