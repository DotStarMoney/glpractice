#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "gl_includes.h"


#define SCRX 1280
#define SCRY 720

#define CUBE_SIDE_LENGTH 30.0f
#define CAMERA_R 12.0f
#define ROTATION_SPEED 30.0f
#define SPHERE_SPEED 40.0f
#define VIEW_Z 1.1f
#define VIEW_WIDTH 2.0f
#define VIEW_HEIGHT 1.20f

void key_callback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mode);

struct lbv
{
	double x;
	double g;
	int g_i;
};

std::string file_to_string(std::string _filename)
{
	std::ifstream text_file;
	text_file = std::ifstream(_filename);
	return std::string(
		std::istreambuf_iterator<char>(text_file),
		std::istreambuf_iterator<char>());
}

bool shader_compile_check(GLuint _id)
{
	GLint comp_success;
	GLchar err_ret[512];
	glGetShaderiv(_id, GL_COMPILE_STATUS, &comp_success);
	if (!comp_success)
	{
		glGetShaderInfoLog(_id, 512, NULL, err_ret);
		std::cout << "Shader compilation error:\n"
			<< err_ret << std::endl;
		return false;
	}
	return true;
}

bool shader_link_check(GLuint _id)
{
	GLint link_success;
	GLchar err_ret[512];
	glGetProgramiv(_id, GL_LINK_STATUS, &link_success);
	if (!link_success)
	{
		glGetProgramInfoLog(_id, 512, NULL, err_ret);
		std::cout << "Shader linkage error:\n"
			<< err_ret << std::endl;
		return false;
	}
	return true;
}


int main()
{
	static GLfloat points[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f };
	static GLuint indices[] = {0, 1, 3, 2};

	GLFWwindow* window;

	GLuint tex_id;
	GLuint vao_id;
	GLuint vbo_id;
	GLuint ibo_id;
	GLuint vert_id;
	GLuint frag_id;
	GLuint prog_id;
	GLuint ray_transform_loc;
	GLuint camera_pos_loc;
	GLuint bpos0_loc;
	GLuint bpos1_loc;

	std::string vert_str;
	std::string frag_str;
	const char *vert_prog;
	const char *frag_prog;

	float time_s;
	long long time_ll;
	mat4x4 ray_transform;
	vec3 camera_pos;
	vec3 bpos0;
	vec3 bpos1;

	unsigned char* tex_image;
	int tex_w;
	int tex_h;
	int tex_c;

	glfwInit();
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	window = glfwCreateWindow(
		SCRX, 
		SCRY,
		"Metaball Raytracing",
		nullptr, 
		nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
	glViewport(0, 0, SCRX, SCRY);

	glGenVertexArrays(1, &vao_id);
	glGenBuffers(1, &vbo_id);
	glGenBuffers(1, &ibo_id);
	glGenTextures(1, &tex_id);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	{
		tex_image = stbi_load(
			"res\\wall2.png", 
			&tex_w, 
			&tex_h, 
			&tex_c, 
			STBI_rgb_alpha);
		if (tex_image == nullptr)
		{
			std::cout << "Failed to load texture." << std::endl;
			return -1;
		}
		glTexImage2D(
			GL_TEXTURE_2D, 
			0, 
			GL_RGBA, 
			tex_w, 
			tex_h, 
			0, 
			GL_RGBA, 
			GL_UNSIGNED_BYTE, 
			tex_image);
		stbi_image_free(tex_image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(
			GL_TEXTURE_2D, 
			GL_TEXTURE_MIN_FILTER, 
			GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	vert_id = glCreateShader(GL_VERTEX_SHADER);
	vert_str = file_to_string("res\\passthrough.vert");
	vert_prog = vert_str.c_str();
	glShaderSource(vert_id, 1, &vert_prog, NULL);
	glCompileShader(vert_id);
	if (!shader_compile_check(vert_id)) return -1;

	frag_id = glCreateShader(GL_FRAGMENT_SHADER);
	frag_str = file_to_string("res\\metaball.frag");
	frag_prog = frag_str.c_str();
	glShaderSource(frag_id, 1, &frag_prog, NULL);
	glCompileShader(frag_id);
	if (!shader_compile_check(frag_id)) return -1;

	prog_id = glCreateProgram();
	glAttachShader(prog_id, vert_id);
	glAttachShader(prog_id, frag_id);
	glLinkProgram(prog_id);
	if (!shader_link_check(prog_id)) return -1;

	glDeleteShader(vert_id);
	glDeleteShader(frag_id);

	glUseProgram(prog_id);

	glBindVertexArray(vao_id);
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
		glBufferData(
			GL_ARRAY_BUFFER,
			sizeof(points),
			points,
			GL_STATIC_DRAW);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			sizeof(indices),
			indices,
			GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(0);
	}

	glProgramUniform1f(prog_id, glGetUniformLocation(prog_id, "scrx"), SCRX);
	glProgramUniform1f(prog_id, glGetUniformLocation(prog_id, "scry"), SCRY);
	glProgramUniform1f(prog_id,
		glGetUniformLocation(prog_id, "cube_side"),
		CUBE_SIDE_LENGTH);
	glProgramUniform1f(prog_id, 
		glGetUniformLocation(prog_id, "view_plane_depth"), 
		VIEW_Z);
	glProgramUniform1f(prog_id, 
		glGetUniformLocation(prog_id, "view_plane_width"), 
		VIEW_WIDTH);
	glProgramUniform1f(prog_id, 
		glGetUniformLocation(prog_id, "view_plane_height"), 
		VIEW_HEIGHT);
	glProgramUniform1f(
		prog_id, 
		glGetUniformLocation(prog_id, "wall_texture"), 
		0);

	ray_transform_loc = glGetUniformLocation(prog_id, "ray_transform");
	camera_pos_loc = glGetUniformLocation(prog_id, "camera_pos");

	bpos0_loc = glGetUniformLocation(prog_id, "bpos0");
	bpos1_loc = glGetUniformLocation(prog_id, "bpos1");


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		time_ll = (std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch())).count();
		time_s = ((time_ll % 360000) / 1000.0f) * 0.01745329251f;

		ray_transform = glm::rotate(
			time_s * ROTATION_SPEED,
			vec3(0.0f, 1.0f, 0.0f));
		camera_pos = ray_transform * glm::vec4(0.0f, 0.0f, -CAMERA_R, 1.0f);

		bpos0 = glm::vec4(-3.5f, 0.0f, 0.0f, 1.0f) * 
			glm::eulerAngleYXZ(time_s * SPHERE_SPEED * 1.5,
				time_s * SPHERE_SPEED * 1.4,
				time_s * SPHERE_SPEED * 1.3);
		bpos1 = glm::vec4(3.5f, 0.0f, 0.0f, 1.0f) *
			glm::eulerAngleYXZ(time_s * SPHERE_SPEED * 1.4,
				time_s * SPHERE_SPEED * 1.3,
				time_s * SPHERE_SPEED * 1.5);

		glProgramUniformMatrix4fv(
			prog_id, 
			ray_transform_loc, 
			1, 
			false, 
			glm::value_ptr(ray_transform));
		glProgramUniform3fv(
			prog_id,
			camera_pos_loc,
			1,
			glm::value_ptr(camera_pos));
		glProgramUniform3fv(
			prog_id,
			bpos0_loc,
			1,
			glm::value_ptr(bpos0));
		glProgramUniform3fv(
			prog_id,
			bpos1_loc,
			1,
			glm::value_ptr(bpos1));



		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void key_callback(
	GLFWwindow* window,
	int key, 
	int scancode, 
	int action, 
	int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}