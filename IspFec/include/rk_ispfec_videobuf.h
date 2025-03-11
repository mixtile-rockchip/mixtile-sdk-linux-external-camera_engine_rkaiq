#ifndef RK_ISPFEC_VIDEOBUF_H
#define RK_ISPFEC_VIDEOBUF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rkfec_buffer {
    uint32_t handle;
    int dmabuf_fd;
    uint64_t size;
    void* map;
};

int rk_ispfec_api_vb_init();
void rk_ispfec_api_vb_deinit(int fd);
int rk_ispfec_api_vb_alloc(int fd, uint32_t size, struct rkfec_buffer* buf);
int rk_ispfec_api_vb_free(int fd, struct rkfec_buffer* buf);

#ifdef __cplusplus
}
#endif

#endif  // RK_ISPFEC_VIDEOBUF_H