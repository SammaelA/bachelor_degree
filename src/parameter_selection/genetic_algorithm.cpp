#include "genetic_algorithm.h"
#include <algorithm>
#include <atomic>
#include "GA_utils.h"
#include <set>

std::atomic<int> next_id(0);

void GeneticAlgorithm::perform(ParameterList &param_list, MetaParameters params, ExitConditions exit_conditions,
                               const std::function<std::vector<float>(std::vector<ParameterList> &)> &f,
                               std::vector<std::pair<float, ParameterList>> &best_results)
{
    t_start = std::chrono::steady_clock::now();

    function = f;
    original_param_list = param_list;
    metaParams = params;
    exitConditions = exit_conditions;
    free_parameters_cnt = 0;

    for (auto &p : original_param_list.categorialParameters)
    {
        parametersMask.categorialParameters.push_back(p);
        if (!p.second.fixed())
            free_parameters_cnt++;
    }
    for (auto &p : original_param_list.ordinalParameters)
    {
        parametersMask.ordinalParameters.push_back(p);
        if (!p.second.fixed())
            free_parameters_cnt++;
    }
    for (auto &p : original_param_list.continuousParameters)
    {
        parametersMask.continuousParameters.push_back(p);
        if (!p.second.fixed())
            free_parameters_cnt++;
    }

    next_id.store(0);
    initialize_population();
    calculate_metric();
    recalculate_fitness();
    debug("iteration 0 Pop: %d Best: %.4f\n", current_population_size, best_metric_ever);
    while (!should_exit())
    {
        next_id.store(1000*(iteration_n + 1));
        kill_old();
        kill_weak(metaParams.weaks_to_kill);
        if (current_population_size < 5)
        {
            logerr("population extincted");
            return;
        }
        else if (current_population_size < 0.1*metaParams.initial_population_size)
        {
            logerr("population %d is about to extinct", current_population_size);
        }
        int space_left = metaParams.max_population_size - current_population_size;
        std::vector<std::pair<int, int>> pairs;
        find_pairs(0.67*space_left, pairs);
        for (auto &p : pairs)
        {
            //logerr("children %d %d", p.first, p.second);
            make_children(population[p.first], population[p.second], 1);
        }
        
        calculate_metric();
        recalculate_fitness();
        for (int i=0;i<population.size();i++)
        {
            if (population[i].alive)
                population[i].age++;
        }
        iteration_n++;
        if (metaParams.evolution_stat)
        {
            float max_val = 0;
            int max_pos = 0;
            for (int i=0;i<heaven.size();i++)
            {
                if (heaven[i].metric > max_val)
                {
                    max_pos = i;
                    max_val = heaven[i].metric;
                }
            }
            crio_camera.push_back(heaven[max_pos]);
        }
        debug("iteration %d Pop: %d Best: %.4f\n", iteration_n, current_population_size, best_metric_ever);
    }

    prepare_best_params(best_results);
}

bool GeneticAlgorithm::should_exit()
{
    iter_start = std::chrono::steady_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(iter_start - t_start).count();
    time_spent_sec = 0.001*time;
    return (time_spent_sec > exitConditions.time_elapsed_seconds || iteration_n >= exitConditions.generations
    || func_called >= exitConditions.function_calculated || best_metric_ever >= exitConditions.function_reached);
}

void add_all_ids(std::set<int> &ids, int id, std::vector<glm::ivec3> &all_births)
{
    ids.emplace(id);
    for (auto &v : all_births)
    {
        if (v.z == id)
        {
            add_all_ids(ids, v.x, all_births);
            add_all_ids(ids, v.y, all_births);
        }
    }
}

