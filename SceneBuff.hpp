#pragma once

#include "GL.hpp"
#include <map>

struct sceneTransform{
  int32_t parent_ref;
  uint32_t name_begin, name_end;
  glm::vec3 translation;
  glm::vec4 rotation;
  glm::vec3 scaling;
};

struct sceneMesh{
  int32_t ref;
  uint32_t name_begin, name_end;
};

std::vector<char> strings;
std::vector<sceneTransform> transforms;
std::vector<sceneMesh> meshes;
std::map< int32_t, std::string > meshesMap;
