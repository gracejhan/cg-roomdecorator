// CSCI-GA.2270-001 Computer Graphics (Fall 2017)
// Assignment 4: Final Project
// Name: Jungwoo Han 
// Email: jh5990@nyu.edu
// Student ID: N17456718

#define STB_IMAGE_IMPLEMENTATION
// OpenGL Helpers to reduce the clutter
#include "Helpers.h"
#include <stdio.h>

// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>

// Linear Algebra Library
#include <Eigen/Core>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <Eigen/Dense>

// Timer
#include <chrono>

// Import image
#include "stb_image.h"

// Fix size and value
#define SCREEN_WIDTH 1400
#define SCREEN_HEIGHT 800
#define PI 3.141582

using namespace std;
using namespace Eigen;

// Count the total number of each objects to render
int obj_count = 0;

// View transformation matrices
Matrix4f view(4,4);
Matrix4f model(4,4);
Matrix4f projection(4,4);
Matrix4f final(4,4);
// Vector4f u_Translation(0., 0., 0., 0.);
Vector3f origin(0., 0., 0.);
Vector3f eye;
Vector3f gaze;
Vector3f up;
Vector3f w;
Vector3f u;
Vector3f v;

// Create vectors to store vec_VBO
typedef std::vector<VertexBufferObject> VBO_vector;
typedef std::vector<MatrixXf> Matrix_vector;

VBO_vector vec_VBO_V;             // Vector with VBO_V which has V
VBO_vector vec_VBO_UV;
VBO_vector vec_VBO_N;
VBO_vector vec_VBO_C;
VBO_vector vec_VBO_N_ver;
// VBO_vector vec_VBO_N_tri;
Matrix_vector vec_Mat_Model;    // Vector with modelMatrix matrix
Matrix_vector vec_Mat_Vertex;   // Vector with V matrix

int objID;
vector<int> objIDs;

Matrix4f M_aspect;
Matrix4f M_vp;
Matrix4f M_cam_0;
Matrix4f M_cam_1;
Matrix4f M_cam;
Matrix4f M_orth;

// Amount of translation for each axis
double Tx = 0;
double Ty = 0;
double Tz = 0;

// Relocate eye position
Vector3f move_eye(0., 0., 0.);

// Amount of scaling for objects
double ScaleRatio = 1.0;

double rot_angle = 0.;

int selected_obj_id;

// Used for changing drawing modes
int draw_count = 0;

