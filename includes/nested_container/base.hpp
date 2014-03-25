#ifndef __NESTED_COMPILER_BASE__
#define __NESTED_COMPILER_BASE__

#include <map>
#include <vector>
#include <type_traits>
#include <sstream>
#include <utility>
#include <cassert>
#include <exception>
#include <initializer_list>
#include <tuple>
#include <typeinfo>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cfloat>
#include <limits>
#include <iostream>

namespace nested_container {

// boost lexical cast, simple non-throwing version
// TODO: Replace with Karma generators and Qi parsers
template<typename Target, typename Source> 
typename std::enable_if<std::is_fundamental<Source>::value, Target>::type
lexical_cast(Source arg) {
  std::stringstream interpreter;
  Target result;

  if(!(interpreter << arg)
      || !(interpreter >> result)
      || !(interpreter >> std::ws).eof())
    return Target();

  return result;
}

template<typename Target, typename Source> 
typename std::enable_if<!std::is_fundamental<Source>::value, Target>::type
lexical_cast(Source const& arg) {
  std::stringstream interpreter;
  Target result;

  if(!(interpreter << arg)
      || !(interpreter >> result)
      || !(interpreter >> std::ws).eof())
    return Target();

  return result;
}

template <typename key_type, typename value_type> using std_map_default_allocators = std::map<key_type, value_type>;
template <typename value_type> using std_vector_default_allocators = std::vector<value_type>;
template <class Container> class attr_init;
template <class Container> class vector_element_init;

template <
  typename Key = std::string
  , class String = std::string
  , typename Int = int
  , typename UInt = unsigned int
  , typename Float = float
  , template <typename InnerKey, class This> class MapTemplate = std_map_default_allocators
  , template <typename This> class VectorTemplate = std_vector_default_allocators
  >
class basic_container final {
 public:
  // Public type declarations
  using key_type = Key;
  using str_type = String;
  using int_type = Int;
  using uint_type = UInt;
  using float_type = Float;
  using map_type = MapTemplate<key_type, basic_container>;
  using vector_type = VectorTemplate<basic_container>;
  using vec_size_type = typename vector_type::size_type;

  class exception_type : std::exception {
    const char* what() const noexcept { return "NestedContainer exception."; }
  };

 private:
  using Map = map_type; 
  using Vector = vector_type;
  using Null = std::nullptr_t;

  enum class value_type : unsigned char  {
    null = '0'
    , map 
    , vector 
    , string
    , floating
    , integer
    , unsigned_integer
    , boolean
  };

  union value final {
    value() {}
    ~value() {}

    Map* dict_;
    Vector* vector_;
    String str_;
    Float float_;
    Int int_;
    UInt uint_;
    bool bool_;
  };

  template<typename A, typename B> using eq = std::is_same<A,B>;

  template <typename T> struct type_proxy {};
  template <bool B> struct bool_proxy {};

  template <typename Member> struct type_traits { 
    using T = typename std::remove_cv<typename std::remove_reference<Member>::type>::type;
    using pure_type = T;
    static constexpr value_type type_value() {
      return eq<bool, T>::value ? value_type::boolean 
        : eq<Int, T>::value ? value_type::integer
        : eq<UInt, T>::value ? value_type::unsigned_integer
        : eq<Float, T>::value ? value_type::floating
        : eq<String, T>::value ? value_type::string 
        : eq<Vector, T>::value ? value_type::vector
        : eq<Map, T>::value ? value_type::map
        : value_type::null;
    }

    static bool constexpr is_from_container = eq<bool, T>::value
        || eq<Int, T>::value 
        || eq<UInt, T>::value 
        || eq<Float, T>::value 
        || eq<String, T>::value 
        || eq<Vector, T>::value 
        || eq<Map, T>::value
        || eq<std::nullptr_t, T>::value
        || eq<basic_container, T>::value;

    static bool constexpr is_collection = eq<Vector, T>::value || eq<Map, T>::value;
    static bool constexpr is_lexical = eq<bool, T>::value || eq<Int, T>::value || eq<UInt, T>::value || eq<Float, T>::value || eq<String, T>::value;
    static bool constexpr is_null = eq<Null, T>::value;
    static bool constexpr is_index = eq<Key, T>::value || eq<vec_size_type, T>::value;
    static bool constexpr is_pure = eq<T, Member>::value;
    static bool constexpr is_self= eq<basic_container, T>::value;

    static_assert(is_from_container, "Type must be one of container's internal types");
  };

