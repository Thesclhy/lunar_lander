#ifndef ENTITY_H
#define ENTITY_H

#include "glm/glm.hpp"
#include "ShaderProgram.h"
enum EntityType { PLATFORM, PLAYER, ENEMY};

class Entity
{
private:
    int fuel;

    EntityType m_entity_type;
    // ————— TRANSFORMATIONS ————— //
    glm::vec3 m_position;
    glm::vec3 m_scale;
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;

    glm::mat4 m_model_matrix;

    float     m_speed,
              m_asc_power;

    bool m_is_ascending, fuel_is_using;

    // ————— TEXTURES ————— //
    GLuint    m_texture_id;

    // ————— ANIMATION ————— //
    int m_animation_cols;
    int m_animation_frames,
        m_animation_index,
        m_animation_rows;

    int* m_animation_indices = nullptr;
    float m_animation_time = 0.0f;

    float m_width = 1.0f,
          m_height = 1.0f;
    // ————— COLLISIONS ————— //
    bool m_collided_top    = false;
    bool m_collided_bottom = false;
    bool m_collided_left   = false;
    bool m_collided_right  = false;

    bool isMoveRight = true;

public:
    // ————— STATIC VARIABLES ————— //
    static constexpr int SECONDS_PER_FRAME = 4;
    static constexpr float GRAVITY = 0.2;

    // ————— METHODS ————— //
    Entity();
    Entity(GLuint texture_id, float speed, glm::vec3 acceleration, float fuel_power,
        float width, float height, EntityType EntityType);
    ~Entity();

    void draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index);
    bool const check_collision(Entity* other) const;
    void update(float delta_time, Entity *player, Entity *collidable_entities, int collidable_entity_count);
    void update(float delta_time);
    void render(ShaderProgram* program);

    //void normalise_movement() { m_movement = glm::normalize(m_movement); }
    void normalise_acceleration() { m_acceleration = glm::normalize(m_acceleration); }

    void accelerate_left() { m_acceleration.x = -m_asc_power;}
    void accelerate_right() { m_acceleration.x = m_asc_power;}
    void accelerate_up() { m_acceleration.y = m_asc_power-GRAVITY;}
    void accelerate_down() { m_acceleration.y = -m_asc_power-GRAVITY;}
    void move_left() { m_velocity.x = -0.15f; }
    void move_right() { m_velocity.x = 0.15f; }


    void ai_slime_move(Entity* player);
    void regular_move(float rightBound, float leftBound);

    void const power_up() { m_is_ascending = true; }
    void const fuel_using() { fuel_is_using = true; }

    // ————— GETTERS ————— //
    EntityType const get_entity_type()    const { return m_entity_type;   };
    glm::vec3 const get_position()     const { return m_position; }
    glm::vec3 const get_velocity()     const { return m_velocity; }
    glm::vec3 const get_acceleration() const { return m_acceleration; }
    glm::vec3 const get_scale()        const { return m_scale; }
    GLuint    const get_texture_id()   const { return m_texture_id; }
    float     const get_speed()        const { return m_speed; }
    bool      const get_collided_top() const { return m_collided_top; }
    bool      const get_collided_bottom() const { return m_collided_bottom; }
    bool      const get_collided_right() const { return m_collided_right; }
    bool      const get_collided_left() const { return m_collided_left; }
    float get_width() const { return m_width; }
    float get_height() const { return m_height; }
    int get_fuel() const { return fuel; };


    // ————— SETTERS ————— //
    void const set_entity_type(EntityType new_entity_type)  { m_entity_type = new_entity_type;};
    void const set_position(glm::vec3 new_position) { m_position = new_position; }
    void const set_velocity(glm::vec3 new_velocity) { m_velocity = new_velocity; }
    void const set_acceleration(glm::vec3 new_acceleration) { m_acceleration = new_acceleration; }
    void const set_scale(glm::vec3 new_scale) { m_scale = new_scale; }
    void const set_texture_id(GLuint new_texture_id) { m_texture_id = new_texture_id; }
    void const set_speed(float new_speed) { m_speed = new_speed; }
    void const set_asc_power(float new_acs_power) { m_asc_power = new_acs_power;}
    void const set_width(float new_width) {m_width = new_width; }
    void const set_height(float new_height) {m_height = new_height; }
    void const set_fuel(int new_fuel) { fuel = new_fuel; }
};

#endif // ENTITY_H
