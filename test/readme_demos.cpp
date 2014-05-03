#include <iostream>
#include <nested_container/container.hpp>
#include <nested_container/driver/json/json_forward.hpp>
#include <nested_container/externalize.hpp>
#include <nested_container/driver/json/externalize_json.hpp>
NESTED_CONTAINER_EXTERNALIZE(NESTED_CONTAINER_CONTAINER_SIGNATURE());
NESTED_CONTAINER_EXTERNALIZE_JSON(NESTED_CONTAINER_CONTAINER_SIGNATURE());

using nested_container::container;
template <typename C> using json = nested_container::json::serializer<C>;

int intro() {
  using _ = nested_container::attr_init<container>;
  container c = {
    _("nom") = "Roger",
    _("prenom") = "Marcel",
    _("attributs") = {
      _("poids") = 95u,
      _("liste") = {1,2.f,"yeah"}
    }
  };

  json<container> serializer;
  std::string json_str = serializer.serialize(c);
  container parsed_c = serializer.deserialize(json_str);
  
  if (parsed_c["attributs"]["poids"].is_uint()) {
    std::cout << parsed_c["attributs"]["liste"][2].ref_string() << std::endl;
    return 0;
  }

  return 1;
}

void basic() {
  container c;  // Is null
  c = "Roger"; 
  c = 7.f; 
  c = -5;
  c = 6u;
  c = true;
  // c = 2.;  // is a double, would not compile !
  
  c["name"] = "Marcel";  // Internal type is transformed to a map
  c["weight"] = 123u;
  
  c[0] = "Marcel";  // Internal type is transformed to a vector
  c[1] = "Robert";
}


void braced_init() {
  using _ = nested_container::attr_init<container>;

  container c = {
    _("nom") = "Roger",
    _("prenom") = "Marcel",
    _("attributs") = {
      _("poids") = 95u,
      _("liste") = {1,2.f,"yeah"}
    }
  };
  
  std::cout << c["attributs"]["liste"][2].ref_string() << std::endl;
}

void accessing_data() {
  container c = "allez la !";
  {
    std::string s1 = c;  // c content is converted to a string if possible then returned
    std::string& s2 = c.ref_string();  // Return a std::string& is c contains a string, throws otherwise
    std::string* ps3 = c.get_string();  // Return a pointer a std::string const if c contains a string, nullptr otherwise
    std::string s4;
    bool success = c.get_string(s4);  // Returns true and affects s4 if c contains a string, returns false otherwise
    std::string& s5 = c.raw_string(); // Directly accesses the underlying data. If you know what you are doing
    bool is_string = c.is_string();  // Returns true if c contains a string
    std::string s6 = c.as_string();  // c content is converted to a string if possible then returned
    std::string& s7 = c.transform_string();  // c content is converted and stored as a string, then returned
  }
  
  // Templated versions are available, static_assert guards you from misuse
  {
    std::string& s2 = c.ref<std::string>();
    std::string* ps3 = c.get<std::string>();
    std::string s4;
    bool success = c.get<std::string>(s4);
    std::string& s5 = c.raw<std::string>();
    std::string s6 = c.as<std::string>();
    std::string& s7 = c.transform<std::string>();
  }
  
  // const versions are available too
  container const &c2 = c;
  {
    std::string s1 = c2;
    std::string const& s2 = c2.ref_string();
    std::string const * ps3 = c2.get_string();
    std::string s4;
    bool success = c2.get(s4);
    std::string const& s5 = c2.raw_string();
    bool is_string = c2.is_string();
    std::string s6 = c2.as_string();
    // "transform" member functions cannot be const
  }
}

void parsing_json() {
  std::string input = R"json({"test1":1, "test2":1.1, "test3":["test1","test2",1.1,2,-4], "test4":true, "test5":null, "test6":{}})json";
  json<container> serializer;
  container c = serializer.deserialize(input);
  std::cout << c["test3"][2].ref_float() << std::endl;
}

void dumping_json() {
  using _ = nested_container::attr_init<container>;
  container c = {_("nom") = "Roger", _("prenom") = "Marcel", _("attributs") = { _("poids") = 95u, _("liste") = {1,2.f,"yeah"}}};
  json<container> serializer;
  std::cout << serializer.serialize(c) << std::endl;
}

int main(void) {
  if (intro()) return 1;
  basic();
  braced_init();
  accessing_data();
  parsing_json();
  dumping_json();

  return 0;
}