  static bool is_lexical(value_type type) { 
    return value_type::boolean == type 
      || value_type::integer == type
      || value_type::unsigned_integer == type
      || value_type::floating == type
      || value_type::string == type;
  }

  static bool is_collection(value_type type) { 
    return value_type::map == type || value_type::vector == type;
  }
      
  value value_;
  value_type type_ = value_type::null;

  void clear() {
    switch (type_) {
      case value_type::map:
        delete value_.dict_;
        break;
      case value_type::vector:
        delete value_.vector_;
        break;
      case value_type::string:
        value_.str_.~str_type();
        break;
      default:
        break;
    }
  }

  // Compile time initializers
  template <typename T> inline void init_member() { init_member(type_proxy<T>()); }
  inline void init_member(type_proxy<std::nullptr_t>) {}
  inline void init_member(type_proxy<Map>) { value_.dict_ = new Map; }
  inline void init_member(type_proxy<Vector>) { value_.vector_ = new Vector; }
  inline void init_member(type_proxy<String>) { new (&(value_.str_)) String; }
  inline void init_member(type_proxy<Float>) { value_.float_ = 0.f; }
  inline void init_member(type_proxy<Int>) { value_.int_ = 0; }
  inline void init_member(type_proxy<UInt>) { value_.uint_ = 0u; }
  inline void init_member(type_proxy<bool>) { value_.bool_ = false; }

  // Init with value -  trivial types
  inline void init_member(std::nullptr_t) {}
  inline void init_member(Float v) { value_.float_ = v; }
  inline void init_member(Int v) { value_.int_ = v; }
  inline void init_member(UInt v) { value_.uint_ = v; }
  inline void init_member(bool v) { value_.bool_ = v; }

  // Init with value - move
  inline void init_member(Map&& v) { value_.dict_ = new Map(v); }
  inline void init_member(Vector&& v) { value_.vector_ = new Vector(v); }
  inline void init_member(String&& v) { new (&(value_.str_)) String(v); }
  inline void init_member(basic_container&& c) {
    switch_to_type(c.type_);
    switch (type_) {
      case value_type::null:
        break; 
      case value_type::map:
        value_.dict_ = c.value_.dict_;
        c.value_.dict_ = nullptr;  // Useless in theory, just in case
        break;
      case value_type::vector:
        value_.vector_ = c.value_.vector_;
        c.value_.vector_ = nullptr;  // Useless in theory, just in case
        break;
      case value_type::string:
        value_.str_ = std::move(c.value_.str_);
        break;
      case value_type::floating:
        value_.float_ = c.value_.float_;
        break;
      case value_type::integer:
        value_.int_ = c.value_.int_;
        break;
      case value_type::unsigned_integer:
        value_.uint_ = c.value_.uint_;
        break;
      case value_type::boolean:
        value_.bool_ = c.value_.bool_;
        break;
      default:
        break;
    }

    c.type_ = value_type::null;
  }

