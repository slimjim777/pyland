#include <exception>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <memory>
#include <string>

//Include GLM
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#ifdef USE_GLES
#include <GLES2/gl2.h>
#endif

#ifdef USE_GL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif


#include "api.hpp"
#include "engine_api.hpp"
#include "layer.hpp"
#include "map.hpp"
#include "map_loader.hpp"
#include "object.hpp"
#include "object_manager.hpp"
#include "walkability.hpp"

Map::Map(const std::string map_src):
    event_step_on(Vec2D(0, 0)),
    event_step_off(Vec2D(0, 0))
    {
        //Load the map
        MapLoader map_loader;
        bool result = map_loader.load_map(map_src);
        if(!result)  {

            LOG(ERROR) << "Couldn't load map";
            return;
        }
        
        //Get the loaded map data
        map_width = map_loader.get_map_width();
        map_height = map_loader.get_map_height();

        // hack to construct postion dispatcher as we need map diametions 
        event_step_on = PositionDispatcher<int>(Vec2D(map_width,map_height));
        event_step_off = PositionDispatcher<int>(Vec2D(map_width,map_height));

        LOG(INFO) << "Map loading: ";
        LOG(INFO) << "Map width: " << map_width << " Map height: " << map_height;

        layers = map_loader.get_layers();
        tilesets = map_loader.get_tilesets();
        objects  = map_loader.get_objects();

        //Get the tilesets
        //TODO: We'll only support one tileset at the moment
        //Get an object list
        blocker = std::vector<std::vector<int>>(map_width, std::vector<int>(map_height, 0));


        //Generate the geometry needed for this map
        init_shaders();
        init_textures();
        generate_tileset_coords(&texture_images[0]);
        generate_data();

}

Map::~Map() {
    // texture_images stored within struct, implicit destruction.
    // release buffers
    delete[] map_data;
    delete[] map_tex_coords;
    delete[] tileset_tex_coords;

    LOG(INFO) << "Map destructed";
}

bool Map::is_walkable(int x_pos, int y_pos) {
    //Default is walkable
    bool walkable = true;
    //return true;
    //Iterate through all objects
    for(auto character : characters) {
        //If its an invalid object
        if (character == 0)
            continue;

        std::shared_ptr<Object> object = ObjectManager::get_instance().get_object<Object>(character);

        //If we cannot walk on this object
        if(object) {
            if(object->get_walkability() == Walkability::BLOCKED) {
                walkable = false;

                //We can stop checking further objects and tiles
                return walkable;
            }
        }
    }

    //Iterate through all tiles
    for(auto layer : layers ) {
    
        //determine if we can walk on the map
        if(layer->get_name() == "Collisions") {

            //if there is a tile, treat it as blocked
            if(layer->get_tile(x_pos, y_pos) != 0) {
                 walkable = false;
                 //We can stop checking further objects and tiles
                 return walkable;
            }
        }
    }

    return walkable;
}

void Map::add_character(int character_id) {
    if(ObjectManager::is_valid_object_id(character_id))
        characters.push_back(character_id);
}

void Map::remove_character(int character_id) {
    if(ObjectManager::is_valid_object_id(character_id)){
        for(auto it = characters.begin(); it != characters.end(); ++it) {
            //If a valid object
            if(*it != 0) {
                //remove it if its the character
                if(*it == character_id) {
                    characters.erase(it);       
                    return;
                }
            }
        }
    }
}

/**
 * The function used to generate the cache of tile texture coordinates.
 */ 
