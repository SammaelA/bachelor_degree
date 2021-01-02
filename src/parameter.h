#pragma once

#include "distribution.h"
#include <vector>
enum RandomnessLevel
{
    NO_RANDOM,
    EXPLICIT_REGENERATION,
    REGENERATE_ON_STATE_CHANGE,
    REGENERATE_ON_GET
};
template<typename T>
class Parameter
{
    public:
    void random_regenerate()
    {
        randomValue = (T)(randomizer->get());
    }
    Parameter &operator=(const T& base_value)
    {
        this->baseValue = baseValue;
        randomnessLevel = NO_RANDOM;
    }
    T get()
    {
        if (randomnessLevel == NO_RANDOM)
        {
            if (state == -1)
                return baseValue;
            else
                return stateParams[state];  
        }
        if (randomnessLevel == REGENERATE_ON_GET)
            random_regenerate();
        T value = randomValue;
        if (state == -1)
            value += baseValue;
        else
            value += stateParams[state];
        
        if (value > maxValue)
            value = maxValue;
        else if (value < minValue)
            value = minValue;
        //printf("my val = %f\n",(float)value);
        return value;
    }
    T operator()()
    {
        return get();
    }
    void set_state(int state) 
    {
        if (state<0 || state >= stateParams.size())
            state = -1;
        if (randomnessLevel == REGENERATE_ON_STATE_CHANGE && this->state != state)
            random_regenerate();
        this->state = state;
    }
    Parameter(T base, T minValue = (T)(-1e10), T maxValue = (T)(1e10))
    {
        baseValue = base;
        randomnessLevel = NO_RANDOM;
        this->maxValue = maxValue;
        this->minValue = minValue;
        state = -1;
    }
    Parameter(T base, std::vector<T> stateParams,
              T minValue = (T)(-1e10), T maxValue = (T)(1e10)):
    Parameter(base,minValue,maxValue)
    {
        this->stateParams = stateParams;
    }
    Parameter(T base, std::vector<T> stateParams, RandomnessLevel rand_level, Distribution *randomizer,
              T minValue = (T)(-1e10), T maxValue = (T)(1e10)):
    Parameter(base,stateParams,minValue,maxValue)
    {
        this->randomnessLevel = rand_level;
        this->randomizer = randomizer;
        if (rand_level != NO_RANDOM)
            random_regenerate();
    }
    Parameter(T base, RandomnessLevel rand_level, Distribution *randomizer,
              T minValue = (T)(-1e10), T maxValue = (T)(1e10)):
    Parameter(base,minValue,maxValue)
    {
        this->randomnessLevel = rand_level;
        this->randomizer = randomizer;
        if (rand_level != NO_RANDOM)
            random_regenerate();
    }
    private:
    T baseValue;
    T randomValue;
    T maxValue;
    T minValue;
    std::vector<T> stateParams;
    int state = -1;
    Distribution *randomizer = nullptr;
    RandomnessLevel randomnessLevel;
    
};
struct TreeStructureParameters
{
    Parameter<int> max_depth;
    Parameter<int> max_segments;
    Parameter<int> max_branching;
    Parameter<int> growth_iterations;

    Parameter<float> scale;
    Parameter<float> seg_len_mult;
    Parameter<float> leaf_size_mult;
    Parameter<float> base_r;
    Parameter<float> r_split_save_pow;

    Parameter<float> dir_conserv;
    Parameter<float> spread;
    Parameter<float> phototrop;
    Parameter<float> gravitrop;

    Parameter<float> seg_dir_conserv; 
    Parameter<float> seg_spread;
    Parameter<float> seg_phototrop;
    Parameter<float> seg_gravitrop;
    Parameter<float> seg_bend;
    Parameter<float> seg_bend_pow;

    Parameter<float> base_branch_feed;
    Parameter<float> base_seg_feed;
    Parameter<float> feed_distribution_min_weight;
    Parameter<float> feed_distribution_d_weight;
    Parameter<float> top_growth_bonus;

    Parameter<float> light_precision;
    Parameter<float> branch_removal;
    Parameter<float> branch_grow_decrease_q;
    Parameter<float> segment_grow_decrease_q;

    Parameter<float> min_branching_chance;
    Parameter<float> max_branching_chance;
    Parameter<float> branching_power;


    void set_state(int state)
    {
        max_depth.set_state(state);
        max_segments.set_state(state);
        max_branching.set_state(state);
        growth_iterations.set_state(state);

        scale.set_state(state);
        seg_len_mult.set_state(state);
        leaf_size_mult.set_state(state);
        base_r.set_state(state);
        r_split_save_pow.set_state(state);

        dir_conserv.set_state(state);
        spread.set_state(state);
        phototrop.set_state(state);
        gravitrop.set_state(state);

        seg_dir_conserv.set_state(state); 
        seg_spread.set_state(state);
        seg_phototrop.set_state(state);
        seg_gravitrop.set_state(state);
        seg_bend.set_state(state);
        seg_bend_pow.set_state(state);

        base_branch_feed.set_state(state);
        base_seg_feed.set_state(state);
        feed_distribution_min_weight.set_state(state);
        feed_distribution_d_weight.set_state(state);
        top_growth_bonus.set_state(state);

        light_precision.set_state(state);
        branch_removal.set_state(state);
        branch_grow_decrease_q.set_state(state);
        segment_grow_decrease_q.set_state(state);

        min_branching_chance.set_state(state);
        max_branching_chance.set_state(state);
        branching_power.set_state(state);

    }