  // Init with value - copy
  inline void init_member(Map const& v) { value_.dict_ = new Map(v); }
  inline void init_member(Vector const& v) { value_.vector_ = new Vector(v); }
  inline void init_member(String const& v) { new (&(value_.str_)) String(v); }
  inline void init_member(basic_container const& c) {
    value_type previous_type = type_;
    if (type_ != c.type_ && is_collection(type_) && is_collection(c.type_)){
      // We have to handle the special case of an affectation from a container
      // sub element to itself : deletion must be defered not to read
      // a deleted memory area
      if(value_type::map == type_) {
        Map* defered_map = nullptr;
        init_member(*c.value_.vector_);
        delete defered_map;
      }
      else {
        Vector* defered_vector = nullptr;
        init_member(*c.value_.dict_);
        delete defered_vector;
      }
      type_ = c.type_;
    }
    else {
      switch_to_type(c.type_);
      switch (type_) {
        case value_type::null:
          break; 
        case value_type::map:
          *value_.dict_ = *c.value_.dict_;
          break;
        case value_type::vector:
          *value_.vector_ = *c.value_.vector_;
          break;
        case value_type::string:
          value_.str_ = c.value_.str_;
          break;
        case value_type::floating:
          value_.float_ = c.value_.float_;
          break;
        case value_type::integer:
          value_.int_ = c.value_.int_;
          break;
        case value_type::unsigned_integer:
          value_.uint_ = c.value_.uint_;
          break;
        case value_type::boolean:
          value_.bool_ = c.value_.bool_;
          break;
        default:
          break;
      }
    }
  }

  // Runtime version
  void init_member(value_type target_type) {
    switch (target_type) {
      case value_type::map:
        init_member<Map>();
        break;
      case value_type::vector:
        init_member<Vector>();
        break;
      case value_type::string:
        init_member<String>();
        break;
      case value_type::floating:
        init_member<Float>();
        break;
      case value_type::integer:
        init_member<Int>();
        break;
      case value_type::unsigned_integer:
        init_member<UInt>();
        break;
      case value_type::boolean:
        init_member<bool>();
        break;
      default:
        break;
    }
  }

  // When target type is known at compilation
  template <typename T> void switch_to_type() {
    value_type constexpr target_type = type_traits<T>::type_value();
    if (target_type == type_) return;
    clear();
    init_member<T>();
    type_ = target_type;
  }

  template <typename T> void switch_to_type(T&& value) {
    value_type constexpr target_type = type_traits<T>::type_value();
    if (target_type == type_) {
      ref_to<T>() = std::forward<T>(value);
      return;
    }
    clear();
    init_member(std::forward<T>(value));
    type_ = target_type;
  }

  // When target type is known at runtime only
  void switch_to_type(value_type target_type) {
    if (target_type == type_) return;
    clear();
    init_member(target_type);
    type_ = target_type;
  }


  template <typename T> basic_container(type_proxy<T>) : type_(type_traits<T>::type_value()) { 
    init_member(type_proxy<T>());
  }

  template <typename T> basic_container(type_proxy<typename type_traits<T>::pure_type>, T&& arg) : type_(type_traits<T>::type_value()) { 
    init_member(std::forward<T>(arg));
  }

  // Accessors, private, not protected against bad behavior, checks must be done before
  template <typename T> inline T* ptr_to() { return ptr_to(type_proxy<T>()); }
  inline Map* ptr_to(type_proxy<Map>) { return value_.dict_; }
  inline Vector* ptr_to(type_proxy<Vector>) { return value_.vector_; }
  inline String* ptr_to(type_proxy<String>) { return &value_.str_; }
  inline Float* ptr_to(type_proxy<Float>) { return &value_.float_; }
  inline Int* ptr_to(type_proxy<Int>) { return &value_.int_; }
  inline UInt* ptr_to(type_proxy<UInt>) { return &value_.uint_; }
  inline bool* ptr_to(type_proxy<bool>) { return &value_.bool_; }

  template <typename T> inline T const* ptr_to() const { return ptr_to(type_proxy<T>()); }
  inline Map const* ptr_to(type_proxy<Map>) const { return value_.dict_; }
  inline Vector const* ptr_to(type_proxy<Vector>) const { return value_.vector_; }
  inline String const* ptr_to(type_proxy<String>) const { return &value_.str_; }
  inline Float const* ptr_to(type_proxy<Float>) const { return &value_.float_; }
  inline Int const* ptr_to(type_proxy<Int>) const { return &value_.int_; }
  inline UInt const* ptr_to(type_proxy<UInt>) const { return &value_.uint_; }
  inline bool const* ptr_to(type_proxy<bool>) const { return &value_.bool_; }

