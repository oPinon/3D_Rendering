#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>

using namespace std;

int currentW = 800;
int currentH = 600;

int targetFramerate = 60;

int mouseLastX, mouseLastY;

int windowId;

GLuint uniform_ratio;

struct keyFunction {
	const string description;
	void(*function)(void);
};

struct Vec3 {
	float x, y, z;
	float norm2() const {
		return x*x + y*y + z*z;
	}
	float norm() const {
		return sqrt(norm2());
	}
	Vec3 operator/ (float v) const {
		return{ x / v, y / v, z / v };
	}
	Vec3 operator* (float v) const {
		return{ x * v, y * v, z * v };
	}
	Vec3 operator+(const Vec3& v) const {
		return{
			x + v.x,
			y + v.y,
			z + v.z
		};
	}
	Vec3 normalize() const {
		return operator/(norm());
	}
	Vec3 cross(const Vec3& v) const {
		return{
			y*v.z - v.y*z,
			z*v.x - v.z*x,
			x*v.y - v.x*y
		};
	}
	void glUniform(const GLuint pos) const {
		glUniform3f(pos, x, y, z);
	}
};

class Camera {

private:
	Vec3 pos, dir, left, up;
public:
	GLuint uniform_pos, uniform_dir, uniform_left, uniform_up; // shader uniforms
	Camera(Vec3 pos, Vec3 dir, Vec3 up = { 0,0,1 }) :
		pos(pos), dir(dir), up(up) {
		left = dir.cross(up).normalize();
	}
	void updateUniforms() {
		pos.glUniform(uniform_pos);
		dir.glUniform(uniform_dir);
		left.glUniform(uniform_left);
		up.glUniform(uniform_up);
	}
	void vertRot(float v) {
		dir = (dir + up * v).normalize();
		up = left.cross(dir).normalize();
	}
	void horzRot(float v) {
		dir = (dir + left * (-v)).normalize();
		left = (dir.cross(up)).normalize();
	}
	void moveHorz(float v) {
		pos = pos + left * (-v);
	}
	void moveVert(float v) {
		pos = pos + up * v;
	}
	void moveDir(float v) {
		pos = pos + dir * v;
	}
};

Camera camera = Camera(
	{ 0, -1.5, 0 },
	{ 0, 1, 0}
);

float camSpeed = 0.01;

unordered_map<char, keyFunction> keys = {
	{
		27 ,
		{
			"Exits the program",
			[](void) {
				glutDestroyWindow(windowId);
				exit(EXIT_SUCCESS);
			}
		}
	},
	{
		'w' ,
		{
			"Moves forward",
			[](void) { camera.moveDir(camSpeed); camera.updateUniforms(); }
		}
	},
	{
		's' ,
		{
			"Moves backward",
			[](void) { camera.moveDir(-camSpeed); camera.updateUniforms(); }
		}
	},
	{
		'a' ,
		{
			"Moves left",
			[](void) { camera.moveHorz(camSpeed); camera.updateUniforms(); }
		}
	},
	{
		'd' ,
		{
			"Moves right",
			[](void) { camera.moveHorz(-camSpeed); camera.updateUniforms(); }
		}
	},
	{
		' ' ,
		{
			"Moves up",
			[](void) { camera.moveVert(camSpeed); camera.updateUniforms(); }
		}
	},
	{
		31 ,
		{
			"Moves down",
			[](void) { camera.moveVert(-camSpeed); camera.updateUniforms(); }
		}
	}
};

string readFile(const string& fileName) {

	ifstream in(fileName, ios::binary | ios::ate);
	if (!in.is_open()) { cerr << "Error, can't read " << fileName.c_str() << endl; throw 1; }
	std::streamsize size = in.tellg();
	in.seekg(0, ios::beg);
	vector<char> buffer(size);
	in.read(buffer.data(), size);
	return string(buffer.data(), buffer.size());
}

struct Shader {

	GLuint vert, frag, prog;
	string name;

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
			cout << header << endl;
			log = (char *)malloc(logLength);
			if (isShader) {
				glGetShaderInfoLog(id, logLength, &charsWritten, log);
			}
			else {
				glGetProgramInfoLog(id, logLength, &charsWritten, log);
			}
			cout << log << endl;
			free(log);
			throw 1;
		}
	}

	Shader() { }
	Shader(const string& vertFile, const string& fragFile) : name(vertFile + " " + fragFile) {

		string vertCode = readFile(vertFile).data();
		const char* vertCodeP = vertCode.data();
		vert = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vert, 1, &vertCodeP, NULL);
		glCompileShader(vert);
		displayProgramErrors(vert, vertFile.c_str());

		string fragCode = readFile(fragFile).data();
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

		cout << "compiled and linked " << name.c_str() << endl;
	}
	GLint getUniformLocation(const char* varName) {
		GLint pos = glGetUniformLocation(prog, varName);
		if (pos == -1) {
			cerr << "cannot find uniform " << varName << " in " << name.c_str() << endl;
			//throw 1; TODO
		}
		return pos;
	}
	void use() {
		glUseProgram(prog);
	}
};

Shader shader;

void init() {

	glClearColor(0.0, 0.0, 0.0, 1.0);

	shader = Shader("vert.glsl", "frag.glsl");
	uniform_ratio = shader.getUniformLocation("ratio");
	camera.uniform_pos = shader.getUniformLocation("camPos");
	camera.uniform_dir = shader.getUniformLocation("camDir");
	camera.uniform_up = shader.getUniformLocation("camUp");
	camera.uniform_left = shader.getUniformLocation("camLeft");

	shader.use();
	glUniform1f(uniform_ratio, currentW / (float)currentH);
	camera.updateUniforms();
}

void display() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS); {
		glVertex3f(-1, -1, 0);
		glVertex3f(-1, 1, 0);
		glVertex3f(1, 1, 0);
		glVertex3f(1, -1, 0);
	}glEnd();

	glutSwapBuffers();
}

void idle() {

	static int nWaitUntil = glutGet(GLUT_ELAPSED_TIME);
	int nTimer = glutGet(GLUT_ELAPSED_TIME);
	if (nTimer >= nWaitUntil) {
		glutPostRedisplay();
		nWaitUntil = nTimer + (int)(1000.0f / targetFramerate);
	}
}

void keyboard(unsigned char key, int x, int y) {

	auto function = keys.find(key);
	if (function != keys.end()) {
		function->second.function();
	}
}

void keyboardSpecial(int key, int x, int y) {

}

void reshape(GLsizei w, GLsizei h) {

	currentW = w;
	currentH = h;
	glViewport(0, 0, w, h);
	glUniform1f(uniform_ratio, currentW / (float)currentH);
	display();
}

void mouseMove(int x, int y) {

	camera.horzRot(-0.001*(x - mouseLastX));
	camera.vertRot(-0.001*(y - mouseLastY));
	camera.updateUniforms();
	mouseLastX = x;
	mouseLastY = y;
}

void mouseClick(int button, int state, int x, int y) {

	if (state == GLUT_DOWN) {
		mouseLastX = x;
		mouseLastY = y;
	}
}

int main(int argc, char *argv[])
{
	cout << "Keys : " << endl;
	for (const auto& key : keys) {
		cout << " '" << key.first << "' " << key.second.description << endl;
	}

	glutInit(&argc, argv);

	glutInitWindowSize(currentW, currentH);

	windowId = glutCreateWindow("Fractal");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyboardSpecial);
	glutReshapeFunc(reshape);
	glutMotionFunc(mouseMove);
	glutMouseFunc(mouseClick);

	glewInit();

	init();

	glutMainLoop();

	return EXIT_SUCCESS;
}