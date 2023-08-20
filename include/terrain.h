#ifndef __LIBDRAGON_TERRAIN_H
#define __LIBDRAGON_TERRAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief terrain file magic header */
#define TERRAIN_MAGIC           0x54455248 // "TERH"
/** @brief terrain loaded model buffer magic */
#define TERRAIN_MAGIC_LOADED    0x5445524C // "TERL"
/** @brief terrain owned model buffer magic */
#define TERRAIN_MAGIC_OWNED     0x5445524F // "TERO"
/** @brief Current version of terrain */
#define TERRAIN_VERSION         1

// FIXME: can move this to the file as well
#define TERRAIN_HALF_WIDTH      12.0f
// FIXME: make this a parameter
#define TERRAIN_GRID_SIZE       17
#define TERRAIN_MESH_NAME_SIZE  32

/** @brief Parameters for a single vertex attribute (part of #primitive_t) */
typedef struct attribute_s {
    uint32_t size;                  ///< Number of components per vertex. If 0, this attribute is not defined
    uint32_t type;                  ///< The data type of each component (for example GL_FLOAT)
    uint32_t stride;                ///< The byte offset between consecutive vertices. If 0, the values are tightly packed
    void *pointer;                  ///< Pointer to the first value
} attribute_t;

/** @brief A single draw call that makes up part of a mesh (part of #mesh_t) */
typedef struct primitive_s {
    uint32_t mode;                  ///< Primitive assembly mode (for example GL_TRIANGLES)
    attribute_t position;           ///< Vertex position attribute, if defined
    attribute_t color;              ///< Vertex color attribyte, if defined
    attribute_t texcoord;           ///< Texture coordinate attribute, if defined
    attribute_t normal;             ///< Vertex normals, if defined
    attribute_t mtx_index;          ///< Matrix indices (aka bones), if defined
    uint32_t vertex_precision;      ///< If the vertex positions use fixed point values, this defines the precision
    uint32_t texcoord_precision;    ///< If the texture coordinates use fixed point values, this defines the precision
    uint32_t index_type;            ///< Data type of indices (for example GL_UNSIGNED_SHORT)
    uint32_t num_vertices;          ///< Number of vertices
    uint32_t num_indices;           ///< Number of indices
    void *indices;                  ///< Pointer to the first index value. If NULL, indices are not used
} primitive_t;

/** @brief A mesh that is made up of multiple primitives (part of #terrain_t) */
typedef struct mesh_s {
    char name[TERRAIN_MESH_NAME_SIZE];
    float transform[16];
    uint32_t num_primitives;        ///< Number of primitives
    primitive_t *primitives;        ///< Pointer to the first primitive
} mesh_t;

/** @brief A terrain file containing a model */
typedef struct terrain_s {
    uint32_t magic;                 ///< Magic header (TERRAIN_MAGIC)
    uint32_t version;               ///< Version of this file
    uint32_t header_size;           ///< Size of the header in bytes
    uint32_t mesh_size;             ///< Size of a mesh header in bytes
    uint32_t primitive_size;        ///< Size of a primitive header in bytes
    uint32_t num_meshes;            ///< Number of meshes
    // TODO: combine height related things into another struct
    mesh_t *meshes;                 ///< Pointer to the first mesh
    float *heightmap;               ///< Pointer to the heightmap data
    int *heightmap_indices;         ///< Pointer to the heightmap indices
} terrain_t;

terrain_t *terrain_load(const char *fn);
terrain_t *terrain_load_buf(void *buf, int sz);
void terrain_free(terrain_t *model);

/**
 * @brief Return the number of meshes in this model.
 */
uint32_t terrain_get_mesh_count(terrain_t *model);

/**
 * @brief Return the mesh at the specified index.
 */
mesh_t *terrain_get_mesh(terrain_t *model, uint32_t mesh_index);

/**
 * @brief Return the number of primitives in this mesh.
 */
uint32_t terrain_get_primitive_count(mesh_t *mesh);

/**
 * @brief Return the primitive at the specified index.
 */
primitive_t *terrain_get_primitive(mesh_t *mesh, uint32_t primitive_index);

/**
 * @brief Draw an entire model.
 * 
 * This will draw all primitives of all meshes that are contained the given model.
 */
void terrain_draw(terrain_t *model);

/**
 * @brief Draw a single mesh.
 * 
 * This will draw all of the given mesh's primitives.
 */
void terrain_draw_mesh(mesh_t *mesh);

/**
 * @brief Draw a single primitive.
 */
void terrain_draw_primitive(primitive_t *primitive);

float terrain_get_height(terrain_t *model, float x, float z);
void terrain_dig(terrain_t *model, float x, float z, float yaw);

#ifdef __cplusplus
}
#endif

#endif // __LIBDRAGON_TERRAIN_H
