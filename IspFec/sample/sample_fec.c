/*
 *  Copyright (c) 2024 Rockchip Corporation
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

#include "sample_fec.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
// #include <linux/videodev2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>

#include "rk_ispfec_api.h"
#ifdef RKFEC_HW_V20
#include "rk_ispfec_videobuf.h"
#endif
#include <rk-video-format.h>
#include "sample_common.h"
#include "sample_drm_buffer.h"

struct mem_ops {
    int (*init)(void);
    void (*deinit)(int fd);
    int (*alloc)(int fd, uint32_t size, struct rkfec_buffer* buf);
    int (*free)(int fd, struct rkfec_buffer* buf);
};

#define IN_IMAGE_FILE "/data/imx415_4k_nv12.yuv"

static const char* file_from;
static const char* file_to;
static const char* mesh_from;
static unsigned stream_count;
static char options[OptLast];

static int width, height, out_width, out_height, pixfmt, out_pixfmt;
static int mesh_density, border_mode, cross_buf_mode;
static rk_ispfec_bg_val bg_val;

static rk_ispfec_ctx_t* g_ispfec_ctx = NULL;
static rk_ispfec_cfg_t g_ispfec_cfg;

struct rkfec_buffer g_rkfec_buf_pic_in;
struct rkfec_buffer g_rkfec_buf_pic_out;

#ifdef RKFEC_HW_V20
struct rkfec_buffer g_rkfec_buf_mesh;
#else
struct rkfec_buffer g_rkfec_buf_xint;
struct rkfec_buffer g_rkfec_buf_xfra;
struct rkfec_buffer g_rkfec_buf_yint;
struct rkfec_buffer g_rkfec_buf_yfra;
#endif

struct mem_ops* cur_mem_pos;
static int dma_node_fd = -1;

#define IS_DMA_INVALID() (dma_node_fd < 0)

static struct option long_options[] = {
    // clang-format off
    {"help", no_argument, 0, OptHelp},
    {"silent", no_argument, 0, OptSilent},
    {"set-fmt-video", required_argument, NULL, OptSetVideoFormat},
    {"set-fmt-video-out", required_argument, NULL, OptSetVideoOutFormat},
    {"stream-count", required_argument, NULL, OptStreamCount},
    {"stream-to", required_argument, NULL, OptStreamTo},
    {"stream-from", required_argument, NULL, OptStreamFrom},
    {"mesh-from", required_argument, NULL, OptMeshFrom},
    {"mesh-density", required_argument, NULL, OptMeshDensity},
    {"border-mode", required_argument, NULL, OptBorderMode},
    {"cross-buf-mode", required_argument, NULL, OptCrossBufMode},
    {"set-bg-val", required_argument, NULL, OptSetBgVal},
    {0, 0, 0, 0}
    // clang-format on
};

void common_usage(void) {
    printf(
        "\nGeneral/Common options:\n"
        "  -v, --set-fmt-video\n"
        "  -x, --set-fmt-video-out "
        "width=<w>,height=<h>,pixelformat=<pf>,field=<f>,colorspace=<c>,\n"
        "  --stream-count <count>\n"
        "                     stream <count> buffers. The default is to keep streaming\n"
        "                     forever. This count does not include the number of initial\n"
        "                     skipped buffers as is passed by --stream-skip.\n"
        "  --stream-from <file> stream from this file.\n"
        "  --stream-to <file>\n"
        "                     stream to this file. The default is to discard the data.\n"
        "  --mesh-from <file> mesh from this file.\n"
        "  --mesh-density <val> mesh density.\n"
        "                       0: 32x16. 1: 16x8. 2: 4x4\n"
        "  --border-mode <val> border mode.\n"
        "                       0: fill with bg_value. 1: copy with the nearest pixel."
        "  --cross-buf-mode <val> cross buffer mode\n"
        "                       0: fill with bg_value. 1: copy with the nearest pixel."
        "  --set-bg-val <val> y=<y>,u=<u>,v=<v>\n"
        "                     fill with the bg_val."
        "  -s, --silent       subpress debug log.\n"
        "  -h, --help         display this help message.\n");
}

struct mem_ops rk_mem_ops_tbl[] = {
    {
        .init   = init_drm,
        .deinit = deinit_drm,
        .alloc  = alloc_drm_buffer,
        .free   = free_drm_buffer,
    },
#ifdef RKFEC_HW_V20
    {
        .init   = rk_ispfec_api_vb_init,
        .deinit = rk_ispfec_api_vb_deinit,
        .alloc  = rk_ispfec_api_vb_alloc,
        .free   = rk_ispfec_api_vb_free,
    },
#endif
};

int ispfec_mem_init(void) {
    int ret = -1;
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(rk_mem_ops_tbl); i++) {
        dma_node_fd = rk_mem_ops_tbl[i].init();
        if (dma_node_fd >= 0) break;
    }

    if (dma_node_fd < 0) {
        ret = -1;
        goto exit;
    }

    cur_mem_pos = &rk_mem_ops_tbl[i];

    ret = 0;
exit:
    return ret;
}

void rkfec_mem_deinit(void) {
    if (IS_DMA_INVALID()) return;

    if (cur_mem_pos) {
        cur_mem_pos->deinit(dma_node_fd);
        cur_mem_pos = NULL;
    }
}

int init_ispfec_bufs(rk_ispfec_cfg_t* cfg) {
    int ret    = 0;

    ret = ispfec_mem_init();
    if (ret < 0) {
        stderr_info("failed to init mem\n");
        return ret;
    }

    int mesh_size = rk_ispfec_api_calFecMeshsize(cfg->out_width, cfg->out_height);
    // 1. alloc mesh buffer
#ifdef RKFEC_HW_V20
    ret = cur_mem_pos->alloc(dma_node_fd, mesh_size * 6, &g_rkfec_buf_mesh);
    if (ret) goto deinit_mem;
    info("mesh fd:%d size:%" PRId64 "\n", g_rkfec_buf_mesh.dmabuf_fd, g_rkfec_buf_mesh.size);
#else
    ret = cur_mem_pos->alloc(dma_node_fd, mesh_size * 2, &g_rkfec_buf_xint);
    if (ret) goto deinit_mem;
    info("xint fd:%d size:%" PRId64 "\n", g_rkfec_buf_xint.dmabuf_fd, g_rkfec_buf_xint.size);

    ret = cur_mem_pos->alloc(dma_node_fd, mesh_size, &g_rkfec_buf_xfra);
    if (ret) goto free_drm_buf_xint;
    info("xfra fd:%d size:%" PRId64 "\n", g_rkfec_buf_xfra.dmabuf_fd, g_rkfec_buf_xfra.size);

    ret = cur_mem_pos->alloc(dma_node_fd, mesh_size * 2, &g_rkfec_buf_yint);
    if (ret) goto free_drm_buf_xfra;
    info("yint fd:%d size:%" PRId64 "\n", g_rkfec_buf_yint.dmabuf_fd, g_rkfec_buf_yint.size);

    ret = cur_mem_pos->alloc(dma_node_fd, mesh_size, &g_rkfec_buf_yfra);
    if (ret) goto free_drm_buf_yint;
    info("yfra fd:%d size:%" PRId64 "\n", g_rkfec_buf_yfra.dmabuf_fd, g_rkfec_buf_yfra.size);
#endif

    // 2. alloc input picture buffer
    ret = cur_mem_pos->alloc(dma_node_fd, cfg->in_width * (cfg->in_height + 1) * 3 / 2,
                             &g_rkfec_buf_pic_in);
    if (ret) goto free_drm_buf_mesh;
    info("in pic fd:%d size: %" PRId64 "\n", g_rkfec_buf_pic_in.dmabuf_fd, g_rkfec_buf_pic_in.size);

    // 3. alloc output picture buffer
    if (out_pixfmt == V4L2_PIX_FMT_FBC0) {
        int stride_y = (cfg->out_width + 63) / 64 * 384;
        int stride_c = (cfg->out_width + 63) / 64 * 16;
        int size     = (stride_y * cfg->out_height + stride_c * cfg->out_height) / 4;

        ret = cur_mem_pos->alloc(dma_node_fd, size, &g_rkfec_buf_pic_out);
	g_rkfec_buf_pic_out.size = size;
    } else {
        ret = cur_mem_pos->alloc(dma_node_fd, cfg->in_width * (cfg->in_height + 1) * 3 / 2,
                                 &g_rkfec_buf_pic_out);
	g_rkfec_buf_pic_out.size = cfg->out_width * cfg->out_height * 3 / 2;
    }
    if (ret) goto free_drm_buf_pic_in;
    info("out pic fd:%d size: %" PRId64 "\n", g_rkfec_buf_pic_out.dmabuf_fd, g_rkfec_buf_pic_out.size);

#ifdef RKFEC_HW_V20
    cfg->mesh_info.dmaFd        = g_rkfec_buf_mesh.dmabuf_fd;
    cfg->mesh_info.size         = g_rkfec_buf_mesh.size;
    cfg->mesh_info.vir_addr     = g_rkfec_buf_mesh.map;
#else
    cfg->mesh_xint.dmaFd    = g_rkfec_buf_xint.dmabuf_fd;
    cfg->mesh_xint.size     = g_rkfec_buf_xint.size;
    cfg->mesh_xint.vir_addr = g_rkfec_buf_xint.map;

    cfg->mesh_xfra.dmaFd    = g_rkfec_buf_xfra.dmabuf_fd;
    cfg->mesh_xfra.size     = g_rkfec_buf_xfra.size;
    cfg->mesh_xfra.vir_addr = g_rkfec_buf_xfra.map;

    cfg->mesh_yint.dmaFd    = g_rkfec_buf_yint.dmabuf_fd;
    cfg->mesh_yint.size     = g_rkfec_buf_yint.size;
    cfg->mesh_yint.vir_addr = g_rkfec_buf_yint.map;

    cfg->mesh_yfra.dmaFd    = g_rkfec_buf_yfra.dmabuf_fd;
    cfg->mesh_yfra.size     = g_rkfec_buf_yfra.size;
    cfg->mesh_yfra.vir_addr = g_rkfec_buf_yfra.map;
#endif

    return ret;

free_drm_buf_pic_in:
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_pic_in);
free_drm_buf_mesh:
#ifdef RKFEC_HW_V20
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_mesh);
#else
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_yfra);
free_drm_buf_yint:
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_yfra);
free_drm_buf_xfra:
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_xfra);
free_drm_buf_xint:
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_xint);
#endif
deinit_mem:
    rkfec_mem_deinit();

    return ret;
}

void deinit_ispfec_bufs() {
#ifdef RKFEC_HW_V20
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_mesh);
#else
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_yfra);
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_yfra);
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_xfra);
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_xint);
#endif
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_pic_in);
    cur_mem_pos->free(dma_node_fd, &g_rkfec_buf_pic_out);
    rkfec_mem_deinit();
}

size_t read_image_from_file(const char* file_path, char* buffer, size_t buffer_size) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        stderr_info("Could not open %s for reading\n", file_path);
        return 0;
    }

    size_t bytes_read = fread(buffer, 1, buffer_size - 1, file);

    fclose(file);
    return bytes_read;
}

FILE* open_output_file() {
    FILE* fout = NULL;

    if (file_to) {
        fout = fopen(file_to, "w+");
        if (!fout) stderr_info("Could not open %s for writing\n", file_to);
        return fout;
    }

    return fout;
}

size_t write_buffer_to_file(FILE* fout, const char* buffer, size_t buffer_size) {
    size_t bytes_written = fwrite(buffer, 1, buffer_size, fout);
    if (bytes_written != buffer_size) {
        stderr_info("Error writing to file");
    }
    return bytes_written;
}

int do_handle_fec(FILE* fout) {
    char ch = '<';
    int ret;

    ret = rk_ispfec_api_process(g_ispfec_ctx, g_rkfec_buf_pic_in.dmabuf_fd,
                                g_rkfec_buf_pic_out.dmabuf_fd);
    if (ret) {
        stderr_info("rk_ispfec_api_process failed: %s\n", strerror(errno));
        return -1;
    }

    if (fout) {
        ret = write_buffer_to_file(fout, g_rkfec_buf_pic_out.map, g_rkfec_buf_pic_out.size);

        if (ret <= 0) {
            stderr_info("write_buffer_to_file failed: %s\n", strerror(errno));
            return -1;
        }

        stderr_info("%c\n", ch);
        fflush(stderr);
    }

    if (stream_count == 0) return 0;

    if (--stream_count == 0) return -1;

    return 0;
}

void parse_path(const char* path, char* directory, char* filename) {
    // 找到最后一个 '/' 的位置
    const char* last_slash = strrchr(path, '/');
    if (last_slash) {
        // 提取目录部分
        size_t dir_len = last_slash - path + 1;  // 包括最后的 '/'
        strncpy(directory, path, dir_len);
        directory[dir_len] = '\0';  // 确保字符串以 '\0' 结尾

        // 提取文件名部分
        strcpy(filename, last_slash + 1);
    } else {
        // 如果没有 '/'，认为路径中只有文件名
        directory[0] = '\0';  // 目录为空
        strcpy(filename, path);
    }
}

int getsubopt(char** opt, char* const* keys, char** val) {
    char* s = *opt;
    int i;

    *val = NULL;
    *opt = strchr(s, ',');
    if (*opt)
        *(*opt)++ = 0;
    else
        *opt = s + strlen(s);

    for (i = 0; keys[i]; i++) {
        size_t l = strlen(keys[i]);
        if (strncmp(keys[i], s, l)) continue;
        if (s[l] == '=')
            *val = s + l + 1;
        else if (s[l])
            continue;
        return i;
    }
    return -1;
}

int parse_subopt(char** subs, const char* const* subopts, char** value) {
    int opt = getsubopt(subs, (char* const*)(subopts), value);

    if (opt == -1) {
        fprintf(stderr, "Invalid suboptions specified\n");
        return -1;
    }
    if (*value == NULL) {
        fprintf(stderr, "No value given to suboption <%s>\n", subopts[opt]);
        return -1;
    }
    return opt;
}

int parse_bg_val(char* optarg, int* bg_y, int* bg_u, int* bg_v) {
    char *value, *subs;

    subs = optarg;
    while (*subs != '\0') {
        static const char* subopts[] = {"y", "u", "v", NULL};

        switch (parse_subopt(&subs, subopts, &value)) {
            case 0:
                *bg_y = strtoul(value, NULL, 0);
                break;
            case 1:
                *bg_u = strtoul(value, NULL, 0);
                break;
            case 2:
                *bg_v = strtoul(value, NULL, 0);
                break;
            default:
                return -1;
        }
    }
    return 0;
}

int parse_fmt(char* optarg, int* width, int* height, int* pixelformat) {
    char *value, *subs;
    bool be_pixfmt;

    subs = optarg;
    while (*subs != '\0') {
        static const char* subopts[] = {"width", "height", "pixelformat", NULL};

        switch (parse_subopt(&subs, subopts, &value)) {
            case 0:
                *width = strtoul(value, NULL, 0);
                break;
            case 1:
                *height = strtoul(value, NULL, 0);
                break;
            case 2:
                be_pixfmt = strlen(value) == 7 && !memcmp(value + 4, "-BE", 3);
                if (be_pixfmt || strlen(value) == 4) {
                    *pixelformat = v4l2_fourcc(value[0], value[1], value[2], value[3]);
                    if (be_pixfmt) *pixelformat |= 1U << 31;
                } else if (isdigit(value[0])) {
                    *pixelformat = strtol(value, NULL, 0);
                } else {
                    fprintf(stderr, "The pixelformat '%s' is invalid\n", value);
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                return -1;
        }
    }
    return 0;
}

int32_t main(int argc, char** argv) {
    int32_t i;

    /* command args */
    int32_t ch;
    char short_options[6 * 2 * 3 + 1];
    int32_t idx = 0;
    FILE* fout  = NULL;

    if (argc == 1) {
        common_usage();
        return 0;
    }
    for (i = 0; long_options[i].name; i++) {
        if (!isalpha(long_options[i].val)) continue;
        short_options[idx++] = long_options[i].val;
        if (long_options[i].has_arg == required_argument) {
            short_options[idx++] = ':';
        } else if (long_options[i].has_arg == optional_argument) {
            short_options[idx++] = ':';
            short_options[idx++] = ':';
        }
    }
    while (1) {
        int option_index = 0;
        int ret          = 0;

        short_options[idx] = 0;
        ch                 = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (ch == -1) break;

        options[(int)ch] = 1;
        switch (ch) {
            case OptHelp:
                common_usage();
                return 0;
            case OptSetVideoFormat:
                ret = parse_fmt(optarg, &width, &height, &pixfmt);
                if (ret < 0) {
                    common_usage();
                    exit(EXIT_FAILURE);
                }
                break;
            case OptSetVideoOutFormat:
                ret = parse_fmt(optarg, &out_width, &out_height, &out_pixfmt);
                if (ret < 0) {
                    common_usage();
                    exit(EXIT_FAILURE);
                }
                break;
            case OptStreamCount:
                stream_count = strtoul(optarg, NULL, 0);
                break;
            case OptStreamTo:
                file_to = optarg;
                break;
            case OptStreamFrom:
                file_from = optarg;
                break;
            case OptMeshFrom:
                mesh_from = optarg;
                break;
            case OptMeshDensity:
                mesh_density = atoi(optarg);
                break;
            case OptBorderMode:
                border_mode = atoi(optarg);
                break;
            case OptCrossBufMode:
                cross_buf_mode = atoi(optarg);
                break;
            case OptSetBgVal:
                ret = parse_bg_val(optarg, &bg_val.bg_y, &bg_val.bg_u, &bg_val.bg_v);
                if (ret < 0) printf("Failed to parse bg_val\n");
                break;
            default:
                break;
        }
    }
    if (optind < argc) {
        printf("unknown arguments: ");
        while (optind < argc) printf("%s ", argv[optind++]);
        printf("\n");
        common_usage();
        exit(EXIT_FAILURE);
    }

    g_ispfec_cfg.in_width       = width;
    g_ispfec_cfg.in_height      = height;
    g_ispfec_cfg.out_width      = out_width;
    g_ispfec_cfg.out_height     = out_height;
    g_ispfec_cfg.in_fourcc      = pixfmt;
    g_ispfec_cfg.out_fourcc     = out_pixfmt;