void GeneticAlgorithm::prepare_best_params(std::vector<std::pair<float, ParameterList>> &best_results)
{
    for (int i=0;i<population.size();i++)
    {
        if (population[i].alive)
            kill_creature(i);
    }
    debug("heaven popultion %d best metric %.4f\n", heaven.size(), heaven[0].metric);
    debug("heaven [ ");
    for (auto &creature : heaven)
    {
        debug("%.4f ", creature.metric);
    }
    debug("]\n");
    for (auto &creature : heaven)
    {
        best_results.emplace_back();
        best_results.back().first = creature.metric;
        best_results.back().second = original_param_list;
        best_results.back().second.from_simple_list(creature.main_genome);
        //best_results.back().second.print();
    }
    for (auto &creature : crio_camera)
    {
        best_results.emplace_back();
        best_results.back().first = creature.metric;
        best_results.back().second = original_param_list;
        best_results.back().second.from_simple_list(creature.main_genome);
        //best_results.back().second.print();
    }

    if (metaParams.evolution_stat)
    {
        DebugGraph g_test;
        g_test.add_node(DebugGraph::Node(glm::vec2(0,0), glm::vec3(1,0,0),0.1));
        g_test.add_node(DebugGraph::Node(glm::vec2(-1,1), glm::vec3(0,1,0),0.15));
        g_test.add_node(DebugGraph::Node(glm::vec2(1,1), glm::vec3(0,0,1),0.2));
        g_test.add_edge(DebugGraph::Edge(0,1,0.02));
        g_test.add_edge(DebugGraph::Edge(1,2,0.03));
        g_test.add_edge(DebugGraph::Edge(0,2,0.04));
        g_test.save_as_image("graph_test", 300, 300);

        DebugGraph stat_g;
        std::map<int, int> pos_by_id;
        int cnt = 0;
        for (auto &p : stat.all_results)
        {
            int iter = p.first/1000;
            int n = p.first % 1000;
            stat_g.add_node(DebugGraph::Node(glm::vec2(iter, 0.1*n), glm::vec3(1 - p.second, p.second, 0), 0.045));
            pos_by_id.emplace(p.first, cnt);
            cnt++;
        }

        int n = 0;
        for (auto &c : heaven)
        {
            DebugGraph stat_gn = stat_g;
            std::set<int> ids;
            add_all_ids(ids, c.id, stat.all_births);
            glm::vec3 color = glm::vec3(n/4%2, n/2%2, n%2);
            if (n % 8 == 0)
            {
                color = glm::vec3(0.5, 0.5, 0.5);
            }
            for (auto &id : ids)
            {
                for (auto &v : stat.all_births)
                {
                    if (v.z == id)
                    {
                        auto itA = pos_by_id.find(v.x);
                        auto itB = pos_by_id.find(v.y);
                        auto itC = pos_by_id.find(v.z);
                        if (itA != pos_by_id.end() && itB != pos_by_id.end() && itC != pos_by_id.end())
                        {
                            stat_gn.add_edge(DebugGraph::Edge(itA->second, itC->second, 0.005, color));
                            stat_gn.add_edge(DebugGraph::Edge(itB->second, itC->second, 0.005, color));
                        }                        
                    }
                }
            }
            n++;
            stat_gn.save_as_image("graph_gen_" + std::to_string(n), 4000, 4000);
        }

        
        for (auto &v : stat.all_births)
        {
            auto itA = pos_by_id.find(v.x);
            auto itB = pos_by_id.find(v.y);
            auto itC = pos_by_id.find(v.z);
            if (itA != pos_by_id.end() && itB != pos_by_id.end() && itC != pos_by_id.end())
            {
                stat_g.add_edge(DebugGraph::Edge(itA->second, itC->second, 0.005));
                stat_g.add_edge(DebugGraph::Edge(itB->second, itC->second, 0.005));
            }

        }
        
        stat_g.save_as_image("graph_gen", 5000, 5000);
    }
}

GeneticAlgorithm::Genome GeneticAlgorithm::random_genes()
{
    Genome g;
    for (auto &p : parametersMask.categorialParameters)
        g.push_back(p.second.possible_values[(int)urandi(0, p.second.possible_values.size())]);
    for (auto &p : parametersMask.ordinalParameters)
        g.push_back((int)(p.second.min_val + urand()*(p.second.max_val - p.second.min_val)));
    for (auto &p : parametersMask.continuousParameters)
        g.push_back((p.second.min_val + urand()*(p.second.max_val - p.second.min_val)));
    return g;
}

void GeneticAlgorithm::initialize_population()
{
    population = std::vector<Creature>(metaParams.max_population_size, Creature());
    for (int i=0;i<metaParams.initial_population_size;i++)
    {
        population[i].main_genome = random_genes();
        for (int j=1;j<metaParams.n_ploid_genes;j++)
            population[i].other_genomes.push_back(random_genes());
        population[i].alive = true;
        population[i].max_age = metaParams.max_age;
        population[i].id = next_id.fetch_add(1);
    }
    current_population_size = metaParams.initial_population_size;
}

