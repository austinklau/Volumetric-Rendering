#define STBVOX_CONFIG_MODE
#define GLEW_STATIC // GLEW
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h> // GLFW
#include <math.h>
#include <stdlib.h>
#include "Shader.h" // include our shader class
#include "camera.h" // include camera class
#include "Mesh.h" // include our Mesh class
#include "Model.h" // include our Model class
#include "resources/frameworks/stb_image.h" // image loading library
#include <glm/glm.hpp> // GLM
#include <glm/gtc/noise.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


int toRender;

//----- LDNI parameters
const unsigned int cubeLength = 128;
void shellGenerator(vector<bool>& model, vector<int>& solids, vector<float>& vertices); // pass by ref


// Creates a mesh from a 3d-array of binary block data
void greedyMesh(vector<vector<vector<bool>>> &vec, vector<float> &vertices) {
    const float chunkSize = vec.size();
    // Sweep over each axis
    for (int d = 0; d < 3; ++d) {
        int i, j, k, l, w, h;
        int u = (d+1) % 3;
        int v = (d+2) % 3;
        int x [3] = {0, 0, 0};
        int q [3] = {0, 0, 0};
        vector<bool> mask(chunkSize * chunkSize);
        q[d] = 1; // direction of sweep
        
        // check each slice of the chunk one at a time
        for (x[d] = -1; x[d] < chunkSize;) {
            
            // compute the mask
            int n = 0;
            for (x[v] = 0; x[v] < chunkSize; ++x[v]) {
                for (x[u] = 0; x[u] < chunkSize; ++x[u]) {
                    // q determines search direction
                    // mask set to true if visible face between 2 blocks
                    bool blockCurrent = 0 <= x[d] ? vec[x[0]][x[1]][x[2]]: true;
                    bool blockCompare = x[d] < chunkSize - 1 ?
                        vec[x[0] + q[0]][x[1] + q[1]][x[2] + q[2]]: true;
                    
                    mask[n++] = blockCurrent != blockCompare;
                
                }
            }
            ++x[d];
            
            n = 0;
            // Generate a mesh from the mask using lexicographic ordering
            // by looping over each block in this slice of the chunk
            for (j = 0; j < chunkSize; ++j) {
                for (i = 0; i < chunkSize;) {
                    if (mask[n]) {
                        // Compute the width of this quad and store it in w
                        // This is done by searching along the current axis
                        // until mask[n + w] is false
                        for (w = 1; i + w < chunkSize && mask[n+w]; w++) { }
                            
                            // Compute the height of this quad and store it in h
                            // This is done by checking if every block next to this row
                            // (range 0 to w) is also part of the mask.
                            // For example, if w is 5 we currently have a quad of dimensions 1 x 5.
                            // To reduce triangle count, greedy meshing will attempt to expand
                            // this quad out to CHUNK_SIZE x 5, but will stop if it reaches a hole in the mask
                            bool done = false;
                            for (h = 1; j + h < chunkSize; h++) {
                                // Check each block next to this quad
                                for (k = 0; k < w; ++k) {
                                    // If hole in the mask, exit
                                    if (!mask[n + k + h * chunkSize]) {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done) break;
                            }
                            x[u] = i;
                            x[v] = j;
                            
                            // du, dv determine size & orientation of this face
                        int du [3] = {0, 0, 0};
                            du[u] = w;
                            
                            int dv [3] = {0, 0, 0};
                            dv[v] = h;
                            
                            // Create a quad (2 tris) for this face
                        
                            // Top left vertex
                            vertices.push_back(x[0]);
                            vertices.push_back(x[1]);
                            vertices.push_back(x[2]);
                            // Top right vertex
                            vertices.push_back(x[0] + du[0]);
                            vertices.push_back(x[1] + du[1]);
                            vertices.push_back(x[2] + du[2]);
                            // Bottom left vertex
                            vertices.push_back(x[0] + dv[0]);
                            vertices.push_back(x[1] + dv[1]);
                            vertices.push_back(x[2] + dv[2]);
                        
                        
                            // Bottom right vertex
                            vertices.push_back(x[0] + du[0] + dv[0]);
                            vertices.push_back(x[1] + du[1] + dv[1]);
                            vertices.push_back(x[2] + du[2] + dv[2]);
                            // Bottom left vertex
                            vertices.push_back(x[0] + dv[0]);
                            vertices.push_back(x[1] + dv[1]);
                            vertices.push_back(x[2] + dv[2]);
                            // Top right vertex
                            vertices.push_back(x[0] + du[0]);
                            vertices.push_back(x[1] + du[1]);
                            vertices.push_back(x[2] + du[2]);
                            
                            // Clear this part of the mask to not add duplicate faces
                            for (l = 0; l < h; ++l) {
                                for (k = 0; k < w; ++k) {
                                    mask[n + k + l * chunkSize] = false;
                                }
                            }
                            // increment counters and continue
                            i += w;
                            n += w;
                        }
                        else {
                            i++;
                            n++;
                    }
                }
            }
        }
    }
}


//----- OPENGL PREREQS

// Callback Functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// Window Parameters
const GLint WIDTH = 1080, HEIGHT = 720; // window size

// Camera
Camera camera(glm::vec3(0, 0, 0));
float lastX = WIDTH / 2, lastY = HEIGHT / 2; // center of window coords
bool firstMouse = true;

// Timing variables
float deltaTime = 0.0f;    // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame


int main()
{
    
    glfwInit(); // initialize GLFW first
    
    // set OpenGL window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // tell GLFW we are using OpenGL v3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // use core profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // essential for MacOS
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE); // allows/prevents window from being resized
    
    // CREATE A WINDOW OBJECT
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "LDNI Greedy Mesher", nullptr, nullptr);
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    
    if (window == NULL) // check if window created successfully
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // GLEW manages function pointers for OpenGL
    // We initialize GLAD before we call any OpenGL function
    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    //-----CALLBACK FUNCTIONS
    // Register after creating window, before render loop
    glViewport(0, 0, screenWidth, screenHeight); // size of rendering window
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // set size callback
    glfwSetCursorPosCallback(window, mouse_callback); // mouse callback
    glfwSetScrollCallback(window, scroll_callback); // scroll callback
    // Fix cursor in center of window if enabled (lets it capture mouse)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Enable depth buffer-prevents objects from overlapping each other
    glEnable( GL_DEPTH_TEST );
    
    
    // Enable face-culling for efficiency
