#pragma once
#include "glm/glm.hpp"
#include <functional>
class Field_2d
{
public:
    Field_2d(glm::vec3 pos, glm::vec2 size, float cell_size);
    Field_2d(glm::vec3 pos, int w, int h);
    Field_2d() {data = nullptr;}
    ~Field_2d();
    float get_bilinear(glm::vec3 pos);
    void set(glm::vec3 pos, float val);
    void fill_const(float val);
    void fill_func(std::function<float(glm::vec2 &)> filler);
    void fill_perlin(float base, float min, float max, glm::ivec2 sh = glm::ivec2(0,0));
    glm::vec2 get_range() {return glm::vec2(min_val,max_val);}
    glm::vec2 get_grad_bilinear(glm::vec3 pos);
    glm::vec4 get_borders();//[x0,y0] - [x1,y1]
    void get_min_max_imprecise(glm::vec2 from, glm::vec2 to, float *min_v, float *max_v, 
                               glm::vec2 *min_pos = nullptr,
                               glm::vec2 *max_pos = nullptr);//real_min >= min_v, real_max <= max_v 
    void print();
protected:
    float get(int x, int y);
    glm::vec2 get_grad(int x, int y);
    void set(int x, int y, float val);
    void set_safe(int x, int y, float val);
    void add(int x, int y, float val);
    void add_safe(int x, int y, float val);
    int w, h;
    float min_val = 1e10, max_val = -1e10;
    glm::vec3 pos;
    glm::vec2 size;
    float cell_size = 1;
    float *data;
    float base_val;
};