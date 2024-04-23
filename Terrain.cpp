#include "Terrain.h"


//Terrain::Terrain()
//{
//    tessHeightMapShader = Shader("shaders/heightmap.vert", "shaders/heightmap.tesc", "shaders/heightmap.tes", "shaders/heightmap.frag");
//    Terrain("heightmaps/heightmap.png",20, 1, &tessHeightMapShader);
//}


Terrain::Terrain(unsigned char *data,  int width, int height, int nrChannels, int rezIn, int sizeDivisor, Shader* tessHeightMapShaderIn)
{
    GLint maxTessLevel;
    glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessLevel);
    std::cout << "Max available tess level: " << maxTessLevel << std::endl;

    // load image, create texture and generate mipmaps
    //int width, height, nrChannels;
    //heightmap - res- 1920 x 938 - verts - 1'800'960
    //unsigned char *data = stbi_load("heightmaps/heightmap.png", &width, &height, &nrChannels, 0);
    //hqheightmap - res - 3840 x 1876 - verts - 7'203'840
    //unsigned char *data = stbi_load("heightmaps/hqheightmap.png", &width, &height, &nrChannels, 0);
    // uhqheightmap - res - 7680 - 4320 - verts - 33,177,600 // The area is also stretched out further, rather than just being a higher resolution
    // this can lead one to perceive the terrain is lower quality than it actually is
    //unsigned char *data = stbi_load("heightmaps/uhqheightmap.png", &width, &height, &nrChannels, 0);
    //unsigned char *data = stbi_load(heightmap, &width, &height, &nrChannels, 0);
    
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        tessHeightMapShader = tessHeightMapShaderIn;
        tessHeightMapShader->setInt("heightMap", 0);
        std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<float> vertices;
    rez = rezIn;
    //(e.g. 20,40,60)(higher is recommended for larger heightmaps): ";
    if(rez < 1)
        rez = 1;
    else if (rez > 120)
        rez = 120;
    //unsigned sizeDivisor = 1;
    //the size divisor of the terrain (e.g. 1,2,4)(to normalize the size of the maps): ";
    height = height / sizeDivisor;
    width = width / sizeDivisor;
    
    for(unsigned i = 0; i <= rez-1; i++)
    {
        for(unsigned j = 0; j <= rez-1; j++)
        {
            vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
            vertices.push_back(i / (float)rez); // u
            vertices.push_back(j / (float)rez); // v

            vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
            vertices.push_back((i+1) / (float)rez); // u
            vertices.push_back(j / (float)rez); // v

            vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
            vertices.push_back(i / (float)rez); // u
            vertices.push_back((j+1) / (float)rez); // v

            vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
            vertices.push_back((i+1) / (float)rez); // u
            vertices.push_back((j+1) / (float)rez); // v
        }
    }
    std::cout << "Loaded " << rez*rez << " patches of 4 control points each" << std::endl;
    std::cout << "Processing " << rez*rez*4 << " vertices in vertex shader" << std::endl;

    // first, configure the cube's VAO (and terrainVBO)
    
    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);
}

void Terrain::draw()
{
    //tessHeightMapShader->setInt("heightMap", 0);
    tessHeightMapShader->use();
    glBindVertexArray(terrainVAO);
    glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS*rez*rez);
}

void Terrain::setShader(Shader *shaderIn)
{
    tessHeightMapShader = shaderIn;
}

Terrain::~Terrain()
{
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
}

void Terrain::clear()
{
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
}
