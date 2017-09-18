
#include <GlewGlut.h>
#include <Mesh.h>
#include <lodepng.h>

// Inspired by http://leegriggs.com/xgen-rendered-with-arnold-for-maya

Plane imagePlane;
GlewGlut::Shader shader;
Mesh landscape;

typedef unsigned char uchar;

struct Image {
	GLuint w, h;
	std::vector<uchar> pixels;
	inline uchar at(size_t x, size_t y, size_t c = 0) const { return this->pixels[4*(y*this->w + x)+c]; }
	Image(std::string fileName) {
		unsigned error = lodepng::decode(pixels, w, h, fileName);
		if (error) {
			std::cerr << "error when opening " << fileName <<
				" " << lodepng_error_text(error) << std::endl; throw 1;
		}
	}
};

void init()
{
	glEnable(GL_DEPTH_TEST);

	imagePlane.init();
	Image image("particleLandscape.png");

	shader = GlewGlut::Shader("imageVert.glsl", "imageFrag.glsl");
	GLuint texId;
	glGenTextures(1,&texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.w, image.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glUniform1ui(shader.getUniformLocation("image"), 0);

	for( size_t y = 0; y < image.h; y++ )
		for( size_t x = 0; x < image.w; x++ )
		{
			Cube cube;
			cube.computeSharpNormals();
			const float offset = 0.1 * ( image.at(x, y, 3) / 255.0f );
			cube.scale( Vec3F(
				1.0 / image.w,
				1.0 / image.h,
				offset
			) );
			cube.translate(Vec3F(
				2 * float(x) / image.w - 1,
				2 * float(y) / image.h - 1,
				offset
			));
			landscape += cube;
		}
}

void displayScene()
{
	shader.use();
	imagePlane.draw();
	landscape.draw();
}

int main(int argc, char* argv[])
{
	GlewGlut::Callbacks callbacks;
	callbacks.init = init;
	callbacks.display = displayScene;
	GlewGlut::Params params;
	GlewGlut::main(callbacks, params);
}
