/*
Title: Sphere-AABB 3D collision Detection
File Name: main.cpp
Copyright � 2015
Original authors: Srinivasan Thiagarajan
Written under the supervision of David I. Schwartz, Ph.D., and
supported by a professional development seed grant from the B. Thomas
Golisano College of Computing & Information Sciences
(https://www.rit.edu/gccis) at the Rochester Institute of Technology.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Description:
This is a collision test between a sphere and a Axis aligned bounding box in 3D.
We Find the closest point on the box, closest tothe center of the circle. Then,
we check if that point lies on/inside the sphere or outside the sphere. 
If it lies within the sphere, then collision is detected. else no collision has occoured.
The movable sphere changes color when a collision is detected.

Use Mouse to move in x-y plane, and "w and s" to move in z axis.

References:
AABB-2D by Brockton Roth
*/


#include "GLIncludes.h"

// We change this variable upon detecting collision
float blue = 0.0f;

//This struct consists of the basic stuff needed for getting the shape on the screen.
struct stuff_for_drawing{
	
	//This stores the address the buffer/memory in the GPU. It acts as a handle to access the buffer memory in GPU.
	GLuint vbo;

	//This will be used to tell the GPU, how many vertices will be needed to draw during drawcall.
	int numberOfVertices;

	//This function gets the number of vertices and all the vertex values and stores them in the buffer.
	void initBuffer(int numVertices, VertexFormat* vertices)
	{
		numberOfVertices = numVertices;

		// This generates buffer object names
		// The first parameter is the number of buffer objects, and the second parameter is a pointer to an array of buffer objects (yes, before this call, vbo was an empty variable)
		glGenBuffers(1, &vbo);
		
		//// Binds a named buffer object to the specified buffer binding point. Give it a target (GL_ARRAY_BUFFER) to determine where to bind the buffer.
		//// There are several different target parameters, GL_ARRAY_BUFFER is for vertex attributes, feel free to Google the others to find out what else there is.
		//// The second paramter is the buffer object reference. If no buffer object with the given name exists, it will create one.
		//// Buffer object names are unsigned integers (like vbo). Zero is a reserved value, and there is no default buffer for each target (targets, like GL_ARRAY_BUFFER).
		//// Passing in zero as the buffer name (second parameter) will result in unbinding any buffer bound to that target, and frees up the memory.
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		//// Creates and initializes a buffer object's data.
		//// First parameter is the target, second parameter is the size of the buffer, third parameter is a pointer to the data that will copied into the buffer, and fourth parameter is the 
		//// expected usage pattern of the data. Possible usage patterns: GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY, GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, 
		//// GL_DYNAMIC_READ, or GL_DYNAMIC_COPY
		//// Stream means that the data will be modified once, and used only a few times at most. Static means that the data will be modified once, and used a lot. Dynamic means that the data 
		//// will be modified repeatedly, and used a lot. Draw means that the data is modified by the application, and used as a source for GL drawing. Read means the data is modified by 
		//// reading data from GL, and used to return that data when queried by the application. Copy means that the data is modified by reading from the GL, and used as a source for drawing.
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFormat) * numVertices, vertices, GL_STATIC_DRAW);

		//// By default, all client-side capabilities are disabled, including all generic vertex attribute arrays.
		//// When enabled, the values in a generic vertex attribute array will be accessed and used for rendering when calls are made to vertex array commands (like glDrawArrays/glDrawElements)
		//// A GL_INVALID_VALUE will be generated if the index parameter is greater than or equal to GL_MAX_VERTEX_ATTRIBS
		glEnableVertexAttribArray(0);

		//// Defines an array of generic vertex attribute data. Takes an index, a size specifying the number of components (in this case, floats)(has a max of 4)
		//// The third parameter, type, can be GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_FIXED, or GL_FLOAT
		//// The fourth parameter specifies whether to normalize fixed-point data values, the fifth parameter is the stride which is the offset (in bytes) between generic vertex attributes
		//// The fifth parameter is a pointer to the first component of the first generic vertex attribute in the array. If a named buffer object is bound to GL_ARRAY_BUFFER (and it is, in this case) 
		//// then the pointer parameter is treated as a byte offset into the buffer object's data.
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)16);
		//// You'll note sizeof(VertexFormat) is our stride, because each vertex contains data that adds up to that size.
		//// You'll also notice we offset this parameter by 16 bytes, this is because the vec3 position attribute is after the vec4 color attribute. A vec4 has 4 floats, each being 4 bytes 
		//// so we offset by 4*4=16 to make sure that our first attribute is actually the position. The reason we put position after color in the struct has to do with padding.
		//// For more info on padding, Google it.

		//// This is our color attribute, so the offset is 0, and the size is 4 since there are 4 floats for color.
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);
	}
};

