#pragma once

#include <vector>
#include <unordered_map>

#include "Vertex.h"
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"

//#define TINYOBJLOADER_IMPLEMENTATION
//#include "tiny_obj_loader.h"

class Mesh
{
public:
	Mesh() {}

	void create(const char* path)
	{
		fastObjMesh* obj = fast_obj_read(path);
		
		if (!obj)
			throw std::runtime_error("Cant load .obj file");

		size_t index_count = 0;

		for (unsigned int i = 0; i < obj->face_count; ++i)
			index_count += 3 * (obj->face_vertices[i] - 2);

		size_t index_offset = 0;

		std::unordered_map<Vertex, uint32_t> vertices;

		if (path == "meshes//chalet.obj")
		{
			for (uint32_t i = 0; i < obj->face_count; ++i)
			{
				for (uint32_t j = 0; j < obj->face_vertices[i]; ++j)
				{			
					fastObjIndex gi = obj->indices[index_offset + j];

					glm::vec3 pos =
					{
						obj->positions[gi.p * 3 + 0],
						obj->positions[gi.p * 3 + 1],
						obj->positions[gi.p * 3 + 2],
					};
					
					glm::vec3 normal =
					{
						obj->normals[gi.n * 3 + 0],
						obj->normals[gi.n * 3 + 1],
						obj->normals[gi.n * 3 + 2],
					};
					
					glm::vec2 uvCoords =
					{
						obj->texcoords[gi.t * 2 + 0],
						-1.0f - obj->texcoords[gi.t * 2 + 1],
					};
					
					Vertex vert = Vertex(pos, m_greenColor, uvCoords, normal);

					//if (vertices.contains(vert)) -> C++20
					if (vertices.count(vert) == 0)
					{
						vertices[vert] = vertices.size();
						m_vertices.push_back(vert);
					}

					m_indices.push_back(vertices[vert]);			
				}

				index_offset += obj->face_vertices[i];
			}
		}
		else
		{
			for (uint32_t i = 0; i < obj->face_count; ++i)
			{
				for (uint32_t j = 0; j < obj->face_vertices[i]; ++j)
				{
					fastObjIndex gi = obj->indices[index_offset + j];

					glm::vec3 pos =
					{
						obj->positions[gi.p * 3 + 0],
						obj->positions[gi.p * 3 + 2],
						obj->positions[gi.p * 3 + 1],
					};

					glm::vec3 normal =
					{
						obj->normals[gi.n * 3 + 0],
						obj->normals[gi.n * 3 + 2],
						obj->normals[gi.n * 3 + 1],
					};

					glm::vec2 uvCoords =
					{
						obj->texcoords[gi.t * 2 + 0],
						-1.0f - obj->texcoords[gi.t * 2 + 1],
					};

					//Vertex vert = Vertex(pos, m_greenColor, uvCoords, normal);
					Vertex vert = Vertex(pos, glm::vec3(0.33f), uvCoords, normal);

					//if (vertices.contains(vert)) -> C++20
					if (vertices.count(vert) == 0)
					{
						vertices[vert] = vertices.size();
						m_vertices.push_back(vert);
					}

					m_indices.push_back(vertices[vert]);
				}

				index_offset += obj->face_vertices[i];
			}
		}

		fast_obj_destroy(obj);
	}

	std::vector<Vertex> getVertices()
	{
		return m_vertices;
	}

	std::vector<uint32_t> getIndices()
	{
		return m_indices;
	}


private:
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	glm::vec3 m_greenColor = { 0.0f, 1.0f, 0.0f };
};

/*
		tinyobj::attrib_t vertexAttributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warningString;
		std::string errorString;


		bool success = tinyobj::LoadObj(&vertexAttributes, &shapes, &materials, &warningString, &errorString, path);

		if (!success)
			throw std::runtime_error(errorString);

		std::unordered_map<Vertex, uint32_t> vertices;

		for (tinyobj::shape_t shape : shapes)
		{
			for (tinyobj::index_t index : shape.mesh.indices)
			{
				glm::vec3 pos =
				{
					vertexAttributes.vertices[3 * index.vertex_index + 0],
					vertexAttributes.vertices[3 * index.vertex_index + 2],			// switched y and z axis
					vertexAttributes.vertices[3 * index.vertex_index + 1],			//
				};

				glm::vec3 normal =
				{
					vertexAttributes.normals[3 * index.normal_index + 0],
					vertexAttributes.normals[3 * index.normal_index + 2],
					vertexAttributes.normals[3 * index.normal_index + 1],
				};

				Vertex vert(pos, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, normal);		//vec2 -> texture coords -> vertexAttributes.texturecoords texture_index

				if (vertices.count(vert) == 0)
				{
					vertices[vert] = vertices.size();
					m_vertices.push_back(vert);

				}

				m_indices.push_back(vertices[vert]);
			}
		}
*/
