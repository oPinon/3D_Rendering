
#include <GlewGlut.h>

void init()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void display()
{
	size_t nLines = 10;
	for (size_t j = 0; j < 8; j++)
	{
		size_t n2 = nLines * pow( 2.0, j );
		float weight = 1.0 * pow( 2.0, - 1.0 * j );
		weight *= GlewGlut::turnAroundCamera.zoom;
		glLineWidth( weight > 1 ? weight : 1 );
		glColor4f(1, 1, 1, weight > 1 ? 1 : weight);
		glBegin(GL_LINES);
		for (size_t i = 0; i <= n2; i++)
		{
			float v = 1 - 2 * float(i) / n2;
			glVertex2f(-1, v);
			glVertex2f(1, v);
			glVertex2f(v, -1);
			glVertex2f(v, 1);
		}
		glEnd();
	}
}

int main(int argc, char* argv[])
{
	GlewGlut::Callbacks callbacks;
	callbacks.display = display;
	callbacks.init = init;
	GlewGlut::main(callbacks);
}
