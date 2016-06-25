#pragma once

#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>

template<typename T>
T max(T a, T b) { return a < b ? b : a; }
template<typename T>
T min(T a, T b) { return a < b ? a : b; }

namespace GlewGlut {

	struct KeyFunction {
		const std::string description;
		void(*function)(bool);
	};

	std::string readFile(const std::string& fileName) {

		std::ifstream in(fileName, std::ios::binary | std::ios::ate);
		if (!in.is_open()) { std::cerr << "Error, can't read " << fileName.c_str() << std::endl; throw 1; }
		std::streamsize size = in.tellg();
		in.seekg(0, std::ios::beg);
		std::vector<char> buffer(size);
		in.read(buffer.data(), size);
		return std::string(buffer.data(), buffer.size());
	}

	struct Shader {

		GLuint vert, frag, prog;
		std::string name;

		static void displayProgramErrors(
			GLuint id,
			const char* header,
			bool isShader = true // is it a shader or a program
		) {
			int logLength = 0;
			int charsWritten = 0;
			char *log;
			if (isShader) {
				glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
			}
			else {
				glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);
			}
			if (logLength > 1)
			{
				std::cout << header << std::endl;
				log = (char *)malloc(logLength);
				if (isShader) {
					glGetShaderInfoLog(id, logLength, &charsWritten, log);
				}
				else {
					glGetProgramInfoLog(id, logLength, &charsWritten, log);
				}
				std::cout << log << std::endl;
				free(log);
				throw 1;
			}
		}

		Shader() { }
		Shader(const std::string& vertFile, const std::string& fragFile) : name(vertFile + " " + fragFile) {

			std::string vertCode = readFile(vertFile).data();
			const char* vertCodeP = vertCode.data();
			vert = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vert, 1, &vertCodeP, NULL);
			glCompileShader(vert);
			displayProgramErrors(vert, vertFile.c_str());

			std::string fragCode = readFile(fragFile).data();
			const char* fragCodeP = fragCode.data();
			frag = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(frag, 1, &fragCodeP, NULL);
			glCompileShader(frag);
			displayProgramErrors(frag, fragFile.c_str());

			prog = glCreateProgram();
			glAttachShader(prog, vert);
			glAttachShader(prog, frag);
			glLinkProgram(prog);
			displayProgramErrors(prog, name.data(), false);

			std::cout << "compiled and linked " << name.c_str() << std::endl;
		}
		GLint getUniformLocation(const char* varName) {
			GLint pos = glGetUniformLocation(prog, varName);
			if (pos == -1) {
				std::cerr << "cannot find uniform " << varName << " in " << name.c_str() << std::endl;
				throw 1;
			}
			return pos;
		}
		GLint getAttributeLocation(const char* varName) {
			GLint pos = glGetAttribLocation(prog, varName);
			if (pos == -1) {
				std::cerr << "cannot find attribute " << varName << " in " << name.c_str() << std::endl;
				throw 1;
			}
			return pos;
		}
		void use() {
			glUseProgram(prog);
		}
	};

	int currentW = 800, currentH = 600;
	int windowId;
	int mouseLastX, mouseLastY;
	float viewTrX = 0, viewY = 0;
	float viewRotX = 30, viewRotZ = 10, zoom = 1;
	float viewTrSpeed = 3.0;
	bool movingCamera;

	void (*displayScene)();

	void display() {

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		gluPerspective(50, ((double)currentW) / currentH, 0.1, 100);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		gluLookAt(
			0, -3, 0,
			0, 0, 0,
			0, 0, 1
		);

		glTranslatef(viewTrX,0,viewY);
		glRotated(viewRotX, 1, 0, 0);
		glRotated(viewRotZ, 0, 0, 1);
		glScalef(zoom, zoom, zoom);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (displayScene != NULL) { displayScene(); }

		glutSwapBuffers();
	}

	void idle() {

		static int nWaitUntil = glutGet(GLUT_ELAPSED_TIME);
		int nTimer = glutGet(GLUT_ELAPSED_TIME);
		if (nTimer >= nWaitUntil) {
			glutPostRedisplay();
			nWaitUntil = nTimer + (int)(1000.0f / 60);
		}
	}

	std::unordered_map<char, KeyFunction> keys = {
		{
			27 ,
			{
				"Exits the program",
				[](bool down) {
					if(down) {
						glutDestroyWindow(windowId);
						exit(EXIT_SUCCESS);
					}
				}
			}
		},
		{
			'w' ,
			{
				"Move the camera",
				[](bool down) {
					movingCamera = down;
				}
			}
		}
	};

	void keyboard(unsigned char key, int x, int y) {

		auto function = keys.find(key);
		if (function != keys.end()) {
			function->second.function(true);
		}
	}

	void keyboardUp(unsigned char key, int x, int y) {

		auto function = keys.find(key);
		if (function != keys.end()) {
			function->second.function(false);
		}
	}

	void keyboardSpecial(int key, int x, int y) {

	}

	void reshape(GLsizei w, GLsizei h) {

		currentW = w;
		currentH = h;
		glViewport(0, 0, w, h);
		display();
	}

	void mouseMove(int x, int y) {

		if(movingCamera) {
			viewTrX += viewTrSpeed*float(x - mouseLastX)/min(currentH,currentW);
			viewY -= viewTrSpeed*float(y - mouseLastY)/min(currentH,currentW);
		} else {
			viewRotX = fmodf(viewRotX + y - mouseLastY, 360);
			viewRotZ = fmodf(viewRotZ + x - mouseLastX, 360);	
		}
		mouseLastX = x;
		mouseLastY = y;
	}

	float zoomSpeed = 0.96;
	void mouseClick(int button, int state, int x, int y) {

		switch (button) {
		case 3: { // Scroll down
			zoom /= zoomSpeed;
		} break;
		case 4: { // Scroll up
			zoom *= zoomSpeed;
		} break;
		default: {
			if (state == GLUT_DOWN) {
				mouseLastX = x;
				mouseLastY = y;
			}
		}
		}
	}

	void main(void(*displaySceneFunction)() = NULL, void(*initFunction)() = NULL) {

		displayScene = displaySceneFunction;

		std::cout << "Keys : " << std::endl;
		for (const auto& key : keys) {
			std::cout << " '" << key.first << "' -> " << key.second.description << std::endl;
		}

		int argc = 0;
		glutInit(&argc, NULL);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

		glutInitWindowSize(currentW, currentH);

		windowId = glutCreateWindow("Scene");

		glutDisplayFunc(display);
		glutIdleFunc(idle);
		glutKeyboardFunc(keyboard);
		glutKeyboardUpFunc(keyboardUp);
		glutSpecialFunc(keyboardSpecial);
		glutReshapeFunc(reshape);
		glutMotionFunc(mouseMove);
		glutMouseFunc(mouseClick);

		glewInit();

		if (initFunction != NULL) { initFunction(); }

		glutMainLoop();
	}
}