void Map::generate_tileset_coords(Image* texture_image) {
    LOG(INFO) << "Generating tileset texture coords";

    if(Engine::get_tile_size() == 0) {
        LOG(ERROR) << "Tile size is 0 in Map::generate_tileset_coords";
        return;
    }

    int num_tiles_x = texture_image->width / Engine::get_tile_size();
    int num_tiles_y = texture_image->height / Engine::get_tile_size();
    LOG(INFO) << "Tileset size: " << num_tiles_x << " " << num_tiles_y;

    //Each tile needs 8 floats to describe its position in the image
    try {
        tileset_tex_coords = new GLfloat[sizeof(GLfloat)* num_tiles_x * num_tiles_y * 4 * 2];
    } 
    catch (std::bad_alloc& ba) {
        LOG(ERROR) << "Out of Memory in Map::generate_tileset_coords";
        return;
    }

    //Check tileset dimensions
    if(texture_image->store_width == 0 || texture_image->store_height == 0) {
        LOG(ERROR) << "Texture image has 0 in at least one dimension in Map::generate_tileset_coords";
        return;
    }

    //Tiles are indexed from top left but Openl uses texture coordinates from bottom left
    //so we remap these 
    GLfloat tileset_inc_x = GLfloat(texture_image->width) / GLfloat(texture_image->store_width) / static_cast<GLfloat>(num_tiles_x);
    GLfloat tileset_inc_y = GLfloat(texture_image->height) / GLfloat(texture_image->store_height) / static_cast<GLfloat>(num_tiles_y);
    //We work from left to right, moving down the gl texture
    GLfloat tileset_offset_x = 0.0;
    GLfloat tileset_offset_y = GLfloat(num_tiles_y - 1) * tileset_inc_y;

    //generate the coordinates for each tile
    for (int y = 0; y < num_tiles_y; y++) {
        for (int x = 0; x < num_tiles_x; x++) {
            //bottom left
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+0] = tileset_offset_x;
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+1] = tileset_offset_y;
 
            //top left
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+2] = tileset_offset_x;
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+3] = tileset_offset_y + tileset_inc_y;
 
            //bottom right
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+4] = tileset_offset_x + tileset_inc_x;
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+5] = tileset_offset_y;
 
            //top right
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+6] = tileset_offset_x + tileset_inc_x;
            tileset_tex_coords[y* num_tiles_x*4*2+x*4*2+7] = tileset_offset_y + tileset_inc_y;
 
            tileset_offset_x += tileset_inc_x;
        }
        tileset_offset_x = 0.0;
        tileset_offset_y -= tileset_inc_y;
    }
}

void Map::generate_data() {
    LOG(INFO) << "Generating map data";

    //Get each layer of the map
    //Start at layer 0
    for(auto layer_iter = layers.begin(); layer_iter != layers.end(); ++layer_iter) {
        auto layer_data = (*layer_iter)->get_layer_data();

        //Don't generate data for the collisions layer
        if ((*layer_iter)->get_name() == "Collisions") {
            continue;
        }

        //Work out if we need a dense or a sparse buffer
        int total_tiles = 0;
        int num_blank_tiles = 0;
        for(auto tile_data = layer_data->begin(); tile_data != layer_data->end(); ++tile_data) {
            int tile_id = tile_data->second;
            if(tile_id == 0) {
                num_blank_tiles++;
            }
            total_tiles++;
        }

        //Spare packing by default
        Layer::Packing layer_packing = Layer::Packing::SPARSE;

        //If more than 50% of layer has tiles, make it dense
        if(num_blank_tiles < total_tiles/2 ) {
            layer_packing = Layer::Packing::DENSE;
        }

        //Create the buffer for the layer
        int num_tiles = total_tiles - num_blank_tiles;
        //The number of bytes needed - *6 for the GLTRIANGLES
        int num_vertices = 6;
        int num_dimensions = 2;
        int tex_data_size = sizeof(GLfloat)*num_tiles*num_vertices*num_dimensions;
        int vert_data_size = sizeof(GLfloat)*num_tiles*num_vertices*num_dimensions;
        GLfloat* layer_tex_coords = nullptr;
        GLfloat* layer_vert_coords = nullptr;

        try {
            layer_tex_coords = new GLfloat[tex_data_size];
            layer_vert_coords = new GLfloat[vert_data_size];
        }
        catch(std::bad_alloc& ba) {
            LOG(ERROR) << "Out of memory in Map::generate_data";
            return;
        }

        //Build the layer data based on the 
        if(layer_packing == Layer::Packing::DENSE) {
            generate_dense_layer_tex_coords(layer_tex_coords, tex_data_size, num_tiles, *(layer_iter));
            generate_dense_layer_vert_coords(layer_vert_coords, vert_data_size, num_tiles,*(layer_iter));
        } 
        else {
            generate_sparse_layer_tex_coords(layer_tex_coords, tex_data_size, num_tiles, (*layer_iter));
            generate_sparse_layer_vert_coords(layer_vert_coords, vert_data_size, num_tiles, (*layer_iter));
        }

        //Set this data in the renderable component for the layer
        RenderableComponent* renderable_component = (*layer_iter)->get_renderable_component();
        renderable_component->set_texture_coords_data(layer_tex_coords, tex_data_size, false);
        renderable_component->set_vertex_data(layer_vert_coords, vert_data_size, false);
        renderable_component->set_num_vertices_render(num_tiles*num_vertices*num_dimensions);

    }
}