// The basic structure for a Circle. We need a center, a radius, and VBO and total number of vertices.
struct Sphere{
	glm::mat4 MVP;
	glm::vec3 origin;
	float radius;
	stuff_for_drawing base;
}sphere;

// the basic structure for a rectangle. We need a center, a length, a breadth and total number of vertices (36, 6 triangles, 2 for eachside).
struct Cuboid{
	glm::mat4 MVP;
	glm::vec3 origin;
	float length;
	float breadth;
	float depth;
	stuff_for_drawing base;
}cuboid;

// This function return the value between min and mx with the least distance value to x. This is called clamping.
float clamp_on_range(float x, float min, float max)
{
	if (x < min)
		return min;
	if (x > max)
		return max;

	return x;
}

// This function clamps a vector on a rectangle. It basically finds the closest point on the rectangle to the point p.
glm::vec3 clamp_on_rectangle(glm::vec3 p, Cuboid &r)
{
	glm::vec3 closest_point;
	closest_point.x = clamp_on_range(p.x,
		r.origin.x - (r.breadth / 2.0f),
		r.origin.x + (r.breadth / 2.0f));

	closest_point.y = clamp_on_range(p.y,
		r.origin.y - (r.length / 2.0f),
		r.origin.y + (r.length / 2.0f));

	closest_point.z = clamp_on_range(p.z,
		r.origin.z - (r.depth / 2.0f),
		r.origin.z + (r.depth / 2.0f));

	return closest_point;
}

//Convert the circle's position from world coordinate system to the box's model cordinate system.
bool is_colliding(Sphere &s, Cuboid &c)
{
	//Gets the closest point on the box to the circle's center.
	glm::vec3 closest_point = clamp_on_rectangle(s.origin, c);

	//gets the distance between the circle's center and the point on box.
	float distance = glm::distance(closest_point, s.origin);

	// if the point lies on/inside the circle; return true . else false.
	if (distance <= s.radius)
		return true;
	else
		return false;
}


