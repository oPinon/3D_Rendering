
#include <GlewGlut.h>
#include <Vec.h>

template <uint S, typename T>
std::ostream &operator<<( std::ostream &os, Vec<S, T> const & vec )
{
	os << "{ ";
	//for( const auto& v : vec )
	for( size_t i = 0; i < S; i++)
	{
		//if( &v != vec.begin() )
		if( i != 0 )
			os << ", ";
		os << vec[i];
	}
	os << " }";
	return os;
}

void init()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

struct ZoomPanCamera : public GlewGlut::AbstractCamera
{
	float zoomSpeed = 1.1;
	float zoom = 1.0;
	Vec2F sceneOffset;
	Vec2I lastMousePosPx;
	inline float pxToSceneScale() const { return 2 / zoom / min(currentW, currentH); }
	inline Vec2F pxToScene(const Vec2I& px) const { return Vec2F(px) * this->pxToSceneScale() - this->sceneOffset; }
	void mouseClick(int button, int state, int x, int y) override
	{
		Vec2I mp = { x, y };
		Vec2F prevPos = pxToScene( mp );
		if (state == GLUT_DOWN )
		{
			switch( button )
			{
			case 3: zoom *= zoomSpeed; break;
			case 4: zoom /= zoomSpeed; break;
			}
		}
		Vec2F newPos = pxToScene( mp );
		//this->sceneOffset += ( newPos - prevPos ) / 2;
		lastMousePosPx = mp;
	}
	void display() override
	{
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		float minS = min( this->currentW, this->currentH );
		float xRatio = float(this->currentW) / minS;
		float yRatio = float(this->currentH) / minS;
		glOrtho(
			-1 * xRatio, 1 * xRatio,
			1 * yRatio, -1 * yRatio,
			-1, 1
		);
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		glScalef(zoom, zoom, zoom);
		glTranslatef(sceneOffset[0], sceneOffset[1], 0);
	}
	void mouseMove(int x, int y) override
	{
		Vec2I newPos = { x, y };
		Vec2F diff = this->pxToScene( newPos ) - this->pxToScene( this->lastMousePosPx );
		sceneOffset += diff;
		this->lastMousePosPx = newPos;
	}
} camera;

void display()
{
	size_t nLines = 10;
	for (size_t j = 0; j < 8; j++)
	{
		size_t n2 = nLines * pow( 2.0, j );
		float weight = 1.0 * pow( 2.0, - 1.0 * j );
		weight *= camera.zoom;
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
	GlewGlut::Params params; params.camera = &camera;
	GlewGlut::main(callbacks,params);
}
