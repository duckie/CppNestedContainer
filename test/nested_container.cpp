#include <nested_container/container.hpp>
#include <nested_container/externalize.hpp>
NESTED_CONTAINER_INSTANTIATE(NESTED_CONTAINER_CONTAINER_SIGNATURE());

#include <nested_container/driver/json/json.hpp>
#include <nested_container/driver/json/externalize_json.hpp>
NESTED_CONTAINER_INSTANTIATE_JSON(NESTED_CONTAINER_CONTAINER_SIGNATURE());

