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
    Terrain(unsigned char* data, int width, int height, int nrChannels, int rezIn, int sizeDivisor,
            Shader* tessHeightMapShaderIn);
    void draw();
    void setShader(Shader *shaderIn);
    int rez;
    ~Terrain();
    void clear();
private:
    Shader *tessHeightMapShader;
    unsigned int terrainVAO, terrainVBO;
};
