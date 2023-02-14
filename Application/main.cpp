/*____________________________________________________________________
|
| File: main.cpp
|
| Description: Main module in program.
|
| Functions:  Program_Get_User_Preferences
|             Program_Init
|							 Init_Graphics
|								Set_Mouse_Cursor
|             Program_Run
|							 Init_Render_State
|             Program_Free
|             Program_Immediate_Key_Handler
|
| (C) Copyright 2013 Abonvita Software LLC.
| Licensed under the GX Toolkit License, Version 1.0.
|___________________________________________________________________*/

#define _MAIN_

/*___________________
|
| Include Files
|__________________*/

#include <first_header.h>
#include "dp.h"
#include "..\Framework\win_support.h"
#include <rom8x8.h>

#include "main.h"
#include "position.h"
#include <time.h>

/*___________________
|
| Type definitions
|__________________*/

typedef struct {
	unsigned resolution;
	unsigned bitdepth;
} UserPreferences;

/*___________________
|
| Function Prototypes
|__________________*/

static int Init_Graphics(unsigned resolution, unsigned bitdepth, unsigned stencildepth, int* generate_keypress_events);
static void Set_Mouse_Cursor();
static void Init_Render_State();

/*___________________
|
| Constants
|__________________*/

#define MAX_VRAM_PAGES  2
#define GRAPHICS_RESOLUTION  \
  (                          \
    gxRESOLUTION_640x480   | \
    gxRESOLUTION_800x600   | \
    gxRESOLUTION_1024x768  | \
    gxRESOLUTION_1152x864  | \
    gxRESOLUTION_1280x960  | \
    gxRESOLUTION_1400x1050 | \
    gxRESOLUTION_1440x1080 | \
    gxRESOLUTION_1600x1200 | \
    gxRESOLUTION_1152x720  | \
    gxRESOLUTION_1280x800  | \
    gxRESOLUTION_1440x900  | \
    gxRESOLUTION_1680x1050 | \
    gxRESOLUTION_1920x1200 | \
    gxRESOLUTION_2048x1280 | \
    gxRESOLUTION_1280x720  | \
    gxRESOLUTION_1600x900  | \
    gxRESOLUTION_1920x1080   \
  )
#define GRAPHICS_STENCILDEPTH 0
#define GRAPHICS_BITDEPTH (gxBITDEPTH_24 | gxBITDEPTH_32)

#define AUTO_TRACKING    1
#define NO_AUTO_TRACKING 0

/*____________________________________________________________________
|
| Function: Program_Get_User_Preferences
|
| Input: Called from CMainFrame::Init
| Output: Allows program to popup dialog boxes, etc. to get any user
|   preferences such as screen resolution.  Returns preferences via a
|   pointer.  Returns true on success, else false to quit the program.
|___________________________________________________________________*/

int Program_Get_User_Preferences(void** preferences)
{
	static UserPreferences user_preferences;

	if (gxGetUserFormat(GRAPHICS_DRIVER, GRAPHICS_RESOLUTION, GRAPHICS_BITDEPTH, &user_preferences.resolution, &user_preferences.bitdepth)) {
		*preferences = (void*)&user_preferences;
		return (1);
	}
	else
		return (0);
}

/*____________________________________________________________________
|
| Function: Program_Init
|
| Input: Called from CMainFrame::Start_Program_Thread()
| Output: Starts graphics mode.  Returns # of user pages available if
|       successful, else 0.
|___________________________________________________________________*/

int Program_Init(void* preferences, int* generate_keypress_events)
{
	UserPreferences* user_preferences = (UserPreferences*)preferences;
	int initialized = FALSE;

	if (user_preferences)
		initialized = Init_Graphics(user_preferences->resolution, user_preferences->bitdepth, GRAPHICS_STENCILDEPTH, generate_keypress_events);

	return (initialized);
}

/*____________________________________________________________________
|
| Function: Init_Graphics
|
| Input: Called from Program_Init()
| Output: Starts graphics mode.  Returns # of user pages available if
|       successful, else 0.
|___________________________________________________________________*/

static int Init_Graphics(unsigned resolution, unsigned bitdepth, unsigned stencildepth, int* generate_keypress_events)
{
	int num_pages;
	byte* font_data;
	unsigned font_size;

	/*____________________________________________________________________
	|
	| Init globals
	|___________________________________________________________________*/
	Pgm_num_pages = 0;
	Pgm_system_font = NULL;

	/*____________________________________________________________________
	|
	| Start graphics mode and event processing
	|___________________________________________________________________*/

	font_data = font_data_rom8x8;
	font_size = sizeof(font_data_rom8x8);

	// Start graphics mode                                      
	num_pages = gxStartGraphics(resolution, bitdepth, stencildepth, MAX_VRAM_PAGES, GRAPHICS_DRIVER);
	if (num_pages == MAX_VRAM_PAGES) {
		// Init system, drawing fonts 
		Pgm_system_font = gxLoadFontData(gxFONT_TYPE_GX, font_data, font_size);
		// Make system font the default drawing font 
		gxSetFont(Pgm_system_font);

		// Start event processing
		evStartEvents(evTYPE_MOUSE_LEFT_PRESS | evTYPE_MOUSE_RIGHT_PRESS |
			evTYPE_MOUSE_LEFT_RELEASE | evTYPE_MOUSE_RIGHT_RELEASE |
			evTYPE_MOUSE_WHEEL_BACKWARD | evTYPE_MOUSE_WHEEL_FORWARD |
			//                   evTYPE_KEY_PRESS | 
			evTYPE_RAW_KEY_PRESS | evTYPE_RAW_KEY_RELEASE,
			AUTO_TRACKING, EVENT_DRIVER);
		*generate_keypress_events = FALSE;  // true if using evTYPE_KEY_PRESS in the above mask

			// Set a custom mouse cursor
		Set_Mouse_Cursor();

		// Set globals
		Pgm_num_pages = num_pages;
	}

	return (Pgm_num_pages);
}

/*____________________________________________________________________
|
| Function: Set_Mouse_Cursor
|
| Input: Called from Init_Graphics()
| Output: Sets default mouse cursor.
|___________________________________________________________________*/

static void Set_Mouse_Cursor()
{
	gxColor fc, bc;

	// Set cursor to a medium sized red arrow
	fc.r = 255;
	fc.g = 0;
	fc.b = 0;
	fc.a = 0;
	bc.r = 1;
	bc.g = 0;
	bc.b = 0;
	bc.a = 0;
	msSetCursor(msCURSOR_MEDIUM_ARROW, fc, bc);
}

/*____________________________________________________________________
|
| Function: reset_Position
|
| Input: Array of monster x and z coordinates
| Output: Resets monster x and z coordinates to outer edge of game area, while also making sure not to reset a monster right next to the player.
|___________________________________________________________________*/

void reset_Position(float monster_x[3][25], float monster_z[3][25], int i, int j, int hitTracker[3][25], gx3dVector position)
{
	float x, z, dist;
	monster_x[i][j] = (rand() % 1500) - 750;
	monster_z[i][j] = (rand() % 1500) - 750;
	hitTracker[i][j] = 0;
	x = (monster_x[i][j] - position.x);
	z = (monster_z[i][j] - position.z);
	dist = (sqrt((x * x) + (z * z)));
	if (dist < 250)
		reset_Position(monster_x, monster_z, i, j, hitTracker, position);
}

/*____________________________________________________________________
|
| Function: Program_Run
|
| Input: Called from Program_Thread()
| Output: Runs program in the current video mode.  Begins with mouse
|   hidden.
|___________________________________________________________________*/

