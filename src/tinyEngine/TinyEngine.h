#pragma once
#include <functional>
#include <initializer_list>

#include "imgui/imgui.h"                    //Interface Dependencies
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#include <GL/glew.h>                                //Rendering Dependencies
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include <sstream>                                  //File / Console IO
#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "utility/texture.h"              //Utility Classes
#include "utility/shader.h"
#include "utility/model.h"
#include "utility/instance.h"
#include "utility/target.h"

#include "view.h"
#include "event.h"
#include "audio.h"

#include <chrono>

/* TINY ENGINE */

namespace Tiny {

static View view;           //Window and Interface  (Requires Initialization)
static Event event;         //Event Handler
static Audio audio;         //Audio Processor       (Requires Initialization)

bool window(std::string windowName, int width, int height);
void quit();

template<typename F, typename... Args>
void loop(F function, Args&&... args){
  while(!event.quit){

    if(Tiny::view.enabled){
      event.input();        //Get Input
      event.handle(view);   //Call the event-handling system
    }

    if(Tiny::audio.enabled) audio.process();      //Audio Processor

    function(args...);      //User-defined Game Loop

    if(Tiny::view.enabled)  view.render();        //Render View

  }
}

}
