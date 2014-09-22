#ifndef BUTTON_H
#define BUTTON_H

#include "component_group.hpp"
#include "gui_text.hpp"
#include "gui_text_data.hpp"
#include "text.hpp"

#include <functional>
#include <memory>

#include <string>
#include <utility>
#include <vector>


#ifdef USE_GLES
#include <GLES2/gl2.h>
#endif

#if defined(USE_GL)
#define GL_GLEXT_PROTOTYPES
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif


class Button : public ComponentGroup {
    std::shared_ptr<GUIText> button_text;
public:
    Button();
    Button(std::shared_ptr<Text> _text, std::function<void (void)> on_click, float _width, float _height, float _x_offset, float _y_offset);

    std::shared_ptr<Text> get_text();

    void set_text(std::shared_ptr<Text> );
    void set_text(std::string);

    int generate_vertex_coords_element(GLfloat* data, int offset, std::tuple<float,float,float,float> bounds);
    int generate_texture_coords_element(GLfloat* data, int offset, std::tuple<float,float,float,float> bounds);

    int generate_tile_element_vertex_coords(GLfloat* data, int offset, std::tuple<float,float,float,float> bounds, float element_width, float element_height);
    int generate_tile_element_texture_coords(GLfloat* data, int offset, std::tuple<float,float,float,float>vertex_bounds, float element_width, float element_height, std::tuple<float,float,float,float> texture_bounds);
    int calculate_num_tile_elements(std::tuple<float,float,float,float> bounds, float element_width, float element_height);

  std::vector<std::pair<GLfloat*, int>> generate_this_vertex_data();

    std::vector<std::pair<GLfloat*, int>> generate_this_texture_data();

    std::vector<std::shared_ptr<GUIText>> generate_this_text_data();

};

#endif
