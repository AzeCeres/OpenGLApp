#pragma once
#include <corecrt_math.h>
#include <vector>

#include "shaderVF.h"
#include "Vertex.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/transform.hpp"

class Sphere
{
private:
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1., 0., 0., 0.);
    glm::vec3 scale = glm::vec3(1.0f);
    unsigned VBO, VAO, EBO;
    const float H_ANGLE = glm::pi<float>() / 180 * 72;
    const float V_ANGLE = atanf(1.0f / 2);
    void update_vertices(std::vector<Vertex> vertices) { this->vertices = vertices; }
    void update_indices(std::vector<unsigned> indices) { this->indices = indices; }
    ShaderVF *shader = nullptr;
    GLenum mode = GL_TRIANGLES;
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
public:
    void set_position(glm::vec3 position) { this->position = position; }
    glm::vec3 get_position() { return position; }
    glm::mat4x4 get_model_matrix() const
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = model * glm::mat4_cast(rotation);
        model = glm::scale(model, scale);
        return model;
    }
    void render() const
    {
        shader->use();
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), &indices[0], GL_STATIC_DRAW);
        glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
    }
    void draw()
    {
        glBindVertexArray(VAO);
        render();
        glBindVertexArray(0);
    }
    void set_shader(ShaderVF *shader) { this->shader = shader; }
    Sphere()
    {
    };
    ~Sphere() {};
    void create(int subdivisions)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned> indices = {
            // top cone
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 5,
            0, 5, 1,

            // center cylinder
            6, 2, 1,
            6, 7, 2,
            7, 3, 2,
            7, 8, 3,
            8, 4, 3,
            8, 9, 4,
            9, 5, 4,
            9, 10, 5,
            10, 1, 5,
            6, 1, 10,

            // bottom cone
            11, 6, 10,
            11, 7, 6,
            11, 8, 7,
            11, 9, 8,
            11, 10, 9
        };
        vertices.resize(12);
        float h_angle1 = -glm::pi<float>() / 2;
        float h_angle2 = h_angle1 - H_ANGLE / 2;
        vertices[0] = Vertex(glm::vec3(0, 0, 1));
        vertices[11] = -vertices[0];
        const auto z = sinf(V_ANGLE);
        const auto xy = cosf(V_ANGLE);
        for (int i = 1; i < 6; i++)
        {
            const auto i2 = i + 5;
            vertices[i] = Vertex(glm::vec3(xy * cosf(h_angle1), xy * sinf(h_angle1), z));
            vertices[i2] = Vertex(glm::vec3(xy * cosf(h_angle2), xy * sinf(h_angle2), -z));
            h_angle1 += H_ANGLE;
            h_angle2 += H_ANGLE;
        }

        if (subdivisions > 0)
        {
            std::vector<Vertex> old_vertices;
            std::vector<unsigned> old_indices;
            unsigned index = 0;
            Vertex v1, v2, v3, nv1, nv2, nv3;
            for (unsigned i = 0; i < subdivisions; i++)
            {
                old_vertices = vertices;
                old_indices = indices;
                vertices.clear();
                indices.clear();
                index = 0;

                for (unsigned j = 0; j < old_indices.size(); j += 3)
                {
                    v1 = old_vertices[old_indices[j]];
                    v2 = old_vertices[old_indices[j + 1]];
                    v3 = old_vertices[old_indices[j + 2]];

                    nv1 = Vertex::compute_half(v1, v2);
                    nv2 = Vertex::compute_half(v2, v3);
                    nv3 = Vertex::compute_half(v1, v3);

                    vertices.push_back(v1);
                    vertices.push_back(nv1);
                    vertices.push_back(nv3);

                    vertices.push_back(nv1);
                    vertices.push_back(v2);
                    vertices.push_back(nv2);

                    vertices.push_back(nv1);
                    vertices.push_back(nv2);
                    vertices.push_back(nv3);

                    vertices.push_back(nv3);
                    vertices.push_back(nv2);
                    vertices.push_back(v3);

                    for (char k = 0; k < 12; k++)
                    {
                        indices.push_back(index + k);
                    }
                    index += 12;
                }
            }
        }

        update_vertices(vertices);
        update_indices(indices);
    }
    void setup(int subdivisions)
    {
        create(subdivisions);
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        // normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
        // texture coord attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoords));

        glBindVertexArray(0);
    }
    void setup()
    {
        setup(2);
    }
};
