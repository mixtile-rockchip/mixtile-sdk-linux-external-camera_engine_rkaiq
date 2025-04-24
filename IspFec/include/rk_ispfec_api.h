/*
 *  Copyright (c) 2022 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _RK_ISPFEC_API_H_
#define _RK_ISPFEC_API_H_

#include "rkfec_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FEC_VERSION_ENCODE(a, b, c) (((a) << 16) + ((b) << 8) + ((c) > 255 ? 255 : (c)))
#define RK_ISP_FEC_API_VERSION      FEC_VERSION_ENCODE(3, 1, 0)

#define RK_ISP_FEC_API_VERSION_MAJOR(v) (((v) >> 16) & 0xff)
#define RK_ISP_FEC_API_VERSION_MINOR(v) (((v) >> 8) & 0xff)
#define RK_ISP_FEC_API_VERSION_PATCH(v) ((v) & 0xff)

typedef struct rk_ispfec_ctx_s rk_ispfec_ctx_t;

enum rk_ispfec_correct_direction {
    RK_ISPFEC_CORRECT_DIRECTION_X = 0x1,
    RK_ISPFEC_CORRECT_DIRECTION_Y,
    RK_ISPFEC_CORRECT_DIRECTION_XY
};

enum rk_ispfec_correct_style {
    RK_ISPFEC_KEEP_ASPECT_RATIO_REDUCE_FOV = 0x1,
    RK_ISPFEC_COMPRES_IMAGE_KEEP_FOV,
};

/**
 * the following prarams are only effective in 'RK_ISPFEC_UPDATE_MESH_ON_LINE'.
 *
 * @light_center: the optical center of the lens.
 * @coefficient: the distortion coefficient of the lens.
 * @direction: configure the correction direction of the generated mesh.
 * @style: configure the correction style of the generated mesh.
 * @correct_level: the distortion type of FEC.
 */
typedef struct gen_mesh_online_info_s {
    double  light_center[2];
    double  coeff[4];
    enum rk_ispfec_correct_direction 	direction;
    enum rk_ispfec_correct_style 		style;

    int     correct_level;

    int calib_width;
    int calib_height;
    int calib_level_maxLimit;
} gen_mesh_online_info_t;

typedef struct mesh_info_s {
    int dmaFd;
    void* vir_addr;
    int size;
    char mesh_file[64];
} mesh_info_t;

typedef struct rk_ispfec_bg_val_s {
    int bg_y;
    int bg_u;
    int bg_v;
} rk_ispfec_bg_val;

// buffer mode configuration (RK_ISPFEC_UPDATE_MESH_FROM_BUFFER)
typedef struct rk_ispfec_buffer_info {
    int dma_fd;      ///< DMA buffer handle (effective when need_copy=0)
    void* vir_addr;  ///< Source virtual address for copy (required when need_copy=1)
    size_t size;     ///< Buffer capacity, must be >= actual data size
    bool need_copy;  ///< Memory copy mode flag. Configuration rules:
                     ///< - DMA handle mode: set 0 to:
                     ///<   a. Directly use dma_fd
                     ///<   b. vir_addr will be ignored
                     ///< - Memory copy mode: set 1 to:
                     ///<   a. Copy size bytes from vir_addr to the in-use DMA buffer
                     ///<   b. dma_fd will be ignored
} rk_ispfec_buffer_info_t;

enum rk_ispfec_update_mesh_mode {
    RK_ISPFEC_UPDATE_MESH_ONLINE = 0,  // generate lut online
    RK_ISPFEC_UPDATE_MESH_FROM_FILE,   // external file import mesh
    RK_ISPFEC_UPDATE_MESH_FROM_BUFFER  // update mesh from buffer descriptors
};

typedef struct rk_ispfec_mesh_config {
    enum rk_ispfec_update_mesh_mode mode;

    union {
        // File mode configuration (RK_ISPFEC_UPDATE_MESH_FROM_FILE)
        struct {
#ifdef RKFEC_HW_V20
            char mesh_path[256];  // Single mesh file path (V20)
#else
            char mesh_xint[256];  // X-axis integer mesh file
            char mesh_xfra[256];  // X-axis fractional mesh file
            char mesh_yint[256];  // Y-axis integer mesh file
            char mesh_yfra[256];  // Y-axis fractional mesh file
#endif
        } file_cfg;

        // Buffer mode configuration (RK_ISPFEC_UPDATE_MESH_FROM_BUFFER)
        struct {
#ifdef RKFEC_HW_V20
            rk_ispfec_buffer_info_t mesh;  // Single buffer information
#else
            struct {
                rk_ispfec_buffer_info_t xint;  // X-axis integer part
                rk_ispfec_buffer_info_t xfra;  // X-axis fractional part
                rk_ispfec_buffer_info_t yint;  // Y-axis integer part
                rk_ispfec_buffer_info_t yfra;  // Y-axis fractional part
            } mesh;                            // Multiple buffers configuration
#endif
        } buffer_cfg;

        // Online generation mode (RK_ISPFEC_UPDATE_MESH_ONLINE)
        gen_mesh_online_info_t online_cfg;  // Online generation parameters
    } params;
} rk_ispfec_mesh_config_t;

