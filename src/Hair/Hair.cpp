#include "../src/GlewGlut.h"
#include "../src/Mesh.h"

Mesh mesh;
Mesh hair;
GlewGlut::Shader meshShader;
GlewGlut::Shader hairShader;
bool turnTable = false;

void reloadShader() {

	try {
		meshShader = GlewGlut::Shader(
			"./vert.glsl",
			"./frag.glsl"
		);
	}
	catch (...) { return; }
	try {
		hairShader = GlewGlut::Shader(
			"./vert.glsl",
			"./fragHair.glsl"
		);
	}
	catch (...) { return; }
}

struct Camera : public GlewGlut::TurnAroundCamera
{
	void display() override
	{
		if (turnTable)
			this->viewRotZ = fmodf( viewRotZ + 1.0, 360 );
		GlewGlut::TurnAroundCamera::display();
	}
} camera;

void displayScene() {

	meshShader.use();
	mesh.draw();
	hairShader.use();
	hair.draw();
}

void init() {

	glEnable(GL_DEPTH_TEST);

	glClearColor(0.5,0.5,0.5,0.0);
	std::cout << "Reading meshes on disk" << std::endl;
	mesh = Mesh::loadWavefront("body.obj");
	hair = Mesh::loadWavefront("hairLines.obj");
	std::cout << "Sending Vertex Buffers" << std::endl;
	mesh.init();
	hair.init();

	reloadShader();
}

int main() {

	GlewGlut::keys.insert({ 't',{
		"TurnTable",
		[](bool down) {
			if(down) { turnTable = !turnTable; }
		}
	} });
	GlewGlut::keys.insert({ 'r',{
		"Reload the Shader",
		[](bool down) {
			if(down) { reloadShader(); }
		}
	} });

	GlewGlut::Callbacks callbacks;
	callbacks.init = init;
	callbacks.display = displayScene;
	GlewGlut::Params params; params.camera = &camera;
	GlewGlut::main( callbacks, params );
}