  template <typename T> inline T& ref_to() { return ref_to(type_proxy<T>()); }
  inline Map& ref_to(type_proxy<Map>) { return *value_.dict_; }
  inline Vector& ref_to(type_proxy<Vector>) { return *value_.vector_; }
  inline String& ref_to(type_proxy<String>) { return value_.str_; }
  inline Float& ref_to(type_proxy<Float>) { return value_.float_; }
  inline Int& ref_to(type_proxy<Int>) { return value_.int_; }
  inline UInt& ref_to(type_proxy<UInt>) { return value_.uint_; }
  inline bool& ref_to(type_proxy<bool>) { return value_.bool_; }

  template <typename T> inline T const& ref_to() const { return ref_to(type_proxy<T>()); }
  inline Map const& ref_to(type_proxy<Map>) const { return *value_.dict_; }
  inline Vector const& ref_to(type_proxy<Vector>) const { return *value_.vector_; }
  inline String const& ref_to(type_proxy<String>) const { return value_.str_; }
  inline Float const& ref_to(type_proxy<Float>) const { return value_.float_; }
  inline Int const& ref_to(type_proxy<Int>) const { return value_.int_; }
  inline UInt const& ref_to(type_proxy<UInt>) const { return value_.uint_; }
  inline bool const& ref_to(type_proxy<bool>) const { return value_.bool_; }

  // [] accessors, Key != size_type version
  basic_container const& access_collection(bool_proxy<false>, Key const& index) const {
    if (!is_map()) throw exception_type();
    return ref_to<Map>().at(index);
  }

  basic_container const& access_collection(bool_proxy<false>, vec_size_type index) const {
    if (!is_vector()) throw exception_type();
    return ref_to<Vector>().at(index);
  }

  basic_container& access_collection(bool_proxy<false>, Key const& index) noexcept {
    if (!is_map()) switch_to_type<Map>();
    return ref_to<Map>()[index];
  }

  basic_container& access_collection(bool_proxy<false>, Key&& index) noexcept {
    if (!is_map()) switch_to_type<Map>();
    return ref_to<Map>()[std::move(index)];
  }

  basic_container& access_collection(bool_proxy<false>, vec_size_type index) noexcept {
    if (!is_vector()) switch_to_type<Vector>();
    if (ref_to<Vector>().size() <= index) ref_to<Vector>().resize(index);
    return ref_to<Vector>()[index];
  }

  //template <typename T> convert_to
  template <typename T> T convert_to(type_proxy<T>) const { 
    static_assert(type_traits<T>::is_pure, "Type must not be a reference nor have cv-qualifiers");
    value_type target_type = type_traits<T>::type_value();
    if (target_type == type_) {
      return ref_to<T>();
    }
    else if (is_lexical(target_type) && is_lexical(type_)) {
      if (value_type::string == type_) {  // lexical cast
        return lexical_cast<T>(ref_to<String>());
      }
      else {  // static cast
        switch (type_) {
          case value_type::floating:
            return static_cast<T>(ref_to<Float>());
          case value_type::integer:
            return static_cast<T>(ref_to<Int>());
          case value_type::unsigned_integer:
            return static_cast<T>(ref_to<UInt>());
          case value_type::boolean:
            return static_cast<T>(ref_to<bool>());
          default:
            break;
        }
      }
    }
    return T();
  }

  // Handling the particular case of types not supporting casts
  std::nullptr_t convert_to(type_proxy<std::nullptr_t>) const { 
    return nullptr;
  }
  str_type convert_to(type_proxy<String>) const { 
    if (value_type::string == type_) {
      return ref_to<String>();
    }
    else if (is_lexical(type_)) {
      switch (type_) {
        case value_type::floating:
          return lexical_cast<String>(ref_to<Float>());
        case value_type::integer:
          std::cout << "Whatssuuuppp  ????" << std::endl;
          return lexical_cast<String>(ref_to<Int>());
        case value_type::unsigned_integer:
          return lexical_cast<String>(ref_to<UInt>());
        case value_type::boolean:
          return lexical_cast<String>(ref_to<bool>());
        default:
          break;
      }
    }
    return String();
  }

  Map convert_to(type_proxy<Map>) const { 
    if (value_type::map == type_) return ref_to<Map>();
    return Map();
  }

