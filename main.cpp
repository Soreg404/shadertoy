#define GLEW_STATIC
#include "glew/glew.h"
#include "glfw/glfw3.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <memory.h>

#define LOG(x, ...) printf(x "\n", ## __VA_ARGS__);

int WW = 800, WH = 600;

void glerr(const char *file, int line) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR) {
		LOG("[%s] %i: opengl error: %x", file, line, err);
	}
}
#define GLERR glerr(__FILE__, __LINE__) 

namespace src {
	
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
	uniform sampler2D	iChannel0, iChannel1, iChannel2, iChannel3;
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
	
}

GLuint shd = 0;
bool loadShd(const char *fragment_path);

struct ULOC {
	GLint
	iResolution,				// viewport resolution (in pixels)                           vec3     
	iTime,						// shader playback time (in seconds)                         float    
	iTimeDelta,					// render time (in seconds)                                  float    
	iFrame,						// shader playback frame                                     int      
	iChannelTime[4],			// channel playback time (in seconds)                        float    
	iChannelResolution[4],		// channel resolution (in pixels)                            vec3     
	iMouse,						// mouse pixel coords. xy: current (if MLB down), zw: click  vec4     
	iChannel0, iChannel1,		// input channel. XX = 2D/Cube                               samplerXX
	iChannel2, iChannel3,
	iDate,						// (year, month, day, time in seconds)                       vec4     
	iSampleRate;				// sound sample rate (i.e., 44100)                           float    
};

void findUniforms(ULOC *str);
ULOC locs;

void createCanvas();

void wResizeClb(GLFWwindow *handle, int w, int h) {
	WW = w; WH = h;
	glViewport(0, 0, WW, WH);
	glUniform3f(locs.iResolution, WW, WH, 1);
	char buff[100];
	sprintf_s(buff, 100, "%ix%i (resize)", WW, WH);
	glfwSetWindowTitle(handle, buff);
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
	
	locs = ULOC();
	findUniforms(&locs);
	
	glUniform3f(locs.iResolution, WW, WH, 1);
	
	createCanvas();
	
	glfwShowWindow(wnd);
	
	float shdTime = 0, lastFTime = glfwGetTime();
	
	while(!glfwWindowShouldClose(wnd)) {
		
		shdTime += glfwGetTime() - lastFTime;
		lastFTime = glfwGetTime();
		
		glfwPollEvents();
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUniform1f(locs.iTime, shdTime);
		
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
	
	size_t footerBeg = (sizeof(src::fragmentHeader) - 1) + fsize;
	std::vector<char> fbuff(footerBeg + sizeof(src::fragmentFooter));
	
	file.read(fbuff.data() + sizeof(src::fragmentHeader) - 1, fsize);
	file.close();
	
	memcpy_s(fbuff.data(), fbuff.size(), src::fragmentHeader, sizeof(src::fragmentHeader) - 1);
	memcpy_s(fbuff.data() + footerBeg, fbuff.size() - footerBeg, src::fragmentFooter, sizeof(src::fragmentFooter));
	
	//LOG("fragment source:\n%s\n", fbuff.data());
	
	GLuint fragID = glCreateShader(GL_FRAGMENT_SHADER);
	const char *src = fbuff.data();
	glShaderSource(fragID, 1, &src, nullptr);
	fbuff.clear();
	glCompileShader(fragID);
	if(!checkSHDErr(fragID)) ret = false;
	
	GLuint vertID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertID, 1, &src::vertexSrc, nullptr);
	glCompileShader(vertID);
	
	glAttachShader(shd, vertID);
	glAttachShader(shd, fragID);
	glLinkProgram(shd);
	glDeleteShader(vertID);
	glDeleteShader(fragID);
	if(ret = checkSHDErr(shd, false)) glUseProgram(shd);
	
	return ret;
	
}

void findUniforms(ULOC *str) {
#define GETL(x) str->x = glGetUniformLocation(shd, #x)
	GETL(iResolution);
	GETL(iTime);
	GETL(iTimeDelta);
	GETL(iFrame);
	GETL(iChannelTime[0]);
	GETL(iChannelTime[1]);
	GETL(iChannelTime[2]);
	GETL(iChannelTime[3]);
	GETL(iChannelResolution[0]);
	GETL(iChannelResolution[1]);
	GETL(iChannelResolution[2]);
	GETL(iChannelResolution[3]);
	GETL(iMouse);
	GETL(iChannel0);
	GETL(iChannel1);
	GETL(iChannel2);
	GETL(iChannel3);
	GETL(iDate);
	GETL(iSampleRate);
	
	//for(int i = 0; i < sizeof(ULOC) / sizeof(GLint); i++) LOG("uloc %i: %i", i, reinterpret_cast<GLint *>(str)[i]);
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



