#include "WalkMesh.hpp"
#include "read_chunk.hpp"

#include <glm/glm.hpp>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <cstddef>

WalkMesh::WalkMesh(std::string const &filename){
	std::ifstream file(filename, std::ios::binary);
	read_chunk(file, "vtc0", &vertices);
	read_chunk(file, "nml0", &normals);
	read_chunk(file, "ply0", &triangles);
	//construct next_vertex map
	for (auto& triangle : triangles) {
    next_vertex[glm::uvec2(triangle.x, triangle.y)] = triangle.z;
    next_vertex[glm::uvec2(triangle.y, triangle.z)] = triangle.x;
    next_vertex[glm::uvec2(triangle.z, triangle.x)] = triangle.y;
  }

}
//source https://gist.github.com/joshuashaffer/99d58e4ccbd37ca5d96e
//source https://www.geometrictools.com/GTEngine/Include/Mathematics/GteDistPointTriangleExact.h
void WalkMesh::closestpt2triangle(std::vector<glm::vec3> &trianglePoints, glm::vec3 const &position, glm::vec3 &closestPoint) const{
	glm::vec3 E0 = trianglePoints[1] - trianglePoints[0];
  glm::vec3 E1 = trianglePoints[2] - trianglePoints[0];
  glm::vec3 v0 = trianglePoints[0] - position;

  float a = glm::dot(E0, E0);
  float b = glm::dot(E0, E1);
  float c = glm::dot(E1, E1);
  float d = glm::dot(E0, v0);
  float e = glm::dot(E1, v0);

  float det = a * c - b * b;
  float s = b * e - c * d;
  float t = b * d - a * e;
	// std::cout << "s: " << s << "t: " << t << "det: " << det <<  std::endl;

	if (s + t <= det)
    {
        if (s < 0)
        {
            if (t < 0)  // region 4
            {
                if (d < 0)
                {
                    t = 0;
                    if (-d >= a)  // V1
                    {
                        s = 1;
                    }
                    else  // E01
                    {
                        s = -d / a;
                    }
                }
                else
                {
                    s = 0;
                    if (e >= 0)  // V0
                    {
                        t = 0;
                    }
                    else if (-e >= c)  // V2
                    {
                        t = 1;
                    }
                    else  // E20
                    {
                        t = -e / c;
                    }
                }
            }
            else  // region 3
            {
                s = 0;
                if (e >= 0)  // V0
                {
                    t = 0;
                }
                else if (-e >= c)  // V2
                {
                    t = 1;
                }
                else  // E20
                {
                    t = -e / c;
                }
            }
        }
        else if (t < 0)  // region 5
        {
            t = 0;
            if (d >= 0)  // V0
            {
                s = 0;
            }
            else if (-d >= a)  // V1
            {
                s = 1;
            }
            else  // E01
            {
                s = -d / a;
            }
        }
        else  // region 0, interior
        {
            float invDet = 1 / det;
            s *= invDet;
            t *= invDet;
        }
    }
    else
    {
        float tmp0, tmp1, numer, denom;

        if (s < 0)  // region 2
        {
            tmp0 = b + d;
            tmp1 = c + e;
            if (tmp1 > tmp0)
            {
                numer = tmp1 - tmp0;
                denom = a - ((float)2)*b + c;
                if (numer >= denom)  // V1
                {
                    s = 1;
                    t = 0;
                }
                else  // E12
                {
                    s = numer / denom;
                    t = 1 - s;
                }
            }
            else
            {
                s = 0;
                if (tmp1 <= 0)  // V2
                {
                    t = 1;
                }
                else if (e >= 0)  // V0
                {
                    t = 0;
                }
                else  // E20
                {
                    t = -e / c;
                }
            }
        }
        else if (t < 0)  // region 6
        {
            tmp0 = b + e;
            tmp1 = a + d;
            if (tmp1 > tmp0)
            {
                numer = tmp1 - tmp0;
                denom = a - ((float)2)*b + c;
                if (numer >= denom)  // V2
                {
                    t = 1;
                    s = 0;
                }
                else  // E12
                {
                    t = numer / denom;
                    s = 1 - t;
                }
            }
            else
            {
                t = 0;
                if (tmp1 <= 0)  // V1
                {
                    s = 1;
                }
                else if (d >= 0)  // V0
                {
                    s = 0;
                }
                else  // E01
                {
                    s = -d / a;
                }
            }
        }
        else  // region 1
        {
            numer = c + e - b - d;
            if (numer <= 0)  // V2
            {
                s = 0;
                t = 1;
            }
            else
            {
                denom = a - ((float)2)*b + c;
                if (numer >= denom)  // V1
                {
                    s = 1;
                    t = 0;
                }
                else  // 12
                {
                    s = numer / denom;
                    t = 1 - s;
                }
            }
        }
    }
		closestPoint = trianglePoints[0] + s * E0 + t * E1;
		// std::cout << "the return value of closest point to triangle" << closestPoint[0] << closestPoint[1] << closestPoint[2] << std::endl;

}

