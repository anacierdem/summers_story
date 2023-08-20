#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdalign.h>

#include "n64sys.h"
#include "GL/gl.h"
#include "terrain.h"
#include "asset.h"
#include "debug.h"
#define PTR_DECODE(model, ptr)    ((void*)(((uint8_t*)(model)) + (uint32_t)(ptr)))
#define PTR_ENCODE(model, ptr)    ((void*)(((uint8_t*)(ptr)) - (uint32_t)(model)))
typedef uint16_t u_int16_t alignas(1);

uint8_t start_color[4] = {255, 251, 199};
uint8_t end_color[4] = {120, 100, 10};

terrain_t *terrain_load_buf(void *buf, int sz)
{
    terrain_t *model = buf;
    assertf(sz >= sizeof(terrain_t), "Model buffer too small (sz=%d)", sz);
    if(model->magic == TERRAIN_MAGIC_LOADED) {
        assertf(0, "Trying to load already loaded model data (buf=%p, sz=%08x)", buf, sz);
    }
    assertf(model->magic == TERRAIN_MAGIC, "invalid model data (magic: %08lx)", model->magic);
    model->meshes = PTR_DECODE(model, model->meshes);
    model->heightmap = PTR_DECODE(model, model->heightmap);
    model->heightmap_indices = PTR_DECODE(model, model->heightmap_indices);
    for (int i = 0; i < model->num_meshes; i++)
    {
        float *transform = model->meshes[i].transform;
        debugf("mesh name: %s \n", model->meshes[i].name);
        debugf("mesh transform: %f %f %f %f \n", transform[0], transform[1], transform[2], transform[3]);
        debugf("                %f %f %f %f \n", transform[4], transform[5], transform[6], transform[7]);
        debugf("                %f %f %f %f \n", transform[8], transform[9], transform[10], transform[11]);
        debugf("                %f %f %f %f \n", transform[12], transform[13], transform[14], transform[15]);
        model->meshes[i].primitives = PTR_DECODE(model, model->meshes[i].primitives);
        for (int j = 0; j < model->meshes[i].num_primitives; j++)
        {
            primitive_t *primitive = &model->meshes[i].primitives[j];
            primitive->position.pointer = PTR_DECODE(model, primitive->position.pointer);
            primitive->color.pointer = PTR_DECODE(model, primitive->color.pointer);
            primitive->texcoord.pointer = PTR_DECODE(model, primitive->texcoord.pointer);
            primitive->normal.pointer = PTR_DECODE(model, primitive->normal.pointer);
            primitive->mtx_index.pointer = PTR_DECODE(model, primitive->mtx_index.pointer);
            primitive->indices = PTR_DECODE(model, primitive->indices);
        }
    }

    mesh_t *mesh = model->meshes;

    attribute_t *a_color = &mesh->primitives->color;
    // char *p_color = mesh->primitives->color.pointer;
    // char *position_p = mesh->primitives->position.pointer;

    assertf(mesh->primitives->position.stride > 0, "unexpected position stride");
    assertf(a_color->size == 4, "invalid color size %lu \n", a_color->size);
    assertf(a_color->type == GL_UNSIGNED_BYTE, "invalid color type %lx \n", a_color->type);
    assertf(a_color->stride > 0, "unexpected color stride");

    // for (int i = 0; i < (TERRAIN_GRID_SIZE * TERRAIN_GRID_SIZE); i++){
        // int idx = model->heightmap_indices[i];
        // uint8_t* cur_color = (uint8_t*)(p_color + (a_color->stride) * idx);
        // u_int16_t* position = (u_int16_t*)(position_p + mesh->primitives->position.stride * index);

        // float vertex_x = position[0] / precision_multiplier;
        // float vertex_y = position[1] / precision_multiplier;
        // float vertex_z = position[2] / precision_multiplier;

        // // Darken the shoreline
        // if (model->heightmap[i] < 1.0) {
        //     cur_color[0] = end_color[0];
        //     cur_color[1] = end_color[1];
        //     cur_color[2] = end_color[2];
        // }

        // cur_color[1] = 255;
        // cur_color[2] = 255;
        // cur_color[3] = 255;
        // debugf("alpha: %d ", cur_color[3]);
    // }

    data_cache_hit_writeback(model, sz);

    assertf(mesh->primitives->position.size == 3, "unexpected position size");
    assertf(mesh->primitives->position.type == GL_HALF_FIXED_N64, "unexpected position type %0lx", mesh->primitives->position.type);
    // assertf(mesh->primitives->normal.size == 3, "unexpected normal size");
    // assertf(mesh->primitives->normal.type == GL_BYTE, "unexpected normal type %0lx", mesh->primitives->normal.type);

    debugf("no vert: %lu\n", mesh->primitives->num_vertices);
    debugf("no ind: %lu\n", mesh->primitives->num_indices);

    debugf("grid size: %u\n", TERRAIN_GRID_SIZE);

    return model;
}