//This function sets up the two shapes we need for this example.
void setup()
{
	//Create a sphere. 
	// yaw is the rotation along y axis.
	// pitch is the rotation along x axis.
	sphere.origin = glm::vec3(0.0f, 0.0f, 0.0f);
	
	std::vector<VertexFormat> vertexSet, vertices2;

	float radius = 0.25f;
	
	sphere.radius = radius;

	float pitch, yaw;
	yaw = 0.0f;
	pitch = 0.0f;
	int i, j;
	float pitchDelta = 360 / DIVISIONS;
	float yawDelta = 360 / DIVISIONS;

	VertexFormat p1, p2, p3, p4;

	for (i = 0; i < DIVISIONS; i++)
	{
		for (j = 0; j < DIVISIONS; j++)
		{

			p1.position.x = radius * sin((pitch)* PI / 180.0) * cos((yaw)* PI / 180.0);
			p1.position.y = radius * sin((pitch)* PI / 180.0) * sin((yaw)* PI / 180.0);;
			p1.position.z = radius * cos((pitch)* PI / 180.0);
			p1.color.r = 0.7f;
			p1.color.g = 0.2f;
			p1.color.b = 0;
			p1.color.a = 1;

			p2.position.x = radius * sin((pitch)* PI / 180.0) * cos((yaw + yawDelta)* PI / 180.0);
			p2.position.y = radius * sin((pitch)* PI / 180.0) * sin((yaw + yawDelta)* PI / 180.0);;
			p2.position.z = radius * cos((pitch)* PI / 180.0);
			p2.color.r = 0.7f;
			p2.color.g = 0.2f;
			p2.color.b = 0;
			p2.color.a = 1;

			p3.position.x = radius * sin((pitch + pitchDelta)* PI / 180.0) * cos((yaw + yawDelta)* PI / 180.0);
			p3.position.y = radius * sin((pitch + pitchDelta)* PI / 180.0) * sin((yaw + yawDelta)* PI / 180.0);;
			p3.position.z = radius * cos((pitch + pitchDelta)* PI / 180.0);
			p3.color.r = 0.7f;
			p3.color.g = 0.2f;
			p3.color.b = 0;
			p3.color.a = 1;

			p4.position.x = radius * sin((pitch + pitchDelta)* PI / 180.0) * cos((yaw)* PI / 180.0);
			p4.position.y = radius * sin((pitch + pitchDelta)* PI / 180.0) * sin((yaw)* PI / 180.0);;
			p4.position.z = radius * cos((pitch + pitchDelta)* PI / 180.0);
			p4.color.r = 0.7f;
			p4.color.g = 0.2f;
			p4.color.b = 0;
			p4.color.a = 1;

			vertexSet.push_back(p1);
			vertexSet.push_back(p2);
			vertexSet.push_back(p3);
			vertexSet.push_back(p1);
			vertexSet.push_back(p3);
			vertexSet.push_back(p4);

			yaw = yaw + yawDelta;
		}

		pitch += pitchDelta;
	}

	sphere.base.initBuffer(vertexSet.size(), &vertexSet[0]);

	cuboid.breadth = 1.0f;
	cuboid.length = 0.5f;
	cuboid.depth = 0.5f;

	//Rectangle Vertex generation
	cuboid.origin = glm::vec3(0.0f, 0.0f, 0.0f);

	VertexFormat A = VertexFormat(
		glm::vec3(cuboid.origin.x - (cuboid.breadth / 2.0f),
		cuboid.origin.y - (cuboid.length / 2.0f),
		cuboid.origin.z + cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));


	VertexFormat B = VertexFormat(
		glm::vec3(cuboid.origin.x + (cuboid.breadth / 2.0f),
		cuboid.origin.y - (cuboid.length / 2.0f),
		cuboid.origin.z + cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));

	VertexFormat C = VertexFormat(
		glm::vec3(cuboid.origin.x + (cuboid.breadth / 2.0f),
		cuboid.origin.y + (cuboid.length / 2.0f),
		cuboid.origin.z + cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));

	VertexFormat D = VertexFormat(
		glm::vec3(cuboid.origin.x - (cuboid.breadth / 2.0f),
		cuboid.origin.y + (cuboid.length / 2.0f),
		cuboid.origin.z + cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));

	VertexFormat A2 = VertexFormat(
		glm::vec3(cuboid.origin.x - (cuboid.breadth / 2.0f),
		cuboid.origin.y - (cuboid.length / 2.0f),
		cuboid.origin.z - cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));


	VertexFormat B2 = VertexFormat(
		glm::vec3(cuboid.origin.x + (cuboid.breadth / 2.0f),
		cuboid.origin.y - (cuboid.length / 2.0f),
		cuboid.origin.z - cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));

	VertexFormat C2 = VertexFormat(
		glm::vec3(cuboid.origin.x + (cuboid.breadth / 2.0f),
		cuboid.origin.y + (cuboid.length / 2.0f),
		cuboid.origin.z - cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));

	VertexFormat D2 = VertexFormat(
		glm::vec3(cuboid.origin.x - (cuboid.breadth / 2.0f),
		cuboid.origin.y + (cuboid.length / 2.0f),
		cuboid.origin.z - cuboid.depth / 2.0f),
		glm::vec4(0.7f, 0.20f, 0.0f, 1.0f));

	/*
	D------------------------C		D2---------------------C2
	|                        |		|						|
	|        FRONT           |		|		BACK			|
	|             FACE       |		|			FACE		|
	|                        |		|						|
	A------------------------B		A2---------------------B2
	*/
	
	//The two traingles constituing a face have to inputed in a counterClosckwise order.

	//Front Face
	vertices2.push_back(A);
	vertices2.push_back(B);
	vertices2.push_back(C);

	vertices2.push_back(A);
	vertices2.push_back(C);
	vertices2.push_back(D);
	
	//Back face
	vertices2.push_back(A2);
	vertices2.push_back(C2);
	vertices2.push_back(B2);

	vertices2.push_back(A2);
	vertices2.push_back(D2);
	vertices2.push_back(C2);
	
	//Left Face
	vertices2.push_back(A2);
	vertices2.push_back(D);
	vertices2.push_back(D2);

	vertices2.push_back(A2);
	vertices2.push_back(A);
	vertices2.push_back(D);
	
	//right Face
	vertices2.push_back(B);
	vertices2.push_back(B2);
	vertices2.push_back(C2);

	vertices2.push_back(B);
	vertices2.push_back(C2);
	vertices2.push_back(C);
	
	//Top Face
	vertices2.push_back(D);
	vertices2.push_back(C);
	vertices2.push_back(C2);

	vertices2.push_back(D);
	vertices2.push_back(C2);
	vertices2.push_back(D2);
	
	//Bottom Face
	vertices2.push_back(A);
	vertices2.push_back(B2);
	vertices2.push_back(B);

	vertices2.push_back(A);
	vertices2.push_back(A2);
	vertices2.push_back(B2);

	//Push all the data to the buffer on the GPU
	cuboid.base.initBuffer(36 , &vertices2[0]);
}


