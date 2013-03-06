#include "stdafx.h"
#include "Graphics/GraphicsContext.h"

#include <GL/glfw.h>

#include "Graphics/ColourValue.h"


static u32 SCR_WIDTH = 640;
static u32 SCR_HEIGHT = 480;


class IGraphicsContext : public CGraphicsContext
{
public:
	virtual ~IGraphicsContext();


	virtual bool Initialise();
	virtual bool IsInitialised() const { return true; }

	virtual void ClearAllSurfaces();
	virtual void ClearZBuffer();
	virtual void ClearColBuffer(const c32 & colour);
	virtual void ClearToBlack();
	virtual void ClearColBufferAndDepth(const c32 & colour);
	virtual	void BeginFrame();
	virtual void EndFrame();
	virtual void UpdateFrame( bool wait_for_vbl );

	virtual void GetScreenSize(u32 * width, u32 * height) const;
	virtual void ViewportType(u32 * width, u32 * height) const;

	virtual void SetDebugScreenTarget( ETargetSurface buffer ) {}
	virtual void DumpNextScreen() {}
	virtual void DumpScreenShot() {}
};

template<> bool CSingleton< CGraphicsContext >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IGraphicsContext();
	return mpInstance->Initialise();
}


IGraphicsContext::~IGraphicsContext()
{
}

bool IGraphicsContext::Initialise()
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return false;
	}

	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);

	// Open a window and create its OpenGL context
	if( !glfwOpenWindow( SCR_WIDTH, SCR_HEIGHT,
						0,0,0,0,			// RGBA bits
						24,					// Depth bits
						0,					// Stencil bits
						GLFW_WINDOW ) )
	{
		fprintf( stderr, "Failed to open GLFW window\n" );

		glfwTerminate();
		return false;
	}

	glfwSetWindowTitle( "Daedalus" );

	// Ensure we can capture the escape key being pressed below
	//glfwEnable( GLFW_STICKY_KEYS );

	// Enable vertical sync (on cards that support it)
	glfwSwapInterval( 1 );

	ClearAllSurfaces();

	// FIXME(strmnnrmn): this needs tidying.
	extern bool initgl();
	return initgl();
}

void IGraphicsContext::GetScreenSize(u32 * width, u32 * height) const
{
	int window_width, window_height;
	glfwGetWindowSize( &window_width, &window_height );

	*width  = window_width;
	*height = window_height;
}

void IGraphicsContext::ViewportType(u32 * width, u32 * height) const
{
	GetScreenSize(width, height);
}

void IGraphicsContext::ClearAllSurfaces()
{
	// FIXME: this should clear/flip a couple of times to ensure the front and backbuffers are cleared.
	// Not sure if it's necessary...
	ClearToBlack();
}


void IGraphicsContext::ClearToBlack()
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void IGraphicsContext::ClearZBuffer()
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClear( GL_DEPTH_BUFFER_BIT );
}

void IGraphicsContext::ClearColBuffer(const c32 & colour)
{
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT );
}

void IGraphicsContext::ClearColBufferAndDepth(const c32 & colour)
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void IGraphicsContext::BeginFrame()
{
	// Get window size (may be different than the requested size)
	int width, height;
	glfwGetWindowSize( &width, &height );

	// Special case: avoid division by zero below
	height = height > 0 ? height : 1;

	glViewport( 0, 0, width, height );

	ClearToBlack();
}

void IGraphicsContext::EndFrame()
{
}

void IGraphicsContext::UpdateFrame( bool wait_for_vbl )
{
	glfwSwapBuffers();
}