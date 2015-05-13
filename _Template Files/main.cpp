#define WINDOW_WIDTH	1024
#define WINDOW_HEIGHT	768

#pragma comment(lib, "sdlmain.lib")
#pragma comment(lib, "sdl.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#include "InputSystem.h"
#include "SDL_Window.h"
#include "MenuDialog.h"
#include "WinsockWrapper/WinsockWrapper.h"

MenuDialog* TestMenuDialog;

bool SDL_ProcessEvents( InputSystem* Input );
void Resize_View( unsigned int Width, unsigned int Height );

void Init_Client( void );
void Init_Server( void );
void Set_Status_Text( char* text );

bool Server = false;
int Socket = -1;
int ClientCount = 0;

int main( int argc , char* argv[] )
{
	//  Create access to the InputSystem
	InputSystem*	input = InputSystem::Get_Instance();

	//  Define the window. If the window definition fails, return a failure flag.
	SDL_Window window;
	if ( !window.Define_Window( WINDOW_WIDTH , WINDOW_HEIGHT , 32 , false , "Winsock Wrapper" ) ) return -1;

	//  Create the Menu Dialog. If the dialog creation fails, return a failure flag.
	TestMenuDialog = MenuDialog::CreateMenuDialog( "WinsockWrapperMenu" );
	if (TestMenuDialog == NULL) return -3;

	TestMenuDialog->Load_Bitmap_Font("Arial");

	MenuElement_TextButton* init_client_button = dynamic_cast<MenuElement_TextButton*>(TestMenuDialog->Get_Element("InitClientButton"));
	if (init_client_button != NULL)
	{
		init_client_button->Set_Click_Event(&Init_Client);
	}

	MenuElement_TextButton* init_server_button = dynamic_cast<MenuElement_TextButton*>(TestMenuDialog->Get_Element("InitServerButton"));
	if (init_server_button != NULL)
	{
		init_server_button->Set_Click_Event(&Init_Server);
	}

	Set_Status_Text( "" );

	//  Initialize the Winsock Wrapper
	Winsock_Init();

	// Set up the general information for the rendering of the window through OpenGL
	glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.02f);

	//  Process any events from the Operating System
	while (SDL_ProcessEvents( input ))
	{
		if (input->Get_Key(27)) break;		// 27 : ESCAPE

		// Get the time slice
		static float time_slice;
		static int elapsed_time[2];
		elapsed_time[0] = SDL_GetTicks();
		time_slice = std::max(std::min((float)(elapsed_time[0] - elapsed_time[1]) / 1000.0f, 1.0f), 0.1f);
		elapsed_time[1] = elapsed_time[0];

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Initiate the 2D Rendering Context
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, (GLdouble)WINDOW_WIDTH, (GLdouble)WINDOW_HEIGHT, 0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		TestMenuDialog->Input( input );
		TestMenuDialog->Update( time_slice );
		TestMenuDialog->Render();

		SDL_GL_SwapBuffers();
		input->Invalidate_Old_Input();
	}

	closesock( Socket );

	return 0;
}

bool SDL_ProcessEvents( InputSystem* input )
{
	//! This function will take in information from SDL's OS
	//! message retrieval system to get things such as input
	//! and important system messages relating to the window.

	//! It will return true if all messages are able to be
	//! handled and false if any message indicates a shutdown
	//! of the program for any reason.

	//  Loop through all events
	SDL_Event E;
	while ( SDL_PollEvent(&E) )
	{
		switch ( E.type )
		{
			case SDL_QUIT:
			{
				//  Return false for the quit event
				return false;
			}

			case SDL_KEYDOWN:
			{	
				// Set the designated key to 1
				input->Set_Key( E.key.keysym.sym , 1 );
				input->Add_Key_To_String( (short)E.key.keysym.sym );
				break;
			}

			case SDL_KEYUP:
			{
				// Set the designated key to 0
				input->Set_Key( E.key.keysym.sym , 0 );
				break;
			}

			case SDL_MOUSEMOTION:
			{
				input->Set_Mouse_X( E.motion.x );
				input->Set_Mouse_Y( E.motion.y );
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			{
				switch ( E.button.button )
				{
				case SDL_BUTTON_LEFT:
					input->Set_Mouse_Button( 0 , 1 );
					break;
				case SDL_BUTTON_RIGHT:
					input->Set_Mouse_Button( 1 , 1 );
					break;
				case SDL_BUTTON_MIDDLE:
					input->Set_Mouse_Button( 2 , 1 );
					break;
				}
				break;
			}

			case SDL_MOUSEBUTTONUP:
			{
				switch ( E.button.button )
				{
				case SDL_BUTTON_LEFT:
					input->Set_Mouse_Button( 0 , 0 );
					break;
				case SDL_BUTTON_RIGHT:
					input->Set_Mouse_Button( 1 , 0 );
					break;
				case SDL_BUTTON_MIDDLE:
					input->Set_Mouse_Button( 2 , 0 );
					break;
				default:
					break;
				}
				break;
			}

			case SDL_VIDEORESIZE:
			{
				Resize_View( E.resize.w , E.resize.h );
				break;
			}

			default:
			{
				break;
			}
		}
	}

	return true;
}

void Resize_View( unsigned int width, unsigned int height )
{
	//! This function will take in a Width and Height, and alter
	//! the viewport of the screen as well as the window size.

	//  Ensure that Height is a positive number
	if ( height <= 0 ) height = 1;

	// Set up the Viewport
	glViewport( 0 , 0 , width , height );

	// Set up Projection View
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 60.0 , (GLdouble)width / (GLdouble)height , 1.0 , 100.0 );

	// Set up Model View
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}

void Init_Client( void )
{
	assert( Socket < 0 && "Socket is already defined!" );
	if (Socket != -1) return;

	Socket = tcpconnect("127.0.0.1", 2345, 1);
	assert( Socket >= 0 && "Connecting to server creation failed!" );
	if (Socket == -1) return;

	Server = false;

	Set_Status_Text( "Successfully connected to server!" );
}

void Init_Server( void )
{
	assert( Socket < 0 && "Socket is already defined!" );
	if (Socket != -1) return;

	// Initialize the Listening Port for Clients
	Socket = tcplisten(2345, 10, 1);
	assert( Socket >= 0 && "Server listening socket creation failed!" );
	if (Socket == -1) return;

	ClientCount = 0;
	Server = true;

	setnagle(Socket, true);

	Set_Status_Text( "Successfully started a server!");
}

void Set_Status_Text( char* text )
{
	static MenuElement_TextBlock* status_text = dynamic_cast<MenuElement_TextBlock*>(TestMenuDialog->Get_Element("StatusText"));
	if ( status_text )
	{
		status_text->Set_Text( text );
	}
}