typedef struct rk_ispfec_cfg_s {
    /**
     * @brief Configuration structure version identifier
     * - Must be set to RK_ISP_FEC_API_VERSION by application
     * - Used for version compatibility check during initialization
     * @note Added in API version 2.0.0. Recommended to use ISP_FEC_CONFIG_INIT()
     *       macro for proper initialization which will set this field automatically
     */
    int version;

    /**
     * @brief Size of this configuration structure
     * - Must be set to sizeof(rk_ispfec_cfg_s) by application
     * - Ensures binary compatibility across library updates
     * @note Added in API version 2.0.0. ISP_FEC_CONFIG_INIT() macro will set
     *       this field to correct value
     */
    int size;

    // 输入输出的分辨率
    int in_width;
    int in_height;
    int out_width;
    int out_height;

    /**
     * @brief Input pixel format (V4L2 fourcc code)
     * Supported formats:
     * - V4L2_PIX_FMT_NV12: YUV420 semi-planar (8-bit)
     * - V4L2_PIX_FMT_TILE420: TILE4x4, YUV420 semi-planar (8-bit)
     */
    int in_fourcc;

    /**
     * @brief Output pixel format (V4L2 fourcc code)
     * Supported formats:
     * - V4L2_PIX_FMT_NV12: YUV420 semi-planar (8-bit)
     * - V4L2_PIX_FMT_TILE420: TILE4x4, YUV420 semi-planar (8-bit)
     * - V4L2_PIX_FMT_FBC0: Frame Buffer Compression format
     * - V4L2_PIX_FMT_QUAD: To AVSP, YUV420 (8-bit)
     */
    int out_fourcc;

#ifdef RKFEC_HW_V20
    /**
     * @brief Border handling mode when mesh coordinates exceed input boundaries
     *
     * Trigger condition: When corrected pixel coordinates fall outside
     * input image boundaries [ (0,0) ~ (srcW-1, srcH-1) ]
     *
     * Mode options:
     * - 0: Fill with bg_val (background color values)
     * - 1: Extend border pixels from input image edges
     */
    int border_mode;

    /**
     * @brief Cross buffer handling mode for local over-shrinking cases
     *
     * Trigger condition: When local scaling ratio exceeds 3.75x
     * (input area / output area > 3.75) in mesh-defined regions
     *
     * Mode options:
     * - 0: Fill with bg_val (consistent background color)
     * - 1: Replicate boundary values from current sampling block
     *
     * @note This works in conjunction with border_mode to handle edge cases
     */
    int cross_buf_mode;

    /**
     * @brief Background Y/U/V component
     * - Range: 0-255 (8-bit)
     * - Typical black value: Y=0, U=128, V=128
     */
    rk_ispfec_bg_val bg_val;

    // mesh 表 buf fd，外部分配，可用 rk_ispfec_api_calFecMeshsize 计算size
    mesh_info_t mesh_info;
#else
    // mesh 表 buf fd，外部分配，可用 rk_ispfec_api_calFecMeshsize 计算size
    mesh_info_t mesh_xint;
    mesh_info_t mesh_xfra;
    mesh_info_t mesh_yint;
    mesh_info_t mesh_yfra;
#endif

    enum rk_ispfec_update_mesh_mode mesh_upd_mode;

    /**
     * @UPDATE_MESH_ONLINE :   configure the online info from fec calibration for 'mesh_file_path'
     * @UPDATE_MESH_FROM_FILE: configure the path of mesh file for 'mesh_online'
     * @UPDATE_MESH_FROM_BUFFER: configure the mesh from buffer
     */
    union {
        char mesh_file_path[128];
        gen_mesh_online_info_t mesh_online;
    } u;

    /**
     * @brief Horizontal offset for input image cropping
     * - Unit: Pixels
     * - Range: 0 to (input_width - 1)
     * - Applied before FEC processing
     * @note Added in API version 3.0.0
     */
    int in_offset_x;

    /**
     * @brief Horizontal offset for output image placement
     * - Unit: Pixels
     * - Range: 0 to (output_width - 1)
     * - Applied after FEC processing
     * @note Added in API version 3.0.0
     */
    int out_offset_x;

    /**
     * @brief Memory stride for input image buffer
     * - Unit: Pixels per line
     * - Set to 0 for automatic calculation based on width/format
     * @note Added in API version 3.0.0
     */
    int in_stride;

    /**
     * @brief Memory stride for output image buffer
     * - Unit: Pixels per line
     * - Set to 0 for automatic calculation based on width/format
     * @note Added in API version 3.0.0
     */
    int out_stride;

    /**
     * @brief Path to lens calibration configuration file (INI format)
     * - Specifies optical calibration parameters
     *
     * @note Behavior rules:
     * - When NULL: Application must manually provide calibration parameters
     *   through mesh_online configuration
     * - When non-NULL: Library will parse the INI file and override any
     *   manually set parameters in mesh_online
     * @note Added in API version 3.1.0
     */
    char* calib_ini_from;
} rk_ispfec_cfg_t;