// Different modes to draw multiple objects
enum Modes {roomfloor, roomwall, object1, object2, DrawCubeMode, ImportBcubeMode, ImportBunnyMode};
// enum Modes {object1, object2, DrawCubeMode, ImportBcubeMode, ImportBunnyMode};
Modes m;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
int inside_triangle(MatrixXf V, int i, Vector2d P);
int triangle_under_cursor(MatrixXf V, Vector2d P);
void TranslateObj(int objID);
void InitTransInputs();
void DeleteObj(int objID);
bool loadOBJ();
void loadFloor();
void loadWall();


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{   
    // Clicking on an object will return the id of the selected object
    // Get the position of the mouse in the window
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Get the size of the window
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    Eigen::Matrix4f M_vp;
    M_vp <<
        width/2., 0,      0, (width-1)/2.,
        0,      height/2., 0, (height-1)/2.,
        0,      0,      1, 0,
        0,      0,      0, 1;

    Vector4f p_screen(xpos,height-1-ypos,0,1);
    Vector4f p_canonical = M_vp.inverse()*p_screen;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        Vector2d mouse_position;
        mouse_position << p_canonical(0), p_canonical(1);
        // double min_depth = 1.;
        for(int i = 0; i < vec_VBO_V.size(); i++)
        {
            int selected = -1;
            selected = triangle_under_cursor(vec_Mat_Vertex[i], mouse_position);
            if(selected != -1)
            {
                selected_obj_id = i;
            }
        }
        cerr << "selected obj id: " << selected_obj_id << endl;
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // Get the position of the mouse in the window
    glfwGetCursorPos(window, &xpos, &ypos);

    // Get the size of the window
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    Eigen::Matrix4f M_vp;
    M_vp <<
        width/2., 0,      0, (width-1)/2.,
        0,      height/2., 0, (height-1)/2.,
        0,      0,      1, 0,
        0,      0,      0, 1;

    Vector4f p_screen(xpos,height-1-ypos,0,1);
    Vector4f p_canonical = M_vp.inverse()*p_screen;

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Update the position of the first vertex if the keys 1,2, or 3 are pressed
    switch (key)
    {
        case  GLFW_KEY_Z:
            if (action == GLFW_PRESS)
            {
                m = roomfloor;
                cerr << "key Floor start" << endl;
                loadFloor();
                cerr << "key Floor end" << endl;
            }
            break;
        case  GLFW_KEY_X:
            if (action == GLFW_PRESS)
            {
                m = roomwall;
                cerr << "key Wall start" << endl;
                loadWall();
                cerr << "key Wall end" << endl;
            }
            break;

        case  GLFW_KEY_1:
            if (action == GLFW_PRESS)
            {
                m = object1;
                cerr << "key 1 start" << endl;
                loadOBJ();
                cerr << "key 1 end" << endl;
            }
            break;
        case  GLFW_KEY_2:
            if (action == GLFW_PRESS)
            {
                m = object2;
                cerr << "key 2 start" << endl;
                loadOBJ();
                cerr << "key 2 end" << endl;
            }
            break;
        case GLFW_KEY_9:
            if (action == GLFW_PRESS) {
                cout << "Rotate counter clockwise (y-axis)" << endl;
                rot_angle = 10.;
                TranslateObj(selected_obj_id);
                rot_angle = 0.;
            }
            break;
        case GLFW_KEY_0:
            if (action == GLFW_PRESS) {
                cout << "Rotate clockwise (y-axis)" << endl;
                rot_angle = -10.;
                TranslateObj(selected_obj_id);
                rot_angle = 0.;
            }
            break;    
        case GLFW_KEY_D:
            if (action == GLFW_PRESS){
                DeleteObj(selected_obj_id);
                cout << "Delete" << endl;
            }
            break;    
        case GLFW_KEY_J:
            if (action == GLFW_PRESS){
                move_eye(0) -= 0.1;
            }
            break;   
        case GLFW_KEY_L:
            if (action == GLFW_PRESS){
                move_eye(0) += 0.1;
            }
            break;                                
        case GLFW_KEY_I:
            if (action == GLFW_PRESS){
                move_eye(1) -= 0.1;
            }
            break;                                
        case GLFW_KEY_K:
            if (action == GLFW_PRESS){
                move_eye(1) += 0.1;
            }
            break;                                

        case GLFW_KEY_UP:
            if (action == GLFW_PRESS){
                InitTransInputs();
                Ty = 0.1;
                TranslateObj(selected_obj_id);
                Ty = 0;
            }
            break;
        case GLFW_KEY_LEFT:
            if (action == GLFW_PRESS){
                InitTransInputs();
                Tx = -0.1;
                TranslateObj(selected_obj_id);
                Tx = 0;
            }
            break;
        case GLFW_KEY_DOWN:
            if (action == GLFW_PRESS){
                InitTransInputs();
                Ty = -0.1;
                TranslateObj(selected_obj_id);
                Ty = 0;
            }
            break;
        case GLFW_KEY_RIGHT:
            if (action == GLFW_PRESS){
                InitTransInputs();
                Tx = 0.1;
                TranslateObj(selected_obj_id);
                Tx = 0;
            }
            break;
        case GLFW_KEY_EQUAL:
            if (action == (GLFW_PRESS && GLFW_MOD_SHIFT)) {
                InitTransInputs();
                ScaleRatio = 1.1;
                TranslateObj(selected_obj_id);
                ScaleRatio = 1.0;
            }
            break;
        case GLFW_KEY_MINUS:
            if (action == GLFW_PRESS) {
                InitTransInputs();
                ScaleRatio = 0.9;
                TranslateObj(selected_obj_id);
                ScaleRatio = 1.0;
            }
            break;
        case GLFW_KEY_7:
            if (action == GLFW_PRESS) {
                draw_count += 1;
            }
            break;            
        case GLFW_KEY_ESCAPE:
            exit(0);
            break;            
        default:
            break;
    }
}

