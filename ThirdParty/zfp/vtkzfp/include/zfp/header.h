class header {
public:
  class exception : public std::runtime_error {
  public:
    exception(const std::string& msg) : runtime_error(msg) {}

    virtual ~exception() throw (){}
  };

  static void concat_sentence(std::string& s, const std::string& msg)
  {
    if (!s.empty())
      s += " ";
    s += msg;
  }

  uchar buffer[BITS_TO_BYTES(ZFP_HEADER_SIZE_BITS)];
};