float terrain_get_height(terrain_t *model, float x, float z)
{
    if (x < -TERRAIN_HALF_WIDTH || x > TERRAIN_HALF_WIDTH || z < -TERRAIN_HALF_WIDTH || z > TERRAIN_HALF_WIDTH) return 0.0f;

    int minus_one = (TERRAIN_GRID_SIZE - 1);

    float terrain_step = 2 * TERRAIN_HALF_WIDTH / minus_one;

    // indices to four neighbours
    int grid_x = (int)((x / terrain_step) + (minus_one/2)) % minus_one;
    int grid_x_next = (grid_x + 1) % minus_one;
    int grid_y = (int)((z / terrain_step) + (minus_one/2)) % minus_one;
    int grid_y_next = (grid_y + 1) % minus_one;

    float height_a = model->heightmap[grid_y * TERRAIN_GRID_SIZE + grid_x];
    float height_b = model->heightmap[grid_y * TERRAIN_GRID_SIZE + grid_x_next];

    float height_c = model->heightmap[grid_y_next * TERRAIN_GRID_SIZE + grid_x];
    float height_d = model->heightmap[grid_y_next * TERRAIN_GRID_SIZE + grid_x_next];

    float remainder_x = fmodf(x + TERRAIN_HALF_WIDTH, terrain_step);
    float remainder_y = fmodf(z + TERRAIN_HALF_WIDTH, terrain_step);

    float height_1 = (height_a * (terrain_step - remainder_x) + height_b * remainder_x) / terrain_step;
    float height_2 = (height_c * (terrain_step - remainder_x) + height_d * remainder_x) / terrain_step;

    return (height_1 * (terrain_step - remainder_y) + height_2 * remainder_y) / terrain_step;
}

void terrain_dig(terrain_t *model, float x, float z, float yaw)
{
    int minus_one = (TERRAIN_GRID_SIZE - 1);

    float terrain_step = 2 * TERRAIN_HALF_WIDTH / minus_one;

    // indices to four neighbours
    int grid_x = (int)((x / terrain_step) + (minus_one/2)) % minus_one;
    int grid_x_next = (grid_x + 1) % minus_one;
    int grid_y = (int)((z / terrain_step) + (minus_one/2)) % minus_one;
    int grid_y_next = (grid_y + 1) % minus_one;


    int idx = grid_y * TERRAIN_GRID_SIZE + grid_x;
    if (yaw > 90.0f && yaw <= 180.0) {
        idx = grid_y_next * TERRAIN_GRID_SIZE + grid_x;
    } else if (yaw > 180.0f && yaw <= 270.0) {
        idx = grid_y_next * TERRAIN_GRID_SIZE + grid_x_next;
    } else if (yaw > 270.0f && yaw <= 360.0) {
        idx = grid_y * TERRAIN_GRID_SIZE + grid_x_next;
    }
    int vertex_idx = model->heightmap_indices[idx];

    mesh_t *mesh = model->meshes;

    attribute_t *a_color = &mesh->primitives->color;
    char *p_color = mesh->primitives->color.pointer;
    uint8_t* cur_color = (uint8_t*)(p_color + (a_color->stride) * vertex_idx);

    float precision_multiplier = 1 << mesh->primitives->vertex_precision;

    char *p_pos = mesh->primitives->position.pointer;
    u_int16_t* cur_pos = (u_int16_t*)(p_pos + mesh->primitives->position.stride * vertex_idx);

    // float vertex_x = (cur_pos)[0] / precision_multiplier;
    // float vertex_y = (cur_pos)[1] / precision_multiplier;
    // float vertex_z = (cur_pos)[2] / precision_multiplier;

    float amount = 0.05;
    (cur_pos)[1] -= (amount * precision_multiplier);
    if ((cur_pos)[1] < 0) (cur_pos)[1] = 0;

    // TODO: maybe we should limit this to a great extent to prevent clipping but keep the feeling
    model->heightmap[idx] -= amount;
    model->heightmap[idx] = fmaxf(model->heightmap[idx], 0.0f);

    float darken = 0.1;
    debugf("c: %d, %d, %d ", cur_color[0], cur_color[1], cur_color[2]);
    cur_color[0] = (float)cur_color[0] * (1.0f - darken) + (float)end_color[0] * (darken);
    cur_color[1] = (float)cur_color[1] * (1.0f - darken) + (float)end_color[1] * (darken);
    cur_color[2] = (float)cur_color[2] * (1.0f - darken) + (float)end_color[2] * (darken);
    debugf("n: %d, %d, %d ", cur_color[0], cur_color[1], cur_color[2]);
}



