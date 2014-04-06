#ifndef NESTED_COMPILER_JSON
#define NESTED_COMPILER_JSON

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/fusion/include/std_pair.hpp> 
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_real.hpp>
#include <boost/spirit/include/karma_int.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/variant.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/fusion/include/at_c.hpp>
#include "json_forward.hpp"

namespace nested_container {
namespace json {

// namespaces aliases
namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;
namespace karma = boost::spirit::karma;

template<typename Container, typename Iterator> struct raw_grammar : qi::grammar<Iterator, Container(), boost::spirit::ascii::space_type> {
  template <typename T> struct strict_real_policies : qi::real_policies<T> { static bool constexpr expect_dot = true; };

  raw_grammar() : raw_grammar<Container, Iterator>::base_type(root) {
    namespace ascii = boost::spirit::ascii;
    using namespace qi::labels;
    using phoenix::at_c;
    using phoenix::push_back;

    unesc_char.add("\\a", '\a')("\\b", '\b')("\\f", '\f')("\\n", '\n')
      ("\\r", '\r')("\\t", '\t')("\\v", '\v')
      ("\\\\", '\\')("\\\'", '\'')("\\\"", '\"')
      (" ",' ')("/",'/')
      ;

    qi::int_parser< typename Container::int_type> int_parser;
    qi::uint_parser< typename Container::uint_type> uint_parser;
    qi::real_parser< typename Container::float_type, strict_real_policies<typename Container::float_type> > float_parser;

    root = object | array;
    null_value = qi::lit("null") [ _val = nullptr ];
    bool_value = qi::bool_;
    int_value = int_parser [_val = _1];
    uint_value = uint_parser [_val = _1];
    float_value = float_parser [_val = _1];
    string_value = '"' >> *(unesc_char | qi::alnum | "\\x" >> qi::hex) >> '"';
    key_value = '"' >> *qi::alnum >> '"';
    value = float_value | uint_value | int_value | bool_value | null_value | string_value;
    array = '[' >> -(value % ',') >>']';
    object = '{' >> -(object_pair % ',') >>'}';
    object_pair =  key_value  >> ':' >> (value | array | object);
  }


  using st_t = ascii::space_type;
  qi::rule<Iterator, Container(), st_t> root;
  qi::rule<Iterator, typename Container::map_type (), st_t> object;
  qi::rule<Iterator, std::pair<typename Container::key_type, Container> (), st_t> object_pair;
  qi::rule<Iterator, typename Container::vector_type (), st_t> array;
  qi::rule<Iterator, boost::optional<Container> (), st_t> value;
  qi::rule<Iterator, typename Container::key_type (), st_t> key_value;
  qi::rule<Iterator, typename Container::str_type (), st_t> string_value;
  qi::rule<Iterator, typename Container::int_type (), st_t> int_value;
  qi::rule<Iterator, typename Container::uint_type (), st_t> uint_value;
  qi::rule<Iterator, typename Container::float_type (), st_t> float_value;
  qi::rule<Iterator, bool (), st_t> bool_value;
  qi::rule<Iterator, typename std::nullptr_t (), st_t> null_value;
  qi::symbols<char const, char const> unesc_char;
};


template <typename Container, typename Iterator> struct out_grammar : karma::grammar<Iterator, Container()>
{
  using str_type      = typename Container::str_type;
  //using ostream_type  = std::basic_ostringstream<typename StreamType::value_type, typename StreamType::traits_type>;
  using key_type      = typename Container::key_type;
  using map_type      = typename Container::map_type;
  using vector_type   = typename Container::vector_type;
  using float_type    = typename Container::float_type;
  using int_type      = typename Container::int_type;
  using uint_type     = typename Container::uint_type;

  // Generators
  karma::real_generator<float_type> float_generator_;
  karma::int_generator<int_type> int_generator_;
  karma::uint_generator<uint_type> uint_generator_;

  // Template helpers
  template <typename T> using refw = std::reference_wrapper<T>;
  //template <typename T> using optional_ref = boost::optional<std::reference_wrapper<T const>>;
  //template <typename T> using optional_ref = boost::optional<T>;
  template <typename T> using optional_ref = T;
  template <typename T> using context = spirit::context<boost::fusion::cons<const T&, boost::fusion::nil_>, boost::fusion::vector0<void>>;
  template <typename T> using optional = boost::optional<T>;
  template <typename T> inline static T const& context_value(context<T> const& context) {
    return boost::fusion::at_c<0>(context.attributes);
  }
          
  // Semantic actions
  static void set_map(optional_ref<map_type>& attribute, context<Container>& context, bool& result) {
    Container const& input = context_value(context);
    if (!input.is_map()) {
      result = false;
      return;
    }
    attribute = input.raw_map();
    result = true;
  }

  static void set_vec(optional_ref<vector_type>& attribute, context<Container>& context, bool& result) {
    Container const& input = context_value(context);
    if (!input.is_vector()) {
      result = false;
      return;
    }
    attribute = input.raw_vector();
    result = true;
  }

  static void set_str(optional_ref<str_type>& attribute, context<Container>& context, bool& result) {
    Container const& input = context_value(context);
    if (!input.is_string()) {
      result = false;
      return;
    }
    attribute = input.raw_string();
    result = true;
  }

  static void set_float(float_type& attribute, context<Container>& context, bool& result) {
    Container const& input = context_value(context);
    if (!input.is_float()) {
      result = false;
      return;
    }
    attribute = input.raw_float();
    result = true;
  }

  static void set_int(int_type& attribute, context<Container>& context, bool& result) {
    Container const& input = context_value(context);
    if (!input.is_int()) {
      result = false;
      return;
    }
    attribute = input.raw_int();
    result = true;
  }

  static void set_uint(uint_type& attribute, context<Container>& context, bool& result) {
    Container const& input = context_value(context);
    if (!input.is_uint()) {
      result = false;
      return;
    }
    attribute = input.raw_uint();
    result = true;
  }

  static void set_bool(bool& attribute, context<Container>& context, bool& result) {
    Container const& input = context_value(context);
    if (!input.is_bool()) {
      result = false;
      return;
    }
    attribute = input.raw_bool();
    result = true;
  }

  out_grammar () : out_grammar ::base_type(root) {
    root = object [set_map] | array [set_vec];
    object = '{' << -(object_pair % ',') << '}';
    array = '[' << -(value % ',') << ']';
    object_pair = '"' << karma::string << '"' << -(':' << value);
    value = 
      object [set_map] 
      | array [set_vec]
      | string_value [set_str]
      | float_generator_ [set_float]
      | int_generator_ [set_int]
      | uint_generator_ [set_uint]
      | karma::bool_ [set_bool]
      //| null_value;
      | null_literal;

    string_value = '"' << karma::string << '"';
    //null_value = karma::eps << "null";
    null_literal = "null";
  }

  karma::rule<Iterator, Container()> root;
  karma::rule<Iterator, optional_ref<map_type&> ()> object;
  karma::rule<Iterator, optional_ref<std::pair<key_type, Container>> ()> object_pair;
  karma::rule<Iterator, optional_ref<vector_type&> ()> array;
  karma::rule<Iterator, optional_ref<Container> ()> value;
  karma::rule<Iterator, optional_ref<key_type> ()> key_value;
  karma::rule<Iterator, optional_ref<str_type> ()> string_value;
  //karma::rule<Iterator, std::nullptr_t()> null_value;
  karma::rule<Iterator, karma::unused_type()> null_literal;
};

// Thos traits are used for the partial karma generator
template <typename T> struct type_traits {
  static bool constexpr is_float = std::is_floating_point<T>::value;
  static bool constexpr is_int = 
    std::is_integral<T>::value 
    && std::is_signed<T>::value 
    && !std::is_same<bool,T>::value
    && !std::is_same<char,T>::value
    && !std::is_same<char16_t,T>::value
    && !std::is_same<char32_t,T>::value
    && !std::is_same<wchar_t,T>::value
    ;
  static bool constexpr is_uint = 
    std::is_integral<T>::value 
    && !std::is_signed<T>::value 
    && !std::is_same<bool,T>::value
    && !std::is_same<char,T>::value
    && !std::is_same<char16_t,T>::value
    && !std::is_same<char32_t,T>::value
    && !std::is_same<wchar_t,T>::value
    ;
};

template <typename T, typename V> struct string_conversion_traits {};

template <typename T> struct string_conversion_traits<T, typename std::enable_if<type_traits<T>::is_int, void>::type> {
  static size_t constexpr max_size = std::numeric_limits<T>::digits + 1u;
};

template <typename T> struct string_conversion_traits<T, typename std::enable_if<type_traits<T>::is_uint, void>::type> {
  static size_t constexpr max_size = std::numeric_limits<T>::digits;
};

template <> struct string_conversion_traits<float, void> {
  static size_t constexpr max_size = 3 + FLT_MANT_DIG - FLT_MIN_EXP;
};

template <> struct string_conversion_traits<double, void> {
  static size_t constexpr max_size = 3 + DBL_MANT_DIG - DBL_MIN_EXP;
};

// Utility to compute a max size of the container once converted in string
template <typename Container> struct visitor_size {
  using string_type         = typename Container::str_type;
  using key_type            = typename Container::key_type;
  using map_type            = typename Container::map_type;
  using vector_type         = typename Container::vector_type;
  using float_type          = typename Container::float_type;
  using int_type            = typename Container::int_type;
  using uint_type           = typename Container::uint_type;

  size_t size_ = 0u;

  void apply(std::nullptr_t) { size_ += 3u; }
  void apply(map_type const& v) {
    size_ += 2u; 
    bool first = true;
    for(auto const& element : v) {
      if (!first) ++size_;
      first = false;
      size_ += 3u + element.first.size();
      element.second.const_visit(*this);
    }
  }

  void apply(vector_type const& v) {
    size_ += 2; // []
    bool first = true;
    for(Container const& element : v) {
      if (!first) ++size_;
      first = false;
      element.const_visit(*this);
    }
  }

  void apply(string_type const& v) { size_ += 2u + v.size(); }
  void apply(float_type v) { size_ += string_conversion_traits<float_type, void>::max_size; }
  void apply(int_type v) { size_ += string_conversion_traits<int_type, void>::max_size; }
  void apply(uint_type v) { size_ += string_conversion_traits<uint_type, void>::max_size; }
  void apply(bool v) { size_ += 5; }

  void reset() { size_ = 0u; }
  size_t size() const { return size_; }
};

template <typename Container, typename StreamType, generation_policies GenPolicy> struct generator_impl {};

template <typename Container, typename StreamType> struct generator_impl<Container, StreamType, generation_policies::full_karma> {
  using string_type         = typename Container::str_type;
  using ostream_type        = std::basic_ostringstream<typename StreamType::value_type, typename StreamType::traits_type>;
  using key_type            = typename Container::key_type;
  using map_type            = typename Container::map_type;
  using vector_type         = typename Container::vector_type;
  using float_type          = typename Container::float_type;
  using int_type            = typename Container::int_type;
  using uint_type           = typename Container::uint_type;


  mutable visitor_size<Container> size_calculator_;
  mutable out_grammar<Container, char*> out_grammar_;

  StreamType serialize(Container const& input) const {
    // Size
    size_calculator_.reset();
    input.const_visit(size_calculator_);

    // Rendering
    char* buffer = reinterpret_cast<char*>(std::malloc(sizeof(typename string_type::value_type)*size_calculator_.size() + 1u));
    char* init_buffer = buffer;
    bool success = karma::generate(buffer, out_grammar_, input);
    std::string result(init_buffer, buffer - init_buffer);
    std::free(init_buffer);
    return result;
  }
};

template <typename Container, typename StreamType> struct generator_impl<Container, StreamType, generation_policies::visitor_partial_karma> {
  using string_type         = typename Container::str_type;
  using ostream_type        = std::basic_ostringstream<typename StreamType::value_type, typename StreamType::traits_type>;
  using key_type            = typename Container::key_type;
  using map_type            = typename Container::map_type;
  using vector_type         = typename Container::vector_type;
  using float_type          = typename Container::float_type;
  using int_type            = typename Container::int_type;
  using uint_type           = typename Container::uint_type;

  // Visitor karma
  struct visitor_karma {
    typename string_type::value_type* init_buffer_ = nullptr;
    typename string_type::value_type* buffer_ = nullptr;

    // Generators
    //using boost::spirit::karma::generate;
    karma::real_generator<float_type> float_generator_;
    karma::int_generator<int_type> int_generator_;
    karma::uint_generator<uint_type> uint_generator_;

    using container_variant = boost::variant< 
      typename Container::map_type
      , typename Container::vector_type
      , typename Container::str_type
      , typename Container::float_type
      , typename Container::int_type
      , typename Container::uint_type
      , std::nullptr_t
      , bool
      >;
    
    void apply(std::nullptr_t) { 
      karma::generate(buffer_,karma::lit("null"));  
    }
    
    void apply(map_type const& v) {
      karma::generate(buffer_,karma::char_, '{');  
      bool first = true;
      for(auto const& element : v) {
        if (!first) karma::generate(buffer_,karma::char_(','));
        first = false;
        karma::generate(buffer_, karma::char_('"') << karma::string(element.first) << karma::lit("\":"));
        element.second.const_visit(*this);
      }
      karma::generate(buffer_,karma::char_, '}');
    }
    
    void apply(vector_type const& v) {
      karma::generate(buffer_,karma::char_, '[');  
      bool first = true;
      for(Container const& element : v) {
        if (!first) karma::generate(buffer_,karma::char_, ',');
        first = false;
        element.const_visit(*this);
      }
      karma::generate(buffer_,karma::char_, ']');  
    }
    
    void apply(string_type const& v) { 
      //karma::generate(buffer_,karma::char_('"') << karma::string(v) << karma::char_('"'));  
      karma::generate(buffer_,karma::char_ << karma::string << karma::char_, '"', v, '"');  
    }
    void apply(float_type v) { 
      karma::generate(buffer_, float_generator_, v);
    }
    void apply(int_type v) { 
      karma::generate(buffer_, int_generator_, v);
    }
    void apply(uint_type v) { 
      karma::generate(buffer_, uint_generator_, v);
    }
    void apply(bool v) { 
      if (v)
        karma::generate(buffer_,karma::lit("true"));  
      else
        karma::generate(buffer_,karma::lit("false"));  
    }
  };

  mutable visitor_size<Container> size_calculator_;
  mutable visitor_karma dumper_;

  StreamType serialize(Container const& input) const {
    // Size
    size_calculator_.reset();
    input.const_visit(size_calculator_);

    // Rendering
    dumper_.buffer_ = reinterpret_cast<char*>(std::malloc(sizeof(typename string_type::value_type)*size_calculator_.size() + 1u));
    dumper_.init_buffer_ = dumper_.buffer_;
    input.const_visit(dumper_);
    std::string result(dumper_.init_buffer_, dumper_.buffer_ - dumper_.init_buffer_);
    std::free(dumper_.init_buffer_);
    return result;
  }
};

template <typename Container, typename StreamType> struct generator_impl<Container, StreamType, generation_policies::visitor_ostream> {
  using string_type         = typename Container::str_type;
  using ostream_type        = std::basic_ostringstream<typename StreamType::value_type, typename StreamType::traits_type>;
  using key_type            = typename Container::key_type;
  using map_type            = typename Container::map_type;
  using vector_type         = typename Container::vector_type;
  using float_type          = typename Container::float_type;
  using int_type            = typename Container::int_type;
  using uint_type           = typename Container::uint_type;

  // Private visitors structures
  struct visitor_ostream {
    ostream_type output_stream;
    
    void apply(std::nullptr_t) { output_stream << "null"; }
    
    void apply(map_type const& v) {
      output_stream << "{";
      bool first = true;
      for(auto const& element : v) {
        if (!first) output_stream << ",";
        first = false;
        output_stream << "\"" << element.first << "\":";
        element.second.const_visit(*this);
      }
      output_stream << "}";
    }
    
    void apply(vector_type const& v) {
      output_stream << "[";
      bool first = true;
      for(Container const& element : v) {
        if (!first) output_stream << ",";
        first = false;
        element.const_visit(*this);
      }
      output_stream << "]";
    }
    
    void apply(string_type const& v) { output_stream << "\"" << v << "\""; }
    void apply(float_type v) { output_stream << v; }
    void apply(int_type v) { output_stream << v; }
    void apply(uint_type v) { output_stream << v; }
    void apply(bool v) { output_stream << (v?"true":"false"); }

    void reset() { return output_stream.str(""); }
    StreamType render() const { return output_stream.str(); }
  };

  mutable visitor_ostream dumper_;

  StreamType serialize(Container const& input) const {
    dumper_.reset();
    input.const_visit(dumper_);
    return dumper_.render();
  }
};

template <typename Container, typename StreamType, parsing_policies GenPolicy> struct parser_impl {};
template <typename Container, typename StreamType> struct parser_impl<Container, StreamType, parsing_policies::full_spirit> {
  using string_type = typename Container::str_type;

  // Grammar member
  using grammar = raw_grammar<Container, typename StreamType::const_iterator>;
  grammar const grammar_;

  Container deserialize(StreamType const& input) const {
    Container result;
    typename string_type::const_iterator iter = input.begin();
    typename string_type::const_iterator end = input.end();
    bool success = qi::phrase_parse(iter, end, grammar_, boost::spirit::ascii::space, result);
    //bool success = qi::phrase_parse(iter, end, grammar_, boost::spirit::ascii::space);
    //std::cout << "Parsing " << (success ? "succeeded":"failed") << std::endl;
    return result;
  }
};

template<
typename Container
, typename StreamType
, generation_policies GenPolicy
, parsing_policies ParsePolicy
> 
serializer_impl_envelop<Container, StreamType, GenPolicy, ParsePolicy>::~serializer_impl_envelop() {}

template<
typename Container
, typename StreamType
, generation_policies GenPolicy
, parsing_policies ParsePolicy
> 
class serializer_impl : public serializer_impl_envelop<Container, StreamType, GenPolicy, ParsePolicy> {
  generator_impl<Container, StreamType, GenPolicy> generator_;
  parser_impl<Container, StreamType, ParsePolicy> parser_;

 public:
  ~serializer_impl() {}
  StreamType serialize(Container const& input) const override { return generator_.serialize(input); };
  Container deserialize(StreamType const& input) const override { return parser_.deserialize(input); };
};

template<typename Container, typename StreamType, generation_policies GenPolicy , parsing_policies ParsePolicy> 
serializer<Container, StreamType, GenPolicy, ParsePolicy>::serializer() 
  : serializer_impl_(new serializer_impl<Container, StreamType, GenPolicy, ParsePolicy>)
{}

template<typename Container, typename StreamType, generation_policies GenPolicy , parsing_policies ParsePolicy> 
StreamType serializer<Container, StreamType, GenPolicy, ParsePolicy>::serialize(Container const& input) const {
  return serializer_impl_->serialize(input); 
}

template<typename Container, typename StreamType, generation_policies GenPolicy , parsing_policies ParsePolicy> 
Container serializer<Container, StreamType, GenPolicy, ParsePolicy>::deserialize(StreamType const& input) const {
  return serializer_impl_->deserialize(input); 
}

}  // namespace json
}  // namespace nested_container

#endif  // __NESTED_COMPILER_JSON__
