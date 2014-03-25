#ifndef __NESTED_COMPILER_EXPERIMENTAL_CJSON__
#define __NESTED_COMPILER_EXPERIMENTAL_CJSON__

#include <string>

namespace nested_container {
namespace experimental {
namespace cjson {

template <typename T> struct cstring_conversion_traits {
  static size_t constexpr max_size = 0u;
  static std::string const printf_code;
};
template <typename T> std::string const cstring_conversion_traits<T>::printf_code = "";

template <> struct cstring_conversion_traits<int> {
  static size_t constexpr max_size = std::numeric_limits<int>::digits + 1u;
  static std::string const printf_code;
};
std::string const cstring_conversion_traits<int>::printf_code = "%d";

template <> struct cstring_conversion_traits<unsigned int> {
  static size_t constexpr max_size = std::numeric_limits<unsigned int>::digits;
  static std::string const printf_code;
};
std::string const cstring_conversion_traits<unsigned int>::printf_code = "%u";

template <> struct cstring_conversion_traits<size_t> {
  static size_t constexpr max_size = std::numeric_limits<size_t>::digits;
  static std::string const printf_code;
}; 
std::string const cstring_conversion_traits<size_t>::printf_code = "%lu";

template <> struct cstring_conversion_traits<float> {
  static size_t constexpr max_size = 3 + FLT_MANT_DIG - FLT_MIN_EXP;
  static std::string const printf_code;
};
std::string const cstring_conversion_traits<float>::printf_code = "%f";

template<typename container_type> class serializer {
  using base_visitor_type = typename container_type::const_visitor;
  using string_type       = typename container_type::str_type;
  using ostream_type      = std::basic_ostringstream<typename string_type::value_type, typename string_type::traits_type>;

  using key_type    = typename container_type::key_type;
  using map_type    = typename container_type::map_type;
  using vector_type = typename container_type::vector_type;
  using float_type  = typename container_type::float_type;
  using int_type    = typename container_type::int_type;
  using uint_type   = typename container_type::uint_type;

  struct visitor_size : public base_visitor_type {
    size_t size = 0u;
    
    virtual void apply(std::nullptr_t) override { size += 3u; }
    
    virtual void apply(map_type const& v) override {
      size += 2u;  // {}
      bool first = true;
      for(auto const& element : v) {
        if (!first) ++size;
        first = false;
        size += 3u + element.first.size();
        element.second.visit(*this);
      }
    }
    
    virtual void apply(vector_type const& v) override {
      size += 2; // []
      bool first = true;
      for(container_type const& element : v) {
        if (!first) ++size;
        first = false;
        element.visit(*this);
      }
    }
    
    virtual void apply(string_type const& v) override { size += 2u + v.size(); }
    virtual void apply(float_type v) override { size += cstring_conversion_traits<float_type>::max_size; }
    virtual void apply(int_type v) override { size += cstring_conversion_traits<int_type>::max_size; }
    virtual void apply(uint_type v) override { size += cstring_conversion_traits<uint_type>::max_size; }
    virtual void apply(bool v) override { size += 5; }
  };

  struct visitor_print : public base_visitor_type {
    char * buffer = nullptr;
    char * cur_buffer = nullptr;
    
    virtual void apply(std::nullptr_t) override { 
      std::strncpy(cur_buffer, "null", 5u);  // Do not forget the \0 end
      cur_buffer += 4u;
    }
    
    virtual void apply(map_type const& v) override {
      *cur_buffer = '{';
      ++cur_buffer;
      bool first = true;
      for(auto const& element : v) {
        if (!first) {
          *cur_buffer = ',';
          ++cur_buffer;
        }
        first = false;
        std::snprintf(cur_buffer, 4u + element.first.size(), "\"%s\":", element.first.c_str());
        cur_buffer += 3u + element.first.size();
        element.second.visit(*this);
      } 
      std::strncpy(cur_buffer,"}",2u);
      ++cur_buffer;
    } 
    
    virtual void apply(vector_type const& v) override {
      *cur_buffer = '[';
      ++cur_buffer;
      bool first = true;
      for(container_type const& element : v) {
        if (!first) {
          *cur_buffer = ',';
          ++cur_buffer;
        }
        first = false;
        element.visit(*this);
      } 
      std::strncpy(cur_buffer,"]",2u);
      ++cur_buffer;
    }
    
    virtual void apply(string_type const& v) override { 
      std::snprintf(cur_buffer, 3u + v.size(), "\"%s\"", v.c_str());
      cur_buffer += 2u + v.size();
    }

    virtual void apply(float_type v) override { 
      int char_written = std::snprintf(
          cur_buffer
          , cstring_conversion_traits<float_type>::max_size
          , cstring_conversion_traits<float_type>::printf_code.c_str()
          , v);
      if (0 < char_written) cur_buffer += char_written;
    }

    virtual void apply(int_type v) override { 
      int char_written = std::snprintf(
          cur_buffer
          , cstring_conversion_traits<int_type>::max_size
          , cstring_conversion_traits<int_type>::printf_code.c_str()
          , v);
      if (0 < char_written) cur_buffer += char_written;
    }

    virtual void apply(uint_type v) override { 
      int char_written = std::snprintf(
          cur_buffer
          , cstring_conversion_traits<uint_type>::max_size
          , cstring_conversion_traits<uint_type>::printf_code.c_str()
          , v);
      if (0 < char_written) cur_buffer += char_written;
    }

    virtual void apply(bool v) override { 
      if (v) {
        std::strncpy(cur_buffer, "true", 5u);
        cur_buffer += 4u;
      }
      else {
        std::strncpy(cur_buffer, "false", 5u);
        cur_buffer += 5u;
      }
    }
  }; 

 public:
  string_type serialize(container_type const& container) const {
    visitor_size size_calculator;
    container.visit(size_calculator);
    visitor_print printer;
    printer.buffer = reinterpret_cast<char*>(std::malloc(size_calculator.size));
    printer.cur_buffer = printer.buffer;
    container.visit(printer);
    std::string result(printer.buffer);
    std::free(printer.buffer);
    return result;
  }

  container_type deserialize(string_type const& input) {
  }
};

}  // namespace cjson
}  // namespace experimental
}  // namespace nested_container

#endif  // __NESTED_COMPILER_EXPERIMENTAL_CJSON__