// Global data members
#pragma region Global Data member
// This is your reference to your shader program.
// This will be assigned with glCreateProgram().
// This program will run on your GPU.
GLuint program;

// These are your references to your actual compiled shaders
GLuint vertex_shader;
GLuint fragment_shader;

// This is a reference to your uniform MVP matrix in your vertex shader
GLuint uniMVP;
GLuint color;

glm::mat4 view;
glm::mat4 proj;
glm::mat4 PV;

// Reference to the window object being created by GLFW.
GLFWwindow* window;
#pragma endregion			  

// Functions called only once every time the program is executed.
#pragma region Helper_functions
std::string readShader(std::string fileName)
{
	std::string shaderCode;
	std::string line;

	// We choose ifstream and std::ios::in because we are opening the file for input into our program.
	// If we were writing to the file, we would use ofstream and std::ios::out.
	std::ifstream file(fileName, std::ios::in);

	// This checks to make sure that we didn't encounter any errors when getting the file.
	if (!file.good())
	{
		std::cout << "Can't read file: " << fileName.data() << std::endl;

		// Return so we don't error out.
		return "";
	}

	// ifstream keeps an internal "get" position determining the location of the element to be read next
	// seekg allows you to modify this location, and tellg allows you to get this location
	// This location is stored as a streampos member type, and the parameters passed in must be of this type as well
	// seekg parameters are (offset, direction) or you can just use an absolute (position).
	// The offset parameter is of the type streamoff, and the direction is of the type seekdir (an enum which can be ios::beg, ios::cur, or ios::end referring to the beginning, 
	// current position, or end of the stream).
	file.seekg(0, std::ios::end);					// Moves the "get" position to the end of the file.
	shaderCode.resize((unsigned int)file.tellg());	// Resizes the shaderCode string to the size of the file being read, given that tellg will give the current "get" which is at the end of the file.
	file.seekg(0, std::ios::beg);					// Moves the "get" position to the start of the file.

	// File streams contain two member functions for reading and writing binary data (read, write). The read function belongs to ifstream, and the write function belongs to ofstream.
	// The parameters are (memoryBlock, size) where memoryBlock is of type char* and represents the address of an array of bytes are to be read from/written to.
	// The size parameter is an integer that determines the number of characters to be read/written from/to the memory block.
	file.read(&shaderCode[0], shaderCode.size());	// Reads from the file (starting at the "get" position which is currently at the start of the file) and writes that data to the beginning
	// of the shaderCode variable, up until the full size of shaderCode. This is done with binary data, which is why we must ensure that the sizes are all correct.

	file.close(); // Now that we're done, close the file and return the shaderCode.

	return shaderCode;
}