int main(void)
{
    GLFWwindow* window;

    // Initialize the library
    if (!glfwInit())
        return -1;

    // Activate supersampling
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "3D Scene Editor", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    #ifndef __APPLE__
      glewExperimental = true;
      GLenum err = glewInit();
      if(GLEW_OK != err)
      {
        /* Problem: glewInit failed, something is seriously wrong. */
       fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif

    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
    printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
    printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Initialize the VAO
    // A Vertex Array Object (or VAO) is an object that describes how the vertex
    // attributes are stored in a Vertex Buffer Object (or VBO). This means that
    // the VAO is not the actual object storing the vertex data,
    // but the descriptor of the vertex data.
    VertexArrayObject VAO;
    VAO.init();
    VAO.bind();

// **************************************************************************************************
    // // Load one texture
    // glActiveTexture(GL_TEXTURE0);    
    // GLuint tex;
    // glGenTextures(1, &tex);
    // glBindTexture(GL_TEXTURE_2D, tex);

    // // Import png image
    // int texture_width, texture_height, bpp;
    // unsigned char * rgb_array = stbi_load("../data/color.png", &texture_width, &texture_height, &bpp, 3);

    // // Check RGB data in png file
    // // for (int i = 0; i<100; ++i)
    // //     cerr << "--" << i << " - " << int(rgb_array[i]) << endl;

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_array);
    // stbi_image_free(rgb_array);

    // if(rgb_array == nullptr)
    //     printf("Cannot load texture image.\n");


    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

// **************************************************************************************************
    // Load multiple texture

    GLuint textures[4];
    glGenTextures(4, textures);

    int texture_width, texture_height, bpp;
    unsigned char * rgb_array;        

    // Texture 0 : Floor
    glActiveTexture(GL_TEXTURE0);   
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    rgb_array = stbi_load("../data/rug.png", &texture_width, &texture_height, &bpp, 3);
    if(rgb_array == nullptr)
        printf("Cannot load texture image.\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_array);
    stbi_image_free(rgb_array);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Texture 1 : Wall
    glActiveTexture(GL_TEXTURE1);   
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    rgb_array = stbi_load("../data/color.png", &texture_width, &texture_height, &bpp, 3);
    if(rgb_array == nullptr)
        printf("Cannot load texture image.\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_array);
    stbi_image_free(rgb_array);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Texture 2 : Obj1
    glActiveTexture(GL_TEXTURE2);   
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    rgb_array = stbi_load("../data/box.png", &texture_width, &texture_height, &bpp, 3);
    if(rgb_array == nullptr)
        printf("Cannot load texture image.\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_array);
    stbi_image_free(rgb_array);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Texture 3 : Obj2
    glActiveTexture(GL_TEXTURE3);   
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    rgb_array = stbi_load("../data/cube2.png", &texture_width, &texture_height, &bpp, 3);
    if(rgb_array == nullptr)
        printf("Cannot load texture image.\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_array);
    stbi_image_free(rgb_array);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);



    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
// **************************************************************************************************

    
    // Initialize the OpenGL Program
    // A program controls the OpenGL pipeline and it must contains
    // at least a vertex shader and a fragment shader to be valid
    Program program;
    const GLchar* vertex_shader =
            "#version 150 core\n"
                    "in vec3 position;"
                    "in vec3 a_normal;"
                    "in vec3 color;"
                    "in vec2 a_uv;"

                    "out vec3 f_color;"
                    "out vec2 uv;"      // uv coordinates to fragment shader
                    "out vec4 v_pos;"   // vertex positions
                    "out vec4 N;"       // vertex normals
                    "out float L;"                    

                    "uniform mat4 final;"
                    "uniform vec3 camera;"    
                    "uniform vec3 light;"
                    "void main()"
                    "{"
                    "    v_pos = final * vec4(position, 1.0);"
                    "    vec3 v = camera - position;"
                    "    vec3 l = light - position;"
                    "    vec3 h = normalize(v + l);"
                    "    L = 0.7 + 0.5 * max(0.0, dot(a_normal, l)) + 0.8 * pow(max(0.0, dot(a_normal, h)), 10);"
                    "    f_color = color;"
                    "    N = normalize(final * vec4(a_normal, 0.0));"
                    "    gl_Position = v_pos;"
                    "    uv = a_uv;"
                    "}";
    const GLchar* fragment_shader =
            "#version 150 core\n"
                    "in vec2 uv;"
                    "in vec4 v_pos;"
                    "in vec4 N;"
                    "in vec3 f_color;"
                    "in float L;"

                    "out vec4 outColor;"

                    "uniform sampler2D my_texture;"               
                    "void main()"
                    "{"
                    // "    outColor = L * vec4(f_color, 1.0);"
                    // "       outColor = texture(my_texture, uv);"
// **************************************************************************************************
                    "       outColor = vec4(texture(my_texture, uv.xy).rgb, 1.0);"
// **************************************************************************************************
                    // "       outColor = L * outColor;"
                    "}";

    // Compile the two shaders and upload the binary to the GPU
    // Note that we have to explicitly specify that the output "slot" called outColor
    // is the one that we want in the fragment buffer (and thus on screen)
    program.init(vertex_shader,fragment_shader,"outColor");
    program.bind();

    // Register the keyboard callback
    glfwSetKeyCallback(window, key_callback);

    // Register the mouse callback
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Register mouse cursor callback
    glfwSetCursorPosCallback(window, cursor_pos_callback);


    glEnable(GL_DEPTH_TEST);   
    glDepthFunc(GL_LESS);  

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Bind your VAO (not necessary if you have only one)
        VAO.bind();

        // Bind your program
        program.bind();

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float aspect_ratio = float(height)/float(width); // corresponds to the necessary width scaling

        float radian = float(PI * rot_angle) / 180.0;
        float cosB = (float)cos(radian);
        float sinB = (float)sin(radian);

        M_aspect <<
        aspect_ratio,0, 0, 0,
        0,           1, 0, 0,
        0,           0, 1, 0,
        0,           0, 0, 1;

        int n_x = width;
        int n_y = height;

        M_vp <<
        n_x/2., 0,      0, (n_x-1)/2.,
        0,      n_y/2., 0, (n_y-1)/2.,
        0,      0,      1, 0,
        0,      0,      0, 1;

        float l_ = -3;
        float r_ = 3;
        float b_ = -3;
        float t_ = 3;
        float n_ = 6;
        float f_ = -6;

        M_orth <<
        2./(r_ - l_), 0,            0,            -(float)(r_ + l_)/(r_ - l_),
        0,            2./(t_ - b_), 0,            -(float)(t_ + b_)/(t_ - b_),
        0,            0,            2./(n_ - f_), -(float)(n_ + f_)/(n_ - f_),
        0,            0,            0,            1;

        eye = Vector3f(1.2,0.6, -2.);
        eye = eye + move_eye;
        gaze = (eye - origin).normalized();
        up = Vector3f(0., 1., 0.);
        w = (-1) * gaze.normalized();
        u = (up.cross(w)).normalized();
        v = w.cross(u);

        M_cam.col(0) << u, 0;
        M_cam.col(1) << v, 0;
        M_cam.col(2) << w, 0;
        M_cam.col(3) << eye, 1;

        M_cam = M_cam.inverse().eval();

        // Set Pmatrix to change perspective style(orthogonal, perspective)
        Matrix4f Pmatrix(4,4);
        Pmatrix <<
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

        Vector3f light(1., 1., 0.);

        // Clear the framebuffer
        glClearColor(1.f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(int i = 0; i < vec_VBO_V.size(); i++)
        {   
            program.bindVertexAttribArray("position",vec_VBO_V[i]);
            program.bindVertexAttribArray("color", vec_VBO_C[i]);
            program.bindVertexAttribArray("a_normal", vec_VBO_N_ver[i]);
            program.bindVertexAttribArray("a_uv", vec_VBO_UV[i]);
            Matrix4f modelMatrix = vec_Mat_Model[i];
            final = M_orth * Pmatrix * M_cam * M_aspect * modelMatrix;
            glUniformMatrix4fv(program.uniform("final"), 1, GL_FALSE, final.data());
            glUniform3fv(program.uniform("camera"), 1, eye.data());
            glUniform3fv(program.uniform("light"), 1, light.data());
            // glUniform1i(glGetUniformLocation(program, "my_texture"), 0);
            // glUniform1i(program.uniform("my_texture"), textures[i]);
            // glUniform1i(program.uniform("my_texture"), i);

            for(int j = 0; j < vec_Mat_Vertex[i].cols()/3; j++)
            {
// **************************************************************************************************
                glUniform1i(program.uniform("my_texture"), objIDs[i]);
// **************************************************************************************************
                glDrawArrays(GL_TRIANGLES, j*3, 3);
                // if(draw_count%2 == 0)
                // {
                //     glDrawArrays(GL_LINE_LOOP, j*3, 3);
                // }
                // else if(draw_count%2 == 1)
                // {
                //     glDrawArrays(GL_LINE_LOOP, j*3, 3);
                //     glDrawArrays(GL_TRIANGLES, j*3, 3);
                // }
            }
        }
        glViewport(0, 0, width, height);
        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Deallocate opengl memory
    program.free();
    VAO.free();
    for(int i = 0; i < vec_VBO_V.size(); i++)
    {
        vec_VBO_V[i].free();
        vec_VBO_C[i].free();
    }
// **************************************************************************************************
    // glDeleteTextures(1, &tex);
// **************************************************************************************************
    // Deallocate glfw internals
    glfwTerminate();
    return 0;
}

int inside_triangle(MatrixXf V, int i, Vector2d P)
{ 
    Vector2d A, B, C;
    A << V(0, 3*i), V(1, 3*i);
    B << V(0, 3*i + 1), V(1, 3*i + 1);
    C << V(0, 3*i + 2), V(1, 3*i + 2);

    // Compute vectors. P is the position of the cursor
    Vector2d v0 = C - A;
    Vector2d v1 = B - A;
    Vector2d v2 = P - A;

    // Compute dot products
    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);

    // Compute barycentric coordinates
    double invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    double a = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double b = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // Check if point is in triangle
    return (a >= 0) && (b >= 0) && (a + b < 1);
}

int triangle_under_cursor(MatrixXf V, Vector2d P)
{
    int n = V.cols() / 3;
    bool result;
    int tri_id = -1;
    for(int i = 0; i < n; i++) {
        result = inside_triangle(V, i, P);
        if(result==1) {
            tri_id = i;
        }
        else if(result == -1) {
            cout << "Not inside triangle" << endl;
        }
    }
    return result;
}

void TranslateObj(int objID)
{
    Matrix4f CurrentModelMatrix = vec_Mat_Model[objID];
    Matrix4f TranslateModelMatrix;
    Matrix4f ScaleModelMatrix;
    Matrix4f RotateModelMatrix;

    TranslateModelMatrix <<
    1, 0, 0, Tx,
    0, 1, 0, Ty,
    0, 0, 1, Tz,
    0, 0, 0, 1;

    ScaleModelMatrix <<
    ScaleRatio, 0, 0, 0,
    0, ScaleRatio, 0, 0,
    0, 0, ScaleRatio, 0, 
    0, 0, 0,          1;

    // Rotate model by y-axis

    float radian = float(PI * rot_angle) / 180.0;
    float cosB = (float)cos(radian);
    float sinB = (float)sin(radian);

    RotateModelMatrix <<
    cosB,       0, sinB, 0,
    0,          1,    0, 0,
    -1 * sinB,  0, cosB, 0,
    0,          0,    0, 1; 

    CurrentModelMatrix = RotateModelMatrix * TranslateModelMatrix * ScaleModelMatrix * CurrentModelMatrix;
    vec_Mat_Model[objID] = CurrentModelMatrix;    
}

void InitTransInputs()
{
    Tx = 0.;
    Ty = 0.;
    Tz = 0.;
    ScaleRatio = 1.0;
}

void DeleteObj(int objID)
{
    vec_VBO_V.erase(vec_VBO_V.begin() + objID);
    vec_VBO_C.erase(vec_VBO_C.begin() + objID);
    vec_Mat_Model.erase(vec_Mat_Model.begin() + objID);
    vec_Mat_Vertex.erase(vec_Mat_Vertex.begin() + objID);
}

bool loadOBJ()
{
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<Vector3f> temp_vertices;
    std::vector<Vector2f> temp_uvs;
    std::vector<Vector3f> temp_normals;

    MatrixXf V(3,0);
    MatrixXf UV(2,0);
    MatrixXf N(3,0);
    FILE * file;


    if(m == object1){
        file = fopen("../data/cube.obj", "r");
    } else if(m == object2){
        file = fopen("../data/box_obj.obj", "r");
    }

    if( file == NULL) {
        printf("File cannot be opened.\n");
        return false;
    }


    while(1) {
        char lineHeader[128];
        // read the first word of the line
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break;      // Quit the loop
        // else: parse lineHeader
        if (strcmp(lineHeader, "v") == 0){
            Vector3f vertex;
            fscanf(file, "%f %f %f\n", &vertex(0), &vertex(1), &vertex(2));
            // cerr << vertex(0) << endl;
            // cerr << vertex(1) << endl;
            // cerr << vertex(2) << endl;
            // cerr << " " << endl;
            temp_vertices.push_back(vertex);
        } else if (strcmp(lineHeader, "vt") == 0){
            Vector2f uv;
            fscanf(file, "%f %f\n", &uv(0), &uv(1));
            // cerr << uv(0) << endl;
            // cerr << uv(1) << endl;
            // cerr << " "  << endl;
            temp_uvs.push_back(uv);
        } else if (strcmp(lineHeader, "vn") == 0){
            Vector3f normal;
            fscanf(file, "%f %f %f\n", &normal(0), &normal(1), &normal(2));
            // cerr << normal(0) << endl;
            // cerr << normal(1) << endl;
            // cerr << normal(2) << endl;
            // cerr << " " << endl;
            temp_normals.push_back(normal);
        } else if (strcmp(lineHeader, "f") == 0){
            std::string vertex1, vertex2, vertex3;
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
                &vertexIndex[0], &uvIndex[0], &normalIndex[0], 
                &vertexIndex[1], &uvIndex[1], &normalIndex[1], 
                &vertexIndex[2], &uvIndex[2], &normalIndex[2] );

            if (matches != 9) {
                printf("File cannot be read correctly.\n");
                return false;
            }
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            uvIndices    .push_back(uvIndex[0]);
            uvIndices    .push_back(uvIndex[1]);
            uvIndices    .push_back(uvIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
        }
        
        // For each vertex of each triangle
        // Fill in the vertex matrix
        V.conservativeResize(3, vertexIndices.size());
        for(unsigned int i=0; i<vertexIndices.size(); i++){
            unsigned int vertexIndex = vertexIndices[i];
            Vector3f vertex = temp_vertices[ vertexIndex-1 ];
            V.col(i) << vertex(0), vertex(1), vertex(2);
        }
        // Fill in the uv matrix
        UV.conservativeResize(2, vertexIndices.size());
        for(unsigned int i=0; i<vertexIndices.size(); i++){
            unsigned int uvIndex = uvIndices[i];
            Vector2f uv = temp_uvs[ uvIndex-1 ];
            UV.col(i) << uv(0), uv(1);
        }

        // Fill in the normal matrix
        N.conservativeResize(3, vertexIndices.size());
        for(unsigned int i=0; i<vertexIndices.size(); i++){
            unsigned int normalIndex = normalIndices[i];
            Vector3f normal = temp_normals[ normalIndex-1 ];
            N.col(i) << normal(0), normal(1), normal(2);
        }
    }

        // cerr << "V.cols(): " << V.cols() << endl;
        // cerr << "UV.cols(): " << UV.cols() << endl;
        // cerr << "N.cols(): " << N.cols() << endl;

        double max_x = V(0,0);
        double min_x = V(0,1);
        for(int j = 0; j < V.cols(); j++)
        {
            if(V(0,j) > max_x) max_x = V(0,j);
            if(V(0,j) < min_x) min_x = V(0,j);
        }
        double len_x = max_x - min_x;
        double center_x = (max_x + min_x) / 2.;

        // Find y-length of the object
        double max_y = V(1,0);
        double min_y = V(1,1);
        for(int j = 0; j < V.cols(); j++)
        {
            if(V(1,j) > max_y) max_y = V(1,j);
            if(V(1,j) < min_y) min_y = V(1,j);
        }
        double len_y = max_y - min_y;
        double center_y = (max_y + min_y) / 2.;        

        // Find z-length of the object
        double max_z = V(2,0);
        double min_z = V(2,1);
        for(int j = 0; j < V.cols(); j++)
        {
            if(V(2,j) > max_z) max_z = V(2,j);
            if(V(2,j) < min_z) min_z = V(2,j);
        }
        double len_z = max_z - min_z; 
        double center_z = (max_z + min_z) / 2.;

        for(int i = 0; i < V.cols(); i++) {
            V(0,i) = V(0,i) - center_x;
            V(1,i) = V(1,i) - center_y;
            V(2,i) = V(2,i) - center_z;
        }
        double scale_rate = len_x;
        if(len_y > scale_rate) scale_rate = len_y;
        else if (len_z > scale_rate) scale_rate = len_z;

        double canonical_min_z;
        canonical_min_z = (min_z - center_z)/scale_rate;
        // if (m == ImportBcubeMode) {
        //     depth_bumpy = canonical_min_z;
        // }
        // else if (m == ImportBunnyMode) {
        //     depth_bunny = canonical_min_z;
        // }

        for(int i = 0; i < V.rows(); i++) {
            for (int j = 0; j < V.cols(); j++) {
                V(i,j) = V(i,j) / scale_rate;
            }
        }        

        Matrix4f modelMatrix;
        modelMatrix <<
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

        VertexBufferObject VBO_V;
        VBO_V.init();
        VBO_V.update(V);

        VertexBufferObject VBO_UV;
        VBO_UV.init();
        VBO_UV.update(UV);

        VertexBufferObject VBO_N;
        VBO_N.init();
        VBO_N.update(N);

        MatrixXf C(3, V.cols());
        for (int i = 0; i < C.cols(); i++)
            C.col(i) << 0., 0., 0.;

        VertexBufferObject VBO_C;
        VBO_C.init();
        VBO_C.update(C);

        vec_VBO_V.push_back(VBO_V);
        vec_VBO_UV.push_back(VBO_UV);
        vec_VBO_N_ver.push_back(VBO_N);
        vec_VBO_C.push_back(VBO_C);

        vec_Mat_Model.push_back(modelMatrix);
        vec_Mat_Vertex.push_back(V);

        if (m == object1)
            objIDs.push_back(2);
        else if (m == object2) 
            objIDs.push_back(3);
}

void loadFloor()
{
    MatrixXf V(3,6);
    MatrixXf UV(2,6);
    MatrixXf N(3,6);

    V.col(0) << -3., -1., 3.;
    V.col(1) << -3., -1., -3.;
    V.col(2) << 3., -1., -3.;
    V.col(3) << 3., -1., -3.;
    V.col(4) << 3., -1., 3.;
    V.col(5) << -3., -1., 3.;

    N.col(0) << 0., 1., 0.;
    N.col(1) << 0., 1., 0.;
    N.col(2) << 0., 1., 0.;
    N.col(3) << 0., 1., 0.;
    N.col(4) << 0., 1., 0.;
    N.col(5) << 0., 1., 0.;

    UV.col(0) << 0., 1.;
    UV.col(1) << 0., 0.;
    UV.col(2) << 1., 0.;
    UV.col(3) << 1., 0.;
    UV.col(4) << 1., 1.;
    UV.col(5) << 0., 1.;

    Matrix4f modelMatrix;
    modelMatrix <<
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1;

    VertexBufferObject VBO_V;
    VBO_V.init();
    VBO_V.update(V);

    VertexBufferObject VBO_UV;
    VBO_UV.init();
    VBO_UV.update(UV);

    VertexBufferObject VBO_N;
    VBO_N.init();
    VBO_N.update(N);

    MatrixXf C(3, V.cols());
    for (int i = 0; i < C.cols(); i++)
        C.col(i) << 0., 0., 0.;

    VertexBufferObject VBO_C;
    VBO_C.init();
    VBO_C.update(C);

    vec_VBO_V.push_back(VBO_V);
    vec_VBO_UV.push_back(VBO_UV);
    vec_VBO_N_ver.push_back(VBO_N);
    vec_VBO_C.push_back(VBO_C);

    vec_Mat_Model.push_back(modelMatrix);
    vec_Mat_Vertex.push_back(V);

    objIDs.push_back(0);
}

void loadWall()
{
    MatrixXf V(3,12);
    MatrixXf UV(2,12);
    MatrixXf N(3,12);

    V.col(0) << -3., 2., -3.;
    V.col(1) << -3., -1., -3.;
    V.col(2) << -3., -1., 3.;
    V.col(3) << -3., 2., -3.;
    V.col(4) << -3., -1., 3.;
    V.col(5) << -3., 2., 3.;

    V.col(6) << -3., 2., 3.;
    V.col(7) << -3., -1., 3.;
    V.col(8) << 3., -1., 3.;
    V.col(9) << -3., 2., 3.;
    V.col(10) << 3., -1., 3.;
    V.col(11) << 3., 2., 3.;


    N.col(0) << 1., 0., 0.;
    N.col(1) << 1., 0., 0.;
    N.col(2) << 1., 0., 0.;
    N.col(3) << 1., 0., 0.;
    N.col(4) << 1., 0., 0.;
    N.col(5) << 1., 0., 0.;

    N.col(6) << 0., 0., -1.;
    N.col(7) << 0., 0., -1.;
    N.col(8) << 0., 0., -1.;
    N.col(9) << 0., 0., -1.;
    N.col(10) << 0., 0., -1.;
    N.col(11) << 0., 0., -1.;

    UV.col(0) << 0., 1.;
    UV.col(1) << 0., 0.;
    UV.col(2) << 1., 0.;
    UV.col(3) << 0., 1.;
    UV.col(4) << 1., 0.;
    UV.col(5) << 1., 1.;

    UV.col(6) << 0., 1.;
    UV.col(7) << 0., 0.;
    UV.col(8) << 1., 0.;
    UV.col(9) << 0., 1.;
    UV.col(10) << 1., 0.;
    UV.col(11) << 1., 1.;


    Matrix4f modelMatrix;
    modelMatrix <<
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1;

    VertexBufferObject VBO_V;
    VBO_V.init();
    VBO_V.update(V);

    VertexBufferObject VBO_UV;
    VBO_UV.init();
    VBO_UV.update(UV);

    VertexBufferObject VBO_N;
    VBO_N.init();
    VBO_N.update(N);

    MatrixXf C(3, V.cols());
    for (int i = 0; i < C.cols(); i++)
        C.col(i) << 0., 0., 0.;

    VertexBufferObject VBO_C;
    VBO_C.init();
    VBO_C.update(C);

    vec_VBO_V.push_back(VBO_V);
    vec_VBO_UV.push_back(VBO_UV);
    vec_VBO_N_ver.push_back(VBO_N);
    vec_VBO_C.push_back(VBO_C);

    vec_Mat_Model.push_back(modelMatrix);
    vec_Mat_Vertex.push_back(V);

    objIDs.push_back(1);
}