void Map::generate_layer_tex_coords(GLfloat* data, int data_size, int num_tiles, std::shared_ptr<Layer> layer, bool dense) {
    auto layer_data = layer->get_layer_data();

    //Get all the tiles in the layer, moving from left to right and down 
    int offset = 0;
    int num_floats = 12;
    int num_blanks = 0;
    int num_tiles_count = 0;
    for(auto tile_data = layer_data->begin(); tile_data != layer_data->end(); ++tile_data) {
        std::string tileset_name = tile_data->first;
        int tile_id = tile_data->second;
        num_tiles_count++;

        if(tile_id == 0) 
            num_blanks++;

        //IF WE ARE GENERATING A SPARSE LAYER
        //Skip out blank tiles:
        //This get's us our sparse data structure
        if(dense == false && tile_id == 0)
            continue;

        //Get the texture coordinates for this tile
        GLfloat *tileset_ptr = &tileset_tex_coords[(tile_id)*8]; //*8 as 8 coordinates per tile

        //bottom left
        data[offset+0] = tileset_ptr[0];
        data[offset+1] = tileset_ptr[1];

        //top left
        data[offset+2] = tileset_ptr[2];
        data[offset+3] = tileset_ptr[3];

        //bottom right
        data[offset+4] = tileset_ptr[4];
        data[offset+5] = tileset_ptr[5];

        //top left
        data[offset+6] = tileset_ptr[2];
        data[offset+7] = tileset_ptr[3];

        //top right
        data[offset+8] = tileset_ptr[6];
        data[offset+9] = tileset_ptr[7];
            
        //bottom right
        data[offset+10] = tileset_ptr[4];
        data[offset+11] = tileset_ptr[5];

        offset += num_floats;
        /*      if(offset >= data_size) {
            LOG(ERROR) << "Offset exceeded data size in Map::generate_layer_tex_coords";
            return;
            }*/
    }
}

void Map::generate_sparse_layer_tex_coords(GLfloat* data, int data_size, int num_tiles, std::shared_ptr<Layer> layer) {
    generate_layer_tex_coords(data, data_size, num_tiles, layer, false);
}

void Map::generate_dense_layer_tex_coords(GLfloat* data, int data_size, int num_tiles, std::shared_ptr<Layer> layer) {
    generate_layer_tex_coords(data, data_size, num_tiles, layer, true);
}