  Vector convert_to(type_proxy<Vector>) const { 
    if (value_type::vector == type_) return ref_to<Vector>();
    return Vector();
  }

 public:
  basic_container() {};
  basic_container(basic_container&& c) : basic_container(type_proxy<basic_container>(), std::move(c)) {}
  basic_container(basic_container const& c) : basic_container(type_proxy<basic_container>(), c) {}
  basic_container(basic_container& c) : basic_container(type_proxy<basic_container>(), c) {}
  // Constructor from other types

  // Array of chars would be nice to support but disambiguate from char const* is hard
  //template <size_t Length> basic_container(typename str_type::value_type const (& arg ) [Length] ) : basic_container(type_proxy<str_type>(), str_type(arg, Length)) {}

  // Handling char* case
  basic_container(typename str_type::value_type const* arg) : basic_container(type_proxy<str_type>(), str_type(arg)) {}
  template <typename T> basic_container(T&& arg) : basic_container(type_proxy<typename type_traits<decltype(arg)>::pure_type>(), std::forward<T>(arg)) {}


  // Initializer lists
  basic_container(std::initializer_list< attr_init<basic_container> > list) : basic_container(type_proxy<Map>()){
    for(auto const& elem : list) ref_to<Map>().emplace(elem.key(), elem.value());
  }
  basic_container(std::initializer_list<vector_element_init<basic_container>> list) : basic_container(type_proxy<Vector>()){
    ref_to<Vector>().reserve(list.size());
    for(auto const& element : list) ref_to<Vector>().emplace_back(element.value());
  }

  static basic_container init_map(std::initializer_list<std::pair<Key, basic_container>> list) {
    basic_container container((type_proxy<Map>()));
    for(std::pair<Key, basic_container> const& elem : list) container.ref_to<Map>().emplace(elem);
    return container;
  }
  
  static basic_container init_vec(std::initializer_list<basic_container> list) {
    basic_container container((type_proxy<Vector>()));
    container.ref_to<Vector>().reserve(list.size());
    for(basic_container const& element : list) container.ref_to<Vector>().emplace_back(element);
    return container;
  }


  // Helper to initialize the container to a given type
  template <typename T> static basic_container init() { return basic_container(type_proxy<T>()); }

  ~basic_container() { clear(); }  // virtual not needed, this class is final

  // Assignement from another type
  //template <typename T> typename std::enable_if<!type_traits<T>::is_self, basic_container&>::type operator=(T&& arg) { 
  basic_container& operator= (basic_container const& c) { init_member(c); return *this; }
  basic_container& operator= (basic_container&& c) { init_member(c); return *this; }
  basic_container& operator= (basic_container& c) { init_member(c); return *this; }
  template <size_t Length> basic_container& operator=(typename str_type::value_type const (& arg) [Length] ) {
    switch_to_type<str_type>();
    init_member(str_type(arg, Length));
    return *this;
  }
  template <typename T> basic_container& operator=(T&& arg) { 
    switch_to_type<typename type_traits<decltype(arg)>::pure_type>();
    init_member(std::forward<T>(arg));
    return *this;
  }
  // Handling char* case
  basic_container& operator=(typename str_type::value_type const* arg) {
    switch_to_type<str_type>();
    init_member(str_type(arg));
    return *this;
  }

  inline bool is_null() const noexcept { return value_type::null == type_; }
  inline bool is_map() const noexcept { return value_type::map == type_; }
  inline bool is_vector() const noexcept { return value_type::vector == type_; }
  inline bool is_string() const noexcept { return value_type::string == type_; }
  inline bool is_float() const noexcept { return value_type::floating == type_; }
  inline bool is_int() const noexcept { return value_type::integer == type_; }
  inline bool is_uint() const noexcept { return value_type::unsigned_integer == type_; }
  inline bool is_bool() const noexcept { return value_type::boolean == type_; }

