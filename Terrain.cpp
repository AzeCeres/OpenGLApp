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

    //Terrain terrain("heightmaps/hqheightmap.png", 40, 2, &tessHeightMapShader);
    //Terrain terrain("heightmaps/uhqheightmap.png", 40, 4, &tessHeightMapShader);
    // load and create a texture
    // -------------------------    
    glGenTextures(1, &heightmapTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmapTexture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
        this->height = height;
        this->width = width;
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
// Function to get the height at a specified point in the heightmap using texture sampling
float Terrain::getHeightAtPixel(float x, float z) {
    // Calculate the corresponding texture coordinates
    float texCoordX = x / static_cast<float>(width);
    float texCoordZ = z / static_cast<float>(height);

    // Activate the texture unit and bind the heightmap texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmapTexture);

    // Sample the texture at the calculated coordinates
    std::vector<float> texelData(width * height); // Assuming single-channel texture format (GL_RED)
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, texelData.data());

    // Convert texture coordinates to pixel coordinates
    int pixelX = static_cast<int>(texCoordX * width);
    int pixelZ = static_cast<int>(texCoordZ * height);

    // Clamp pixel coordinates to valid range
    pixelX = std::max(0, std::min(width - 1, pixelX));
    pixelZ = std::max(0, std::min(height - 1, pixelZ));

    // Calculate index of the pixel in the texture data
    int index = pixelZ * width + pixelX;

    // Retrieve the height value at the specified point
    float heightValue = texelData[index];
    heightValue *= 64.0 - 16.0; // Scale the height value to the desired range, same as within the shader
    return heightValue;
}

float Terrain::getHeightAtPoint(float x, float z)
{
    x += width / 2.0f;
    z += height / 2.0f;
    return interpolateHeightAtPoint(x,z);
}

// Function to interpolate the height at a specified point using barycentric coordinates
float Terrain::interpolateHeightAtPoint(float x, float z) {
    // Calculate the corresponding texture coordinates
    float texCoordX = x / static_cast<float>(width);
    float texCoordZ = z / static_cast<float>(height);

    // Convert texture coordinates to pixel coordinates
    int pixelX = static_cast<int>(texCoordX * width);
    int pixelZ = static_cast<int>(texCoordZ * height);

    // Clamp pixel coordinates to valid range
    pixelX = std::max(0, std::min(width - 1, pixelX));
    pixelZ = std::max(0, std::min(height - 1, pixelZ));

    // Determine the three pixels that form a triangle around the given coordinates
    glm::ivec2 pixel1(pixelX, pixelZ);
    glm::ivec2 pixel2(pixelX + 1, pixelZ);
    glm::ivec2 pixel3(pixelX, pixelZ + 1);

    // Calculate the barycentric coordinates of the point within the triangle
    glm::vec2 point(texCoordX * width, texCoordZ * height);
    glm::vec3 barycentric = calculateBarycentricCoordinates(point, pixel1, pixel2, pixel3);

    // Retrieve the heights of the three pixels
    float height1 = getHeightAtPixel(pixel1.x, pixel1.y);
    float height2 = getHeightAtPixel(pixel2.x, pixel2.y);
    float height3 = getHeightAtPixel(pixel3.x, pixel3.y);

    // Interpolate the height using barycentric coordinates
    float interpolatedHeight = height1 * barycentric.x + height2 * barycentric.y + height3 * barycentric.z;

    return interpolatedHeight;
}

// Function to calculate barycentric coordinates of a point within a triangle
glm::vec3 Terrain::calculateBarycentricCoordinates(const glm::vec2& point, const glm::ivec2& p1, const glm::ivec2& p2, const glm::ivec2& p3) {
    // Calculate vectors from point to vertices of the triangle
    glm::vec2 v0 = p3 - p1;
    glm::vec2 v1 = p2 - p1;
    glm::vec2 v2 = glm::vec2(point.x - p1.x, point.y - p1.y);

    // Compute dot products
    float dot00 = glm::dot(v0, v0);
    float dot01 = glm::dot(v0, v1);
    float dot02 = glm::dot(v0, v2);
    float dot11 = glm::dot(v1, v1);
    float dot12 = glm::dot(v1, v2);

    // Compute barycentric coordinates
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    float w = 1.0f - u - v;

    return glm::vec3(u, v, w);
}