    TreeStructureParameters():
    max_depth(4),
    max_segments(20,std::vector<int>{30,24,18,14,12}),
    max_branching(1),
    growth_iterations(300),

    scale(3),
    seg_len_mult(4,std::vector<float>{3.5,1.75,1,0.55,0.4}),
    leaf_size_mult(2),
    base_r(1,std::vector<float>{1,0.5,0.3,0.2,0.12}),
    r_split_save_pow(2.7,std::vector<float>{2,3,3,4,4},EXPLICIT_REGENERATION, new Normal(0,0.25)),

    dir_conserv(1),
    spread(1,std::vector<float>{1,8,5,5,5},REGENERATE_ON_GET, new Uniform(-1,1),0.1,10),
    phototrop(1,std::vector<float>{0.1,0.5,1,2,3},REGENERATE_ON_GET, new Uniform(-0.2,0.2),0.1,10),
    gravitrop(1,std::vector<float>{2,2,0.5,0.25,0.25},REGENERATE_ON_GET, new Uniform(-0.1,0.1),0.1,10),

    seg_dir_conserv(50,std::vector<float>{50,40,30,20,20},REGENERATE_ON_GET, new Uniform(-0.05,0.05),0,1000), 
    seg_spread(0,std::vector<float>{0,0,0,0.5,1}),
    seg_phototrop(1,std::vector<float>{3,2,1,1,1},REGENERATE_ON_GET, new Uniform(-1,2),0,10),
    seg_gravitrop(2,std::vector<float>{5,3,1.5,0.5,0.5},REGENERATE_ON_GET, new Uniform(-2,4),0,50),
    seg_bend(1,std::vector<float>{1,1,2,3,5},REGENERATE_ON_GET, new Uniform(-1,1),0,10),
    seg_bend_pow(2,std::vector<float>{2,2,2,2},REGENERATE_ON_GET, new Uniform(-0.05,0.05),0,10),

    base_branch_feed(1000,std::vector<float>{800,700,550,400,300},REGENERATE_ON_GET, new Uniform(-200,200)),
    base_seg_feed(250,std::vector<float>{100,100,100,75,50},REGENERATE_ON_GET, new Uniform(-30,30)),
    feed_distribution_min_weight(0.07),
    feed_distribution_d_weight(0.1),
    top_growth_bonus(0.07),

    light_precision(0.35),
    branch_removal(1.2,std::vector<float>{0,0.75,1,1.2,1.2}),
    branch_grow_decrease_q(0.9),
    segment_grow_decrease_q(0.09),

    min_branching_chance(0,std::vector<float>{-0.25,0,0.3,0.5,0.7}),
    max_branching_chance(1),
    branching_power(0.5,std::vector<float>{1.2,0.8,0.5,0.5,0.4})
    {

    }
};
/*struct Parameters
{
    glm::vec3 pos = glm::vec3(0,0,0);
    int max_depth = 4;
    int max_segments = 20;
    int max_branching = 2;
    int iterations = 60;

    float min_feed = 10;
    float max_feed = 10;
    float feed_v_mult = 0.01;

    float dir_conserv = 1.0; 
    float spread = 4;
    float phototrop = 1.0;
    float gravitrop = 0.8;

    float seg_dir_conserv = 1.0; 
    float seg_spread = 0.0;
    float seg_phototrop = 0.075;
    float seg_gravitrop = 0.1;
    float seg_bend = 0.075;
    float seg_bend_pow = 2.0;

    float base_r = 1.0;
    float r_level_mult = 0.4;
    float r_split_save_pow = 2.7;
    float r_feed_pow = 0.33;

    float scale = 3;
    float seg_len_mult = 5;
    float leaf_size_mult = 1.25;
    float len_level_mult = 0.45;

    float split_rate = 0.5; 
    float seg_split_rate = 0.1;
    float base_branch_feed = 1000;
    float base_seg_feed = 200;

    float light_precision = 1;
    float branch_grow_decrease_q = 0.9;
    float segment_grow_decrease_q = 0.09;

    float min_branching_chance = 0.0;
    float max_branching_chance = 1.0;
    float branching_power = 0.5;
    
    float feed_distribution_min_weight = 0.07;
    float feed_distribution_d_weight = 0.1;
    float top_growth_bonus = 0.15;

    float branch_removal = 0.75;
};*/