  // By reference accessors
  template <typename T> T const& ref() const { 
    static_assert(type_traits<T>::is_pure, "Type must not be a reference nor have cv-qualifiers");
    if (type_traits<T>::type_value() != type_) throw exception_type();
    return ref_to<T>();
  }
  inline Null ref_null() const { return nullptr; }
  inline Map const& ref_map() const { return ref<Map>(); }
  inline Vector const& ref_vector() const { return ref<Vector>(); }
  inline String const& ref_string() const { return ref<String>(); }
  inline Float const& ref_float() const { return ref<Float>(); }
  inline Int const& ref_int() const { return ref<Int>(); }
  inline UInt const& ref_uint() const { return ref<UInt>(); }
  inline bool const& ref_bool() const { return ref<bool>(); }

  template <typename T> T& ref() { 
    static_assert(type_traits<T>::is_pure, "Type must not be a reference nor have cv-qualifiers");
    if (type_traits<T>::type_value() != type_) throw exception_type();
    return ref_to<T>();
  }
  inline Map& ref_map() { return ref<Map>(); }
  inline Vector& ref_vector() { return ref<Vector>(); }
  inline String& ref_string() { return ref<String>(); }
  inline Float& ref_float() { return ref<Float>(); }
  inline Int& ref_int() { return ref<Int>(); }
  inline UInt& ref_uint() { return ref<UInt>(); }
  inline bool& ref_bool() { return ref<bool>(); }

  // Forced type switch
  template <typename T> T& transform() noexcept { 
    static_assert(type_traits<T>::is_pure, "Type must not be a reference nor have cv-qualifiers");
    if (type_traits<T>::type_value() != type_) switch_to_type(convert_to(type_proxy<T>()));
    return ref_to<T>();
  }
  inline Null transform_null() { switch_to_type<Null>(); return nullptr; }
  inline Map& transform_map() { return transform<Map>(); }
  inline Vector& transform_vector() { return transform<Vector>(); }
  inline String& transform_string() { return transform<String>(); }
  inline Float& transform_float() { return transform<Float>(); }
  inline Int& transform_int() { return transform<Int>(); }
  inline UInt& transform_uint() { return transform<UInt>(); }
  inline bool& transform_bool() { return transform<bool>(); }

  // Raw accessors - use at your own risk
  template <typename T> T const& raw() const { 
    static_assert(type_traits<T>::is_from_container, "Type must be compatible with this container.");
    return ref_to<T>();
  }
  inline Null raw_null() const { return nullptr; }
  inline Map const& raw_map() const { return raw<Map>(); }
  inline Vector const& raw_vector() const { return raw<Vector>(); }
  inline String const& raw_string() const { return raw<String>(); }
  inline Float const& raw_float() const { return raw<Float>(); }
  inline Int const& raw_int() const { return raw<Int>(); }
  inline UInt const& raw_uint() const { return raw<UInt>(); }
  inline bool const& raw_bool() const { return raw<bool>(); }

  template <typename T> T& raw() { 
    static_assert(type_traits<T>::is_from_container, "Type must be compatible with this container.");
    return ref_to<T>();
  }
  inline Map& raw_map() { return raw<Map>(); }
  inline Vector& raw_vector() { return raw<Vector>(); }
  inline String& raw_string() { return raw<String>(); }
  inline Float& raw_float() { return raw<Float>(); }
  inline Int& raw_int() { return raw<Int>(); }
  inline UInt& raw_uint() { return raw<UInt>(); }
  inline bool& raw_bool() { return raw<bool>(); }

  // Lazy non throwing accessors
  template <typename T> T const* get() const noexcept { 
    return type_traits<T>::type_value() == type_ ? ptr_to<T>() : nullptr;
  }

  template <typename T> T* get() noexcept { 
    return type_traits<T>::type_value() == type_ ? ptr_to<T>() : nullptr;
  }

  template <typename T> bool get(T& output_value) const noexcept { 
    if (type_traits<T>::type_value() == type_) {
      output_value = ref_to<T>();
      return true;
    }
    return false;
  }

  basic_container const& operator[] (typename key_type::value_type const* index) const {
    return access_collection(bool_proxy<eq<Key, vec_size_type>::value>(), index);
  }

