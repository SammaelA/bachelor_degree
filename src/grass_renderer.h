#pragma once
#include "terrain.h"

class GrassRenderer
{
public:
    GrassRenderer();
    void render(glm::mat4 prc, glm::mat4 shadow_tr, GLuint shadow_tex, glm::vec3 camera_pos,
                HeightmapTex &heightmap_tex, DirectedLight &light);
private:
    Texture grass_base;
    Texture grass_tall;
    Texture perlin;
    Texture noise;
    Shader shader;
    Model m;
};