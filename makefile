CC = g++
LIBS = lib\glew32s.lib -l glfw3 -l gdi32 -l opengl32

main.exe: main.cpp
	$(CC) $^ -o $@ -I include -L lib $(LIBS)