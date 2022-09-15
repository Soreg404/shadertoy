#define GLEW_STATIC
#include "glew.h"
#include "glfw3.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <memory.h>

#define LOG(x, ...) printf(x "\n" __VA_OPT__(,) __VA_ARGS__);

int WW = 800, WH = 600;

GLuint shd = 0;
bool loadShd(const char *fragment_path);

void createCanvas();

const char *vertexSrc = "#version 330 core\nlayout(location=0)in vec2 pos;void main(){gl_Position=vec4(pos, 1, 1);}";

const char fragmentHeader[] =
R"(#version 330 core
uniform vec3		iResolution;
uniform float		iTime;
uniform float		iTimeDelta;
uniform int			iFrame;
uniform float		iChannelTime[4];
uniform vec3		iChannelResolution[4];
uniform vec4		iMouse;
uniform sampler2D	iChannel[4];
uniform vec4		iDate;
uniform float		iSampleRate;
)";

const char fragmentFooter[] =
R"(
out vec4 mainFragmentColor;
void main() {
	mainImage(mainFragmentColor, gl_FragCoord.xy);
}
)";

namespace uloc {
	
	GLint iResolution = 0;								// viewport resolution (in pixels)                           vec3     
	GLint iTime = 1;									// shader playback time (in seconds)                         float    
	GLint iTimeDelta = 2;								// render time (in seconds)                                  float    
	GLint iFrame = 3;									// shader playback frame                                     int      
	GLint iChannelTime[4] = {4, 5, 6, 7};				// channel playback time (in seconds)                        float    
	GLint iChannelResolution[4] = {8, 9, 10, 11};		// channel resolution (in pixels)                            vec3     
	GLint iMouse = 12;									// mouse pixel coords. xy: current (if MLB down), zw: click  vec4     
	GLint iChannel[4] = {13, 14, 15, 16};				// input channel. XX = 2D/Cube                               samplerXX
	GLint iDate = 17;									// (year, month, day, time in seconds)                       vec4     
	GLint iSampleRate = 18;								// sound sample rate (i.e., 44100)                           float    
	
}

void wResizeClb(GLFWwindow *handle, int w, int h) {
	WW = w; WH = h;
	glViewport(0, 0, WW, WH);
	glUniform3f(uloc::iResolution, WW, WH, 1);
}

int main(int argc, const char *argv[]) {
	
	if(!glfwInit()) {
		LOG("glfw init error");
		return 1;
	}
	
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *wnd = glfwCreateWindow(WW, WH, "hello world!", nullptr, nullptr);
	glfwMakeContextCurrent(wnd);
	glViewport(0, 0, WW, WH);
	
	glfwSetWindowSizeCallback(wnd, wResizeClb);
	
	if(glewInit() != GLEW_OK) {
		LOG("context init error");
		return 1;
	}
	
	glClearColor(.1, .1, .1, 1);
	
	if(!loadShd(argc > 1 ? argv[1] : "shd.frag")) return 1;
	
	glUniform3f(uloc::iResolution, WW, WH, 1);
	
	createCanvas();
	
	glfwShowWindow(wnd);
	
	float shdTime = 0, lastFTime = glfwGetTime();
	
	while(!glfwWindowShouldClose(wnd)) {
		
		shdTime += glfwGetTime() - lastFTime;
		lastFTime = glfwGetTime();
		
		glfwPollEvents();
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUniform1f(uloc::iTime, shdTime);
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		glfwSwapBuffers(wnd);
		
	}
	
	glfwTerminate();
	
	return 0;
}

int checkSHDErr(GLuint shd, bool cmp = true) {
	int success = 0;
	if(cmp) glGetShaderiv(shd, GL_COMPILE_STATUS, &success);
	else glGetProgramiv(shd, GL_LINK_STATUS, &success);

	if(!success) {
		char infoLog[1000] = { 0 };
		if(cmp) glGetShaderInfoLog(shd, 1000, NULL, infoLog);
		else glGetProgramInfoLog(shd, 1000, NULL, infoLog);
		printf("error %s shader:\n%s\n\n", (cmp ? "compiling" : "linking"), infoLog);
	}
	return success;
}
bool loadShd(const char *fragment_path) {
	
	shd = glCreateProgram();
	
	bool ret = true;
	
	std::fstream file(fragment_path, std::ios::in | std::ios::binary | std::ios::ate);
	if(!file.good()) {
		LOG("couldn't load file");
		return false;
	}
	size_t fsize = file.tellg();
	file.clear();
	file.seekg(file.beg);
	
	size_t footerBeg = (sizeof(fragmentHeader) - 1) + fsize;
	std::vector<char> fbuff(footerBeg + sizeof(fragmentFooter));
	
	file.read(fbuff.data() + sizeof(fragmentHeader) - 1, fsize);
	file.close();
	
	memcpy_s(fbuff.data(), fbuff.size(), fragmentHeader, sizeof(fragmentHeader) - 1);
	memcpy_s(fbuff.data() + footerBeg, fbuff.size() - footerBeg, fragmentFooter, sizeof(fragmentFooter));
	
	//LOG("fragment source:\n%s\n", fbuff.data());
	
	GLuint fragID = glCreateShader(GL_FRAGMENT_SHADER);
	const char *src = fbuff.data();
	glShaderSource(fragID, 1, &src, nullptr);
	fbuff.clear();
	glCompileShader(fragID);
	if(!checkSHDErr(fragID)) ret = false;
	
	GLuint vertID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertID, 1, &vertexSrc, nullptr);
	glCompileShader(vertID);
	
	glAttachShader(shd, vertID);
	glAttachShader(shd, fragID);
	glLinkProgram(shd);
	glDeleteShader(vertID);
	glDeleteShader(fragID);
	if(ret = checkSHDErr(shd, false)) glUseProgram(shd);
	
	return ret;
	
}

void createCanvas() {
	float verts[] = {
		-1, 1,
		1, 1,
		-1, -1,
		1, -1
	};
	unsigned int inx[] = { 0, 2, 1, 1, 2, 3 };
	
	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inx), inx, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}



