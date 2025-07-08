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
#define CABINET1 6
#define TV 7
#define RADIATOR 8
#define RAT 9
#define TABLE 10
#define BREAD 11
#define REFRIGERATOR 12
#define SINK 13
#define REMOTE 14
#define FAN 15
uniform int object_id;

uniform sampler2D TextureImage0;

out vec4 color;

void main()
{
    // Iluminação
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;
    vec4 p = position_world;
    vec4 n = normalize(normal); 

    vec4 Lp = vec4(0.0,0.0,0.0,1.0);
    vec4 l = normalize(Lp-p);
    vec4 v = normalize(camera_position - p);
    vec4 r = normalize(-l+2*n*(dot(n,l))); 
    vec4 w = normalize(vec4(0.0,-1.0,0.0,0.0));
    vec4 h = normalize(v+l);

    vec3 Ks;
    vec3 Ka; 
    vec3 Kd0;
    float q;
    float alfa;

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
        //final_color = vec3(0.5, 0.25, 0.0);

        U = texcoords.x;
        V = texcoords.y;
        
        final_color = (0.4, 0.4, 0.4) * texture(TextureImage0, vec2(U, V)).rgb;
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
    else if (object_id == TV) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Ka = vec3(0.000000, 0.000000, 0.000000);
        Ks = vec3(0.000000, 0.000000, 0.000000);
        q = 84916;
        alfa = 1.000000;
        Kd0 = vec3(0.9, 0.9, 0.9) * texture(TextureImage0, fract(vec2(U,V))).rgb;

        vec3 I = vec3(1.0, 1.0, 1.0); 
        vec3 Ia = vec3(0.2, 0.2, 0.2); 

        vec3 lambert = I*max(0,dot(n,l));
        vec3 ambient_term = Ka*Ia;
        vec3 blinn_phong_specular_term  = Ks*I*pow(dot(n.xyz,h.xyz),q);

        final_color = Kd0 * (lambert) + ambient_term + blinn_phong_specular_term;
    }
    else if (object_id == RADIATOR) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Ka = vec3(0.000000, 0.000000, 0.000000);
        Ks = vec3(0.000000, 0.000000, 0.000000);
        q = 84916;
        alfa = 1.000000;
        Kd0 = vec3(0.9, 0.9, 0.9) * texture(TextureImage0, fract(vec2(U,V))).rgb;


        vec3 I = vec3(1.0, 1.0, 1.0); 
        vec3 lambert = I*max(0,dot(n,l));
        
        final_color = Kd0 * (lambert);
    }
    else if (object_id == RAT) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == TABLE) 
    {
        U = texcoords.x;
        V = texcoords.y;
        
        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == BREAD) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Ka = vec3(0.000000, 0.000000, 0.000000);
        Ks = vec3(0.000000, 0.000000, 0.000000);
        q = 84916;
        alfa = 1.000000;
        Kd0 = vec3(0.9, 0.9, 0.9) * texture(TextureImage0, fract(vec2(U,V))).rgb;

        vec3 I = vec3(1.0, 1.0, 1.0); 
        vec3 Ia = vec3(0.2, 0.2, 0.2); 

        vec3 lambert = I*max(0,dot(n,l));
        vec3 ambient_term = Ka*Ia;
        vec3 blinn_phong_specular_term  = Ks*I*pow(dot(n.xyz,h.xyz),q);

        final_color = Kd0 * (lambert) + ambient_term + blinn_phong_specular_term;
    }
    else if (object_id == REFRIGERATOR) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == SINK) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == REMOTE) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == REMOTE) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }
    else if (object_id == FAN) 
    {
        U = texcoords.x;
        V = texcoords.y;

        final_color = texture(TextureImage0, vec2(U, V)).rgb;
    }

    color.rgb = final_color;
    color.a = 1;
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);

}
