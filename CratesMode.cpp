#include "CratesMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "Sound.hpp"
#include "MeshBuffer.hpp"
#include "WalkMesh.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>

Load< MeshBuffer > crates_meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("phone-bank.pnc"));
});

Load< GLuint > crates_meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(crates_meshes->make_vao_for_program(vertex_color_program->program));
});

Load< Sound::Sample > sample_dot(LoadTagDefault, [](){
	return new Sound::Sample(data_path("dot.wav"));
});
Load< Sound::Sample > sample_loop(LoadTagDefault, [](){
	return new Sound::Sample(data_path("loop.wav"));
});
Load<WalkMesh> walk_mesh(LoadTagDefault, []() {
  return new WalkMesh(data_path("walk-mesh.scene"));
});

CratesMode::CratesMode() {
	//----------------
	//set up scene:
	//TODO: this should load the scene from a file!
	auto attach_object = [this](Scene::Transform *transform, std::string const &name) {
		Scene::Object *object = scene.new_object(transform);
		object->program = vertex_color_program->program;
		object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
		object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
		object->vao = *crates_meshes_for_vertex_color_program;
		MeshBuffer::Mesh const &mesh = crates_meshes->lookup(name);
		object->start = mesh.start;
		object->count = mesh.count;
		return object;
	};
	std::ifstream file(data_path("phone-bank.scene"), std::ios::binary);
	struct sceneTransform{
	  int32_t parent_ref;
	  uint32_t name_begin, name_end;
		int32_t self_ref;
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

	read_chunk(file, "str0", &strings);
  read_chunk(file, "xfh0", &transforms);
  read_chunk(file, "msh0", &meshes);

  // read_chunk(file, "cam0", &meshes);
  // read_chunk(file, "lmp0", &meshes);



  //import meshes from .scene file
  for(auto& entry : meshes){
    if (!(entry.name_begin <= entry.name_end && entry.name_end <= strings.size())) {
      throw std::runtime_error("index entry has out-of-range name begin/end");
    }
    std::string name(&strings[0] + entry.name_begin, &strings[0] + entry.name_end);
    meshesMap.insert(std::make_pair(entry.ref, name));
		// std::cout << "the names of the meshes!!!!!" << name << std::endl;

  }
	//import transformation of objects in .scene
	for(auto& iter : transforms){
		if (!(iter.name_begin <= iter.name_end && iter.name_end <= strings.size())) {
			throw std::runtime_error("index entry has out-of-range name begin/end");
		}
		std::string name(&strings[0] + iter.name_begin, &strings[0] + iter.name_end);
		Scene::Transform *transform = scene.new_transform();
		transform->position = iter.translation;
		transform->rotation = glm::quat(iter.rotation.w, iter.rotation.x,
																			iter.rotation.y, iter.rotation.z);
		transform->scale = iter.scaling;
		// std::cout << "the names of the transformations!!!!!" << name << std::endl;
		// found the objects in the meshesMap that was built before this loop
		if(meshesMap.find(iter.self_ref) != meshesMap.end()){
			Scene::Object *obj_temp = attach_object(transform, meshesMap.at(iter.self_ref));

			if(name == "Player"){
				player = obj_temp;
			}
			if(name == "Phone.001"){
				phone_001 = obj_temp;
			}
			if(name == "Phone.002"){
				phone_002 = obj_temp;
			}
			if(name == "Phone.003"){
				phone_003 = obj_temp;
			}
			if(name == "Phone.004"){
				phone_004 = obj_temp;
			}

		}


	}

	{ //Camera looking at the origin:
		Scene::Transform *transform = scene.new_transform();
		transform->position = glm::vec3(0.0f, -10.0f, 1.0f);
		//Cameras look along -z, so rotate view to look at origin:
		transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		camera = scene.new_camera(transform);
	}


	walk_point = walk_mesh->start(glm::vec3(0.0f, 0.0f, 0.0f));
	player_normal = walk_mesh->world_normal(walk_point);
	//start the 'loop' sample playing at the large crate:
	loop = sample_loop->play(phone_001->transform->position, 1.0f, Sound::Loop);
}

CratesMode::~CratesMode() {
	if (loop) loop->stop();
}

bool CratesMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}
	//handle tracking the state of WSAD for movement control:
	if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
		if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
			controls.forward = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
			controls.backward = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
			controls.left = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
			controls.right = (evt.type == SDL_KEYDOWN);
			return true;
		}
	}
	//handle tracking the mouse for rotation control:
	if (!mouse_captured) {
		if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
			Mode::set_current(nullptr);
			return true;
		}
		if (evt.type == SDL_MOUSEBUTTONDOWN) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			mouse_captured = true;
			return true;
		}
	} else if (mouse_captured) {
		if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			mouse_captured = false;
			return true;
		}
		if (evt.type == SDL_MOUSEMOTION) {
			//Note: float(window_size.y) * camera->fovy is a pixels-to-radians conversion factor
			float yaw = evt.motion.xrel / float(window_size.y) * camera->fovy;
			float pitch = evt.motion.yrel / float(window_size.y) * camera->fovy;
			yaw = -yaw;
			pitch = -pitch;
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}
	return false;
}