#ifdef RKFEC_HW_V20
    g_ispfec_cfg.border_mode    = border_mode;
    g_ispfec_cfg.cross_buf_mode = cross_buf_mode;
    g_ispfec_cfg.bg_val         = bg_val;
#endif

#if 1
    g_ispfec_cfg.mesh_upd_mode = RK_ISPFEC_UPDATE_MESH_FROM_FILE;
#ifdef RKFEC_HW_V20
    parse_path(mesh_from, g_ispfec_cfg.u.mesh_file_path, g_ispfec_cfg.mesh_info.mesh_file);
#else
    strcpy(g_ispfec_cfg.u.mesh_file_path, "/etc/iqfiles/FEC_mesh_3840_2160_imx415_3.6mm/");
    strcpy(g_ispfec_cfg.mesh_xint.mesh_file, "meshxi_level0.bin");
    strcpy(g_ispfec_cfg.mesh_xfra.mesh_file, "meshxf_level0.bin");
    strcpy(g_ispfec_cfg.mesh_yint.mesh_file, "meshyi_level0.bin");
    strcpy(g_ispfec_cfg.mesh_yfra.mesh_file, "meshyf_level0.bin");
#endif
#else
    g_ispfec_cfg.mesh_upd_mode                 = RK_ISPFEC_UPDATE_MESH_ONLINE;
    g_ispfec_cfg.u.mesh_online.light_center[0] = 1956.3909119999998438;
    g_ispfec_cfg.u.mesh_online.light_center[1] = 1140.6355200000000422;
    g_ispfec_cfg.u.mesh_online.coeff[0]        = -2819.4072493821618081;
    g_ispfec_cfg.u.mesh_online.coeff[1]        = 0.0000316126581792;
    g_ispfec_cfg.u.mesh_online.coeff[2]        = 0.0000000688410142;
    g_ispfec_cfg.u.mesh_online.coeff[3]        = -0.0000000000130686;
    g_ispfec_cfg.u.mesh_online.correct_level   = 250;
    g_ispfec_cfg.u.mesh_online.direction       = RK_ISPFEC_CORRECT_DIRECTION_XY;
    g_ispfec_cfg.u.mesh_online.style           = RK_ISPFEC_KEEP_ASPECT_RATIO_REDUCE_FOV;
#endif

    int ret = init_ispfec_bufs(&g_ispfec_cfg);
    if (ret < 0) {
        stderr_info("init_ispfec_bufs failed: %s\n", strerror(errno));
        return -1;
    }

    g_ispfec_ctx = rk_ispfec_api_init(&g_ispfec_cfg);
    if (!g_ispfec_ctx) {
        stderr_info("rk_ispfec_api_init failed: %s\n", strerror(errno));
        goto error_init;
    }

    if (!file_from) file_from = IN_IMAGE_FILE;

    ret = read_image_from_file(file_from, g_rkfec_buf_pic_in.map, g_rkfec_buf_pic_in.size);
    if (ret <= 0) {
        stderr_info("read_image_from_file failed: %s\n", strerror(errno));
        goto clean_up;
    }

    fout = open_output_file();

    while (1) {
        ret = do_handle_fec(fout);
        if (ret < 0) break;

        FEC_STATIC_FPS_CALCULATION(sample_fec, 30);
    }

clean_up:
    if (fout) fclose(fout);
    rk_ispfec_api_deinit(g_ispfec_ctx);
error_init:
    deinit_ispfec_bufs();
    g_ispfec_ctx = NULL;

    return 0;
}
