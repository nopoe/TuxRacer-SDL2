/* 
 * Tux Racer 
 * Copyright (C) 1999-2001 Jasmin F. Patry
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "tuxracer.h"
#include "fonts.h"
#include "gl_util.h"
#include "textures.h"
#include "fps.h"
#include "phys_sim.h"
#include "multiplayer.h"
#include "ui_mgr.h"
#include "game_logic_util.h"
#include "course_load.h"
#include "fonts.h"
#include "loop.h"
#include "racing.h"
#include "paused.h"
#ifdef TARGET_OS_IPHONE
    #include "sharedGeneralFunctions.h"
#endif

static int step = -1;
static Uint32 first_time_true = 0;
static bool_t is_condition_verified = False;
static bool_t training_abort = False;
static bool_t pause_for_long_tutorial_explanation = False;
static bool_t resume_from_tutorial_explanation = False;


static void print_instruction(const char* string, int line) {

    char* binding = "instructions";
    font_t *font;
    int w, asc, desc;
    
    if ( !get_font_binding( binding, &font ) ) {
        print_warning( IMPORTANT_WARNING,
                      "Couldn't get font for binding %s", binding );
        return;
    }
    
    bind_font_texture( font );
    
    get_font_metrics( font, (char*)string, &w, &asc, &desc );
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0,1.0,1.0,0.4);
    
	{
    GLfloat vertices[]={
		0.0, (float)(200-(line-2)*(asc+desc)) -5.0, 0,
		0.0, (float)(200-(line-1)*(asc+desc)) -5.0, 0,
		480.0, (float)(200-(line-1)*(asc+desc)) -5.0, 0,
		480.0, (float)(200-(line-2)*(asc+desc)) -5.0, 0};

	GLubyte indices[] = {0, 1, 2, 2, 3, 0};

	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
		
	glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_TEXTURE_2D);
    glPushMatrix();
    {
        glTranslatef( 240.0-(float)w/2.0,
                     200-(line-1)*(asc+desc),
                     0 );
        draw_string( font, (char*)string );
    }
    glPopMatrix();
	}
}

static void drawRedCircle(GLint x, GLint y, GLint radius) {
    int x_org = x;
    int y_org = y;
    GLuint texobj;
    point2d_t tll, tur;
    point2d_t ll, ur;
    if ( !get_texture_binding( "red_circle", &texobj ) ) {
        texobj = 0;
    }
    glColor4f(1.0,0.0,0.0,0.5);
    
    glBindTexture( GL_TEXTURE_2D, texobj );

        
    ll = make_point2d( x_org, y_org);
    ur = make_point2d( x_org + radius, y_org + radius );
    tll = make_point2d( 0, 0 );
    tur = make_point2d(1, 1 );
    
	{
 	GLfloat texcoords[]={
		tll.x, tll.y,
		tll.x, tur.y,
		tur.x, tur.y,
		tur.x, tll.y};

	GLfloat vertices[]={
		ll.x, ll.y, 0,
		ll.x, ur.y, 0,
		ur.x, ur.y, 0,
		ur.x, ll.y, 0};

	GLubyte indices[] = {0, 1, 2, 2, 3, 0};

    glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
		
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	}
}  

/* verifie qu'une condition a bien été vérifiée pendant au moins "sec" secondes */
static bool_t check_condition_for_time(bool_t condition, unsigned int ms) {
    if (condition)
	{
        if (first_time_true==UINT_MAX)
		{
			first_time_true = SDL_GetTicks();
		}
		else
		{
			if (SDL_GetTicks()-first_time_true>ms)
			{
				return True;
			}
		}
	}
	else
	{
		first_time_true=UINT_MAX;
	}
	return False;
}

static void training_pause_for_tutorial_explanation(void)
{
    if(!pause_for_long_tutorial_explanation)
    {
        pause_for_long_tutorial_explanation = True;
        resume_from_tutorial_explanation = False;
		force_pause_for_ticks(1000);
        set_game_mode(PAUSED);
    }
}

static bool_t training_is_resumed(void)
{
    if(resume_from_tutorial_explanation)
    {
        resume_from_tutorial_explanation = False;
        pause_for_long_tutorial_explanation = False;
        return True;
    }
    return False;
}

bool_t game_abort_is_for_tutorial(void)
{
    return training_abort;
}

bool_t pause_is_for_long_tutorial_explanation(void)
{
    return pause_for_long_tutorial_explanation;
}

