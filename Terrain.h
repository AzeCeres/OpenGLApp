#pragma once
#include "glad/glad.h"
#include <iostream>
#include <vector>
#include "shader_t.h"
class Terrain
{
public:
    const unsigned int NUM_PATCH_PTS = 4;
    Terrain();
    //Terrain(const char* heightmap, int rezIn, int sizeDivisor, Shader* tessHeightMapShader);
    Terrain(unsigned char* data, int width, int height, int nrChannels, int rezIn, float sizeDivisorIn,
            ShaderT* tessHeightMapShaderIn);
    void bind();
    Terrain(unsigned char* data, int width, int height, int nrChannels, int rezIn, float sizeDivisorXIn, float sizeDivisorYIn, float sizeDivisorZIn,
            ShaderT* tessHeightMapShaderIn);
    void draw();
    void setShader(ShaderT *shaderIn);
    int rez;
    ~Terrain();
    void clear();
    float getHeightAtPixel(float x, float z);
    float getHeightAtPoint(float x, float z);
    float interpolateHeightAtPoint(float x, float z);
    glm::vec3 calculateBarycentricCoordinates(const glm::vec2& point, const glm::ivec2& p1, const glm::ivec2& p2, const glm::ivec2& p3);
    float sizeDivisorX,sizeDivisorY,sizeDivisorZ;
private:
    std::vector<float> vertices;
    int width;
    int height;
    
    ShaderT *tessHeightMapShader;
    unsigned int terrainVAO, terrainVBO;
    unsigned int heightmapTexture;
    std::vector<float> texelData;
    void setupTexelData();
};
