#pragma once
#include <string>

#include "las.h"
#include <glm/gtx/vec_swizzle.hpp>

#include "shaderVF.h"
#include "Transform.h"
#include "Vertex.h"
#include "glad/glad.h"
#include "glm/ext/scalar_constants.hpp"

class PointCloud
{
public:
    Transform transform;
    void update_vertices(std::vector<Vertex> vertices) { this->vertices = vertices; }
    void update_indices(std::vector<unsigned> indices) { this->indices = indices; }
    std::vector<Vertex> get_vertices() const { return vertices; }
    std::vector<unsigned> get_indices() const { return indices; }
    void set_shader(ShaderVF *shader) { this->shader = shader; }
    void set_mode(GLenum mode) { this->mode = mode; }
    void hasData()
    {
        std::cout << data.empty() << std::endl;
        std::cout << data.size()  << std::endl;
        
    }
    
    void render() const
    {
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), &indices[0], GL_STATIC_DRAW);
        glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
    }
    unsigned VBO, VAO, EBO;
    void draw() const
    {
        // pre_render();
        glBindVertexArray(VAO);
        render();
        glBindVertexArray(0);
    }
    
private:
    void pre_render() const
    {
        shader->use();
    }
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    // Shaders can be shared between objects
    ShaderVF *shader = nullptr;
    GLenum mode = GL_TRIANGLES;
    Vertex bounding_box[2];
    std::vector<char> data; // Full data of the point cloud file
    LASFile lasFile;        // Parsed data of the point cloud file
    glm::vec3 startPoint = {0, 0, 0};
    int points_x = 0;
    int points_z = 0;

    void push_point(glm::vec3 pos, std::vector<Vertex> &vertices)
    {
        vertices.push_back({pos, {0, 0, 0}, {0, 0}});
        if (pos.x <= glm::epsilon<float>() && pos.z <= glm::epsilon<float>())
        {
            startPoint = pos;
        }
        else
        {
            auto offsetX = abs(pos.x - startPoint.x);
            auto offsetZ = abs(pos.z - startPoint.z);
            if (offsetX > glm::epsilon<float>() && offsetZ <= glm::epsilon<float>())
            {
                points_x++;
            }
            if (offsetZ > glm::epsilon<float>() && offsetX <= glm::epsilon<float>())
            {
                points_z++;
            }
        }
    }

    void push_color_point(glm::vec3 pos, glm::vec3 color, std::vector<Vertex> &vertices)
    {
        vertices.push_back({pos, color, {0, 0}});
        if (pos.x <= glm::epsilon<float>() && pos.z <= glm::epsilon<float>())
        {
            startPoint = pos;
        }
        else
        {
            if (pos.x - startPoint.x > glm::epsilon<float>())
            {
                points_x++;
            }
            if (pos.z - startPoint.z > glm::epsilon<float>())
            {
                points_z++;
            }
        }
    }

public:
    void setup()
    {
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
    PointCloud(std::string file)
    {
        load_las_file(file.c_str(), data);
        parse_las_file(data.data(), data.size(), &lasFile);
        glm::vec3 scale = {lasFile.header.x_scale_factor, lasFile.header.y_scale_factor, lasFile.header.z_scale_factor};
        std::vector<Vertex> vertices;
        glm::vec3 min = {lasFile.header.min_x, lasFile.header.min_y, lasFile.header.min_z};
        glm::vec3 max = {lasFile.header.max_x, lasFile.header.max_y, lasFile.header.max_z};
        glm::vec3 translate = (max + min) / 2.0f;
        for (int i = 0; i < lasFile.header.number_of_point_records; ++i)
        {
            switch (lasFile.header.point_data_format)
            {
            case 0:
                Point0 point;
                parse_point0(lasFile.point_data + i * lasFile.header.point_data_record_length, &point);
                push_point(xzy(glm::vec3(point.x, point.y, point.z) * scale - translate), vertices);
                break;
            case 1:
                Point1 point1;
                parse_point1(lasFile.point_data + i * lasFile.header.point_data_record_length, &point1);
                push_point(xzy(glm::vec3(point1.x, point1.y, point1.z) * scale - translate), vertices);
                break;
            case 2:
                Point2 point2;
                parse_point2(lasFile.point_data + i * lasFile.header.point_data_record_length, &point2);
                push_color_point(xzy(glm::vec3(point2.x, point2.y, point2.z) * scale - translate), {point2.red, point2.green, point2.blue}, vertices);
                break;
            case 3:
                Point3 point3;
                parse_point3(lasFile.point_data + i * lasFile.header.point_data_record_length, &point3);
                push_color_point(xzy(glm::vec3(point3.x, point3.y, point3.z) * scale - translate), {point3.red, point3.green, point3.blue}, vertices);
                break;

            default:
                break;
            }
        }
        update_vertices(vertices);
        std::vector<unsigned> indices(vertices.size());
        for (size_t i = 0; i < indices.size(); i++)
        {
            indices[i] = i;
        }
        update_indices(indices);
        set_mode(GL_POINTS);
    }
    ~PointCloud() {}
    int get_points_x() { return points_x; }
    int get_points_z() { return points_z; }
};