  basic_container& operator[] (typename key_type::value_type const* index) {
    return access_collection(bool_proxy<eq<Key, vec_size_type>::value>(), index);
  }

  template <typename T> basic_container const& operator[] (T const& index) const {
    return access_collection(bool_proxy<eq<Key, vec_size_type>::value>(), index);
  }

  template <typename T> basic_container& operator[] (T&& index) {
    return access_collection(bool_proxy<eq<Key, vec_size_type>::value>(), std::forward<T&&>(index));
  }


  // Conversion.
  template <typename T> inline operator T () const { return convert_to(type_proxy<T>()); }
  template <typename T> inline T as() const { return convert_to(type_proxy<T>()); }
  inline Null as_null() const { return as<Null>(); }
  inline Map as_map() const { return as<Map>(); }
  inline Vector as_vector() const { return as<Vector>(); }
  inline String as_string() const { return as<String>(); }
  inline Float as_float() const { return as<Float>(); }
  inline Int as_int() const { return as<Int>(); }
  inline UInt as_uint() const { return as<UInt>(); }
  inline bool as_bool() const { return as<bool>(); }

  // Visiting
  struct const_visitor {
    virtual void apply(std::nullptr_t) {}
    virtual void apply(Map const&) {}
    virtual void apply(Vector const&) {}
    virtual void apply(String const&) {}
    virtual void apply(Float) {}
    virtual void apply(Int) {}
    virtual void apply(UInt) {}
    virtual void apply(bool) {}
  };

  void visit(const_visitor& v) const {
    switch (type_) {
      case value_type::null:
        v.apply(nullptr);
        break; 
      case value_type::map:
        v.apply(ref_to<Map>()); 
        break;
      case value_type::vector:
        v.apply(ref_to<Vector>()); 
        break;
      case value_type::string:
        v.apply(ref_to<String>()); 
        break;
      case value_type::floating:
        v.apply(ref_to<Float>()); 
        break;
      case value_type::integer:
        v.apply(ref_to<Int>()); 
        break;
      case value_type::unsigned_integer:
        v.apply(ref_to<UInt>()); 
        break;
      case value_type::boolean:
        v.apply(ref_to<bool>()); 
        break;
      default:
        break;
    }
  };
};

template <class Container> class attr_init final {
  using key_type = typename Container::str_type;

  mutable key_type key_;
  mutable Container value_;
 public:
  explicit attr_init(key_type const& key) : key_(key) {}
  explicit attr_init(key_type&& key) : key_(std::move(key)) {}
  explicit attr_init(key_type const& key, Container const& value) : key_(key), value_(value) {}
  explicit attr_init(key_type && key, Container const& value) : key_(std::move(key)), value_(value) {}
  explicit attr_init(key_type const& key, Container && value) : key_(key), value_(std::move(value)) {}
  explicit attr_init(key_type && key, Container && value) : key_(std::move(key)), value_(std::move(value)) {}

  attr_init& operator= (Container const& value) { value_ = value; return *this; }
  attr_init& operator= (Container&& value) { value_ = std::move(value); return *this; }
  attr_init& operator() (Container const& value) { value_ = value; return *this; }
  attr_init& operator() (Container&& value) { value_ = std::move(value); return *this; }

  // Always consumed with move
  inline key_type key() const { return std::move(key_); }
  inline Container value() const { return std::move(value_); }
};

template <class Container> class vector_element_init final {
  mutable Container value_;
 public:
  vector_element_init(Container const& value) : value_(value) {}
  vector_element_init(Container && value) : value_(std::move(value)) {} 
  template <typename T> vector_element_init(T&& value) : value_(std::forward<T>(value)) {}

  vector_element_init & operator= (Container const& value) { value_ = value; return *this; }
  vector_element_init & operator= (Container&& value) { value_ = std::move(value); return *this; }
  vector_element_init & operator() (Container const& value) { value_ = value; return *this; }
  vector_element_init & operator() (Container&& value) { value_ = std::move(value); return *this; }

  // Always consumed with move
  inline Container value() const { return std::move(value_); }
};


}  // namespace nested_container

#endif  // __NESTED_COMPILER__