void Map::generate_layer_vert_coords(GLfloat* data, int data_size, int num_tiles, std::shared_ptr<Layer> layer, bool dense) {
    LOG(INFO) << "Generating map coordinate data";

    float scale = (float)(Engine::get_tile_size() * Engine::get_global_scale());
    int num_floats = 12;
    int offset = 0;
    ///
    /// Vertex winding order:
    /// 1, 3   4
    ///  * --- * 
    ///  |     |
    ///  |     | 
    ///  * --- *
    /// 0       2,5
    ///

    //The current tile's data
    auto tile_data = layer->get_layer_data()->begin();

    //Generate one layer's worth of data
    for(int y = 0; y < map_height; y++) {
        for(int x = 0; x < map_width; x++) {
            //If we exhaust the layer's data
            if(tile_data == layer->get_layer_data()->end()) {
                LOG(ERROR) << "Layer had less data than map dimensions in Map::generate_layer_vert_coords";
                return;
            }

            //IF GENERATING A SPARSE LAYER
            //Skip empty tiles
            int tile_id = tile_data->second;
            if(dense == false && tile_id == 0) 
                continue;

            //bottom left
            data[offset+ 0] = scale * float(x);
            data[offset+ 1] = scale * float(y);
             
            //top left
            data[offset+ 2] = scale * float(x);
            data[offset+ 3] = scale * float(y + 1);

            //bottom right
            data[offset+ 4] = scale * float(x + 1);
            data[offset+ 5] = scale * float(y);
            
            //top left
            data[offset+ 6]  = scale * float(x);
            data[offset+ 7] = scale * float(y+1);
        
            //top right
            data[offset+ 8] = scale * float(x+1);
            data[offset+ 9] = scale * float(y+1);

            //bottom right
            data[offset+10] = scale * float(x+1);
            data[offset+11] = scale * float(y);

            offset += num_floats;
            ++tile_data;
        }
    }
}

void Map::generate_dense_layer_vert_coords(GLfloat* data, int data_size, int num_tiles, std::shared_ptr<Layer> layer) {
    generate_layer_vert_coords(data, data_size, num_tiles, layer, true);
}

void Map::generate_sparse_layer_vert_coords(GLfloat* data, int data_size, int num_tiles, std::shared_ptr<Layer> layer) {
    generate_layer_vert_coords(data, data_size, num_tiles, layer, false);
}

void Map::init_textures() {
    texture_images[0] = Image("../resources/basictiles_2.png");

    //Set the texture data in the rederable component
    for(auto layer: layers) {
        layer->get_renderable_component()->set_texture_image(&texture_images[0]);
    }
}

/**
 * This function initialises the shader, creating and loading them.
 */ 
bool Map::init_shaders() {
    Shader* shader = nullptr;
    try {
#ifdef USE_GLES
        shader = new Shader("vert_shader.glesv", "frag_shader.glesf");
#endif
#ifdef USE_GL
        shader = new Shader("vert_shader.glv", "frag_shader.glf");
#endif
    }
    catch (std::exception e) {
        delete shader;
        shader = nullptr;
        LOG(ERROR) << "Failed to create the shader";
        return false;
    }
    for(auto layer: layers) {
        layer->get_renderable_component()->set_shader(shader);
    }

    return true;

}

Map::Blocker::Blocker(Vec2D tile, std::vector <std::vector<int>>* blocker):
    tile(tile), blocker(blocker) {
        (*blocker)[tile.x][tile.y]++;

        LOG(INFO) << "Block level at tile " << tile.x << " " <<tile.y
          << " increased from " << (*blocker)[tile.x][tile.y] - 1
          << " to " << (*blocker)[tile.x][tile.y] << ".";
}

Map::Blocker::Blocker(const Map::Blocker &other):
    tile(other.tile), blocker(other.blocker) {
        (*blocker)[tile.x][tile.y]++;

        LOG(INFO) << "Block level at tile " << tile.x << " " <<tile.y
          << " increased from " << (*blocker)[tile.x][tile.y] - 1
          << " to " << (*blocker)[tile.x][tile.y] << ".";
}

Map::Blocker::~Blocker() {
    (*blocker)[tile.x][tile.y] = (*blocker)[tile.x][tile.y] - 1 ;

    LOG(INFO) << "Block level at tile " << tile.x << " " <<tile.y
      << " decreased from " << (*blocker)[tile.x][tile.y] + 1
      << " to " << (*blocker)[tile.x][tile.y] << ".";
}

Map::Blocker Map::block_tile(Vec2D tile) {
    return Blocker(tile, &blocker);
}



