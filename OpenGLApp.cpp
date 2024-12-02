#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader_t.h"
#include "shaderVF.h"
#include "camera.h"

#include <iostream>
#include <vector>

#include "NormalMap.h"
#include "pointcloud.h"
#include "Sphere.h"

#include "Terrain.h"
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifiers);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool wireframe = false;
bool isTerrain = true;

// camera - give pretty starting point
Camera camera(glm::vec3(1.0f, 3.0f, 20.5f),
              glm::vec3(0.0f, 1.0f, 0.0f),
              -0.0f, -1.f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// global objects
Terrain* terrain;
auto pointCloud = new PointCloud("pointcloud/island3dn.las");

//// lighting
//glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
//// positions of the point lights
//glm::vec3 pointLightPositions[] = {
//    glm::vec3( 0.7f,  0.2f,  2.0f),
//    glm::vec3( 2.3f, -3.3f, -4.0f),
//    glm::vec3(-4.0f,  2.0f, -12.0f),
//    glm::vec3( 0.0f,  0.0f, -3.0f)
//};
int main();

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tesselated Pointcloud Terrain", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    
    // configure global opengl state
    // -----------------------------|
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE); // todo disable?              // not needed for pointcloud //  not needed for terrain// !Destroys! Terrain
    glDepthMask(GL_TRUE);   // todo disable?            // not needed for pointcloud // not needed for terrain // works with terrain
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// todo disable? // not needed for pointcloud // not needed for terrain // works with terrain
    glEnable(GL_BLEND);     // todo disable?              // not needed for pointcloud // not needed for terrain // works with terrain
    // build and compile our shader program
    // ------------------------------------
    ShaderT tessHeightMapShader("shaders/gpuheight.vs", "shaders/gpuheight.fs",
                                "shaders/gpuheight.tcs", "shaders/gpuheight.tes");
    
    ShaderVF noLightShader("shaders/default.vs", "shaders/noLight.fs");
    
    //unsigned char *data = stbi_load("heightmaps/uhqheightmap.png", &width, &height, &nrChannels, 0); // rez 15-20, sizediv 1
    //unsigned char *data = stbi_load("heightmaps/hqheightmap.png", &width, &height, &nrChannels, 0); // rez 20-25, sizediv 2
    //unsigned char *data = stbi_load("heightmaps/uhqheightmap.png", &width, &height, &nrChannels, 0); // rez 20-25, sizediv 4
    //Terrain terrain(data, width, height, nrChannels, 20, 4, &tessHeightMapShader);
    std::vector<Sphere> spheres;
    //spheres.emplace_back();
    //spheres[0].setup(2);
    //spheres[0].set_position(glm::vec3(13,10,12));
    //spheres[0].set_shader(&noLightShader);
    pointCloud->set_shader(&noLightShader);
    pointCloud->hasData();
    std::string path = "heightmaps/Island3Heightmap.png"; // !NB! if the las file is changed but the output imagePath/png remains, then it'll skip making a new image, thinking it's the same image
    //!NB! Will throw a pop-up with an abort button when the image doesn't exist. That is Expected! just let it run, you'll see it start iterating over the image in the console.
    pointCloud->setup(path);
    NormalMap normal_map;
    normal_map.create_normal_from_height(path, 2);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0); // rez 15-20, sizediv 1
    terrain = new Terrain(data, width, height, nrChannels, 7, 4,4,2, &tessHeightMapShader); // rez 15-20, sizediv 1

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        //std::cout << deltaTime << "ms (" << 1.0f / deltaTime << " FPS)" << std::endl; //

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // be sure to activate shader when setting uniforms/drawing objects
        if (isTerrain)
        {
            tessHeightMapShader.use();
        }
        else
        {
            noLightShader.use(); 
        }
        //glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        if (isTerrain)
        {
            tessHeightMapShader.setMat4("projection", projection);
            tessHeightMapShader.setMat4("view", view);
            tessHeightMapShader.setFloat("heightDiv", terrain->sizeDivisorY);
        }
        else
        {
            noLightShader.setMat4("projection", projection);
            noLightShader.setMat4("view", view);
        }
        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        if (isTerrain)
        {
            tessHeightMapShader.setMat4("model", model);
        }
        else
        {
            noLightShader.setMat4("model", model);
        }
        
        // render the terrain
        if (wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (isTerrain)
        {
            terrain->draw();
            float terrainHeight =terrain->getHeightAtPoint(camera.Position.x,camera.Position.z);
            if(camera.Position.y < terrainHeight+1)
            {
                std::cout << "Camera below terrain!" << std::endl;
                camera.Position.y = terrainHeight+1;
            }
            std::cout << "Height at (" << camera.Position.x << " " << camera.Position.z << "):" << terrain->getHeightAtPoint(camera.Position.x,camera.Position.z) << std::endl;
        }
        else
        {
            pointCloud->draw();
        }
        noLightShader.use();
        noLightShader.setMat4("projection", projection);
        noLightShader.setMat4("view", view);
        noLightShader.setMat4("model", model);
        for (int i = 0; i < spheres.size(); ++i)
        {
            spheres[i].draw();
        }

        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    terrain->clear();
    stbi_image_free(data);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever a key event occurs, this callback is called
// ---------------------------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifiers)
{
    if(action == GLFW_PRESS)
    {
        switch(key)
        {
        case GLFW_KEY_F:
            wireframe = !wireframe;
            break;
        case GLFW_KEY_T:
            isTerrain = !isTerrain;
            if (isTerrain)
            {
                terrain->bind();
            }
            else
            {
                pointCloud->bind();
            }
            break;
        }
    }
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

