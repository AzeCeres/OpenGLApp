#pragma once
#include <algorithm>
#include <string>

#include <glm/gtx/vec_swizzle.hpp>
#include "las.h"

#include <map>
#include <numeric>

#include "CImg.h"
#include "shaderVF.h"
#include "Transform.h"
#include "Vertex.h"
#include "glad/glad.h"
#include "glm/ext/scalar_constants.hpp"
template<typename T>
    T scalarDiff(T startVal, T endVal)
{
    return (endVal/startVal);
}
template<typename T>
double getAverage(std::vector<T> const& v) {
    if (v.empty()) {
        return 0;
    }
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}
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
    cimg_library::CImg<float> vertConnect(cimg_library::CImg<float> newImage, std::vector<int>* filledColumns, int maxBlanks);
    cimg_library::CImg<float> horzConnect(cimg_library::CImg<float> newImage, std::vector<int>* rowsToFill,
                                          std::vector<int>* filledRows, std::vector<int>* filledCollumns, int maxBlanks);
    void convertToImage(std::string imagePath);
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
    void bind()
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

    void setup(std::string imagePath)
    {
        using namespace cimg_library;
        CImg<float> image;
        try { image.load(imagePath.c_str()); } catch (CImgIOException) { convertToImage(imagePath); }  //Rudimentary check to see if there is an image, not necessarily one for the correct las file.
        //convertToImage(imagePath);
        bind();
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


cimg_library::CImg<float> PointCloud::vertConnect(cimg_library::CImg<float> newImage, std::vector<int>* filledColumns, int maxBlanks)
{
    for (int x = 0; x < newImage.width(); ++x)
    {
        auto it = std::find(filledColumns->begin(),filledColumns->end(), x);
        if(it != filledColumns->end()) continue; // skips row if it has already been filled
        std::vector<int> blueValsInARow;
        std::vector<int> redValsInARow;
        for (int y = 0; y < newImage.height(); ++y)
        {
            auto blueVal = newImage.atXY(x,y,0,2);
            //auto redVal = newImage.atXY(x,y,0,0);
            if (blueVal >= 254.5)
            {
                if(blueValsInARow.empty())
                {
                    if (y!=0) 
                        blueValsInARow.emplace_back(y-1);
                }
                blueValsInARow.emplace_back(y);
                redValsInARow.clear();
            }
            else {
                redValsInARow.emplace_back(y);
                if(redValsInARow.size() == newImage.height())
                {
                    auto it = std::find(filledColumns->begin(),filledColumns->end(), x);
                    if(it == filledColumns->end()) // not found, then add
                        filledColumns->emplace_back(x);
                }
                if(blueValsInARow.empty()) continue;
                blueValsInARow.emplace_back(y);
                float firstBlueVal = newImage.atXY(x,blueValsInARow[0],0,2);
                float lastBlueVal = newImage.atXY(x,blueValsInARow[blueValsInARow.size()-1],0,2);
                float firstRedVal = newImage.atXY(x,blueValsInARow[0],0,0);
                float lastRedVal = newImage.atXY(x,blueValsInARow[blueValsInARow.size()-1],0,0);
                //check if it has reds on both sides
                if(firstBlueVal < 254.5 && lastBlueVal < 254.5) //if neither ends are blue, aka both are red
                {
                    if (blueValsInARow.size()-2 > maxBlanks)
                    {
                        blueValsInARow.clear();
                        continue;
                    }
                    for (int i = 1; i < blueValsInARow.size()-1; ++i)
                    {
                        float colorVal = std::lerp(firstRedVal,lastRedVal, (float)(i)/(float)(blueValsInARow.size()-1));;
                        const float color[] = {colorVal ,0.f,0.f };
                        newImage.draw_point(x,blueValsInARow[i], 0, color);
                    }
                }
                //check if it only bottom is red
                else if (firstBlueVal >= 254.5) //if left is blue, then bottom is red
                {
                    if (blueValsInARow.size()-1 > maxBlanks)
                    {
                        blueValsInARow.clear();
                        continue;
                    }
                    const float color[] = { lastRedVal ,0.f,0.f };
                    for (int i = 0; i <= y; ++i)
                    {
                        newImage.draw_point(x,i, 0, color);
                    }
                }
                blueValsInARow.clear();
            }
            if(blueValsInARow.size()-1 > maxBlanks) continue;
            if (!blueValsInARow.empty() && y == newImage.height()-1 && blueValsInARow.size() != newImage.height())
            {
                float firstRedVal = newImage.atXY(x,blueValsInARow[0],0,0);
                const float color[] = {firstRedVal ,0.f,0.f };
                for (int i = 0; i <= blueValsInARow.size(); ++i)
                {
                    newImage.draw_point(x,y-i, 0, color);
                }
            }
        }
    }
    newImage.save_png("tempHeightVert.png", 8);
    return newImage;
}

cimg_library::CImg<float> PointCloud::horzConnect(cimg_library::CImg<float> newImage, std::vector<int>* rowsToFill, std::vector<int>* filledRows, std::vector<int>* filledCollumns, int maxBlanks)
{
    for (int y = 0; y < newImage.height(); ++y)
    {
        auto it = std::find(filledRows->begin(),filledRows->end(), y);
        if(it != filledRows->end()) continue; // skips row if it has already been filled
        std::vector<int> blueValsInARow;
        std::vector<int> redValsInARow;
        std::vector<int> prevRedValsInARow;
        for (int x = 0; x < newImage.width(); ++x)
        {
            auto blueVal = newImage.atXY(x,y,0,2);
            //auto redVal = newImage.atXY(x,y,0,0);
            if (blueVal >= 254)
            {
                if(blueValsInARow.empty())
                {
                    if (x!=0) 
                        blueValsInARow.emplace_back(x-1);
                    prevRedValsInARow.clear();
                    prevRedValsInARow = redValsInARow;
                    redValsInARow.clear();
                }
                blueValsInARow.emplace_back(x);
            }
            else
            {
                redValsInARow.emplace_back(x);
                if(redValsInARow.size() == newImage.width())
                {
                    auto it = std::find(filledRows->begin(),filledRows->end(), y);
                    if(it == filledRows->end()) // not found, then add
                        filledRows->emplace_back(y);
                }
                if(blueValsInARow.empty()) continue;
                blueValsInARow.emplace_back(x);
                //float firstBlueVal = newImage.atXY(blueValsInARow[0],y,0,2);
                //float lastBlueVal = newImage.atXY(blueValsInARow[blueValsInARow.size()-1],y,0,2);
                float firstRedVal = newImage.atXY(blueValsInARow[0],y,0,0);
                float lastRedVal = newImage.atXY(blueValsInARow[blueValsInARow.size()-1],y,0,0);
                //check if it has reds on both sides
                if(firstRedVal >= 0.05f && lastRedVal >= 0.05f) //if both ends are red
                {
                    if (blueValsInARow.size()-2 > maxBlanks)
                    {
                        blueValsInARow.clear();
                        continue;
                    }
                    for (int i = 1; i < blueValsInARow.size()-1; ++i)
                    {
                        float colorVal = std::lerp(firstRedVal,lastRedVal, (float)(i)/(float)(blueValsInARow.size()-1));
                        const float color[] = {colorVal ,0.f , 0.f};
                        int xPos = //(x-(blueValsInARow.size()-1))+i;
                            blueValsInARow[i];
                        newImage.draw_point(xPos,y, 0, color);
                        //prevRedValsInARow.emplace_back(xPos);
                    }
                }
                else//check if it only has right red
                {
                    if (blueValsInARow.size()-1 > maxBlanks)
                    {
                        blueValsInARow.clear();
                        continue;
                    }
                    const float color[] = { lastRedVal ,0.f,0.f };
                    for (int i = 0; i <= x; ++i)
                    {
                        newImage.draw_point(i,y, 0, color);
                        //prevRedValsInARow.emplace_back(i);
                    }
                }
                //for (int i = 0; i < redValsInARow.size(); ++i)
                //{
                //    int xPos = redValsInARow[i];
                //    //auto it = std::find(prevRedValsInARow.begin(),prevRedValsInARow.end(), xPos);
                //    //if(it != prevRedValsInARow.end()) 
                //        prevRedValsInARow.emplace_back(xPos);
                //}
                //redValsInARow = prevRedValsInARow;
                
                //else if(redValsInARow.size() > newImage.width())
                // std::cout << "Something SUS happend, TOO MANY IN A ROW " << redValsInARow.size();
                blueValsInARow.clear();
            }
            if(x != newImage.width()-1) continue;
            if(blueValsInARow.empty()) continue;
            if(blueValsInARow.size()-1 > maxBlanks) continue;
            //if(blueValsInARow.size() == newImage.width()) continue;
            
            float firstRedVal = newImage.atXY(blueValsInARow[0],y,0,0);
            const float color[] = {firstRedVal ,0.f,0.f };
            for (int i = 0; i <= blueValsInARow.size(); ++i)
            {
                newImage.draw_point(x-i,y, 0, color);
                prevRedValsInARow.emplace_back(x-i);
            }
            //redValsInARow = prevRedValsInARow;
        }
    }
    newImage.save_png("tempHeightHorz.png", 8);
    return vertConnect(newImage, filledCollumns, maxBlanks);
}

inline void PointCloud::convertToImage(std::string imagePath)
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
        if(vertex.position.x <= minX)
            minX=vertex.position.x;
        if(vertex.position.x >= maxX)
            maxX=vertex.position.x;
        if(vertex.position.y <= minY)
            minY=vertex.position.y;
        if(vertex.position.y >= maxY)
            maxY=vertex.position.y;
    }
    
    for (auto vertex : vertices)
    {
        //check to see if current Z-value already has a column, if not, create one
        if (point_map.find(vertex.position.z) == point_map.end())
        {
            point_map[vertex.position.z] = {};
        }
        //Add vertex position to the correct Z-column
        point_map[vertex.position.z].push_back(vertex.position);
    }
    // sorts all of the positions by their x-values so that they are all in order
    for (auto &pair : point_map)
    {
        std::sort(pair.second.begin(), pair.second.end(), [](glm::vec3 a, glm::vec3 b)
                  { return a.x < b.x; });
    }
    //size is set to the number of unique z values in point_map
    auto size = point_map.size();
    using namespace cimg_library;
    auto doubleSize = static_cast<int>(size);
    
    int   lengthX = maxX-minX;
    float lengthY = maxY-minY;
    int   lengthZ = maxZ-minZ;
    
    float differenceZ = abs(lengthZ-doubleSize);
    float difPercentY = scalarDiff(maxY+abs(minY),255.f); // minY should be 0, maxY should be 255
    float difPercentX = (scalarDiff(lengthZ, doubleSize) + 1);
    std::cout<< "length Z = " << lengthZ << " amount of rows = " << doubleSize << " Difference = " << differenceZ << " length X = " << lengthX << std::endl;
    std::cout<< "maxY = " << maxY << " minY = " << minY << " difY = " << difPercentY << " maxY + minY = " << (maxY+abs(minY))*difPercentY << std::endl;
    
    const int imgWidth = lengthX * difPercentX *2;
    std::cout << "width: "<< imgWidth << std::endl;
    const int imgHeight = lengthZ * 2;
    float xStep = (float)lengthX/(float)imgWidth;
    CImg<float> image;
    
    image.assign((float)imgWidth,(float)imgHeight, 1, 3);
    const float blue[] = { 0.f,0.f,255.f };
    image.draw_fill(0,0,0, blue, 1.f);
    std::cout << lengthX*2 << std::endl;
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
        int imgYPos = (abs(minZ) + actual_map_index)*2;
        std::cout << "x points in row "<< actual_map_index << " " << point_map_row.size() << std::endl;
        std::vector<float> heightsToAverage;
        std::vector<float> xVals;
        float currentStep = xStep;
        for (int j = 0; j < size; j++)
        {
            auto curIndex = static_cast<int>(point_map_row.size() * (static_cast<float>(j) / size));
            float xToImgVal = point_map_row[curIndex].x + abs(minX);
            if (xToImgVal > currentStep)
            {
                float averageHeight = getAverage(heightsToAverage);
                const float color[]{(abs(minY)+averageHeight)*difPercentY, 0,0};
                image.draw_point((int)(currentStep/xStep)-1,imgYPos, 0, color);
                heightsToAverage.clear();
                xVals.clear();
                while(xToImgVal >=  currentStep)
                    currentStep += xStep;
                
            }
            if (xToImgVal <= currentStep)
            {
                heightsToAverage.emplace_back(point_map_row[curIndex].y);
                xVals.emplace_back(point_map_row[curIndex].x);
            }
            //image.draw_point(j, (int)(actual_map_index+minZ)*2, );
            points.push_back(point_map_row[curIndex]);
        }
    }
    
    image.save_png("heightmapRaw.png", 4);
    CImgDisplay local(image, "heightmapRaw");
    CImg<float>  newImage;
    newImage = image;
    std::vector<int> blankRows;
    std::vector<int> filledRows;
    std::vector<int> filledColumns;
    int maxBlanks = 1;
    while (filledRows.size() != newImage.height() && filledColumns.size() != newImage.width())
    {
        newImage = horzConnect(newImage, &blankRows, &filledRows, &filledColumns, maxBlanks);
        //newImage = vertConnect(newImage, blankRows); //called through the horzConnect
        std::string newFileName = "heightmap";
        newFileName += std::to_string(maxBlanks);
        newFileName += ".png";
        newImage.save_png(newFileName.data(), 4);
        local.display(newImage);
        std::cout << "Finished Iteration " << maxBlanks << std::endl;
        std::cout << "  Current Rows Filled = " << filledRows.size() << std::endl;
        std::cout << "  Current Cols Filled = " << filledColumns.size() << std::endl;
        maxBlanks += maxBlanks/4 + 1;
    }
    CImg<float>  blurPhoto;
    blurPhoto = newImage;
    blurPhoto.blur(1);
    blurPhoto.save_png(imagePath.c_str(), 4);

    using namespace std;
    ofstream outputFile("heightmaps/HeightmapInfo.txt");
    if (!outputFile.is_open()) {
        cerr << "Error opening the output file!" << "\n";
        return;
    }
    outputFile << blurPhoto.height() << endl;
    outputFile << blurPhoto.width() << endl;
    outputFile << difPercentX << endl;
    outputFile << difPercentY << endl;
    
    
    //std::vector<float> knot_vector = BSpline<glm::vec3>::get_knot_vector(size - 1);
    //auto surface = new BSplineSurface(2, 2, size - 1, size - 1, knot_vector, knot_vector, points, 0.5);
    //return surface;
}