// This method will consolidate some of the shader code we've written to return a GLuint to the compiled shader.
// It only requires the shader source code and the shader type.
GLuint createShader(std::string sourceCode, GLenum shaderType)
{
	// glCreateShader, creates a shader given a type (such as GL_VERTEX_SHADER) and returns a GLuint reference to that shader.
	GLuint shader = glCreateShader(shaderType);
	const char *shader_code_ptr = sourceCode.c_str(); // We establish a pointer to our shader code string
	const int shader_code_size = sourceCode.size();   // And we get the size of that string.

	// glShaderSource replaces the source code in a shader object
	// It takes the reference to the shader (a GLuint), a count of the number of elements in the string array (in case you're passing in multiple strings), a pointer to the string array 
	// that contains your source code, and a size variable determining the length of the array.
	glShaderSource(shader, 1, &shader_code_ptr, &shader_code_size);
	glCompileShader(shader); // This just compiles the shader, given the source code.

	GLint isCompiled = 0;

	// Check the compile status to see if the shader compiled correctly.
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE)
	{
		char infolog[1024];
		glGetShaderInfoLog(shader, 1024, NULL, infolog);

		// Print the compile error.
		std::cout << "The shader failed to compile with the error:" << std::endl << infolog << std::endl;

		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(shader); // Don't leak the shader.

		// NOTE: I almost always put a break point here, so that instead of the program continuing with a deleted/failed shader, it stops and gives me a chance to look at what may 
		// have gone wrong. You can check the console output to see what the error was, and usually that will point you in the right direction.
	}

	return shader;
}

// Initialization code
void init()
{
	// Initializes the glew library
	glewInit();

	// Enables the depth test, which you will want in most cases. You can disable this in the render loop if you need to.
	glEnable(GL_DEPTH_TEST);

	// Read in the shader code from a file.
	std::string vertShader = readShader("../VertexShader.glsl");
	std::string fragShader = readShader("../FragmentShader.glsl");

	// createShader consolidates all of the shader compilation code
	vertex_shader = createShader(vertShader, GL_VERTEX_SHADER);
	fragment_shader = createShader(fragShader, GL_FRAGMENT_SHADER);

	// A shader is a program that runs on your GPU instead of your CPU. In this sense, OpenGL refers to your groups of shaders as "programs".
	// Using glCreateProgram creates a shader program and returns a GLuint reference to it.
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);		// This attaches our vertex shader to our program.
	glAttachShader(program, fragment_shader);	// This attaches our fragment shader to our program.

	// This links the program, using the vertex and fragment shaders to create executables to run on the GPU.
	glLinkProgram(program);
	// End of shader and program creation

	// Creates the view matrix using glm::lookAt.
	// First parameter is camera position, second parameter is point to be centered on-screen, and the third paramter is the up axis.
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Creates a projection matrix using glm::perspective.
	// First parameter is the vertical FoV (Field of View), second paramter is the aspect ratio, 3rd parameter is the near clipping plane, 4th parameter is the far clipping plane.
	proj = glm::perspective(45.0f, 800.0f / 800.0f, 0.1f, 100.0f);

	PV = proj * view;

	// This gets us a reference to the uniform variable in the vertex shader, which is called "color".
	// We're using this variable to change color during runtime, without changing the buffer values.
	// Only 2 parameters required: A reference to the shader program and the name of the uniform variable within the shader code.
	uniMVP = glGetUniformLocation(program, "MVP");
	color = glGetUniformLocation(program, "blue");

	// This is not necessary, but I prefer to handle my vertices in the clockwise order. glFrontFace defines which face of the triangles you're drawing is the front.
	// Essentially, if you draw your vertices in counter-clockwise order, by default (in OpenGL) the front face will be facing you/the screen. If you draw them clockwise, the front face 
	// will face away from you. By passing in GL_CW to this function, we are saying the opposite, and now the front face will face you if you draw in the clockwise order.
	// If you don't use this, just reverse the order of the vertices in your array when you define them so that you draw the points in a counter-clockwise order.
	glFrontFace(GL_CCW);

	// This is also not necessary, but more efficient and is generally good practice. By default, OpenGL will render both sides of a triangle that you draw. By enabling GL_CULL_FACE, 
	// we are telling OpenGL to only render the front face. This means that if you rotated the triangle over the X-axis, you wouldn't see the other side of the triangle as it rotated.
	//glEnable(GL_CULL_FACE);
	//We are disabling hte cull face, because we wish to see both the front and back of the objects in wireframe mode for better understanding the depth.

	// Determines the interpretation of polygons for rasterization. The first parameter, face, determines which polygons the mode applies to.
	// The face can be either GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK
	// The mode determines how the polygons will be rasterized. GL_POINT will draw points at each vertex, GL_LINE will draw lines between the vertices, and 
	// GL_FILL will fill the area inside those lines.
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

#pragma endregion

