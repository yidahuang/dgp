// Copyright (C) 2014 by Andrea Tagliasacchi
// Simple mesh viewer based on GLFW
// @see https://code.google.com/p/opengl-tutorial-org/source/browse/tutorial08_basic_shading/tutorial08.cpp

#include <OpenGeometry/surface_mesh/Surface_mesh.h>
#include <OpenGeometry/surface_mesh/bounding_box.h>
#include <OpenGeometry/GL/simple_glfw_window.h>

/// @todo update once Eigen integrates the changes
#include <OpenGeometry/GL/EigenOpenGLSupport3.h> 

using namespace surface_mesh;
using namespace std;

/// Viewer global status
Surface_mesh mesh;
std::vector<unsigned int> triangles; 

/// OpenGL initialization
void init(){
    ///----------------------- DATA ----------------------------
    auto vpoints = mesh.get_vertex_property<Point>("v:point");
    auto vnormals = mesh.get_vertex_property<Normal>("v:normal");
    assert(vpoints);
    assert(vnormals);    
    
    ///---------------------- TRIANGLES ------------------------
    triangles.clear();
    for(auto f: mesh.faces())
        for(auto v: mesh.vertices(f))
            triangles.push_back(v.idx());
    
    ///---------------------- OPENGL GLOBALS--------------------
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f); ///< background
    glEnable(GL_DEPTH_TEST); // Enable depth test
    // glDisable(GL_CULL_FACE); // Cull triangles which normal is not towards the camera
        
    /// Compile the shaders
    GLuint programID = load_shaders( "vshader.glsl", "fshader.glsl" );
    if(!programID) exit(EXIT_FAILURE);
    glUseProgram( programID );
    
    ///---------------------- CAMERA ----------------------------
    {
        typedef Eigen::Vector3f vec3;
        typedef Eigen::Matrix4f mat4;
        
        /// Define projection matrix (FOV, aspect, near, far)
        mat4 projection = Eigen::perspective(45.0f, 4.0f/3.0f, 0.1f, 10.f);
        // cout << projection << endl;

        /// Define the view matrix (camera extrinsics)
        vec3 cam_pos(0,0,5);
        vec3 cam_look(0,0,-1); /// Remember: GL swaps viewdir
        vec3 cam_up(0,1,0);
        mat4 view = Eigen::lookAt(cam_pos, cam_look, cam_up);
        // cout << view << endl;
        
        /// Define the modelview matrix
        mat4 model = Eigen::scale(.5f,.5f,.5f) * Eigen::translate(-.5f,-.5f,.0f);
        // mat4 model = mat4::Identity();
        // cout << model << endl;
        
        /// Assemble the "Model View Projection" matrix
        mat4 mvp = projection * view * model; 
        // cout << mvp << endl;
         
        /// Pass the matrix to the shader
        /// The glUniform call below is equivalent to the OpenGL function call:
        /// \code glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &mvp[0][0]); \endcode
        const char* MVP = "MVP"; ///< Name of variable in the shader
        GLuint mvp_id = glGetUniformLocation(programID, MVP);
        Eigen::glUniform(mvp_id, mvp);
    }
    
    ///---------------------- VARRAY ----------------------------    
    {
        GLuint VertexArrayID;
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);  
    }
        
    ///---------------------- BUFFERS ----------------------------    
    GLuint vertexbuffer, normalbuffer, trianglebuffer; 
    {
        /// Load mesh vertices
        glGenBuffers(1, &vertexbuffer); 
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mesh.n_vertices()) * sizeof(Vec3f), vpoints.data(), GL_STATIC_DRAW); 
        
        /// Load mesh normals    
        glGenBuffers(1, &normalbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mesh.n_vertices()) * sizeof(Vec3f), vnormals.data(), GL_STATIC_DRAW);     
        
        /// Triangle indexes buffer
        glGenBuffers(1, &trianglebuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglebuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(unsigned int), &triangles[0], GL_STATIC_DRAW);
    }

    ///---------------------- SHADER ATTRIBUTES ----------------------------    
    {
        /// Readability constants
        enum ATTRIBUTES{VPOS=0, VNOR=1};      
        
        /// Vertex positions in VPOS
        glEnableVertexAttribArray(VPOS);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(VPOS, 3, GL_FLOAT, NOT_NORMALIZED, ZERO_STRIDE, ZERO_BUFFER_OFFSET);
        
        /// Vertex normals in VNOR
        glEnableVertexAttribArray(VNOR);
        glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
        glVertexAttribPointer(VNOR, 3, GL_FLOAT, NOT_NORMALIZED, ZERO_STRIDE, ZERO_BUFFER_OFFSET);
    }
    
    ///---------------------- ENABLE BUFFER ----------------------------
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglebuffer); //< used by glDrawElements
}

/// OpenGL render loop
void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, ZERO_BUFFER_OFFSET);
}

/// Entry point
int main(int argc, char** argv){
    assert(argc==2);
    mesh.read(argv[1]);
    mesh.triangulate();
    mesh.update_vertex_normals();
    cout << "input: '" << argv[1] << "' num vertices " << mesh.vertices_size() << endl;
    cout << "BBOX: " << bounding_box(mesh) << endl;
    // mesh.property_stats();
    simple_glfw_window("mesh viewer", 640, 480, init, display);
    return EXIT_SUCCESS;
}
