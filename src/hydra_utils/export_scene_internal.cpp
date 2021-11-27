
#include "graphics_utils/terrain.h"
#include "hydra_scene_exporter.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

#include <FreeImage.h>
#include <GLFW/glfw3.h>
#include "HydraAPI/hydra_api/HydraAPI.h"
#include "HydraAPI/hydra_api/HydraXMLVerify.h"
#include "HydraAPI/utils/mesh_utils.h"
#include "HydraAPI/hydra_api/LiteMath.h"

#include "core/scene.h"
#include "graphics_utils/modeling.h"
namespace hlm = HydraLiteMath;
using pugi::xml_node;
extern GLFWwindow* g_window;

  static inline void WriteMatrix4x4(pugi::xml_node a_node, const wchar_t* a_attrib_name, float a_value[16])
  {
    std::wstringstream outStream;
    outStream << a_value[0]  << L" " << a_value[1]  << L" " << a_value[2]  << L" " << a_value[3]  << L" "
              << a_value[4]  << L" " << a_value[5]  << L" " << a_value[6]  << L" " << a_value[7]  << L" "
              << a_value[8]  << L" " << a_value[9]  << L" " << a_value[10] << L" " << a_value[11] << L" "
              << a_value[12] << L" " << a_value[13] << L" " << a_value[14] << L" " << a_value[15];

    a_node.attribute(a_attrib_name).set_value(outStream.str().c_str());
  }

