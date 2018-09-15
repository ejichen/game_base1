#include "SceneBuff.hpp"
#include "read_chunk.hpp"

#include <glm/glm.hpp>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <cstddef>

//create a class for reading and splitting .scene file
SceneBuffer::SceneBuffer(std::string const &filename) {
  std::ifstream file(filename, std::ios::binary);

  read_chunk(file, "str0", &strings);
  read_chunk(file, "xfh0", &transforms);
  read_chunk(file, "msh0", &meshes);

  // read_chunk(file, "cam0", &meshes);
  // read_chunk(file, "lmp0", &meshes);

  std::string name(&strings[0] + entry.name_begin, &strings[0] + entry.name_end);



  //import meshes from .scene file
  for(auto& entry : meshes){
    if (!(entry.name_begin <= entry.name_end && entry.name_end <= strings.size())) {
      throw std::runtime_error("index entry has out-of-range name begin/end");
    }
    std::string name(&strings[0] + entry.name_begin, &strings[0] + entry.name_end);
    meshesMap[entry.ref] = name;
  }


}

const MeshBuffer::Mesh &MeshBuffer::lookup(std::string const &name) const {

}

GLuint MeshBuffer::make_vao_for_program(GLuint program) const {

}
