#pragma once
#include <algorithm>
#include <string>

#include <glm/gtx/vec_swizzle.hpp>
#include "las.h"

#include <map>
#include "CImg.h"
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
    void convertToImage();
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
        convertToImage();
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


inline void PointCloud::convertToImage()
{
    std::vector<glm::vec3> points = {};
    std::map<float, std::vector<glm::vec3>> point_map = {};
    std::vector<Vertex> vertices = get_vertices();
    float minZ{(float)INT32_MAX}, maxZ{(float)INT32_MIN}, minX{(float)INT32_MAX}, maxX{(float)INT32_MIN}, minY{(float)INT32_MAX}, maxY{(float)INT32_MIN};
    //Sort all of the vertices into Z-columns
    for (auto vertex : vertices)
    {
        if(vertex.position.z <= minZ)
            minZ=vertex.position.z;
        if(vertex.position.z >= maxZ)
            maxZ=vertex.position.z;
        //check to see if current Z-value already has a column, if not, create one
        if (point_map.find(vertex.position.z) == point_map.end())
        {
            point_map[vertex.position.z] = {};
        }
        //Add vertex position to the correct Z-column
        point_map[vertex.position.z].push_back(vertex.position);
    }
    int lengthZ = maxZ-minZ;
    // sorts all of the positions by their x-values so that they are all in order
    for (auto &pair : point_map)
    {
        std::sort(pair.second.begin(), pair.second.end(), [](glm::vec3 a, glm::vec3 b)
                  { return a.x < b.x; });
    }
    //size is set to the number of unique z values in point_map
    auto size = point_map.size();
    for (int i = 0; i < size; i++)
    {
        auto map_index = 0;
        auto actual_map_index = 0.0f;
        for (auto it = point_map.begin(); it != point_map.end(); ++it)
        {
            if (map_index == i)
            {
                actual_map_index = it->first;
                break;
            }
            map_index++;
        }
        auto point_map_row = point_map[actual_map_index];
        std::cout << "x points in row "<< actual_map_index << " " <<point_map_row.size() << std::endl;
        for (int j = 0; j < size; j++)
        {
            auto curIndex = static_cast<int>(point_map_row.size() * (static_cast<float>(j) / size));
            if(point_map_row[curIndex].x <= minX)
                minX=point_map_row[curIndex].x;
            if(point_map_row[curIndex].x >= maxX)
                maxX=point_map_row[curIndex].x;
            if(point_map_row[curIndex].y <= minY)
                minX=point_map_row[curIndex].y;
            if(point_map_row[curIndex].y >= maxY)
                maxX=point_map_row[curIndex].y;
            points.push_back(point_map_row[curIndex]);
        }
    }
    using namespace cimg_library;
    auto doubleSize = static_cast<double>(size);
    
    int lengthX = maxX-minX;
    
    float differenceZ = abs(lengthZ-doubleSize);
    float difPercent = (doubleSize-lengthZ)/abs(lengthZ);
    std::cout<< "length Z = " << lengthZ << " amount of rows = " << doubleSize << " Difference = " << differenceZ << " difference percentage = " << difPercent*100 << " length X = " << lengthX << std::endl;
    lengthX *= (1+difPercent);
    CImg<unsigned char> image;
    image.assign(64,64,1,3,0).fill(1,1,1,255).resize(lengthX*2,doubleSize*2);
    
    image.save_jpeg("heightmap.jpg",100);
    //InitializeMagick("");
    //Image heightMap(Geometry(lengthX*2,doubleSize*2), Color(0, 0, 0, 0));
    //emptyImage.modifyImage();
    
    
    
    //std::vector<float> knot_vector = BSpline<glm::vec3>::get_knot_vector(size - 1);
    //auto surface = new BSplineSurface(2, 2, size - 1, size - 1, knot_vector, knot_vector, points, 0.5);
    //return surface;
}