void initGLIfNeeded(int a_width, int a_height, const char* name);
void model_to_simple_mesh(Model &model, SimpleMesh &mesh)
{
  mesh.vPos = std::move(model.positions);
  mesh.vNorm = std::move(model.normals);

  mesh.triIndices = std::vector<int>(model.indices.size(), 0);
  memcpy(mesh.triIndices.data(), model.indices.data(), mesh.triIndices.size() * sizeof(int));

  mesh.vTexCoord = std::vector<GLfloat>(model.colors.size() / 2, 0);
  for (int i = 0; i < model.colors.size() / 4; i++)
  {
    mesh.vTexCoord[2 * i] = model.colors[4 * i];
    mesh.vTexCoord[2 * i + 1] = model.colors[4 * i + 1];
  }
}
void heightmap_to_simple_mesh(Heightmap &h, SimpleMesh &mesh)
{
  Visualizer vis;
  Model model;
  vis.heightmap_to_model(h, &model, glm::vec2(1000, 1000), glm::vec2(1000, 1000), 10, 0);
  model_to_simple_mesh(model, mesh);
}
void packed_branch_to_simple_mesh(SimpleMesh &mesh, GrovePacked *source, InstancedBranch &branch, 
                                  int up_to_level, bool need_leaves, int wood_mat_id, int leaves_mat_id)
{
  if (branch.branches.empty())
        return;
    //clusterization process guarantees that type of all branches in instance
    //will be the same
    Model model;
    //uint type = source->instancedCatalogue.get(branch.branches.front()).type_id;

    Visualizer v = Visualizer();
    uint ind_offset = model.indices.size();
    uint verts = model.positions.size();
    for (int id : branch.branches)
    {
        if (id < 0)
        {
            logerr("invalid id = %d", id);
            continue;//invalid id - TODO fix it
        }
        PackedBranch &b = source->instancedCatalogue.get(id);
        if (b.level <= up_to_level && !b.joints.empty())
            v.packed_branch_to_model(b, &model, false, up_to_level);
    }
    uint l_ind_offset = model.indices.size();
    uint l_verts = model.positions.size();

    verts = l_verts;
    if (need_leaves)
    {
        int type_slice = 0;
        for (int id : branch.branches)
        {
            if (id < 0)
            {
                logerr("invalid id = %d", id);
                continue;//invalid id - TODO fix it
            }
            PackedBranch &b = source->instancedCatalogue.get(id);
            if (!b.joints.empty())
                v.packed_branch_to_model(b, &model, true, up_to_level);
        }
    }
    uint l_end = model.indices.size();
    l_verts = model.positions.size();
    int count = l_ind_offset - ind_offset;
    int l_count = l_end - l_ind_offset;

    if (model.positions.size() % 3 || model.normals.size() % 3 || model.indices.size() % 3
        || model.colors.size() % 4)
    {
      logerr("error creating model from packed branch");
    }
    else
    {
      mesh.vPos = std::move(model.positions);
      mesh.vNorm = std::move(model.normals);
      
      mesh.triIndices = std::vector<int>(model.indices.size(),0);
      memcpy(mesh.triIndices.data(),model.indices.data(),mesh.triIndices.size()*sizeof(int));
      
      mesh.vTexCoord = std::vector<GLfloat>(model.colors.size()/2, 0);
      for (int i = 0;i<model.colors.size()/4;i++)
      {
        mesh.vTexCoord[2*i] = model.colors[4*i];
        mesh.vTexCoord[2*i + 1] = model.colors[4*i + 1];
      }

      mesh.matIndices = std::vector<int>(model.indices.size()/3,wood_mat_id);

      for (int i=count/3;i<model.indices.size()/3;i++)
      {
        mesh.matIndices[i] = leaves_mat_id;
      }
    }
}
bool HydraSceneExporter::export_internal2(std::string directory, Scene &scene, Block &export_settings)
{
  const int DEMO_WIDTH  = 1024;
  const int DEMO_HEIGHT = 1024;
  
  hrErrorCallerPlace(L"demo_02_load_obj");
  hrSceneLibraryOpen(L"demos/demo_test", HR_WRITE_DISCARD);

  HRTextureNodeRef texWood = hrTexture2DCreateFromFile(L"data/textures/wood.jpg");
  HRTextureNodeRef texLeaf = hrTexture2DCreateFromFile(L"data/textures/leaf.png");
  HRTextureNodeRef texGrass = hrTexture2DCreateFromFile(L"data/textures/grass_atlas.bmp");
  HRTextureNodeRef texLeafOpacity = hrTexture2DCreateFromFile(L"data/textures/leaf_opacity.png");
  HRTextureNodeRef texGrassOpacity = hrTexture2DCreateFromFile(L"data/textures/grass_atlas_op.bmp");
  HRTextureNodeRef texTerrain = hrTexture2DCreateFromFile(L"data/textures/terrain4.jpg");

  HRTextureNodeRef cube[6] = {
      hrTexture2DCreateFromFile(L"data/textures/clouds1/clouds1_east.bmp"),
      hrTexture2DCreateFromFile(L"data/textures/clouds1/clouds1_west.bmp"),
      hrTexture2DCreateFromFile(L"data/textures/clouds1/clouds1_up.bmp"),
      hrTexture2DCreateFromFile(L"data/textures/clouds1/clouds1_down.bmp"),
      hrTexture2DCreateFromFile(L"data/textures/clouds1/clouds1_north.bmp"),
      hrTexture2DCreateFromFile(L"data/textures/clouds1/clouds1_south.bmp"),
    };

  HRTextureNodeRef texEnv = HRUtils::Cube2SphereLDR(cube);


  HRMaterialRef mat_land = hrMaterialCreate(L"land_material");
  hrMaterialOpen(mat_land, HR_WRITE_DISCARD);
  {
    auto matNode = hrMaterialParamNode(mat_land);
    auto diff = matNode.append_child(L"diffuse");
    diff.append_attribute(L"brdf_type").set_value(L"lambert");

    auto color = diff.append_child(L"color");
    color.append_attribute(L"val").set_value(L"0.6 0.6 0.6");
    color.append_attribute(L"tex_apply_mode").set_value(L"multiply");
    auto texNode = hrTextureBind(texTerrain, color);

    VERIFY_XML(matNode);
  }
  hrMaterialClose(mat_land);

  HRMaterialRef mat_wood = hrMaterialCreate(L"mat_wood");
  hrMaterialOpen(mat_wood, HR_WRITE_DISCARD);
  {
    auto matNode = hrMaterialParamNode(mat_wood);
    auto diff = matNode.append_child(L"diffuse");
    diff.append_attribute(L"brdf_type").set_value(L"lambert");

    auto color = diff.append_child(L"color");
    color.append_attribute(L"val").set_value(L"1.0 1.0 1.0");
    color.append_attribute(L"tex_apply_mode").set_value(L"replace");
    auto texNode = hrTextureBind(texWood, color);

    VERIFY_XML(matNode);
  }
  hrMaterialClose(mat_wood);

  HRMaterialRef mat_leaf = hrMaterialCreate(L"mat_leaf");
  hrMaterialOpen(mat_leaf, HR_WRITE_DISCARD);
  {
    auto matNode = hrMaterialParamNode(mat_leaf);
  auto diff = matNode.append_child(L"diffuse");
    diff.append_attribute(L"brdf_type").set_value(L"lambert");

    auto color = diff.append_child(L"color");
    color.append_attribute(L"val").set_value(L"1.0 1.0 1.0");
    color.append_attribute(L"tex_apply_mode").set_value(L"multiply");
    auto texNode = hrTextureBind(texLeaf, color);

    auto opacity = matNode.append_child(L"opacity");
    opacity.append_child(L"skip_shadow").append_attribute(L"val").set_value(0);
    auto texNodeOp = hrTextureBind(texLeafOpacity, opacity);
   
    auto transl = matNode.append_child(L"translucency");
    auto colorTrans = transl.append_child(L"color");
    colorTrans.append_attribute(L"val").set_value(L"1.0 1.0 1.0");
    colorTrans.append_attribute(L"tex_apply_mode").set_value(L"multiply");

    auto texNodeTrans = hrTextureBind(texLeaf, transl);

    VERIFY_XML(matNode);
  }
  hrMaterialClose(mat_leaf);

  HRMaterialRef mat_grass = hrMaterialCreate(L"mat_grass");
  hrMaterialOpen(mat_grass, HR_WRITE_DISCARD);
  {
    auto matNode = hrMaterialParamNode(mat_grass);

    auto opacity = matNode.append_child(L"opacity");
    opacity.append_child(L"skip_shadow").append_attribute(L"val").set_value(0);
    auto texNodeOp = hrTextureBind(texGrassOpacity, opacity);
    
    auto diff = matNode.append_child(L"diffuse");
    diff.append_attribute(L"brdf_type").set_value(L"lambert");
    auto color = diff.append_child(L"color");
    color.append_attribute(L"val").set_value(L"1.0 1.0 1.0");
    color.append_attribute(L"tex_apply_mode").set_value(L"multiply");
    auto texNode = hrTextureBind(texGrass, diff);

    auto transl = matNode.append_child(L"translucency");
    auto colorTrans = transl.append_child(L"color");
    colorTrans.append_attribute(L"val").set_value(L"1.0 1.0 1.0");
    colorTrans.append_attribute(L"tex_apply_mode").set_value(L"multiply");
    auto texNodeTrans = hrTextureBind(texGrass, transl);

    VERIFY_XML(matNode);
  }
  hrMaterialClose(mat_grass);

  SimpleMesh terrain;
  heightmap_to_simple_mesh(*(scene.heightmap), terrain);
  HRMeshRef terrainMeshRef = hrMeshCreate(L"terrain");
  hrMeshOpen(terrainMeshRef, HR_TRIANGLE_IND3, HR_WRITE_DISCARD);
  {
    hrMeshVertexAttribPointer3f(terrainMeshRef, L"pos",      &terrain.vPos[0]);
    hrMeshVertexAttribPointer3f(terrainMeshRef, L"norm",     &terrain.vNorm[0]);
    hrMeshVertexAttribPointer2f(terrainMeshRef, L"texcoord", &terrain.vTexCoord[0]);
    
    hrMeshMaterialId(terrainMeshRef, mat_land.id);
    logerr("tri id 1 %d",int(terrain.triIndices.size()));
    hrMeshAppendTriangles3(terrainMeshRef, int(terrain.triIndices.size()), &terrain.triIndices[0]);
  }
  hrMeshClose(terrainMeshRef);

  std::vector<HRMeshRef> branches;
  std::vector<SimpleMesh> meshes;
  std::vector<InstanceDataArrays *> IDAs;

  for (auto &pb : scene.grove.instancedBranches)
  {
    meshes.emplace_back();
    SimpleMesh &br = meshes.back();
    packed_branch_to_simple_mesh(br,&(scene.grove), pb, 1000, true, mat_wood.id, mat_leaf.id);
    if (br.triIndices.size() > 0)
    {
      std::wstring name = L"branch" + std::to_wstring(branches.size());
      IDAs.push_back(&(pb.IDA));
      branches.push_back(hrMeshCreate(name.c_str()));
      hrMeshOpen(branches.back(), HR_TRIANGLE_IND3, HR_WRITE_DISCARD);
      {
        hrMeshVertexAttribPointer3f(branches.back(), L"pos",      &br.vPos[0]);
        hrMeshVertexAttribPointer3f(branches.back(), L"norm",     &br.vNorm[0]);
        hrMeshVertexAttribPointer2f(branches.back(), L"texcoord", &br.vTexCoord[0]);
        hrMeshPrimitiveAttribPointer1i(branches.back(), L"mind", br.matIndices.data());

        logerr("tri id 2 %d",int(br.triIndices.size()));
        hrMeshAppendTriangles3(branches.back(), int(br.triIndices.size()), br.triIndices.data());
      }
      hrMeshClose(branches.back());
    }
  }

    std::vector<Model *> grass_models;
    std::vector<SimpleMesh> grass_meshes;
    std::vector<HRMeshRef> grasses;

    std::vector<int> inst_offsets;
    std::vector<int> inst_counts;
    std::vector<float> vertexes = {-0.5,0,0, -0.5,1,0, 0.5,0,0, 0.5,1,0,   0,0,-0.5, 0,1,-0.5, 0,0,0.5, 0,1,0.5};
    std::vector<float> tc = {0,1,0,0, 0,0,0,0, 1,1,0,0, 1,0,0,0, 0,1,0,0, 0,0,0,0, 1,1,0,0, 1,0,0,0};
    std::vector<float> normals = {0,0,1, 0,0,1, 0,0,1, 0,0,1, 1,0,0, 1,0,0, 1,0,0, 1,0,0};
    std::vector<GLuint> indices = {0, 1, 3, 2, 0, 3, 4,5,7, 6,4,7};

    glm::vec4 tex_transform = glm::vec4(1,1,0,0);

    int total_instances = 0;
    
    for (auto &p : scene.grass.grass_instances)
    {
        tex_transform = scene.grass.grass_textures.tc_transform(p.first);
        grass_models.push_back(new Model());
        grass_models.back()->positions = vertexes;
        grass_models.back()->colors = tc;
        for (int i=0;i<grass_models.back()->colors.size();i+=4)
        {
            grass_models.back()->colors[i] = tex_transform.x*(grass_models.back()->colors[i] + tex_transform.z);
            grass_models.back()->colors[i + 1] = 1 - tex_transform.y*(grass_models.back()->colors[i + 1] + tex_transform.w);
        }
        grass_models.back()->normals = normals;
        grass_models.back()->indices = indices;
        grass_models.back()->update();
        inst_offsets.push_back(total_instances);
        inst_counts.push_back(p.second.size());
        total_instances += p.second.size();
    }

    glm::mat4 *matrices = new glm::mat4[total_instances];
    int i=0;
    for (auto &p : scene.grass.grass_instances)
    {
        for (auto &in : p.second)
        {
            matrices[i] = glm::scale(
                          glm::rotate(glm::translate(glm::mat4(1.0f),glm::vec3(in.pos)),
                                      in.rot_y,glm::vec3(0,1,0)), glm::vec3(in.size));
            i++;
        }
    }
    
    for (auto *model : grass_models)
    {
      std::wstring name = L"grass" + std::to_wstring(grasses.size());
      grass_meshes.emplace_back();
      SimpleMesh &mesh = grass_meshes.back();
      model_to_simple_mesh(*model, mesh);

      grasses.push_back(hrMeshCreate(name.c_str()));
      hrMeshOpen(grasses.back(), HR_TRIANGLE_IND3, HR_WRITE_DISCARD);
      {
        hrMeshVertexAttribPointer3f(grasses.back(), L"pos",      &mesh.vPos[0]);
        hrMeshVertexAttribPointer3f(grasses.back(), L"norm",     &mesh.vNorm[0]);
        hrMeshVertexAttribPointer2f(grasses.back(), L"texcoord", &mesh.vTexCoord[0]);
        
        hrMeshMaterialId(grasses.back(), mat_grass.id);
        logerr("tri id 3 %d",int(mesh.triIndices.size()));
        hrMeshAppendTriangles3(grasses.back(), int(mesh.triIndices.size()), mesh.triIndices.data());
      }
      hrMeshClose(grasses.back());

      delete model;
    }

  HRLightRef rectLight = hrLightCreate(L"my_area_light");
  hrLightOpen(rectLight, HR_WRITE_DISCARD);
  {
    pugi::xml_node lightNode = hrLightParamNode(rectLight);

    lightNode.attribute(L"type").set_value(L"area");
    lightNode.attribute(L"shape").set_value(L"rect");
    lightNode.attribute(L"distribution").set_value(L"diffuse");

    pugi::xml_node sizeNode = lightNode.append_child(L"size");

    sizeNode.append_attribute(L"half_length") = 1.0f;
    sizeNode.append_attribute(L"half_width")  = 1.0f;

    pugi::xml_node intensityNode = lightNode.append_child(L"intensity");

    intensityNode.append_child(L"color").append_attribute(L"val")      = L"1 1 1";
    intensityNode.append_child(L"multiplier").append_attribute(L"val") = 1.5f;

    VERIFY_XML(lightNode);
  }
  hrLightClose(rectLight);

  HRLightRef directLight = hrLightCreate(L"my_direct_light");
  hrLightOpen(directLight, HR_WRITE_DISCARD);
  {
    pugi::xml_node lightNode = hrLightParamNode(directLight);

    lightNode.attribute(L"type").set_value(L"directional");
    lightNode.attribute(L"shape").set_value(L"point");

    pugi::xml_node sizeNode = lightNode.append_child(L"size");

    sizeNode.append_child(L"inner_radius").append_attribute(L"val") = 1.0f;
    sizeNode.append_child(L"outer_radius").append_attribute(L"val") = 2.0f;

    pugi::xml_node intensityNode = lightNode.append_child(L"intensity");

    intensityNode.append_child(L"color").append_attribute(L"val") = L"1 1 1";
    intensityNode.append_child(L"multiplier").append_attribute(L"val") = 2.0f * PI;
  }
  hrLightClose(directLight);

  HRLightRef sky = hrLightCreate(L"sky");
  hrLightOpen(sky, HR_WRITE_DISCARD);
  {
    auto lightNode = hrLightParamNode(sky);
    lightNode.attribute(L"type").set_value(L"sky");
	  lightNode.attribute(L"distribution").set_value(L"map");
    auto intensityNode = lightNode.append_child(L"intensity");
    intensityNode.append_child(L"color").append_attribute(L"val").set_value(L"1 1 1");
    intensityNode.append_child(L"multiplier").append_attribute(L"val").set_value(L"1.0");

	  auto texNode = hrTextureBind(texEnv, intensityNode.child(L"color"));
    //texNode.append_attribute(L"input_gamma").set_value(1.0f);
    //texNode.append_attribute(L"input_alpha").set_value(L"rgb");

	  VERIFY_XML(lightNode);
  }
  hrLightClose(sky);
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  // camera
  //
  HRCameraRef camRef = hrCameraCreate(L"my camera");
  
  hrCameraOpen(camRef, HR_WRITE_DISCARD);
  {
    xml_node camNode = hrCameraParamNode(camRef);
    
    camNode.append_child(L"fov").text().set(L"90");
    camNode.append_child(L"nearClipPlane").text().set(L"0.01");
    camNode.append_child(L"farClipPlane").text().set(L"1000.0");
    
    camNode.append_child(L"up").text().set(L"0 1 0");
    camNode.append_child(L"position").text().set(L"-140 80 20");
    camNode.append_child(L"look_at").text().set(L"0 40 0");
  
    VERIFY_XML(camNode);
  }
  hrCameraClose(camRef);
  
  // set up render settings
  //
  HRRenderRef renderRef = hrRenderCreate(L"HydraModern"); // "HydraModern" is our main renderer name
  hrRenderEnableDevice(renderRef, 0, true);
  
  hrRenderOpen(renderRef, HR_WRITE_DISCARD);
  {
    pugi::xml_node node = hrRenderParamNode(renderRef);
    
    node.append_child(L"width").text()  = DEMO_WIDTH;
    node.append_child(L"height").text() = DEMO_HEIGHT;
    
    node.append_child(L"method_primary").text()   = L"pathtracing";
    node.append_child(L"method_secondary").text() = L"pathtracing";
    node.append_child(L"method_tertiary").text()  = L"pathtracing";
    node.append_child(L"method_caustic").text()   = L"pathtracing";
    
    node.append_child(L"trace_depth").text()      = 4;
    node.append_child(L"diff_trace_depth").text() = 3;
    node.append_child(L"maxRaysPerPixel").text()  = 128;
    node.append_child(L"qmc_variant").text()      = (HYDRA_QMC_DOF_FLAG | HYDRA_QMC_MTL_FLAG | HYDRA_QMC_LGT_FLAG); // enable all of them, results to '7'
  }
  hrRenderClose(renderRef);
  
  // create scene
  //
  HRSceneInstRef scnRef = hrSceneCreate(L"my scene");
  
  const float DEG_TO_RAD = float(3.14159265358979323846f) / 180.0f;
  
  hrSceneOpen(scnRef, HR_WRITE_DISCARD);
  {
    auto mind = hlm::scale4x4(hlm::float3(1,1,1));
    hrMeshInstance(scnRef, terrainMeshRef, mind.L());
    
    for (int i=0;i<branches.size();i++)
    {
      for (auto &mat : IDAs[i]->transforms)
      {
        const float mat_vals2[16] = {mat[0][0],mat[1][0],mat[2][0],mat[3][0],
                                    mat[0][1],mat[1][1],mat[2][1],mat[3][1],
                                    mat[0][2],mat[1][2],mat[2][2],mat[3][2],
                                    mat[0][3],mat[1][3],mat[2][3],mat[3][3]};
        auto m = hlm::float4x4(mat_vals2);
        hrMeshInstance(scnRef, branches[i], m.L());
      }
    }
    
    for (int i=0;i<inst_counts.size();i++)
    {
      for (int in = inst_offsets[i];in < inst_offsets[i] + inst_counts[i];in++)
      {
        auto &mat = matrices[in];
        const float mat_vals[16] = {mat[0][0],mat[1][0],mat[2][0],mat[3][0],
                                    mat[0][1],mat[1][1],mat[2][1],mat[3][1],
                                    mat[0][2],mat[1][2],mat[2][2],mat[3][2],
                                    mat[0][3],mat[1][3],mat[2][3],mat[3][3]};
        auto m = hlm::float4x4(mat_vals);
        hrMeshInstance(scnRef, grasses[i], m.L());
        //break;
      }
      logerr("added %d grass instances", inst_counts[i]);
    }
    
    delete matrices;
    hrLightInstance(scnRef, sky, mind.L());
    auto mres = hlm::mul(hlm::rotate_Z_4x4(1.0f * DEG_TO_RAD), hlm::translate4x4({0.0f, 100.0f, 0.0f}));
    hrLightInstance(scnRef, directLight, mres.L());
  }
  hrSceneClose(scnRef);
  
  hrFlush(scnRef, renderRef, camRef);
  
  //////////////////////////////////////////////////////// opengl
  std::vector<int32_t> image(DEMO_WIDTH*DEMO_HEIGHT);
  initGLIfNeeded(DEMO_WIDTH,DEMO_HEIGHT, "load 'obj.' file demo");
  glViewport(0,0,DEMO_WIDTH,DEMO_HEIGHT);
  //////////////////////////////////////////////////////// opengl
  
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    HRRenderUpdateInfo info = hrRenderHaveUpdate(renderRef);
    
    if (info.haveUpdateFB)
    {
      auto pres = std::cout.precision(2);
      std::cout << "rendering progress = " << info.progress << "% \r"; std::cout.flush();
      std::cout.precision(pres);
      
      hrRenderGetFrameBufferLDR1i(renderRef, DEMO_WIDTH, DEMO_HEIGHT, &image[0]);
  
      //////////////////////////////////////////////////////// opengl
      glDisable(GL_TEXTURE_2D);
      glDrawPixels(DEMO_WIDTH, DEMO_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
      
      glfwSwapBuffers(g_window);
      glfwPollEvents();
      //////////////////////////////////////////////////////// opengl
    }
    
    if (info.finalUpdate)
      break;
  }
  
  hrRenderSaveFrameBufferLDR(renderRef, L"demos/demo_test/z_out.png");
  return true;
}