void Program_Run()
{
	// Important constants
	const int MAX_SHOOT = 11;
	const int MAX_HIT_SOUND = 15;
	const int MAX_TREES = 150;
	const int MAX_FLOWERS = 400;
	const int MAX_MONSTERS = 25;
	const int MAX_HIT = 25;
	const int MAX_EVENTS = 3;
	const int MONSTER_TYPES = 3;
	const float MAX_HEALTH = 3000.0;

	unsigned elapsed_time, new_time;
	bool quit = false;
	float health = MAX_HEALTH;
	int health_percentage = 0;
	int deadmonsters = 0;
	int score = 0;
	int tree_x[MAX_TREES];
	int tree_z[MAX_TREES];
	int flower_x[MAX_FLOWERS];
	int flower_z[MAX_FLOWERS];
	float monster_x[MONSTER_TYPES][MAX_MONSTERS];
	float monster_z[MONSTER_TYPES][MAX_MONSTERS];
	float monster_speed[MONSTER_TYPES][MAX_MONSTERS];
	bool monsters_onscreen[MONSTER_TYPES][MAX_MONSTERS];
	int hitTimer[MAX_HIT];
	int hitIndex = 0;
	int hitTracker[MONSTER_TYPES][MAX_MONSTERS];
	bool fastMovement = false;
	bool start = true;
	bool instructions = false;
	bool game_over = false;
	bool game_over2 = false;
	bool victory = false;
	bool victory2 = false;
	int event_location_x[MAX_EVENTS];
	int event_location_y[MAX_EVENTS];
	int event_location_z[MAX_EVENTS];
	float first_aid_x[MAX_EVENTS];
	float first_aid_z[MAX_EVENTS];
	int timer = 0;
	int count = 0;

	gx3dVector hitPosition[MAX_HIT];

	gxRelation monster_relations[MONSTER_TYPES][MAX_MONSTERS];
	gxRelation flower_relations[MAX_FLOWERS];
	gxRelation tree_relations[MAX_TREES];
	gxRelation firstaid_relations[MAX_EVENTS];

	gx3dBox box[MAX_TREES];
	gx3dSphere monster_spheres[MONSTER_TYPES][MAX_MONSTERS];
	gx3dSphere flower_spheres[MAX_FLOWERS];
	gx3dSphere firstaid_spheres[MAX_EVENTS];

	evEvent event;
	gx3dDriverInfo dinfo;
	gxColor color, color_yellow, color_red, color_green, color_black;
	char str[256];

	color_yellow.r = 255;
	color_yellow.g = 225;
	color_yellow.b = 33;
	color_yellow.a = 1;
	color_red.r = 128;
	color_red.g = 17;
	color_red.b = 42;
	color_red.a = 1;
	color_green.r = 9;
	color_green.g = 140;
	color_green.b = 27;
	color_green.a = 1;
	color_black .r = 0;
	color_black.g = 0;
	color_black.b = 0;
	color_black.a = 1;

	gx3dObject* obj_tree, * obj_tree2, * obj_clouddome, * obj_ghost;
	gx3dMatrix m, m1, m2, m3, m4, m5;
	gx3dColor color3d_white = { 1, 1, 1, 0 };
	gx3dColor color3d_dim = { 0.1f, 0.1f, 0.1f };
	gx3dColor color3d_black = { 0, 0, 0, 0 };
	gx3dColor color3d_darkgray = { 0.3f, 0.3f, 0.3f, 0 };
	gx3dColor color3d_gray = { 0.5f, 0.5f, 0.5f, 0 };
	gx3dMaterialData material_default = {
	  { 1, 1, 1, 1 }, // ambient color
	  { 1, 1, 1, 1 }, // diffuse color
	  { 1, 1, 1, 1 }, // specular color
	  { 0, 0, 0, 0 }, // emissive color
	  10              // specular sharpness (0=disabled, 0.01=sharp, 10=diffused)
	};

	/*____________________________________________________________________
	|
	| Print info about graphics driver to debug file.
	|___________________________________________________________________*/

	gx3d_GetDriverInfo(&dinfo);
	debug_WriteFile("_______________ Device Info ______________");
	sprintf(str, "max texture size: %dx%d", dinfo.max_texture_dx, dinfo.max_texture_dy);
	debug_WriteFile(str);
	sprintf(str, "max active lights: %d", dinfo.max_active_lights);
	debug_WriteFile(str);
	sprintf(str, "max user clip planes: %d", dinfo.max_user_clip_planes);
	debug_WriteFile(str);
	sprintf(str, "max simultaneous texture stages: %d", dinfo.max_simultaneous_texture_stages);
	debug_WriteFile(str);
	sprintf(str, "max texture stages: %d", dinfo.max_texture_stages);
	debug_WriteFile(str);
	sprintf(str, "max texture repeat: %d", dinfo.max_texture_repeat);
	debug_WriteFile(str);
	debug_WriteFile("__________________________________________");

	/*____________________________________________________________________
	|
	| Initialize the sound library
	|___________________________________________________________________*/

	snd_Init(22, 16, 2, 1, 1);
	snd_SetListenerDistanceFactorToFeet(snd_3D_APPLY_NOW);
	Sound s_walk, s_run, s_ambience, s_zombie1[MAX_MONSTERS], s_zombie2[MAX_MONSTERS], s_zombie3[MAX_MONSTERS], s_shoot[MAX_SHOOT], s_hit[MAX_HIT_SOUND], s_collect, s_cough,
		s_start, s_game_over, s_victory;
	s_ambience = snd_LoadSound("wav\\ambience.wav", snd_CONTROL_VOLUME, 0);
	s_walk = snd_LoadSound("wav\\walk.wav", snd_CONTROL_VOLUME, 0);
	s_run = snd_LoadSound("wav\\run.wav", snd_CONTROL_VOLUME, 0);
	s_start = snd_LoadSound("wav\\title.wav", snd_CONTROL_VOLUME, 0);
	s_game_over = snd_LoadSound("wav\\gameover.wav", snd_CONTROL_VOLUME, 0);
	s_victory = snd_LoadSound("wav\\win.wav", snd_CONTROL_VOLUME, 0);
	// Shoot
	for (int i = 0; i < MAX_SHOOT; i++) {
		s_shoot[i] = snd_LoadSound("wav\\shoot.wav", snd_CONTROL_VOLUME, 0);
		snd_SetSoundVolume(s_shoot[i], 75);
	}
	// Hit
	for (int i = 0; i < MAX_HIT_SOUND; i++) {
		s_hit[i] = snd_LoadSound("wav\\gun_hit.wav", snd_CONTROL_VOLUME, 0);
		snd_SetSoundVolume(s_hit[i], 75);
	}
	// Collect
	s_collect = snd_LoadSound("wav\\collect.wav", snd_CONTROL_VOLUME, 0);
	// Cough
	s_cough = snd_LoadSound("wav\\cough.wav", snd_CONTROL_VOLUME, 0);
	// Monsters
	for (int i = 0; i < MONSTER_TYPES; i++) {
		for (int j = 0; j < MAX_MONSTERS; j++) {
			if (i == 0) {
				s_zombie1[j] = snd_LoadSound("wav\\zombie1.wav", snd_CONTROL_3D, 0);
				snd_SetSoundMode(s_zombie1[j], snd_3D_MODE_ORIGIN_RELATIVE, snd_3D_APPLY_NOW);
				snd_SetSoundMinDistance(s_zombie1[j], 5, snd_3D_APPLY_NOW);
				snd_SetSoundMaxDistance(s_zombie1[j], 75, snd_3D_APPLY_NOW);
			}
			else if (i == 1) {
				s_zombie2[j] = snd_LoadSound("wav\\zombie1.wav", snd_CONTROL_3D, 0);
				snd_SetSoundMode(s_zombie2[j], snd_3D_MODE_ORIGIN_RELATIVE, snd_3D_APPLY_NOW);
				snd_SetSoundMinDistance(s_zombie2[j], 5, snd_3D_APPLY_NOW);
				snd_SetSoundMaxDistance(s_zombie2[j], 75, snd_3D_APPLY_NOW);
			}
			else if (i == 2) {
				s_zombie3[j] = snd_LoadSound("wav\\zombie2.wav", snd_CONTROL_3D, 0);
				snd_SetSoundMode(s_zombie3[j], snd_3D_MODE_ORIGIN_RELATIVE, snd_3D_APPLY_NOW);
				snd_SetSoundMinDistance(s_zombie3[j], 5, snd_3D_APPLY_NOW);
				snd_SetSoundMaxDistance(s_zombie3[j], 75, snd_3D_APPLY_NOW);
			}
		}
	}
	// Set volumes
	snd_SetSoundVolume(s_walk, 55);
	snd_SetSoundVolume(s_run, 65);
	snd_SetSoundVolume(s_cough, 80);
	snd_SetSoundVolume(s_collect, 95);
	snd_SetSoundVolume(s_ambience, 75);
	snd_SetSoundVolume(s_start, 65);

	/*____________________________________________________________________
	|
	| Initialize the graphics state
	|___________________________________________________________________*/

	// Set 2d graphics state
	Pgm_screen.xleft = 0;
	Pgm_screen.ytop = 0;
	Pgm_screen.xright = gxGetScreenWidth() - 1;
	Pgm_screen.ybottom = gxGetScreenHeight() - 1;
	gxSetWindow(&Pgm_screen);
	gxSetClip(&Pgm_screen);
	gxSetClipping(FALSE);

	Health_Bar_Border.xleft = 100;
	Health_Bar_Border.ybottom = gxGetScreenHeight() - 100;
	Health_Bar_Border.xright = 600;
	Health_Bar_Border.ytop = gxGetScreenHeight() - 150;

	Health_Bar_Fill.xleft = 100;
	Health_Bar_Fill.ybottom = gxGetScreenHeight() - 100;
	Health_Bar_Fill.xright = 600;
	Health_Bar_Fill.ytop = gxGetScreenHeight() - 150;

	// Set the 3D viewport
	gx3d_SetViewport(&Pgm_screen);
	// Init other 3D stuff
	Init_Render_State();

	/*____________________________________________________________________
	|
	| Init support routines
	|___________________________________________________________________*/

	gx3dVector heading, position;

	// Set starting camera position
	position.x = 0;
	position.y = 6;
	position.z = -20;
	// Set starting camera view direction (heading)
	heading.x = 0;  // {0,0,1} for cubic environment mapping to work correctly
	heading.y = 0;
	heading.z = 1;
	Position_Init(&position, &heading, RUN_SPEED);

	/*____________________________________________________________________
	|
	| Init 3D graphics
	|___________________________________________________________________*/

	// Set projection matrix
	float fov = 75; // degrees field of view
	float near_plane = 0.1f;
	float far_plane = 2750;
	gx3d_SetProjectionMatrix(fov, near_plane, far_plane);

	gx3d_SetFillMode(gx3d_FILL_MODE_GOURAUD_SHADED);

	// Clear the 3D viewport to all black
	color.r = 0;
	color.g = 0;
	color.b = 0;
	color.a = 0;

	/*____________________________________________________________________
	|
	| Create particle systems
	|___________________________________________________________________*/

	gx3dParticleSystem psys_fire[MAX_EVENTS];
	for (int i = 0; i < MAX_EVENTS; i++) {
		psys_fire[i] = Script_ParticleSystem_Create("fire.gxps");
	}
	gx3dParticleSystem psys_poison = Script_ParticleSystem_Create("poison.gxps");

	/*____________________________________________________________________
	|
	| Load 3D models
	|___________________________________________________________________*/

	// Trees
	gx3d_ReadLWO2File("Objects\\ptree6.lwo", &obj_tree, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_tree = gx3d_InitTexture_File("Objects\\Images\\ptree_d512.bmp", "Objects\\Images\\ptree_d512_fa.bmp", 0);

	// Flowers
	gx3dObject* obj_flower;
	gx3d_ReadLWO2File("Objects\\flower2.lwo", &obj_flower, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_flower = gx3d_InitTexture_File("Objects\\Images\\flower.bmp", "Objects\\Images\\flower_fa.bmp", 0);

	// Monsters
	gx3dObject* obj_monster[MONSTER_TYPES];
	gx3dTexture tex_monster[MONSTER_TYPES];
	gx3d_ReadLWO2File("Objects\\monster1.lwo", &obj_monster[0], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_monster[0] = gx3d_InitTexture_File("Objects\\Images\\zombie.bmp", "Objects\\Images\\zombie_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\monster2.lwo", &obj_monster[1], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_monster[1] = gx3d_InitTexture_File("Objects\\Images\\zombie2.bmp", "Objects\\Images\\zombie_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\monster3.lwo", &obj_monster[2], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_monster[2] = gx3d_InitTexture_File("Objects\\Images\\crawler.bmp", "Objects\\Images\\crawler_fa.bmp", 0);

	// Ground
	gx3dObject* obj_ground;
	gx3d_ReadLWO2File("Objects\\ground2.lwo", &obj_ground, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_ground = gx3d_InitTexture_File("Objects\\Images\\ground2.bmp", 0, 0);

	// Skydome
	gx3dObject* obj_skydome;
	gx3d_ReadLWO2File("Objects\\sky.lwo", &obj_skydome, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_skydome = gx3d_InitTexture_File("Objects\\Images\\sky.bmp", 0, 0);

	// Hit
	gx3dObject* obj_hit;
	gx3d_ReadLWO2File("Objects\\hit.lwo", &obj_hit, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_hit = gx3d_InitTexture_File("Objects\\Images\\hit.bmp", "Objects\\Images\\hit2.bmp", 0);

	// Kills
	gx3dObject* obj_kills;
	gx3d_ReadLWO2File("Objects\\kills_billboard.lwo", &obj_kills, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_kills = gx3d_InitTexture_File("Objects\\Images\\kills.bmp", "Objects\\Images\\kills_fa.bmp", 0);

	// First Aid
	gx3dObject* obj_firstaid;
	gx3d_ReadLWO2File("Objects\\firstaid.lwo", &obj_firstaid, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_firstaid = gx3d_InitTexture_File("Objects\\Images\\firstaid.bmp", "Objects\\Images\\firstaid_fa.bmp", 0);

	// Crosshair
	gx3dObject* obj_crosshair;
	gx3d_ReadLWO2File("Objects\\crosshair.lwo", &obj_crosshair, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_crosshair = gx3d_InitTexture_File("Objects\\Images\\crosshair.bmp", "Objects\\Images\\crosshair_fa.bmp", 0);

	// Start
	gx3dObject* obj_start;
	gx3d_ReadLWO2File("Objects\\title.lwo", &obj_start, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_start = gx3d_InitTexture_File("Objects\\Images\\title.bmp", 0, 0);

	// Instructions
	gx3dObject* obj_instructions;
	gx3d_ReadLWO2File("Objects\\instructions.lwo", &obj_instructions, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_instructions = gx3d_InitTexture_File("Objects\\Images\\instructions.bmp", 0, 0);

	// Game Over
	gx3dObject* obj_game_over;
	gx3d_ReadLWO2File("Objects\\gameover.lwo", &obj_game_over, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_game_over = gx3d_InitTexture_File("Objects\\Images\\gameover.bmp", 0, 0);

	// Victory
	gx3dObject* obj_victory;
	gx3d_ReadLWO2File("Objects\\victory.lwo", &obj_victory, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3dTexture tex_victory = gx3d_InitTexture_File("Objects\\Images\\victory.bmp", 0, 0);

	// Numbers
	gx3dObject* obj_numbers[10];
	gx3dTexture tex_num[10];
	gx3d_ReadLWO2File("Objects\\Numbers\\0.lwo", &obj_numbers[0], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[0] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\0.bmp", "Objects\\Images\\Numbers\\0_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\1.lwo", &obj_numbers[1], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[1] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\1.bmp", "Objects\\Images\\Numbers\\1_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\2.lwo", &obj_numbers[2], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[2] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\2.bmp", "Objects\\Images\\Numbers\\2_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\3.lwo", &obj_numbers[3], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[3] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\3.bmp", "Objects\\Images\\Numbers\\3_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\4.lwo", &obj_numbers[4], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[4] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\4.bmp", "Objects\\Images\\Numbers\\4_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\5.lwo", &obj_numbers[5], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[5] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\5.bmp", "Objects\\Images\\Numbers\\5_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\6.lwo", &obj_numbers[6], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[6] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\6.bmp", "Objects\\Images\\Numbers\\6_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\7.lwo", &obj_numbers[7], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[7] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\7.bmp", "Objects\\Images\\Numbers\\7_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\8.lwo", &obj_numbers[8], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[8] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\8.bmp", "Objects\\Images\\Numbers\\8_fa.bmp", 0);
	gx3d_ReadLWO2File("Objects\\Numbers\\9.lwo", &obj_numbers[9], gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	tex_num[9] = gx3d_InitTexture_File("Objects\\Images\\Numbers\\9.bmp", "Objects\\Images\\Numbers\\9_fa.bmp", 0);

	/*____________________________________________________________________
	|
	| create lights
	|___________________________________________________________________*/

	gx3dLightData event_light_data[MAX_EVENTS];
	gx3dLight event_light[MAX_EVENTS];
	gx3dLight player_light;
	gx3dLightData player_light_data;
	player_light_data.light_type = gx3d_LIGHT_TYPE_POINT;
	player_light_data.point.diffuse_color.r = 1;
	player_light_data.point.diffuse_color.g = 0;
	player_light_data.point.diffuse_color.b = 0;
	player_light_data.point.diffuse_color.a = 0;
	player_light_data.point.specular_color.r = 1;
	player_light_data.point.specular_color.g = 0;
	player_light_data.point.specular_color.b = 0;
	player_light_data.point.specular_color.a = 0;
	player_light_data.point.ambient_color.r = 1;
	player_light_data.point.ambient_color.g = 1;
	player_light_data.point.ambient_color.b = 1;
	player_light_data.point.ambient_color.a = 0;
	player_light_data.point.src = position;
	player_light_data.point.range = 200;
	player_light_data.point.constant_attenuation = 0;
	player_light_data.point.linear_attenuation = 0.1;
	player_light_data.point.quadratic_attenuation = 0;
	player_light = gx3d_InitLight(&player_light_data);
	for (int i = 0; i < MAX_EVENTS; i++) {
		event_light_data[i].light_type = gx3d_LIGHT_TYPE_POINT;
		event_light_data[i].point.diffuse_color.r = 1;
		event_light_data[i].point.diffuse_color.g = 0.68;
		event_light_data[i].point.diffuse_color.b = 0.25;
		event_light_data[i].point.diffuse_color.a = 0;
		event_light_data[i].point.specular_color.r = 1;
		event_light_data[i].point.specular_color.g = 0.68;
		event_light_data[i].point.specular_color.b = 0.25;
		event_light_data[i].point.specular_color.a = 0;
		event_light_data[i].point.ambient_color.r = 1;
		event_light_data[i].point.ambient_color.g = 1;
		event_light_data[i].point.ambient_color.b = 1;
		event_light_data[i].point.ambient_color.a = 0;
		event_light_data[i].point.src.x = 0;
		event_light_data[i].point.src.y = 0;
		event_light_data[i].point.src.z = 0;
		event_light_data[i].point.range = 75;
		event_light_data[i].point.constant_attenuation = 0;
		event_light_data[i].point.linear_attenuation = 0.1;
		event_light_data[i].point.quadratic_attenuation = 0;
		event_light[i] = gx3d_InitLight(&event_light_data[i]);
	}

	/*____________________________________________________________________
	|
	| Flush input queue
	|___________________________________________________________________*/

	int move_x, move_y;	// mouse movement counters

	// Flush input queue
	evFlushEvents();
	// Zero mouse movement counters
	msGetMouseMovement(&move_x, &move_y);  // call this here so the next call will get movement that has occurred since it was called here                                    
	  // Hide mouse cursor
	msHideMouse();

	/*____________________________________________________________________
	|
	| Main game loop
	|___________________________________________________________________*/

	unsigned last_time = 0;
	bool force_update = false;
	unsigned cmd_move = 0;
	bool draw_wireframe = false;
	bool first_aid_collected[MAX_EVENTS] = { false, false, false };
	health = MAX_HEALTH;
	int counter = 0;
	int shot_counter = 0;
	int hit_counter = 0;

	// Create events coordinates at random and place lights and first aids at them
	for (int i = 0; i < MAX_EVENTS; i++) {
		// events
		event_location_x[i] = (rand() % 1500) - 750;
		event_location_y[i] = 0;
		event_location_z[i] = (rand() % 1500) - 750;
		// lights
		event_light_data[i].point.src.x = event_location_x[i];
		event_light_data[i].point.src.y = event_location_y[i];
		event_light_data[i].point.src.z = event_location_z[i];
		gx3d_UpdateLight(event_light[i], &event_light_data[i]);
		// first aids
		first_aid_x[i] = event_location_x[i] + 5;
		first_aid_z[i] = event_location_z[i] + 5;
	}

	// Randomly set coordinates of trees/flowers/monsters
	// trees
	for (int i = 0; i < MAX_TREES; i++) {
		tree_x[i] = (rand() % 1500) - 750;
		tree_z[i] = (rand() % 1500) - 750;
	}
	// flowers
	for (int i = 0; i < MAX_FLOWERS; i++) {
		flower_x[i] = (rand() % 1500) - 750;
		flower_z[i] = (rand() % 1500) - 750;
	}
	// monsters
	for (int i = 0; i < MONSTER_TYPES; i++) {
		for (int j = 0; j < MAX_MONSTERS; j++) {
			if (i == 0)
				monster_speed[i][j] = 0.15;
			else if (i == 1)
				monster_speed[i][j] = 0.25;
			else
				monster_speed[i][j] = 0.35;
			monster_x[i][j] = (rand() % 1500) - 750;
			monster_z[i][j] = (rand() % 1500) - 750;
			hitTracker[i][j] = 0;
		}
	}

	// Begin the game
	while (quit != true) {

		if (!snd_IsPlaying(s_ambience) && !start && !game_over && !victory)
			snd_PlaySound(s_ambience, 0);
		if (!start && snd_IsPlaying(s_start))
			snd_StopSound(s_start);
			

		// End game loop if win/loss conditions are met.
		if (health <= 0) {
			game_over = true;
		}
		if (first_aid_collected[0] == true && first_aid_collected[1] == true && first_aid_collected[2] == true)
				victory = true;
	

		/*____________________________________________________________________
		|
		| Update clock
		|___________________________________________________________________*/

		// Get the current time (# milliseconds since the program started)
		new_time = timeGetTime();
		// Compute the elapsed time (in milliseconds) since the last time through this loop
		if (last_time == 0)
			elapsed_time = 0;
		else
			elapsed_time = new_time - last_time;
		last_time = new_time;

		/*____________________________________________________________________
		|
		| Update counter used for cough frequency
		|___________________________________________________________________*/
		if(counter < 10000)
			counter += elapsed_time;
		if (counter >= (health * 2) && health < MAX_HEALTH && !victory && !game_over) {
			if (!snd_IsPlaying(s_cough))
				snd_PlaySound(s_cough, 0);
			counter = 0;
		}

		/*____________________________________________________________________
		|
		| Process user input
		|___________________________________________________________________*/

		if (evGetEvent(&event)) {
			// key press
			if (event.type == evTYPE_RAW_KEY_PRESS) {
				// If ESC pressed, exit the program
				if (event.keycode == evKY_ESC)
					quit = true;
				else if (event.keycode == 'w' && !start && !game_over && !victory)
					cmd_move |= POSITION_MOVE_FORWARD;
				else if (event.keycode == 's' && !start && !game_over && !victory)
					cmd_move |= POSITION_MOVE_BACK;
				else if (event.keycode == 'a' && !start && !game_over && !victory)
					cmd_move |= POSITION_MOVE_LEFT;
				else if (event.keycode == 'd' && !start && !game_over && !victory)
					cmd_move |= POSITION_MOVE_RIGHT;
				else if (event.keycode == evKY_SHIFT && !start && !game_over && !victory)
					fastMovement = true;
				else if (event.keycode == evKY_TAB) {
					if (instructions == true)
						instructions = false;
					else
						instructions = true;
				}
				else if (event.keycode == evKY_ENTER && start == true)
					start = false;
				else if (event.keycode == evKY_F12)
					victory = true;
			}
			// key release
			else if (event.type == evTYPE_RAW_KEY_RELEASE && !start && !game_over && !victory) {
				if (event.keycode == 'w')
					cmd_move &= ~(POSITION_MOVE_FORWARD);
				else if (event.keycode == 's')
					cmd_move &= ~(POSITION_MOVE_BACK);
				else if (event.keycode == 'a')
					cmd_move &= ~(POSITION_MOVE_LEFT);
				else if (event.keycode == 'd')
					cmd_move &= ~(POSITION_MOVE_RIGHT);
				else if (event.keycode == evKY_SHIFT)
					fastMovement = false;
			}
			// left click
			else if (event.type == evTYPE_MOUSE_LEFT_PRESS && !start && !game_over && !victory) {
				snd_PlaySound(s_shoot[shot_counter], 0);
				shot_counter++;
				if (shot_counter >= 11)
					shot_counter = 0;
				gx3dRay viewVector;
				viewVector.origin = position;
				viewVector.direction = heading;
				for (int i = 0; i < MONSTER_TYPES; i++) {
					for (int j = 0; j < MAX_MONSTERS; j++) {
						if (monsters_onscreen[i][j]) {
							gxRelation rel = gx3d_Relation_Ray_Sphere(&viewVector, &monster_spheres[i][j]);
							if (rel != gxRELATION_OUTSIDE) {

								snd_PlaySound(s_hit[hit_counter], 0);
								hit_counter++;
								if (hit_counter >= 15)
									hit_counter = 0;

								// increases the hit tracker
								hitTracker[i][j]++;

								// Create a new hit
								hitPosition[hitIndex] = monster_spheres[i][j].center;
								hitTimer[hitIndex] = 1000 + elapsed_time;
								hitIndex = (hitIndex + 1) % MAX_HIT;

								// checks to see if the monster has been hit 3 times, if so then increment deadmonsters and then reset position of the monster
								if (hitTracker[i][j] >= 3) {
									deadmonsters++;
									reset_Position(monster_x, monster_z, i, j, hitTracker, position);
								}
							}
						}
					}
				}
			}
			// walking sound if walking
			if (cmd_move != 0 && fastMovement == false) {
				if (!snd_IsPlaying(s_walk))
					snd_PlaySound(s_walk, 1);
				if (snd_IsPlaying(s_run))
					snd_StopSound(s_run);
			}
			// running sound if running
			else if (cmd_move != 0 && fastMovement == true) {
				if (!snd_IsPlaying(s_run))
					snd_PlaySound(s_run, 1);
			}
			// else stop sound
			else {
				if (snd_IsPlaying(s_walk))
					snd_StopSound(s_walk);
				if (snd_IsPlaying(s_run))
					snd_StopSound(s_run);
			}
		}
		// Check for camera movement (via mouse)
		msGetMouseMovement(&move_x, &move_y);

		/*____________________________________________________________________
		|
		| Check for collected first aids
		|___________________________________________________________________*/

		if (cmd_move != 0) {
			for (int i = 0; i < MAX_EVENTS; i++) {
				float x = (first_aid_x[i] - position.x);
				float z = (first_aid_z[i] - position.z);
				float collect_dist = (sqrt((x * x) + (z * z)));
				if (collect_dist < 10) {
					snd_PlaySound(s_collect, 0);
					first_aid_collected[i] = true;
					first_aid_x[i] = 2000.0;
					first_aid_z[i] = 2000.0;
					if (health <= 2500)
						health += 500;
					else
						health = 3000;
				}
			}
		}

		/*____________________________________________________________________
		|
		| Update camera view
		|___________________________________________________________________*/

		if (fastMovement)
			Position_Set_Speed(RUN_SPEED * 3);
		else
			Position_Set_Speed(RUN_SPEED);

		bool position_changed, camera_changed;
		Position_Update(elapsed_time, cmd_move, move_y, move_x, force_update,
			&position_changed, &camera_changed, &position, &heading);
		snd_SetListenerPosition(position.x, position.y, position.z, snd_3D_APPLY_NOW);
		snd_SetListenerOrientation(heading.x, heading.y, heading.z, 0, 1, 0, snd_3D_APPLY_NOW);

		/*____________________________________________________________________
		|
		| Update player light position as player moves
		|___________________________________________________________________*/

		player_light_data.point.src = position;
		gx3d_UpdateLight(player_light, &player_light_data);

		/*____________________________________________________________________
		|
		| Draw 3D graphics
		|___________________________________________________________________*/

		// Render the screen
		gx3d_ClearViewport(gx3d_CLEAR_SURFACE | gx3d_CLEAR_ZBUFFER, color, gx3d_MAX_ZBUFFER_VALUE, 0);
		// Start rendering in 3D           
		if (gx3d_BeginRender()) {
			// Set the default material
			gx3d_SetMaterial(&material_default);

			if (!start && !game_over && !victory) {

				gx3d_SetAmbientLight(color3d_dim);
				gx3d_EnableLight(player_light);
				gx3d_EnableLight(event_light[0]);
				gx3d_EnableLight(event_light[1]);
				gx3d_EnableLight(event_light[2]);

				// Draw skydome
				gx3d_GetTranslateMatrix(&m, 0, -1, 0);
				gx3d_SetObjectMatrix(obj_skydome, &m);
				gx3d_SetTexture(0, tex_skydome);
				gx3d_DrawObject(obj_skydome, 0);

				// Set ambient light black to make the models drawn dark
				gx3d_SetAmbientLight(color3d_black);

				// Draw ground 
				gx3d_GetTranslateMatrix(&m, 0, 0, 0);
				gx3d_SetObjectMatrix(obj_ground, &m);
				gx3d_SetTexture(0, tex_ground);
				gx3d_DrawObject(obj_ground, 0);

				// Enable alpha blending for the rest of the models being drawn
				gx3d_EnableAlphaBlending();
				gx3d_EnableAlphaTesting(128);


				// Draw flowers
				for (int i = 0; i < MAX_FLOWERS; i++) {
					flower_spheres[i] = obj_flower->bound_sphere;
					flower_spheres[i].center.x = flower_x[i];
					flower_spheres[i].center.z = flower_z[i];
					flower_relations[i] = gx3d_Relation_Sphere_Frustum(&flower_spheres[i]);
					if (flower_relations[i] != gxRELATION_OUTSIDE) {
						gx3d_GetTranslateMatrix(&m, flower_x[i], 0, flower_z[i]);
						gx3d_SetObjectMatrix(obj_flower, &m);
						gx3d_SetTexture(0, tex_flower);
						gx3d_DrawObject(obj_flower, 0);
					}
				}

				// Draw trees
				for (int i = 0; i < MAX_TREES; i++) {
					box[i] = obj_tree->bound_box;
					gx3d_GetTranslateMatrix(&m, tree_x[i], 0, tree_z[i]);
					tree_relations[i] = gx3d_Relation_Box_Frustum(&box[i], &m);
					if (tree_relations[i] != gxRELATION_OUTSIDE) {
						gx3d_GetTranslateMatrix(&m, tree_x[i], 0, tree_z[i]);
						gx3d_SetObjectMatrix(obj_tree, &m);
						gx3d_SetTexture(0, tex_tree);
						gx3d_DrawObject(obj_tree, 0);
					}
				}

				// Assign monsters new x and z coordinates before drawing them
				for (int i = 0; i < MONSTER_TYPES; i++) {
					for (int j = 0; j < MAX_MONSTERS; j++) {

						// checks if the monster should go after player instead of moving toward center
						float x = monster_x[i][j] - position.x;
						float z = monster_z[i][j] - position.z;
						float x2 = 0;
						float z2 = 0;
						float dist = (sqrt((x * x) + (z * z)));

						// moves toward player if close or has been shot by player
						if (dist < 150 || hitTracker[i][j] > 0) {
							if (dist > 10) {
								if (monster_x[i][j] < position.x)
									monster_x[i][j] += monster_speed[i][j];
								else if (monster_x[i][j] > position.x)
									monster_x[i][j] -= monster_speed[i][j];
								if (monster_z[i][j] < position.z)
									monster_z[i][j] += monster_speed[i][j];
								else if (monster_z[i][j] > position.z)
									monster_z[i][j] -= monster_speed[i][j];
							}
							// deals damage if monster is within 25 of player
							if (dist < 15) {
								health -= elapsed_time;
							}
							if (dist < 75) {
								if (i == 0) {
									if (!snd_IsPlaying(s_zombie1[j]))
										snd_PlaySound(s_zombie1[j], 0);
								}
								else if (i == 1) {
									if (!snd_IsPlaying(s_zombie2[j]))
										snd_PlaySound(s_zombie2[j], 0);
								}
								else if (i == 2) {
									if (!snd_IsPlaying(s_zombie3[j]))
										snd_PlaySound(s_zombie3[j], 0);
								}
							}
							continue;
						}

						// otherwise move toward assigned events (monster1 to event1, monster2 to event2, etc..)
						// if monster reaches assigned events, relocate it to a random coordinate
						x = monster_x[i][j] - event_location_x[i];
						z = monster_z[i][j] - event_location_z[i];
						dist = (sqrt((x * x) + (z * z)));
						x2 = (event_location_x[i] - monster_x[i][j]) / dist;
						z2 = (event_location_z[i] - monster_z[i][j]) / dist;
						monster_x[i][j] += x2 * 25 * 0.01f;
						monster_z[i][j] += z2 * 25 * 0.01f;
						if (dist < 5)
							reset_Position(monster_x, monster_z, i, j, hitTracker, position);
					}
				}

				// setting position of monster sounds to update to new monster positions
				for (int i = 0; i < MONSTER_TYPES; i++) {
					for (int j = 0; j < MAX_MONSTERS; j++) {
						if (i == 0) {
							snd_SetSoundPosition(s_zombie1[j], monster_x[i][j], 5, monster_z[i][j], snd_3D_APPLY_NOW);
						}
						else if (i == 1) {
							snd_SetSoundPosition(s_zombie2[j], monster_x[i][j], 5, monster_z[i][j], snd_3D_APPLY_NOW);
						}
						else if (i == 2) {
							snd_SetSoundPosition(s_zombie3[j], monster_x[i][j], 5, monster_z[i][j], snd_3D_APPLY_NOW);
						}
					}
				}

				// draw monsters along with particle effect
				gx3dVector billboard_normal = { 0,0,1 };
				for (int i = 0; i < MONSTER_TYPES; i++) {
					for (int j = 0; j < MAX_MONSTERS; j++) {
						monster_spheres[i][j] = obj_monster[i]->bound_sphere;
						monster_spheres[i][j].center.x = monster_x[i][j];
						monster_spheres[i][j].center.z = monster_z[i][j];
						monster_relations[i][j] = gx3d_Relation_Sphere_Frustum(&monster_spheres[i][j]);
						if (monster_relations[i][j] != gxRELATION_OUTSIDE) {
							gx3d_GetBillboardRotateYMatrix(&m1, &billboard_normal, &heading);
							gx3d_GetTranslateMatrix(&m2, monster_x[i][j], 0, monster_z[i][j]);
							gx3d_MultiplyMatrix(&m1, &m2, &m);
							gx3d_SetObjectMatrix(obj_monster[i], &m);
							gx3d_SetTexture(0, tex_monster[i]);
							gx3d_DrawObject(obj_monster[i], 0);
							monsters_onscreen[i][j] = true;
							gx3d_GetTranslateMatrix(&m2, monster_x[i][j], 8, monster_z[i][j]);
							gx3d_SetParticleSystemMatrix(psys_poison, &m2);
							gx3d_UpdateParticleSystem(psys_poison, elapsed_time);
							gx3d_DrawParticleSystem(psys_poison, &heading, draw_wireframe);
						}
					}
				}

				// Process and draw hit markers
				const float HIT_SCALE = 1;
				for (int i = 0; i < MAX_HIT; i++) {
					if (hitTimer[i] > 0)
						hitTimer[i] -= elapsed_time;
				}
				gx3d_SetAmbientLight(color3d_white);
				for (int i = 0; i < MAX_HIT; i++) {
					if (hitTimer[i] > 0) {
						gx3d_GetScaleMatrix(&m1, HIT_SCALE, HIT_SCALE, HIT_SCALE);
						gx3d_GetBillboardRotateYMatrix(&m2, &billboard_normal, &heading);
						float y = hitPosition[i].y + (1 - (hitTimer[i] / 1000.0f)) * (10);
						gx3d_GetTranslateMatrix(&m3, hitPosition[i].x, y + 9, hitPosition[i].z);
						gx3d_MultiplyMatrix(&m1, &m2, &m);
						gx3d_MultiplyMatrix(&m, &m3, &m);
						gx3d_SetObjectMatrix(obj_hit, &m);
						gx3d_SetTexture(0, tex_hit);
						gx3d_DrawObject(obj_hit, 0);
					}
				}


				/*____________________________________________________________________
				|
				| Draw fire particle system
				|___________________________________________________________________*/

				for (int i = 0; i < MAX_EVENTS; i++) {
					gx3d_GetTranslateMatrix(&m, event_location_x[i], event_location_y[i], event_location_z[i]);
					gx3d_SetParticleSystemMatrix(psys_fire[i], &m);
					gx3d_UpdateParticleSystem(psys_fire[i], elapsed_time);
					gx3d_DrawParticleSystem(psys_fire[i], &heading, draw_wireframe);
				}

				/*____________________________________________________________________
				|
				| Draw first aids
				|___________________________________________________________________*/
				for (int i = 0; i < MAX_EVENTS; i++) {
					// If first aid has not been collected, then draw .
					firstaid_spheres[i] = obj_firstaid->bound_sphere;
					firstaid_spheres[i].center.x = first_aid_x[i];
					firstaid_spheres[i].center.z = first_aid_z[i];
					firstaid_relations[i] = gx3d_Relation_Sphere_Frustum(&firstaid_spheres[i]);
					if (firstaid_relations[i] != gxRELATION_OUTSIDE) {
						if (first_aid_collected[i] == false) {
							gx3d_GetBillboardRotateYMatrix(&m1, &billboard_normal, &heading);
							gx3d_GetTranslateMatrix(&m2, event_location_x[i] + 5, event_location_y[i], event_location_z[i] + 5);
							gx3d_MultiplyMatrix(&m1, &m2, &m);
							gx3d_SetObjectMatrix(obj_firstaid, &m);
							gx3d_SetTexture(0, tex_firstaid);
							gx3d_DrawObject(obj_firstaid, 0);
						}
					}
				}
			}

			/*____________________________________________________________________
			|
			| Draw 2D
			|___________________________________________________________________*/

			//save view matrix
			gx3dMatrix view_save;
			gx3d_GetViewMatrix(&view_save);

			// Set new view Matrix
			gx3dVector tfrom = { 0, 0, -1 }, tto = { 0,0,0 }, twup = { 0,1,0 };
			gx3d_CameraSetPosition(&tfrom, &tto, &twup, gx3d_CAMERA_ORIENTATION_LOOKTO_FIXED);
			gx3d_CameraSetViewMatrix();
			//gx3d_DisableZBuffer();

			gx3d_SetAmbientLight(color3d_white);
			// Draw 2d icons
			char kills[3] = { 0,0,0 };
			if (!start && !game_over && !victory) {

				// Score
				sprintf(kills, "%d", score);

				// Health
				if (health < 1000) {
					gxSetColor(color_red);
				}
				else if (health < 2000) {
					gxSetColor(color_yellow);
				}
				else
					gxSetColor(color_green);
				gxDrawRectangle(Health_Bar_Border.xleft, Health_Bar_Border.ytop, Health_Bar_Border.xright, Health_Bar_Border.ybottom);
				float percentage = health / MAX_HEALTH * 100.0;
				if (health > 0) {
					gxDrawFillRectangle(Health_Bar_Fill.xleft, Health_Bar_Fill.ytop, (percentage * 5) + 100, Health_Bar_Fill.ybottom);
				}

				// Crosshair
				gx3d_GetScaleMatrix(&m1, 0.012, 0.012, 0.012);
				gx3d_GetRotateYMatrix(&m2, 180);
				gx3d_GetTranslateMatrix(&m3, 0, 0, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_MultiplyMatrix(&m, &m3, &m);
				gx3d_SetObjectMatrix(obj_crosshair, &m);
				gx3d_SetTexture(0, tex_crosshair);
				gx3d_DrawObject(obj_crosshair, 0);

				// First Digit
				gx3d_GetScaleMatrix(&m1, 0.012, 0.012, 0.012);
				gx3d_GetRotateYMatrix(&m2, 180);
				gx3d_GetTranslateMatrix(&m3, -0.70, 0.35, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_MultiplyMatrix(&m, &m3, &m);
				if (score <= 99) {
					gx3d_SetObjectMatrix(obj_numbers[0], &m);
					gx3d_SetTexture(0, tex_num[0]);
					//gx3d_DrawObject(obj_numbers[0], 0);
				}
				else {
					int digit1 = kills[0] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit1], &m);
					gx3d_SetTexture(0, tex_num[digit1]);
					gx3d_DrawObject(obj_numbers[digit1], 0);
				}

				// Second Digit
				gx3d_GetScaleMatrix(&m1, 0.012, 0.012, 0.012);
				gx3d_GetRotateYMatrix(&m2, 180);
				gx3d_GetTranslateMatrix(&m3, -0.67, 0.35, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_MultiplyMatrix(&m, &m3, &m);
				if (score <= 9) {
					gx3d_SetObjectMatrix(obj_numbers[0], &m);
					gx3d_SetTexture(0, tex_num[0]);
					//gx3d_DrawObject(obj_numbers[0], 0);
				}
				else if (score >= 10 && score >= 100) {
					int digit2 = kills[1] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit2], &m);
					gx3d_SetTexture(0, tex_num[digit2]);
					gx3d_DrawObject(obj_numbers[digit2], 0);
				}
				else {
					int digit2 = kills[0] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit2], &m);
					gx3d_SetTexture(0, tex_num[digit2]);
					gx3d_DrawObject(obj_numbers[digit2], 0);
				}

				// Third Digit
				gx3d_GetScaleMatrix(&m1, 0.012, 0.012, 0.012);
				gx3d_GetRotateYMatrix(&m2, 180);
				gx3d_GetTranslateMatrix(&m3, -0.64, 0.35, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_MultiplyMatrix(&m, &m3, &m);
				if (score <= 0) {
					gx3d_SetObjectMatrix(obj_numbers[0], &m);
					gx3d_SetTexture(0, tex_num[0]);
					gx3d_DrawObject(obj_numbers[0], 0);
				}
				else if (score >= 10 && score <= 99) {
					int digit2 = kills[1] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit2], &m);
					gx3d_SetTexture(0, tex_num[digit2]);
					gx3d_DrawObject(obj_numbers[digit2], 0);
				}
				else if (score >= 0 && score <= 9) {
					int digit3 = kills[0] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit3], &m);
					gx3d_SetTexture(0, tex_num[digit3]);
					gx3d_DrawObject(obj_numbers[digit3], 0);
				}
				else {
					int digit3 = kills[2] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit3], &m);
					gx3d_SetTexture(0, tex_num[digit3]);
					gx3d_DrawObject(obj_numbers[digit3], 0);
				}
			}
			gx3d_DisableAlphaTesting();
			gx3d_DisableAlphaBlending();

			gxSetColor(color_black);
			// draw start screen
			if (start == true) {
				if (!snd_IsPlaying(s_start))
					snd_PlaySound(s_start, 0);
				gxDrawFillRectangle(0, 0, gxGetScreenWidth(), gxGetScreenHeight());
				gx3d_GetTranslateMatrix(&m1, 0, 0, 0);
				gx3d_GetScaleMatrix(&m2, 0.1f, 0.08f, 0.08f);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_SetObjectMatrix(obj_start, &m);
				gx3d_SetTexture(0, tex_start);
				gx3d_DrawObject(obj_start, 0);
			}

			// draw game over screen
			if (game_over) {
				if (!game_over2) {
					game_over2 = true;
					snd_PlaySound(s_game_over, 0);
				}
				if (snd_IsPlaying(s_walk))
					snd_StopSound(s_walk);
				if (snd_IsPlaying(s_run))
					snd_StopSound(s_run);
				gxDrawFillRectangle(0, 0, gxGetScreenWidth(), gxGetScreenHeight());
				gx3d_GetTranslateMatrix(&m1, 0, 0, 0);
				gx3d_GetScaleMatrix(&m2, 0.1f, 0.08f, 0.08f);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_SetObjectMatrix(obj_game_over, &m);
				gx3d_SetTexture(0, tex_game_over);
				gx3d_DrawObject(obj_game_over, 0);
			}

			health_percentage = (health / MAX_HEALTH) * 100;
			timer += elapsed_time;
			if (timer > 1000) {
				count++;
				timer = 0;
			}
			if(count > 1 && !victory)
				score = deadmonsters * 100 / count + health_percentage;
			if (victory) {
				char final_score[3];
				sprintf(final_score, "%d", score);
				snd_StopSound(s_ambience);
				if (!victory2) {
					victory2 = true;
					snd_PlaySound(s_victory, 0);
				}
				if (snd_IsPlaying(s_walk))
					snd_StopSound(s_walk);
				if (snd_IsPlaying(s_run))
					snd_StopSound(s_run);
				gxDrawFillRectangle(0, 0, gxGetScreenWidth(), gxGetScreenHeight());
				gx3d_GetTranslateMatrix(&m1, 0, 0, 0);
				gx3d_GetScaleMatrix(&m2, 0.1f, 0.08f, 0.08f);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_SetObjectMatrix(obj_victory, &m);
				gx3d_SetTexture(0, tex_victory);
				gx3d_DrawObject(obj_victory, 0);

				gx3d_EnableAlphaBlending();
				gx3d_EnableAlphaTesting(128);
				// First Digit
				gx3d_GetScaleMatrix(&m1, 0.012f, 0.012f, 0.012f);
				gx3d_GetRotateYMatrix(&m2, 180);
				gx3d_GetTranslateMatrix(&m3, -0.02, -0.08, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_MultiplyMatrix(&m, &m3, &m);
				if (score <= 99) {
					gx3d_SetObjectMatrix(obj_numbers[0], &m);
					gx3d_SetTexture(0, tex_num[0]);
					//gx3d_DrawObject(obj_numbers[0], 0);
				}
				else {
					int digit1 = final_score[0] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit1], &m);
					gx3d_SetTexture(0, tex_num[digit1]);
					gx3d_DrawObject(obj_numbers[digit1], 0);
				}

				// Second Digit
				gx3d_GetScaleMatrix(&m1, 0.012f, 0.012f, 0.012f);
				gx3d_GetRotateYMatrix(&m2, 180);
				gx3d_GetTranslateMatrix(&m3, 0.01, -0.08, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_MultiplyMatrix(&m, &m3, &m);
				if (score <= 9) {
					gx3d_SetObjectMatrix(obj_numbers[0], &m);
					gx3d_SetTexture(0, tex_num[0]);
					//gx3d_DrawObject(obj_numbers[0], 0);
				}
				else if (score >= 10 && score >= 100) {
					int digit2 = final_score[1] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit2], &m);
					gx3d_SetTexture(0, tex_num[digit2]);
					gx3d_DrawObject(obj_numbers[digit2], 0);
				}
				else {
					int digit2 = final_score[0] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit2], &m);
					gx3d_SetTexture(0, tex_num[digit2]);
					gx3d_DrawObject(obj_numbers[digit2], 0);
				}

				// Third Digit
				gx3d_GetScaleMatrix(&m1, 0.012f, 0.012f, 0.012f);
				gx3d_GetRotateYMatrix(&m2, 180);
				gx3d_GetTranslateMatrix(&m3, 0.04, -0.08, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_MultiplyMatrix(&m, &m3, &m);
				if (score <= 0) {
					gx3d_SetObjectMatrix(obj_numbers[0], &m);
					gx3d_SetTexture(0, tex_num[0]);
					gx3d_DrawObject(obj_numbers[0], 0);
				}
				else if (score >= 10 && score <= 99) {
					int digit2 = final_score[1] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit2], &m);
					gx3d_SetTexture(0, tex_num[digit2]);
					gx3d_DrawObject(obj_numbers[digit2], 0);
				}
				else if (score >= 0 && score <= 9) {
					int digit3 = final_score[0] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit3], &m);
					gx3d_SetTexture(0, tex_num[digit3]);
					gx3d_DrawObject(obj_numbers[digit3], 0);
				}
				else {
					int digit3 = final_score[2] - 48;
					gx3d_SetObjectMatrix(obj_numbers[digit3], &m);
					gx3d_SetTexture(0, tex_num[digit3]);
					gx3d_DrawObject(obj_numbers[digit3], 0);
				}
				gx3d_DisableAlphaBlending();
				gx3d_DisableAlphaTesting();
			}

			if (instructions) {
				gxDrawFillRectangle(0, 0, gxGetScreenWidth(), gxGetScreenHeight());
				gx3d_GetTranslateMatrix(&m1, 0, 0, 0);
				gx3d_GetScaleMatrix(&m2, 0.1f, 0.08f, 0.08f);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_SetObjectMatrix(obj_instructions, &m);
				gx3d_SetTexture(0, tex_instructions);
				gx3d_DrawObject(obj_instructions, 0);
			}

			//gx3d_EnableZBuffer();

			// Restore view matrix
			gx3d_SetViewMatrix(&view_save);
			// Stop rendering
			gx3d_EndRender();

			// Page flip (so user can see it)
			gxFlipVisualActivePages(FALSE);
		}
	}
	/*____________________________________________________________________
	|
	| Free stuff and exit
	|___________________________________________________________________*/

	gx3d_FreeObject(obj_flower);
	gx3d_FreeObject(obj_monster[0]);
	gx3d_FreeObject(obj_monster[1]);
	gx3d_FreeObject(obj_monster[2]);
	gx3d_FreeObject(obj_numbers[0]);
	gx3d_FreeObject(obj_numbers[1]);
	gx3d_FreeObject(obj_numbers[2]);
	gx3d_FreeObject(obj_numbers[3]);
	gx3d_FreeObject(obj_numbers[4]);
	gx3d_FreeObject(obj_numbers[5]);
	gx3d_FreeObject(obj_numbers[6]);
	gx3d_FreeObject(obj_numbers[7]);
	gx3d_FreeObject(obj_numbers[8]);
	gx3d_FreeObject(obj_numbers[9]);
	gx3d_FreeObject(obj_hit);
	gx3d_FreeObject(obj_kills);
	gx3d_FreeObject(obj_tree);
	gx3d_FreeObject(obj_ground);
	gx3d_FreeObject(obj_skydome);
	gx3d_FreeObject(obj_start);
	gx3d_FreeObject(obj_victory);
	gx3d_FreeObject(obj_game_over);
	gx3d_FreeObject(obj_instructions);
	snd_Free();
}

/*____________________________________________________________________
|
| Function: Init_Render_State
|
| Input: Called from Program_Run()
| Output: Initializes general 3D render state.
|___________________________________________________________________*/

static void Init_Render_State()
{
	// Enable zbuffering
	gx3d_EnableZBuffer();

	// Enable lighting
	gx3d_EnableLighting();

	// Set the default alpha blend factor
	gx3d_SetAlphaBlendFactor(gx3d_ALPHABLENDFACTOR_SRCALPHA, gx3d_ALPHABLENDFACTOR_INVSRCALPHA);

	// Init texture addressing mode - wrap in both u and v dimensions
	gx3d_SetTextureAddressingMode(0, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
	gx3d_SetTextureAddressingMode(1, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
	// Texture stage 0 default blend operator and arguments
	gx3d_SetTextureColorOp(0, gx3d_TEXTURE_COLOROP_MODULATE, gx3d_TEXTURE_ARG_TEXTURE, gx3d_TEXTURE_ARG_CURRENT);
	gx3d_SetTextureAlphaOp(0, gx3d_TEXTURE_ALPHAOP_SELECTARG1, gx3d_TEXTURE_ARG_TEXTURE, 0);
	// Texture stage 1 is off by default
	gx3d_SetTextureColorOp(1, gx3d_TEXTURE_COLOROP_DISABLE, 0, 0);
	gx3d_SetTextureAlphaOp(1, gx3d_TEXTURE_ALPHAOP_DISABLE, 0, 0);

	// Set default texture coordinates
	gx3d_SetTextureCoordinates(0, gx3d_TEXCOORD_SET0);
	gx3d_SetTextureCoordinates(1, gx3d_TEXCOORD_SET1);

	// Enable trilinear texture filtering
	gx3d_SetTextureFiltering(0, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
	gx3d_SetTextureFiltering(1, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
}

/*____________________________________________________________________
|
| Function: Program_Free
|
| Input: Called from CMainFrame::OnClose()
| Output: Exits graphics mode.
|___________________________________________________________________*/

void Program_Free()
{
	// Stop event processing 
	evStopEvents();
	// Return to text mode 
	if (Pgm_system_font)
		gxFreeFont(Pgm_system_font);
	gxStopGraphics();
}