terrain_t *terrain_load(const char *fn)
{
    int sz;
    void *buf = asset_load(fn, &sz);
    terrain_t *model = terrain_load_buf(buf, sz);
    model->magic = TERRAIN_MAGIC_OWNED;
    return model;
}

static void terrain_unload(terrain_t *model)
{
    for (int i = 0; i < model->num_meshes; i++)
    {
        for (int j = 0; j < model->meshes[i].num_primitives; j++)
        {
            primitive_t *primitive = &model->meshes[i].primitives[j];
            primitive->position.pointer = PTR_ENCODE(model, primitive->position.pointer);
            primitive->color.pointer = PTR_ENCODE(model, primitive->color.pointer);
            primitive->texcoord.pointer = PTR_ENCODE(model, primitive->texcoord.pointer);
            primitive->normal.pointer = PTR_ENCODE(model, primitive->normal.pointer);
            primitive->mtx_index.pointer = PTR_ENCODE(model, primitive->mtx_index.pointer);
            primitive->indices = PTR_ENCODE(model, primitive->indices);
        }
        model->meshes[i].primitives = PTR_ENCODE(model, model->meshes[i].primitives);
    }
    model->meshes = PTR_ENCODE(model, model->meshes);
}

void terrain_free(terrain_t *model)
{
    terrain_unload(model);
    if (model->magic == TERRAIN_MAGIC_OWNED) {
        #ifndef NDEBUG
        // To help debugging, zero the model structure
        memset(model, 0, sizeof(terrain_t));
        #endif

        free(model);
    }
}

uint32_t terrain_get_mesh_count(terrain_t *model)
{
    return model->num_meshes;
}
mesh_t *terrain_get_mesh(terrain_t *model, uint32_t mesh_index)
{
    return &model->meshes[mesh_index];
}

uint32_t terrain_get_primitive_count(mesh_t *mesh)
{
    return mesh->num_primitives;
}
primitive_t *terrain_get_primitive(mesh_t *mesh, uint32_t primitive_index)
{
    return &mesh->primitives[primitive_index];
}

void terrain_draw_primitive(primitive_t *primitive)
{
    if (primitive->position.size > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        if (primitive->position.type == GL_HALF_FIXED_N64) {
            glVertexHalfFixedPrecisionN64(primitive->vertex_precision);
        }
        glVertexPointer(primitive->position.size, primitive->position.type, primitive->position.stride, primitive->position.pointer);
    } else {
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    
    if (primitive->color.size > 0) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(primitive->color.size, primitive->color.type, primitive->color.stride, primitive->color.pointer);
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }
    
    if (primitive->texcoord.size > 0) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        if (primitive->texcoord.type == GL_HALF_FIXED_N64) {
            glTexCoordHalfFixedPrecisionN64(primitive->texcoord_precision);
        }
        glTexCoordPointer(primitive->texcoord.size, primitive->texcoord.type, primitive->texcoord.stride, primitive->texcoord.pointer);
    } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    
    if (primitive->normal.size > 0) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(primitive->normal.type, primitive->normal.stride, primitive->normal.pointer);
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    
    if (primitive->mtx_index.size > 0) {
        glEnableClientState(GL_MATRIX_INDEX_ARRAY_ARB);
        glMatrixIndexPointerARB(primitive->mtx_index.size, primitive->mtx_index.type, primitive->mtx_index.stride, primitive->mtx_index.pointer);
    } else {
        glDisableClientState(GL_MATRIX_INDEX_ARRAY_ARB);
    }

    if (primitive->num_indices > 0) {
        glDrawElements(primitive->mode, primitive->num_indices, primitive->index_type, primitive->indices);
    } else {
        glDrawArrays(primitive->mode, 0, primitive->num_vertices);
    }
}

void terrain_draw_mesh(mesh_t *mesh)
{
    for (uint32_t i = 0; i < terrain_get_primitive_count(mesh); i++)
    {
        terrain_draw_primitive(terrain_get_primitive(mesh, i));
    }
}

void terrain_draw(terrain_t *model)
{
    for (uint32_t i = 0; i < terrain_get_mesh_count(model); i++) {
        terrain_draw_mesh(terrain_get_mesh(model, i));
    }
}
