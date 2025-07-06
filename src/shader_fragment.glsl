#version 330 core

in vec4 position_world;
in vec4 normal;
in vec4 position_model;
in vec2 texcoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define ROOM 0
#define BED  1
#define FLOOR 2
#define STOVE 3
#define SOFA 4
#define DOOR 5
#define CABINET1 5
uniform int object_id;

uniform sampler2D TextureImage0;

out vec4 color;

void main()
{
    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    vec3 final_color;

    if ( object_id == ROOM )
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if ( object_id == BED )
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if ( object_id == FLOOR )
    {
        final_color = vec3(0.5, 0.25, 0.0);
    }
    else if (object_id == STOVE) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == SOFA) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == DOOR) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == CABINET1) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }

    color.rgb = final_color;
    color.a = 1;
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);

}
