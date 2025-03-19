#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 12
#define ENEMIES_COUNT 6
#define FONTBANK_SIZE 16
#define FUEL_AMOUNT 5000

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"
#include <string>

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
    Entity* enemies;
};

enum Game_Process{RUNNING, LOSE, WIN};

// ––––– CONSTANTS ––––– //
Game_Process game_process = RUNNING;
const int WINDOW_WIDTH  = 1280,
          WINDOW_HEIGHT = 960;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char SPRITESHEET_FILEPATH[] = "assets/fireball.png";
const char PLATFORM_FILEPATH[]    = "assets/platform.png";
const char ENEMY_FILEPATH[] = "assets/enemy.png";
const char FONT_FILEPATH[] = "assets/font1.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

const int CD_QUAL_FREQ    = 44100,
          AUDIO_CHAN_AMT  = 2,     // stereo
          AUDIO_BUFF_SIZE = 4096;

const char BGM_FILEPATH[] = "assets/crypto.mp3",
           SFX_FILEPATH[] = "assets/bounce.wav";

const int PLAY_ONCE = 0,    // play once, loop never
          NEXT_CHNL = -1,   // next available channel
          ALL_SFX_CHNL = -1;



// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

//
GLuint font_texture_id;

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
        texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Hello, Physics (again)!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
 
    
    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    GLuint enemy_texture_id = load_texture(ENEMY_FILEPATH);
    
    g_state.platforms = new Entity[PLATFORM_COUNT];
    g_state.enemies = new Entity[ENEMIES_COUNT];
    
    // Set the type of every platform entity to PLATFORM
    for (int i = 0; i < PLATFORM_COUNT-2; i++)
    {
        g_state.platforms[i].set_texture_id(platform_texture_id);
        g_state.platforms[i].set_position(glm::vec3(i - 4.5f, -3.8f, 0.0f));
        g_state.platforms[i].set_width(1.0f);
        g_state.platforms[i].set_height(1.0f);
        g_state.platforms[i].set_entity_type(PLATFORM);
        g_state.platforms[i].update(0.0f, NULL, NULL, 0);        
    }

    g_state.platforms[PLATFORM_COUNT - 1].set_texture_id(platform_texture_id);
    g_state.platforms[PLATFORM_COUNT - 1].set_position(glm::vec3(rand()%3, -1.8f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 1].set_width(1.0f);
    g_state.platforms[PLATFORM_COUNT - 1].set_height(1.0f);
    g_state.platforms[PLATFORM_COUNT - 1].set_entity_type(PLATFORM);
    g_state.platforms[PLATFORM_COUNT - 1].update(0.0f, NULL, NULL, 0);

    g_state.platforms[PLATFORM_COUNT - 2].set_texture_id(platform_texture_id);
    g_state.platforms[PLATFORM_COUNT - 2].set_position(glm::vec3(-rand() % 3, 1.8f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 2].set_width(1.0f);
    g_state.platforms[PLATFORM_COUNT - 2].set_height(1.0f);
    g_state.platforms[PLATFORM_COUNT - 2].set_entity_type(PLATFORM);
    g_state.platforms[PLATFORM_COUNT - 2].update(0.0f, NULL, NULL, 0);
    
    // Set the enemies
    for (int i = 0; i < ENEMIES_COUNT; i++)
    {
        g_state.enemies[i].set_texture_id(enemy_texture_id);
        g_state.enemies[i].set_position(glm::vec3(2*i - 4.5f, -3.1f, 0.0f));
        g_state.enemies[i].set_width(0.5f);
        g_state.enemies[i].set_height(0.5f);
        g_state.enemies[i].set_entity_type(ENEMY);
        g_state.enemies[i].set_scale(glm::vec3(0.5f, 0.5f, 0.5f));
        g_state.enemies[i].update(0.0f, NULL, NULL, 0);
    }

    //FONT
    font_texture_id = load_texture(FONT_FILEPATH);


    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    glm::vec3 acceleration = glm::vec3(0.0f,0.0f, 0.0f);

    g_state.player = new Entity(
        player_texture_id,         // texture id
        5.0f,                      // speed
        acceleration,              // acceleration
        0.4f,                      // fuel power
        0.5f,                      // width
        0.5f,                       // height
        PLAYER
    );


    //g_state.player->set_asc_power(0.20f);
    g_state.player->set_scale(glm::vec3(0.5f, 0.5f, 0.5f));
    g_state.player->set_position(glm::vec3(0.0f, 3.6f, 0.0f));
    g_state.player->set_fuel(FUEL_AMOUNT);
    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_acceleration(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                       
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        if (g_state.player->get_fuel() > 0) {
            g_state.player->fuel_using();
            g_state.player->accelerate_left();
        }
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        if (g_state.player->get_fuel() > 0) {
            g_state.player->fuel_using();
            g_state.player->accelerate_right();
        }
    }
    
    if (glm::length(g_state.player->get_acceleration()) > 1.0f)
    {
        g_state.player->normalise_acceleration();
    }
    if (key_state[SDL_SCANCODE_UP])
    {
        if (g_state.player->get_fuel() > 0) {
            g_state.player->fuel_using();
            g_state.player->power_up();
            g_state.player->accelerate_up();
        }    
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        if (g_state.player->get_fuel() > 0) {
            g_state.player->fuel_using();
            g_state.player->accelerate_down();
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }   

    while (delta_time >= FIXED_TIMESTEP)
    {
        if (game_process == RUNNING) {
            for (int i = 0; i < PLATFORM_COUNT; i++) {
                if (g_state.player->check_collision(&g_state.platforms[i])) {
                    game_process = LOSE;
                }
            }

            for (int i = 0; i < ENEMIES_COUNT; i++) {
                g_state.enemies[i].ai_slime_move(g_state.player);
                g_state.enemies[i].update(FIXED_TIMESTEP);
                if (g_state.player->check_collision(&g_state.enemies[i])) {
                    game_process = WIN;
                }
            }
            g_state.platforms[PLATFORM_COUNT - 1].regular_move(2.0f,-2.0f);
            g_state.platforms[PLATFORM_COUNT - 2].regular_move(2.0f, -2.0f);
            g_state.platforms[PLATFORM_COUNT - 1].update(FIXED_TIMESTEP);
            g_state.platforms[PLATFORM_COUNT - 2].update(FIXED_TIMESTEP);

        }
        g_state.player->update(FIXED_TIMESTEP, g_state.enemies, g_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;

    
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    switch (game_process)
    {
    case RUNNING:
        g_state.player->render(&g_program);
        for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
        for (int i = 0; i < ENEMIES_COUNT; i++) g_state.enemies[i].render(&g_program);
        draw_text(&g_program, font_texture_id, "MP: "+std::to_string(g_state.player->get_fuel()), 0.5f, -0.2f, glm::vec3(-4.7f, 3.5f, 0.0f));
        break;
    case WIN:
        draw_text(&g_program, font_texture_id, "YOU WIN", 0.5f, 0.05f, glm::vec3(-3.5f, 2.0f, 0.0f));
        break;
    case LOSE:
        draw_text(&g_program, font_texture_id, "YOU LOSE", 0.5f, 0.05f, glm::vec3(-3.5f, 2.0f, 0.0f));
        break;
    default:
        break;
    }
 
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_state.platforms;
    delete[] g_state.enemies;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