//source https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
void WalkMesh::barycentric(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c, float &u, float &v, float &w) const{
	 glm::vec3  v0 = b - a, v1 = c - a, v2 = p - a;
	 float d00 = glm::dot(v0, v0);
	 float d01 = glm::dot(v0, v1);
	 float d11 = glm::dot(v1, v1);
	 float d20 = glm::dot(v2, v0);
	 float d21 = glm::dot(v2, v1);
	 float denom = d00 * d11 - d01 * d01;
	 v = (d11 * d20 - d01 * d21) / denom;
	 w = (d00 * d21 - d01 * d20) / denom;
	 u = 1.0f - v - w;

}

WalkMesh::WalkPoint WalkMesh::start(glm::vec3 const &world_point) const {
	WalkPoint closest;
	//TODO: iterate through triangles
	//TODO: for each triangle, find closest point on triangle to world_point
	//TODO: if point is closest, closest.triangle gets the current triangle, closest.weights gets the barycentric coordinates
	// std::cout << "the length of triangles:  " << triangles.size() << std::endl;
	std::vector< glm::vec3 > trianglePoints;
	trianglePoints.push_back(glm::vec3(0.0f));
	trianglePoints.push_back(glm::vec3(0.0f));
	trianglePoints.push_back(glm::vec3(0.0f));
	int counter = 0;
	float dis;
	float min =  std::numeric_limits<float>::max();
	for(auto& triangle : triangles){
		trianglePoints[0] = vertices[triangle[0]];
		trianglePoints[1] = vertices[triangle[1]];
		trianglePoints[2] = vertices[triangle[2]];
		glm::vec3 closestPoint;
		WalkMesh::closestpt2triangle(trianglePoints, world_point, closestPoint);
		dis = glm::distance(closestPoint, world_point);
		// std::cout << min << " " << dis /<< std::endl;
		if(dis < min){
			counter++;
			min = dis;
			closest.triangle = triangle;
			float u =  0.f;
			float v =  0.f;
			float w =  0.f;
			WalkMesh::barycentric(closestPoint, vertices[triangle[0]], vertices[triangle[1]],
                      vertices[triangle[2]], u, v, w);
			// std::cout << u << " " << v << " " << w << std::endl;
			closest.weights = glm::vec3(u, v, w);
		}
	}
	// std::cout << "DEBUG " << closest.triangle[0] << std::endl;
	// std::cout << counter << std::endl;
	return closest;
}

void WalkMesh::walk(WalkPoint &wp, glm::vec3 const &step) const {
	//TODO: project step to barycentric coordinates to get weights_step
	glm::vec3 post_point = world_point(wp) + glm::vec3(1.0);
	glm::vec3 weights_step;
	float u =  0.f;
	float v =  0.f;
	float w =  0.f;
	WalkMesh::barycentric(post_point, vertices[wp.triangle[0]], vertices[wp.triangle[1]],
									vertices[wp.triangle[2]], u, v, w);
	weights_step = glm::vec3(u, v, w) + wp.weights;
// std::cout << u << " " << v << " " << w << std::endl;
	//TODO: when does wp.weights + t * weights_step cross a triangle edge?
	// float t = 1.0f;
	// t = weights_step[0] + weights_step[1] + weights_step[2];

	if (u >= 0 && v >= 0 && w >=0) { //if a triangle edge is not crossed
		//TODO: wp.weights gets moved by weights_step, nothing else needs to be d1.

		wp.weights = glm::vec3(u, v, w);
	} else { //if a triangle edge is crossed
		// WalkPoint new_wp;
		//find which edge has been crossed
		//reference eric1221bday
		float reduced_coeff = 0.f;
		glm::uvec2 crossed_edge;
		if (u < 0) {
			//cross point 1 and 2
		 reduced_coeff = wp.weights.x / -weights_step.x;
		 crossed_edge = glm::uvec2(wp.triangle[2], wp.triangle[1]);
	 } else if (v < 0) {
		 reduced_coeff = wp.weights.y / -weights_step.y;
		 crossed_edge = glm::uvec2(wp.triangle[0], wp.triangle[2]);
	 } else  {
		 //w < 0, where the edge fromed by point 0 and point 1 has been crossed
		 reduced_coeff = wp.weights.z / -weights_step.z;
		 crossed_edge = glm::uvec2(wp.triangle[1], wp.triangle[0]);
	 }

	 //find the next triangle
	 if (next_vertex.find(crossed_edge) !=  next_vertex.end()) {
		 std::cout << "find edge" << std::endl;
		 // auto third_vertex = next_vertex.at(crossed_edge);

	 }


	}
}
