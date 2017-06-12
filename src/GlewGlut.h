#pragma once

#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <math.h>
#include <assert.h>
#include <string>

template<typename T>
T max(T a, T b) { return a < b ? b : a; }
template<typename T>
T min(T a, T b) { return a < b ? a : b; }

template<typename A, typename B>
inline A mod(const A& a, const B& b)
{ return a - floor( a / b ) * b; }

namespace GlewGlut {

	struct KeyFunction {
		std::string description;
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

	struct AbstractCamera
	{
		size_t currentW, currentH;

		virtual void init() {};
		virtual void display() {};
		virtual void mouseClick(int button, int state, int x, int y) {};
		virtual void mouseMove(int x, int y) {};
		virtual void reshape(GLsizei w, GLsizei h)
		{
			currentW = w;
			currentH = h;
			glViewport(0, 0, w, h);
		}
	};

	struct TurnAroundCamera : public AbstractCamera
	{
		int mouseLastX, mouseLastY;
		float viewTrX = 0, viewY = 0;
		float viewRotX = 30, viewRotZ = 10, zoom = 1;
		float viewTrSpeed = 3.0;
		float zoomSpeed = 0.96f;
		bool movingCamera;

		void display() override
		{
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

			glTranslatef(viewTrX, 0, viewY);
			glRotated(viewRotX, 1, 0, 0);
			glRotated(viewRotZ, 0, 0, 1);
			glScalef(zoom, zoom, zoom);
		}

		void mouseMove(int x, int y) override
		{
			if (movingCamera) {
				viewTrX += viewTrSpeed*float(x - mouseLastX) / fminf(currentH, currentW);
				viewY -= viewTrSpeed*float(y - mouseLastY) / fminf(currentH, currentW);
			}
			else {
				viewRotX = mod(viewRotX + y - mouseLastY, 360);
				viewRotZ = mod(viewRotZ + x - mouseLastX, 360);
			}
			mouseLastX = x;
			mouseLastY = y;
		}

		void mouseClick(int button, int state, int x, int y) override
		{
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

		inline void init() override;
	};

	int windowId;
	AbstractCamera fixedCamera;
	TurnAroundCamera turnAroundCamera;
	AbstractCamera* camera = NULL;

	struct Callbacks {
		void(*display)() = NULL;
		void(*init)() = NULL;
		void(*reshape)() = NULL;
	} callbacks;

	struct Params {

		AbstractCamera* camera = &turnAroundCamera;
		GLsizei defaultW = 800, defaultH = 600;

	} params;

	void display() {

		camera->display();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (callbacks.display != NULL) { callbacks.display(); }

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
		}
	};

	inline void TurnAroundCamera::init()
	{
		static TurnAroundCamera* s_cam = NULL; // HACK
		s_cam = this;
		keys.insert({ 'w',{
			"Move the camera",
			[](bool down) {
				s_cam->movingCamera = down;
			}
		} });
	}

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

		camera->reshape(w,h);
		if(callbacks.reshape != NULL) { callbacks.reshape(); }
		display();
	}

	void mouseMove(int x, int y) {
		camera->mouseMove(x, y);
	}

	void mouseClick(int button, int state, int x, int y) {
		camera->mouseClick( button, state, x, y );
	}

	void main( const Callbacks& callbacks, const Params& params = {} ) {

		GlewGlut::callbacks = callbacks;
		GlewGlut::params = params;

		int argc = 0;
		glutInit(&argc, NULL);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

		glutInitWindowSize(params.defaultW,params.defaultW);
		camera = params.camera;
		camera->init();
		camera->reshape(params.defaultW, params.defaultW);

		std::cout << "Keys : " << std::endl;
		for (const auto& key : keys) {
			std::cout << " '" << key.first << "' -> " << key.second.description << std::endl;
		}

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

		if (callbacks.init != NULL) { callbacks.init(); }

		glutMainLoop();
	}

	// Legacy. TODO : remove
	void main(void(*displayFunction)() = NULL, void(*initFunction)() = NULL) {
		Callbacks callbacks;
		callbacks.display = displayFunction;
		callbacks.init = initFunction;
		main(callbacks);
	}
}
