#pragma once
#include "abstract_generator.h"
#include "parameter.h"

struct SimpleTreeStructureParameters : public ParametersSet
{
    Parameter<float> max_depth;
    Parameter<float> segment_size;
    Parameter<float> segment_count;
    Parameter<float> base_thickness;
    Parameter<float> base_dir_mult;
    Parameter<float> rand_dir_mult;
    Parameter<float> up_dir_mult;
    Parameter<float> down_dir_mult;
    Parameter<float> up_dir_pow;
    Parameter<float> down_dir_pow;
    Parameter<float> sectors_count;
    Parameter<float> base_branch_angle;
    Parameter<float> branching_chance;
    Parameter<float> leaves_chance;
    Parameter<float> leaf_mult;

    virtual void set_state(int state) override
    {
        max_depth.set_state(state);
        segment_size.set_state(state);
        segment_count.set_state(state);
        base_thickness.set_state(state);
        base_dir_mult.set_state(state);
        rand_dir_mult.set_state(state);
        up_dir_mult.set_state(state);
        down_dir_mult.set_state(state);
        up_dir_pow.set_state(state);
        down_dir_pow.set_state(state);
        sectors_count.set_state(state);
        base_branch_angle.set_state(state);
        branching_chance.set_state(state);
        leaves_chance.set_state(state);
        leaf_mult.set_state(state);
    }
    SimpleTreeStructureParameters():
    max_depth(4),
        segment_size(1,std::vector<float>{1.5,0.7,0.25,0.25}, REGENERATE_ON_GET, distibutionGenerator.get_uniform(-0.15, 0.15)),
        segment_count(15, std::vector<float>{15, 12, 10, 15}, REGENERATE_ON_GET, distibutionGenerator.get_uniform(-10, 10)),
        base_thickness(1.5,std::vector<float>{0.8,0.4,0.25,0.1}, REGENERATE_ON_GET, distibutionGenerator.get_uniform(-0.08, 0.08)),
        base_dir_mult(5),
        rand_dir_mult(1),
        up_dir_mult(1),
        down_dir_mult(1),
        up_dir_pow(1),
        down_dir_pow(2),
        sectors_count(6),
        base_branch_angle(PI/4),
        branching_chance(0.75),
        leaves_chance(0.8),
        leaf_mult(1.25)
    {

    };
    virtual void get_parameter_list(std::vector<std::pair<ParameterTinyDesc,Parameter<float> &>> &list,
                            ParameterVariablesSet v_set = ParameterVariablesSet::ALL_VALUES) override
    {

    }
};

class SimpleTreeGenerator : public AbstractTreeGenerator
{
public:
    virtual void create_grove(GroveGenerationData ggd, ::Tree *trees_external, Heightmap &h) override;
private:
    void create_tree(Tree *tree, glm::vec3 pos);
    void create_branch(Tree *tree, Branch *branch, glm::vec3 start_pos, glm::vec3 base_dir, glm::vec3 normal, int level, 
                       float base_r, float leaves_chance);
    BaseParameterSetWrapper<SimpleTreeStructureParameters> params;
    GroveGenerationData ggd;
    Heightmap *h;
};