/**
 * @brief Initialize FEC configuration structure
 * @param cfg Configuration structure to initialize
 *
 * @note This macro will:
 * 1. Set version field to RK_ISP_FEC_API_VERSION
 * 2. Set size field to actual structure size
 *
 * Usage example:
 * @code
 * rk_ispfec_cfg_t cfg;
 * ISP_FEC_CONFIG_INIT(cfg);
 * @endcode
 */
#define ISP_FEC_CONFIG_INIT(cfg)                    \
    do {                                            \
        memset(&(cfg), 0, sizeof(rk_ispfec_cfg_t)); \
        (cfg).version = RK_ISP_FEC_API_VERSION;     \
        (cfg).size    = sizeof(rk_ispfec_cfg_t);    \
    } while (0)

rk_ispfec_ctx_t* rk_ispfec_api_init(rk_ispfec_cfg_t* cfg);

// prepare 时cfg可为 NULL，为NULL时使用 init时的。
// prepare 可用于 reconfig 参数
int rk_ispfec_api_prepare(rk_ispfec_ctx_t* ctx, rk_ispfec_cfg_t* cfg);

int rk_ispfec_api_process(rk_ispfec_ctx_t* ctx, int src_fd, int dst_fd);

void rk_ispfec_api_deinit(rk_ispfec_ctx_t* ctx);

// helper functions
// 用于计算 mesh buf 所需 size
int rk_ispfec_api_calFecMeshsize(int width, int height);

/**
 * @brief Get current FEC processing configuration
 * @param[in] ctx Context handle created by rk_ispfec_api_init
 * @param[out] cfg Pointer to store configuration parameters
 * @return 0 for success, non-zero for error code
 */
int rk_ispfec_api_get_cfg(rk_ispfec_ctx_t* ctx, rk_ispfec_cfg_t* cfg);

/**
 * @brief Update FEC processing configuration
 * @param[in] ctx Context handle created by rk_ispfec_api_init
 * @param[in] cfg Pointer to new configuration parameters
 * @return 0 for success, non-zero for error code
 */
int rk_ispfec_api_set_cfg(rk_ispfec_ctx_t* ctx, rk_ispfec_cfg_t* cfg);

/**
 * @brief Lock FEC processing pipeline for thread-safe parameter updates
 * @param[in] ctx Context handle created by rk_ispfec_api_init
 * @return 0 on success, non-zero error code on failure
 *
 * @details Typical usage scenario:
 * 1. Application calls this function to acquire lock
 * 2. Update ctx->_cfg.mesh_info.vir_addr contents
 * 3. Call rk_ispfec_api_unlock_process() when done
 *
 * @note This will block until any in-progress processing completes.
 * Must be paired with unlock call.
 */
int rk_ispfec_api_lock_process(rk_ispfec_ctx_t* ctx);

/**
 * @brief Unlock FEC processing pipeline
 * @param[in] ctx Context handle created by rk_ispfec_api_init
 * @return 0 on success, non-zero error code on failure
 *
 * @details Releases the lock acquired by rk_ispfec_api_lock_process().
 *
 * @warning Failing to call unlock may lead to deadlocks. Use RAII pattern
 * in C++ or try-finally blocks in other languages for safety.
 */
int rk_ispfec_api_unlock_process(rk_ispfec_ctx_t* ctx);

/**
 * @brief Update FEC mesh correction configuration
 * @param[in] ctx Initialized FEC context handle
 * @param[in] config Pointer to mesh configuration parameters
 * @return 0 on success, -1 for invalid parameters
 *
 * @details Handles three operation modes:
 * - RK_ISPFEC_UPDATE_MESH_ONLINE: Generate mesh using lens coefficients
 * - RK_ISPFEC_UPDATE_MESH_FROM_FILE: Load mesh from filesystem
 * - RK_ISPFEC_UPDATE_MESH_FROM_BUFFER: Load mesh from pre-allocated buffers
 *
 * @note For file mode:
 * - V20 expects single mesh file path
 * - Legacy hardware requires four component files
 * Thread-safe through internal mutex locking
 */
int rk_ispfec_api_set_mesh_config(rk_ispfec_ctx_t* ctx, rk_ispfec_mesh_config_t* config);

/**
 * @brief Retrieve current mesh correction configuration
 * @param[in] ctx Initialized FEC context handle
 * @param[out] config Pointer to store current configuration
 * @return 0 on success, -1 for invalid parameters
 *
 * @details Provides actual configuration used by hardware:
 * - Online mode returns applied correction coefficients
 * - File mode returns full mesh file paths
 * - DMA mode returns active buffer descriptors
 *
 * @warning Returned DMA file descriptors remain valid until next configuration
 *          change or context destruction
 */
int rk_ispfec_api_get_mesh_config(rk_ispfec_ctx_t* ctx, rk_ispfec_mesh_config_t* config);

#ifdef __cplusplus
}
#endif

#endif