#ifndef DIY_STORAGE_HPP
#define DIY_STORAGE_HPP

#include <string>
#include <map>
#include <fstream>
#include <fcntl.h>

#include "serialization.hpp"
#include "thread.hpp"
#include "log.hpp"
#include "io/utils.hpp"

namespace diy
{
  namespace detail
  {
    using Save = std::function<void(const void*, BinaryBuffer&)>;
    using Load = std::function<void(void*, BinaryBuffer&)>;

    struct FileBuffer: public BinaryBuffer
    {
                          FileBuffer(FILE* file_): file(file_), head(0), tail(0)    {}

      // TODO: add error checking
      virtual inline void save_binary(const char* x, size_t count) override   { fwrite(x, 1, count, file); head += count; }
      virtual inline void append_binary(const char* x, size_t count) override
      {
          auto temp_pos = ftell(file);
          fseek(file, static_cast<long>(tail), SEEK_END);
          fwrite(x, 1, count, file);
          tail += count;
          fseek(file, temp_pos, SEEK_SET);
      }
      virtual inline void load_binary(char* x, size_t count) override         { auto n = fread(x, 1, count, file); DIY_UNUSED(n);}
      virtual inline void load_binary_back(char* x, size_t count) override    { fseek(file, static_cast<long>(tail), SEEK_END); auto n = fread(x, 1, count, file); tail += count; fseek(file, static_cast<long>(head), SEEK_SET); DIY_UNUSED(n);}
      virtual inline char* grow(size_t) override                              { throw std::runtime_error("Cannot grow a FileBuffer"); }
      virtual inline char* advance(size_t) override                           { throw std::runtime_error("Cannot advance a FileBuffer"); }

      // TODO: for now, we just throw, but obviously it should be possile to store binary blobs in a file; might want to fall back
      using Blob = BinaryBlob;
      virtual inline void save_binary_blob(const char*, size_t) override      { throw std::runtime_error("Cannot save binary blobs in a FileBuffer"); }

      virtual inline void save_binary_blob(const char*, size_t, Blob::Deleter) override { throw std::runtime_error("Cannot save binary blobs in a FileBuffer"); }

      virtual inline Blob load_binary_blob() override                         { throw std::runtime_error("Cannot load binary blobs from a FileBuffer"); }

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

      virtual int   put(MemoryBuffer& bb) override
      {
        auto log = get_logger();
        std::string     filename;
        int fh = open_random(filename);

        log->debug("FileStorage::put(): {}; buffer size: {}", filename, bb.size());

        size_t sz = bb.buffer.size();
#if defined(_WIN32)
        using r_type = int;
        r_type written = _write(fh, &bb.buffer[0], static_cast<unsigned int>(sz));
#else
        using r_type = ssize_t;
        r_type written = write(fh, &bb.buffer[0], sz);
#endif
        if (written < static_cast<r_type>(sz) || written == r_type(-1))
          log->warn("Could not write the full buffer to {}: written = {}; size = {}", filename, written, sz);
        io::utils::close(fh);
        bb.wipe();

#if 0       // double-check the written file size: only for extreme debugging
        FILE* fp = fopen(filename.c_str(), "r");
        fseek(fp, 0L, SEEK_END);
        int fsz = ftell(fp);
        if (fsz != sz)
            log->warn("file size doesn't match the buffer size, {} vs {}", fsz, sz);
        fclose(fp);
#endif

        return make_file_record(filename, sz);
      }

      virtual int    put(const void* x, detail::Save save) override
      {
        std::string     filename;
        int fh = open_random(filename);
#if defined(_WIN32)
        detail::FileBuffer fb(_fdopen(fh, "wb"));
#else
        detail::FileBuffer fb(fdopen(fh, "w"));
#endif
        save(x, fb);
        size_t sz = fb.size();
        fclose(fb.file);
        io::utils::sync(fh);

        return make_file_record(filename, sz);
      }

      virtual void   get(int i, MemoryBuffer& bb, size_t extra) override
      {
        FileRecord fr = extract_file_record(i);

        get_logger()->debug("FileStorage::get(): {}", fr.name);

        bb.buffer.reserve(fr.size + extra);
        bb.buffer.resize(fr.size);
#if defined(_WIN32)
        int fh = -1;
        _sopen_s(&fh, fr.name.c_str(), _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD);
        _read(fh, &bb.buffer[0], static_cast<unsigned int>(fr.size));
#else
        int fh = open(fr.name.c_str(), O_RDONLY | O_SYNC, 0600);
        auto n = read(fh, &bb.buffer[0], fr.size);
        DIY_UNUSED(n);
#endif
        io::utils::close(fh);
        remove_file(fr);
      }

      virtual void   get(int i, void* x, detail::Load load) override
      {
        FileRecord fr = extract_file_record(i);

        //int fh = open(fr.name.c_str(), O_RDONLY | O_SYNC, 0600);
#if defined(_WIN32)
        int fh = -1;
        _sopen_s(&fh, fr.name.c_str(), _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD);
        detail::FileBuffer fb(_fdopen(fh, "rb"));
#else
        int fh = open(fr.name.c_str(), O_RDONLY, 0600);
        detail::FileBuffer fb(fdopen(fh, "r"));
#endif
        load(x, fb);
        fclose(fb.file);

        remove_file(fr);
      }

      virtual void  destroy(int i) override
      {
        FileRecord      fr;
        {
          CriticalMapAccessor accessor = filenames_.access();
          fr = (*accessor)[i];
          accessor->erase(i);
        }
        io::utils::remove(fr.name);
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
          io::utils::remove(it->second.name);
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
            filename  = filename_templates_[static_cast<size_t>(std::rand()) % filename_templates_.size()].c_str();
        }
        int fh = diy::io::utils::mkstemp(filename);
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
        io::utils::remove(fr.name);
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