void GeneticAlgorithm::mutation(Genome &G, float mutation_power, int mutation_genes_count)
{
    for (int gene = 0; gene < mutation_genes_count; gene++)
    {
        bool found = false;
        int tries = 0;
        while (!found && tries < 100)
        {
            //found = true;

            int pos = urandi(0, G.size());
            //logerr("mutation pos %d G size %d %f", pos, G.size(), (float)urandi(0, G.size()));
            int g_pos = pos;
            if (pos < parametersMask.categorialParameters.size())
            {
                if (!parametersMask.categorialParameters[pos].second.fixed())
                {
                    if (urand() < mutation_power)
                    {
                        int val_pos = urandi(0, parametersMask.categorialParameters[pos].second.possible_values.size());
                        G[g_pos] = parametersMask.categorialParameters[pos].second.possible_values[val_pos];
                    }
                    found = true;
                }
            }
            else if (pos < parametersMask.categorialParameters.size() + parametersMask.ordinalParameters.size())
            {
                pos -= parametersMask.categorialParameters.size();
                if (!parametersMask.ordinalParameters[pos].second.fixed())
                {
                    float len = mutation_power*(parametersMask.ordinalParameters[pos].second.max_val - parametersMask.ordinalParameters[pos].second.min_val);
                    float from = CLAMP(G[g_pos] - len, parametersMask.ordinalParameters[pos].second.min_val, parametersMask.ordinalParameters[pos].second.max_val);
                    float to = CLAMP(G[g_pos] + len, parametersMask.ordinalParameters[pos].second.min_val, parametersMask.ordinalParameters[pos].second.max_val);
                    float val = urand(from,to);
                    val = CLAMP(val, parametersMask.ordinalParameters[pos].second.min_val, parametersMask.ordinalParameters[pos].second.max_val);
                    G[g_pos] = val;
                    found = true;
                }
            }
            else
            {
                pos -= parametersMask.categorialParameters.size() + parametersMask.ordinalParameters.size();
                //logerr("continuous pos %d mutation", pos);
                if (!parametersMask.continuousParameters[pos].second.fixed())
                {
                    float len = mutation_power*(parametersMask.continuousParameters[pos].second.max_val - parametersMask.continuousParameters[pos].second.min_val);
                    float from = CLAMP(G[g_pos] - len, parametersMask.continuousParameters[pos].second.min_val, parametersMask.continuousParameters[pos].second.max_val);
                    float to = CLAMP(G[g_pos] + len, parametersMask.continuousParameters[pos].second.min_val, parametersMask.continuousParameters[pos].second.max_val);
                    float val = urand(from,to);
                    val = CLAMP(val, parametersMask.continuousParameters[pos].second.min_val, parametersMask.continuousParameters[pos].second.max_val);
                    G[g_pos] = val;
                    //logerr("applied pos val %d %f in [%f %f]",pos, val, parametersMask.continuousParameters[pos].second.min_val, parametersMask.continuousParameters[pos].second.max_val);
                    found = true;
                }
            }
            tries++;
        }
    }
}

void GeneticAlgorithm::find_pairs(int cnt, std::vector<std::pair<int, int>> &pairs)
{
    float fitsum = 0;
    for (auto &c : population)
    {
        if (c.alive)
            fitsum += c.fitness;
    }
    int vals[2];
    for (int n=0;n<cnt;n++)
    {
        vals[0] = -1;
        for (int k=0;k<2;k++)
        {
            bool found = false;
            while (!found)
            {
                float v = urand(0, fitsum);
                for (int i=0;i<population.size();i++)
                {
                    if (population[i].alive)
                    {
                        if (v < population[i].fitness && i != vals[0])
                        {
                            vals[k] = i;
                            found = true;
                            break;
                        }
                        else
                        {
                            v -= population[i].fitness;
                        }
                    }
                }
            }
        }
        pairs.push_back(std::pair<int, int>(vals[0], vals[1]));
        //logerr("parents %d %d chosen", vals[0], vals[1]);
    }
}