void CratesMode::update(float elapsed) {
	glm::mat3 directions = glm::mat3_cast(camera->transform->rotation);
	float amt = 5.0f * elapsed;
	if (controls.right) walk_mesh->walk(walk_point, amt * directions[0]);
	if (controls.left) walk_mesh->walk(walk_point, -amt * directions[0]);
	if (controls.backward) walk_mesh->walk(walk_point, amt * directions[2]);
	if (controls.forward) walk_mesh->walk(walk_point, -amt * directions[2]);
	player_normal = walk_mesh->world_normal(walk_point);
  player->transform->position = walk_mesh->world_point(walk_point);
	//TODO: update the direction of the player with the normal of the triangle
	// player->transform->rotation = player_normal;
	{ //set sound positions:
		glm::mat4 cam_to_world = camera->transform->make_local_to_world();
		Sound::listener.set_position( cam_to_world[3] );
		//camera looks down -z, so right is +x:
		Sound::listener.set_right( glm::normalize(cam_to_world[0]) );

		if (loop) {
			glm::mat4 phone_002_to_world = phone_002->transform->make_local_to_world();
			loop->set_position( phone_002_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) );
		}
	}

	dot_countdown -= elapsed;
	if (dot_countdown <= 0.0f) {
		dot_countdown = (rand() / float(RAND_MAX) * 2.0f) + 0.5f;
		glm::mat4x3 phone_001_to_world = phone_001->transform->make_local_to_world();
		sample_dot->play( phone_001_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) );
	}
}

void CratesMode::draw(glm::uvec2 const &drawable_size) {
	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light position + color:
	glUseProgram(vertex_color_program->program);
	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.45f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));
	glUseProgram(0);

	//fix aspect ratio of camera
	camera->aspect = drawable_size.x / float(drawable_size.y);

	scene.draw(camera);

	if (Mode::current.get() == this) {
		glDisable(GL_DEPTH_TEST);
		std::string message;
		if (mouse_captured) {
			message = "ESCAPE TO UNGRAB MOUSE * WASD MOVE";
		} else {
			message = "CLICK TO GRAB MOUSE * ESCAPE QUIT";
		}
		float height = 0.06f;
		float width = text_width(message, height);
		draw_text(message, glm::vec2(-0.5f * width,-0.99f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
		draw_text(message, glm::vec2(-0.5f * width,-1.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		glUseProgram(0);
	}

	GL_ERRORS();
}


void CratesMode::show_pause_menu() {
	std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

	std::shared_ptr< Mode > game = shared_from_this();
	menu->background = game;

	menu->choices.emplace_back("PAUSED");
	menu->choices.emplace_back("RESUME", [game](){
		Mode::set_current(game);
	});
	menu->choices.emplace_back("QUIT", [](){
		Mode::set_current(nullptr);
	});

	menu->selected = 1;

	Mode::set_current(menu);
}