//    glEnable(GL_CULL_FACE);
//    glFrontFace(GL_CCW);
//    glCullFace(GL_BACK);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframes

    
    
    // build and compile shaders
    Shader shader("resources/shaders/core.vs", "resources/shaders/core.fs");
   
    // 3D vector of volumetric space xyz
    vector<vector<vector<bool>>> vec(cubeLength, vector<vector<bool>>(cubeLength, vector<bool>(cubeLength, true)));
    
    // vector of vertices to render
    vector<float> vertices;
    
    
    // Terrain generation for test case
    for (int x = 0; x < cubeLength; x++) {
        for (int z = 0; z < cubeLength; z++) {
            float yScale = glm::simplex(glm::vec2{x / 80.f, z / 80.f}); // generate a random height map
            yScale = (yScale + 1) / 2;
            // yScale = (int) (yScale * cubeLength / 10);
            yScale = (int) (yScale * 64);
            for (int y = 0; y <= yScale; y++) {
                vec[x][y][z] = false;
            }
        }
    }

    
    
    // debug
    float preProTime = glfwGetTime();
    
    
    greedyMesh(vec, vertices); // run greedy mesh algo

    
    // debug
    cout << "vertices size: " << vertices.size() << endl;
    float elapsed = glfwGetTime() - preProTime;
    cout << "preprocessing " << elapsed << " s" << endl;

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    
    toRender = vertices.size() / 3; // number of triangles
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // be sure to activate shader when setting uniforms/drawing objects
        shader.use();
        shader.setVec3("viewPos", camera.Position);

        // configure transformation matrices
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        
        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        // model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));  // scale down
        shader.setMat4("model", model);
               
    
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        
    
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}



//----- OpenGL-specific Functions

// Callback function that activates each time window is resized
void framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
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
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) // added fly
        camera.ProcessKeyboard(FLY, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
        || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) // added sink
        camera.ProcessKeyboard(SINK, deltaTime);
    
    // Other controls
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) { // reset view to default
        camera.GoHome();
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // close window
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        toRender -= 1000;
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    toRender += 1000;
}


// glfw: whenever the mouse moves, this callback is called
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
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// (double) rand() / (RAND_MAX);