void GeneticAlgorithm::kill_creature(int n)
{
    if (heaven.size() < metaParams.best_genoms_count)
    {
        heaven.push_back(population[n]);
        best_metric_ever = MAX(best_metric_ever, population[n].metric);
    }
    else
    {
        for (int i=0;i<heaven.size();i++)
        {
            if (heaven[i].metric < population[n].metric)
            {
                heaven[i] = population[n];
                best_metric_ever = MAX(best_metric_ever, population[n].metric);
                break;
            }
        }
    }
    population[n].alive = false;
    population[n].fitness = -1;
    current_population_size--;
}

void GeneticAlgorithm::kill_old()
{
    for (int i=0;i<population.size();i++)
    {
        if (population[i].age >= population[i].max_age)
            kill_creature(i);
    }
}

void GeneticAlgorithm::kill_weak(float percent)
{
    std::sort(population.begin(), population.end(), 
              [&](const Creature& a, const Creature& b) -> bool{return a.fitness < b.fitness;});
    
    int cnt_to_kill = percent*current_population_size;
    int killed = 0;
    for (int i=0;i<population.size();i++)
    {
        //logerr("%d) %d fitness %f",i,population[i].alive, population[i].fitness);
    }
    for (int i=0;i<population.size();i++)
    {
        if (population[i].alive)
        {
           //logerr("killed weak %d metric %f", i, population[i].metric);
           kill_creature(i); 
           killed++;
        }
        if (killed >= cnt_to_kill)
            break;
    }
}

void GeneticAlgorithm::crossingover(Genome &A, Genome &B, Genome &res)
{
    res = A;
    for (int i=0;i<A.size();i++)
    {
        if (urand() > 0.5)
            res[i] = B[i];
    }
}

void GeneticAlgorithm::make_children(Creature &A, Creature &B, int count)
{
    for (int ch=0;ch<count;ch++)
    {
        int pos = -1;
        for (int i=0;i<population.size();i++)
        {
            if (!population[i].alive)
            {
                pos = i;
                population[i] = Creature();
                break;
            }
        }
        population[pos].alive = true;
        population[pos].age = 0;
        population[pos].max_age = metaParams.max_age;
        population[pos].id = next_id.fetch_add(1);
        if (metaParams.evolution_stat)
        {
            stat.all_births.push_back(glm::ivec3(A.id, B.id, population[pos].id));
        }
        //logerr("child %d created", pos);
        current_population_size++;

        if (metaParams.n_ploid_genes == 1)
        {
            crossingover(A.main_genome, B.main_genome, population[pos].main_genome);
        }
        else
        {
            //TODO: implement diploid genomes
        }
        if (urand() < 0.2)
            mutation(population[pos].main_genome, 1.0, urandi(1, 0.6*free_parameters_cnt));
        else
            mutation(population[pos].main_genome, 0.33, urandi(1, MIN(3,free_parameters_cnt)));
        for (auto &g : population[pos].other_genomes)
        {
            mutation(g, 0.5, urandi(1, MIN(3,free_parameters_cnt)));
        }
    }
}

void GeneticAlgorithm::calculate_metric()
{
    std::vector<ParameterList> params;
    std::vector<int> positions;
    int i=0;
    for (auto &p : population)
    {
        if (p.alive && p.metric < 0)
        {
            params.push_back(original_param_list);
            params.back().from_simple_list(p.main_genome);
            positions.push_back(i);
        }
        i++;
    }

    std::vector<float> metrics = function(params);
    for (int i=0;i<metrics.size();i++)
    {
        population[positions[i]].metric = metrics[i];
        //logerr("metric[%d] = %f", positions[i], metrics[i]);
    }
}
void GeneticAlgorithm::recalculate_fitness()
{
    for (auto &p : population)
    {
        if (p.alive && p.metric < metaParams.dead_at_birth_thr)
        {
            p.alive = false;
            current_population_size--;
        }
        if (p.alive)
            p.fitness = SQR(p.metric);
        else
            p.fitness = -1;
    }
    /*
    std::sort(population.begin(), population.end(), 
              [&](const Creature& a, const Creature& b) -> bool{return a.fitness < b.fitness;});
    float min_v = 0.025;
    float step = 0.025;
    int n = 0;

    for (auto &p : population)
    {
        if (p.alive && p.fitness > 0)
        {
            p.fitness = MAX(min_v, 1 - step*n);
            n++;
        }
    }
    */
   if (metaParams.evolution_stat)
   {
       for (auto &c : population)
       {
           if (c.alive)
           {
               stat.all_results.emplace(c.id, c.metric);
           }
       }
   }
}