// Functions called between every frame. game logic
#pragma region util_functions
// This runs once every physics timestep.
void update()
{
	if (is_colliding(sphere, cuboid))
	{
		blue = 1.0f;
	}
	else
		blue = 0.0f;

	// Get the cursor position with respect ot hte window.
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	//Since the cursor posiiton is represented in pixel values, 
	//dividing it by the window specs will reduce the position to 0-1 range.
	//multiplying it with 2 and then subtracting it with 1 wil bring the 
	//mouse position to -1 ot 1.
	
	sphere.origin.x = ((x / 800.0f)*2.0f) - 1.0f;
	sphere.origin.y = -(((y / 800.0f)*2.0f) - 1.0f);

	//create the translation matrix. Later when the object is scaled and rotated, thet would also be in this matrix
	glm::mat4 translation;

	translation = glm::translate(sphere.origin);
	sphere.MVP = PV * translation;					// multiply the translation matrix with the projection and view matrix. This makes the objects look 3D, (perspective view)

	translation = glm::translate(cuboid.origin);
	cuboid.MVP = PV * translation;					// Since the screen's origin is on the top, we need to negate the y values.	
}

// This function runs every frame
void renderScene()
{
	// Clear the color buffer and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Clear the screen to white
	glClearColor(1.0, 1.0, 1.0, 1.0);

	// Tell OpenGL to use the shader program you've created.
	glUseProgram(program);

	//Draw the Sphere
	
	glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(sphere.MVP));
	glUniform1f(color, blue);
	glBindBuffer(GL_ARRAY_BUFFER, sphere.base.vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)16);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);
	glDrawArrays(GL_TRIANGLES, 0, sphere.base.numberOfVertices);

	/*
	we are using a vec3 to store 2 variables. The first 2 are used to store the x and y transformation.
	the last float is used to store the blue  color component. We are doing this so, there are less calls to be made to the GPU
	*/

	// Draw the cube/Box
	// Set the uniform matrix in our shader to our MVP matrix for the Box.
	glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(cuboid.MVP));
	glUniform1f(color, 0.0f);
	glBindBuffer(GL_ARRAY_BUFFER, cuboid.base.vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)16);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);
	glDrawArrays(GL_TRIANGLES, 0, cuboid.base.numberOfVertices);

}


// This function is used to handle key inputs.
// It is a callback funciton. i.e. glfw takes the pointer to this function (via function pointer) and calls this function every time a key is pressed in the during event polling.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	float moverate = 0.25f;

	////This set of controls are used to move one point (point1) of the line.
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		sphere.origin.z -= moverate;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		sphere.origin.z += moverate;
	
}

#pragma endregion


void main()
{
	glfwInit();

	// Creates a window given (width, height, title, monitorPtr, windowPtr).
	// Don't worry about the last two, as they have to do with controlling which monitor to display on and having a reference to other windows. Leaving them as nullptr is fine.
	window = glfwCreateWindow(800, 800, "Some title", nullptr, nullptr);

	std::cout << "\n This is a collision test between a sphere \n and a Axis aligned bounding box in 3D.\n\n\n\n\n";
	std::cout << "Use Mouse to move in x-y plane, and \"w and s\" to move in z axis.";

	// Makes the OpenGL context current for the created window.
	glfwMakeContextCurrent(window);

	// Sets the number of screen updates to wait before swapping the buffers.
	// Setting this to zero will disable VSync, which allows us to actually get a read on our FPS. Otherwise we'd be consistently getting 60FPS or lower, 
	// since it would match our FPS to the screen refresh rate.
	// Set to 1 to enable VSync.
	glfwSwapInterval(0);

	// Initializes most things needed before the main loop
	init();

	// Sends the funtion as a funtion pointer along with the window to which it should be applied to.
	glfwSetKeyCallback(window, key_callback);

	setup();

	// Enter the main loop.
	while (!glfwWindowShouldClose(window))
	{
		// Call to update() which will update the gameobjects.
		update();

		// Call the render function.
		renderScene();

		// Swaps the back buffer to the front buffer
		// Remember, you're rendering to the back buffer, then once rendering is complete, you're moving the back buffer to the front so it can be displayed.
		glfwSwapBuffers(window);

		// Checks to see if any events are pending and then processes them.
		glfwPollEvents();
	}

	// After the program is over, cleanup your data!
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glDeleteProgram(program);
	// Note: If at any point you stop using a "program" or shaders, you should free the data up then and there.


	// Frees up GLFW memory
	glfwTerminate();
}