void training_resume_from_tutorial_explanation(void)
{
    resume_from_tutorial_explanation = True;
}

static void draw_instructions(player_data_t *plyr)
{
    switch (step) {
        case 0:
            print_instruction(Localize("Welcome to the basic tutorial.", ""),1);
            print_instruction(Localize("You will learn here how to turn,", ""),2);
            print_instruction(Localize("accelerate, brake and pause.", ""),3);
            //N'est affiché que endant l'intro
            if(g_game.time>0) step++;
            break;
        case 1:
            print_instruction(Localize("Try to make tux turn right.", ""),1);
			if (SDL_GetNumTouchDevices()>0)
			{
				print_instruction(Localize("(Turn your device to the right)", ""),2);
			}
			else
			{
				print_instruction(Localize("(Use the left analog stick)", ""),2);
			}
            if (plyr->control.turn_fact>0.5) step++;
            break;
        case 2:
            print_instruction(Localize("Now make tux turn left.", ""),1);
            if (plyr->control.turn_fact<-0.4) step++;
            break;
        case 3:
			print_instruction(Localize("Tux can paddle to go faster.", ""),1);
			if (SDL_GetNumTouchDevices()>0)
			{
				print_instruction(Localize("(Tap the bottom right corner of the screen)", ""),2);
	            drawRedCircle(getparam_x_resolution()-150, 50, 100);
			}
			else
			{
				print_instruction(Localize("(Push the analog stick forward)", ""),2);
			}
            if (check_condition_for_time(plyr->control.is_accelerating,1000)) step++;
            break;
        case 4:
            print_instruction(Localize("Braking lets tux turn harder at high speed.", ""),1);
			if (SDL_GetNumTouchDevices()>0)
			{
				print_instruction(Localize("Press and hold the bottom right corner to brake.", ""),2);
	            drawRedCircle(10, 10, 100);
			}
			else
			{
				print_instruction(Localize("Pull the analog stick back to brake.", ""),2);
			}
            if (check_condition_for_time(plyr->control.is_braking,1000))
			{
				step++;
				check_condition_for_time(False,0);
			}
            break;
        case 5:
            print_instruction(Localize("Now let's learn how to pause.", ""),1);
            if(check_condition_for_time(True,2000)) step++;
            break;
        case 6:
 			if (SDL_GetNumTouchDevices()>0)
			{
	            print_instruction(Localize("Tap the center of the screen", ""),1);
			}
			else
			{
	            print_instruction(Localize("Press the Y button", ""),1);
			}
            if(g_game.race_paused==True) step++;
            break;
        case 7:
            print_instruction(Localize("Cool! Now come back to game.", ""),0);
 			if (SDL_GetNumTouchDevices()>0)
			{
	            print_instruction(Localize("Tap anywhere on the screen.", ""),1);
			}
			else
			{
	            print_instruction(Localize("Press any button", ""),1);
			}
            if(g_game.race_paused==False) step=-1;
            break;
        case -1:
            print_instruction(Localize("Good job. Now you know the basics of racing", ""),1);
            print_instruction(Localize("The next tutorial will teach you how to jump", ""),2);
            print_instruction(Localize("and do tricks!", ""),3);
            set_game_mode( GAME_OVER );
            break;
            					/* Fin du premier Tutorial */
                                
                                /* Début du second Tutorial */
        case 10:
            print_instruction(Localize("Welcome to the Jump tutorial where", ""),1);
            print_instruction(Localize("you'll will learn to jump, fly and", ""),2);
            print_instruction(Localize("do tricks!", ""),3);
            //N'est affiché que endant l'intro
            if(g_game.time>0) step=11;
            break;
        case 11:
 			if (SDL_GetNumTouchDevices()>0)
			{
				print_instruction(Localize("Push the red area to accumulate enough", ""),1);
				print_instruction(Localize("energy to jump.", ""),2);
				drawRedCircle(370.0, 200, 100);
			}
			else
			{
				print_instruction(Localize("Hold down the O button to accumulate enough", ""),1);
				print_instruction(Localize("energy to jump.", ""),2);
			}
			check_condition_for_time(False,0);
            if (plyr->control.jump_charging) step++;
            break;
        case 12:
            print_instruction(Localize("You can see the energy gauge filling...", ""),1);
            if (check_condition_for_time(True,1000))
			{
				check_condition_for_time(False,0);
				step++;
			}
            break;
        case 13:
            print_instruction(Localize("Release the jump button to jump!", ""),1);
            if (plyr->control.jumping) step++;
            break;
        case 14:
            print_instruction(Localize("Ok, now try to do a longer jump. See", ""),1);
            print_instruction(Localize("if you can spend a whole second in the", ""),2);
            print_instruction(Localize("air. Use the terrain to your advantage.", ""),3);
            if (check_condition_for_time(True,5000))
			{
				check_condition_for_time(False,0);
				step++;
			}
            break;
        case 15:
            print_instruction(Localize("Try to do a longer jump (>1sec) off a bump.", ""),-3);
            if (check_condition_for_time(plyr->control.is_flying,1000)) step++;
            break;
        case 16:
            print_instruction(Localize("Great!", ""),1);
            if (check_condition_for_time( True,1)) step++;
            break;
        case 17:
            if (check_condition_for_time( True,1)) step++;
            break;
        case 18:
            training_pause_for_tutorial_explanation();
            print_instruction(Localize("Thera are two main things you can do while", ""),1);
            print_instruction(Localize("you're in the air. First, you can try and", ""),2);
            print_instruction(Localize("do a cool trick. Second, you can accelerate", ""),3);
            print_instruction(Localize("to fly longer and go faster.", ""),4);
            if(training_is_resumed())
			{
				step++;
			}
			else
			{
				break;
			}
        case 19:
            training_pause_for_tutorial_explanation();
            print_instruction(Localize("To do a trick, find a bump while you're going", ""),1);
            print_instruction(Localize("fast, jump off of it. While you're in the air,", ""),2);
 			if (SDL_GetNumTouchDevices()>0)
			{
				print_instruction(Localize("tap the upper right corner of the screen.", ""),3);
	            drawRedCircle(getparam_x_resolution()-150, getparam_y_resolution()-150, 100);
			}
			else
			{
				print_instruction(Localize("press the A button.", ""),3);
			}
            if(training_is_resumed()) step++;
            break;
        case 20:
            print_instruction(Localize("Try to jump and do a trick.", ""),1);
			check_condition_for_time(False,0);
            if (plyr->tricks>0) step++;
            break;
        case 21:
            print_instruction(Localize("Great!", ""),1);
            if (check_condition_for_time(True,2000)) step++;
            break;
        case 22:
            training_pause_for_tutorial_explanation();
            print_instruction(Localize("Now, to finish, try to do a big jump", ""),1);
            print_instruction(Localize("while flapping your wings to go faster.", ""),2);
            if(training_is_resumed()) {
                step++;
                if ((plyr->pos.z)<(-200)) {
                    point_t p = make_point(48.0,-105.8,-200.0);
                    racing_init_for_tutorial(p);
                }
            }
            break;
        case 23:
            print_instruction(Localize("Try to do a long jump flying (>1sec).", ""),-3);
            if (check_condition_for_time( (bool_t)(plyr->control.is_flying && plyr->control.is_accelerating),1000)) step = -2;
            break;
        case -2:
            print_instruction(Localize("Congratulations, you finished this tutorial.", ""),1);
            print_instruction(Localize("You can apply these skills to different", ""),2);
            print_instruction(Localize("courses. Sometimes you should focus on speed", ""),3);
            print_instruction(Localize("and picking up food. Sometimes you have to", ""),4);
            print_instruction(Localize("do tricks to get a good score.", ""),5);
            set_game_mode( GAME_OVER );
            break;
            						/* Fin du second Tutorial */ 
            break;
            		        			/*    abandon     */
        case -100:
            print_instruction(Localize("You didn't finish this tutorial.", ""),1);
            print_instruction(Localize("You should try again!", ""),2);
            break;
            
        default:
            break;
    }
    
}

void draw_hud_training( player_data_t *plyr )
{
    if (!g_game.practicing) {
        vector_t vel;
        scalar_t speed;
        
        vel = plyr->vel;
        speed = normalize_vector( &vel );
        
        ui_setup_display();
        draw_instructions(plyr);
    }
    //draw_gauge( speed * M_PER_SEC_TO_KM_PER_H, plyr->control.jump_amt );
    
}


void init_starting_tutorial_step(int i){
    is_condition_verified=False;
    training_abort=False;
    pause_for_long_tutorial_explanation=False;
    step = i;
}

