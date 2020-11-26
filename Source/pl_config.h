
#ifndef PL_CONFIG_UNDEF


//-----------------------------------------------------<Window>-----------------------------------------------------
//------------------------------------------<Render types>----------------------------
#define PL_BLIT_BITMAP 1
#define PL_OPENGL 2
//------------------------------------------</Render types>---------------------------
#define PL_WINDOW_RENDERTYPE PL_BLIT_BITMAP	//Set to the render type needed for the window 


//-----------------------------------------------------</Window>----------------------------------------------------

//-----------------------------------------------------<Input>------------------------------------------------------
#define PL_INPUT_KEYBOARD_MAX_KEYS 255
//-----------------------------------------------------</Input>-----------------------------------------------------


#else	//Undefining the Macros
#undef PL_BLIT_BITMAP
#undef PL_OPENGL
#undef PL_WINDOW_RENDERTYPE

#undef PL_INPUT_KEYBOARD_MAX_KEYS

#undef PL_CONFIG_UNDEF
#endif