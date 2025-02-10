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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "RkAinrManager.h"

#if RKAIQ_HAVE_AINR

#define CEIL_DOWN(x, y)                              (((x) + ((y) - 1)) / (y))
#define FLOOR_BY(v, r)                               (((v) / (r)) * (r))
#define CEIL_BY(v, r)                                FLOOR_BY(((v) + (r) - 1), (r))

static bool getValueFromFile(const char* path, int* pos);
static void calcIspBufSize(struct rkaiisp_ispbuf_info *ispbuf_info, int raw_wid, int raw_hgt)
{
    int gain_wid = raw_wid / 8;
    int gain_hgt = raw_hgt / 2;

    ispbuf_info->iir_width  = raw_wid;
    ispbuf_info->iir_height = raw_hgt;
    ispbuf_info->narmap_width  = CEIL_BY(raw_wid / 4, 16);
    ispbuf_info->narmap_height = gain_hgt;

    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            ispbuf_info->raw_width[i-1]  = raw_wid;
            ispbuf_info->raw_height[i-1] = raw_hgt;
        }
        ispbuf_info->sig_width[i]  = raw_wid / 2;
        ispbuf_info->sig_height[i] = raw_hgt / 2;

        raw_hgt = CEIL_BY(CEIL_DOWN(raw_hgt, 2), 2);
        raw_wid = CEIL_BY(CEIL_DOWN(raw_wid, 2), 2);
    }

}

static int getKwtMaxSize(char *model_buf, uint32_t *kwt_size)
{
    struct rkaiisp_model_info *model_info = (struct rkaiisp_model_info *)model_buf;
    int ret = -1;

    *kwt_size = 0;
    if (model_info) {
        struct rkaiisp_kwt_cfg *kwt_cfg;
        uint32_t runcnt;

        runcnt = model_info->model_runcnt;
        kwt_cfg = &model_info->kwt_cfg;
        *kwt_size = kwt_cfg->kwt_offet[runcnt-1] + kwt_cfg->kwt_pad_size[runcnt-1];
        ret = 0;
    }

    return ret;
}

static int getParamInfo(AinrManager_t* pAinrManager, struct rkaiisp_param_info *param_info)
{
    uint32_t kwt_size, max_size;
    RkAiqAinrModelBuf_t *pAinrModel = &pAinrManager->mAinrModel;

    max_size = 0;
    for (int i = 0; i < AINR_ISO_STEP_MAX; i++) {
        if (getKwtMaxSize(pAinrModel->model_buf[i], &kwt_size) != 0) {
            LOGE_AINR("%s: get param info(%s) failed!", __func__, pAinrModel->model_file[i]);
        }

        if (kwt_size > max_size)
            max_size = kwt_size;
    }

    param_info->exemode    = AINR_AIISP_MODE;
    param_info->max_runcnt = pAinrManager->model_max_runcnt;
    param_info->para_size  = max_size + sizeof(struct rkaiisp_params);
    return 0;
}

static void aiisp_params_print(struct rkaiisp_params *aiisp_cfg)
{
    struct rkaiisp_other_cfg *other_cfg = &aiisp_cfg->other_cfg;
    struct rkaiisp_kwt_cfg *kwt_cfg = &aiisp_cfg->kwt_cfg;
    struct rkaiisp_model_cfg *model_cfg;
    u32 i, j;

    LOGD_AINR("aiisp: frame_id %d, module_update = 0x%x", aiisp_cfg->frame_id, aiisp_cfg->module_update);
    if (aiisp_cfg->module_update & RKAIISP_OTHER_UPDATE) {
        LOGD_AINR("sw_neg_noiselimit = 0x%x", other_cfg->sw_neg_noiselimit);
        LOGD_AINR("sw_pos_noiselimit = 0x%x", other_cfg->sw_pos_noiselimit);
        LOGD_AINR("sw_prev_blacklvl = 0x%x", other_cfg->sw_prev_blacklvl);
        LOGD_AINR("sw_post_blacklvl = 0x%x", other_cfg->sw_post_blacklvl);

        for (i = 0; i < 33; i++) {
            LOGD_AINR("sw_in_comp_y[%d] = 0x%x", i, other_cfg->sw_in_comp_y[i]);
        }
        for (i = 0; i < 33; i++) {
            LOGD_AINR("sw_out_decomp_y[%d] = 0x%x", i, other_cfg->sw_out_decomp_y[i]);
        }
    }

    if (aiisp_cfg->module_update & RKAIISP_MODEL_UPDATE) {
        for (i = 0; i < aiisp_cfg->model_runcnt; i++) {
            model_cfg = (struct rkaiisp_model_cfg *)&aiisp_cfg->model_cfg[i];

            LOGD_AINR("model_cfg %d:", i);
            for (j = 0; j < 7; j++) {
                LOGD_AINR("mi_chn_src[%d] = 0x%x", j, model_cfg->mi_chn_src[j]);
            }
            LOGD_AINR("sw_aiisp_mode = %d", model_cfg->sw_aiisp_mode);
            LOGD_AINR("sw_aiisp_level_num = %d", model_cfg->sw_aiisp_level_num);
            LOGD_AINR("sw_aiisp_l1_level_num = %d", model_cfg->sw_aiisp_l1_level_num);
            LOGD_AINR("sw_aiisp_op_mode = %d", model_cfg->sw_aiisp_op_mode);
            LOGD_AINR("sw_aiisp_drop_en = %d", model_cfg->sw_aiisp_drop_en);
            for (j = 0; j < 16; j++) {
                LOGD_AINR("sw_aiisp_lv_active[%d] = 0x%x", j, model_cfg->sw_aiisp_lv_active[j]);
            }
            for (j = 0; j < 16; j++) {
                LOGD_AINR("sw_aiisp_lv_mode[%d] = 0x%x", j, model_cfg->sw_aiisp_lv_mode[j]);
            }
            for (j = 0; j < 7; j++) {
                LOGD_AINR("sw_mi_chn_en[%d] = 0x%x", j, model_cfg->sw_mi_chn_en[j]);
            }
            for (j = 0; j < 7; j++) {
                LOGD_AINR("sw_mi_chn_mode[%d] = 0x%x", j, model_cfg->sw_mi_chn_mode[j]);
            }
            for (j = 0; j < 7; j++) {
                LOGD_AINR("sw_mi_chn_num[%d] = 0x%x", j, model_cfg->sw_mi_chn_num[j]);
            }
            for (j = 0; j < 7; j++) {
                LOGD_AINR("sw_mi_chn_data_mode[%d] = 0x%x", j, model_cfg->sw_mi_chn_data_mode[j]);
            }
            LOGD_AINR("sw_mi_chn1_sel = %d", model_cfg->sw_mi_chn1_sel);
            LOGD_AINR("sw_mi_chn3_sel = %d", model_cfg->sw_mi_chn3_sel);
            LOGD_AINR("sw_out_d2s_en = %d", model_cfg->sw_out_d2s_en);
            LOGD_AINR("sw_out_mode = %d", model_cfg->sw_out_mode);
            LOGD_AINR("sw_lastlvlm1_clip8bit = %d", model_cfg->sw_lastlvlm1_clip8bit);
        }

        for (i = 0; i < RKAIISP_MAX_RUNCNT; i++) {
            LOGD_AINR("kwt_offet[%d] = 0x%x", i, kwt_cfg->kwt_offet[i]);
        }
        for (i = 0; i < RKAIISP_MAX_RUNCNT; i++) {
            LOGD_AINR("kwt_size[%d] = 0x%x", i, kwt_cfg->kwt_size[i]);
        }
        for (i = 0; i < RKAIISP_MAX_RUNCNT; i++) {
            LOGD_AINR("kwt_pad_size[%d] = 0x%x", i, kwt_cfg->kwt_pad_size[i]);
        }
    }
}

static int checkModelSum(char *model_file, char *model_buf, size_t file_len)
{
    uint32_t* pTmp = (unsigned int*)model_buf;
    uint32_t i, checksum, realsum;

    realsum = 0;
    checksum = *pTmp++;
    for (i = 1; i < file_len / 4; i++) {
        realsum += *pTmp++;
    }

    if (checksum != realsum) {
        LOGE_AINR("model(%s) sum check error 0x%x, 0x%x", model_file, checksum, realsum);
        XCAM_ASSERT(false);
        return -1;
    }

    return 0;
}

static int checkModelCfg(struct rkaiisp_model_cfg *model_cfg)
{
    uint32_t i;

    if (model_cfg->sw_aiisp_mode > 1) {
        LOGE_AINR("%s: sw_aiisp_mode %d", __func__, model_cfg->sw_aiisp_mode);
        return -1;
    }
    if (model_cfg->sw_aiisp_op_mode > 1) {
        LOGE_AINR("%s: sw_aiisp_op_mode %d", __func__, model_cfg->sw_aiisp_op_mode);
        return -1;
    }
    if (model_cfg->sw_out_mode > 2) {
        LOGE_AINR("%s: sw_out_mode %d", __func__, model_cfg->sw_out_mode);
        return -1;
    }
    if (model_cfg->sw_aiisp_level_num > 16) {
        LOGE_AINR("%s: sw_aiisp_level_num %d", __func__, model_cfg->sw_aiisp_level_num);
        return -1;
    }

    for (i = 0; i < 7; i++) {
        if (model_cfg->sw_mi_chn_mode[i] > 2) {
            LOGE_AINR("%s: sw_mi_chn_mode %d", __func__, model_cfg->sw_mi_chn_mode[i]);
            return -1;
        }
        if (model_cfg->sw_mi_chn_data_mode[i] > 2) {
            LOGE_AINR("%s: sw_mi_chn_data_mode %d", __func__, model_cfg->sw_mi_chn_data_mode[i]);
            return -1;
        }
    }

    for (i = 0; i < 16; i++) {
        if (model_cfg->sw_aiisp_lv_mode[i] > 4) {
            LOGE_AINR("%s: sw_aiisp_lv_mode %d", __func__, model_cfg->sw_aiisp_lv_mode[i]);
            return -1;
        }
    }

    return 0;
}

static XCamReturn readModel(AinrManager_t* pAinrManager, CamCalibDbV2Context_t* pCalibDbV2)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAinrModelBuf_t *ainr_model = &pAinrManager->mAinrModel;
    ainr_api_attrib_t *ainr_attrib = (ainr_api_attrib_t*)(CALIBDBV2_GET_MODULE_PTR(pCalibDbV2, ainr));
    uint32_t model_max_runcnt = 0;

    for (int i = 0; i < AINR_ISO_STEP_MAX; i++) {
        if (ainr_model->model_buf[i] != NULL) {
            free(ainr_model->model_buf[i]);
            ainr_model->model_buf[i] = NULL;
        }
    }

    if (!ainr_attrib->en)
        return XCAM_RETURN_BYPASS;

    for (int i = 0; i < AINR_ISO_STEP_MAX; i++) {
        ainr_param_dyn_t* pdyn = &(ainr_attrib->stAuto.dyn[i]);
        ainr_param_tunning_t *tunning = &pdyn->tunning;
        struct rkaiisp_model_info model_info;
        size_t file_size, correct_size, runcnt;
        struct rkaiisp_kwt_cfg *kwt_cfg;
        FILE *fp;

        fp = fopen(tunning->sw_ainrT_model_file, "rb");
        if (fp) {
            if (fread(&model_info, sizeof(model_info), 1, fp) != 1) {
                LOGE_AINR("%s(%d): %s: file head is error\n",
                    __FUNCTION__, __LINE__, tunning->sw_ainrT_model_file);
                return XCAM_RETURN_ERROR_FILE;
            }
            fseek(fp, 0, 2);
            file_size = ftell(fp);

            runcnt = model_info.model_runcnt;
            kwt_cfg = &model_info.kwt_cfg;
            if (runcnt != 3 && runcnt != 4) {
                LOGE_AINR("%s(%d): %s: runcnt(%d) is error\n",
                    __FUNCTION__, __LINE__, tunning->sw_ainrT_model_file, runcnt);
                XCAM_ASSERT(false);
                return XCAM_RETURN_ERROR_FILE;
            }

            if (runcnt > model_max_runcnt)
                model_max_runcnt = runcnt;

            correct_size = kwt_cfg->kwt_offet[runcnt-1] + kwt_cfg->kwt_pad_size[runcnt-1];
            if (file_size != correct_size) {
                LOGE_AINR("%s(%d): %s: file size(%d, %d) is error\n",
                    __FUNCTION__, __LINE__, tunning->sw_ainrT_model_file, file_size, correct_size);
                XCAM_ASSERT(false);
                return XCAM_RETURN_ERROR_FILE;
            }

            for (uint32_t j = 0; j < model_info.model_runcnt; j++) {
                if (checkModelCfg(&model_info.model_cfg[j]) != 0) {
                    LOGE_AINR("%s(%d): %s: model info check failure\n",
                        __FUNCTION__, __LINE__, tunning->sw_ainrT_model_file);
                    XCAM_ASSERT(false);
                    return XCAM_RETURN_ERROR_FILE;
                }
            }

            ainr_model->model_buf[i] = aiq_malloc(file_size);
            if (ainr_model->model_buf[i]) {
                fseek(fp, 0, 0);
                fread(ainr_model->model_buf[i], 1, file_size, fp);
                fclose(fp);
                strcpy(ainr_model->model_file[i], tunning->sw_ainrT_model_file);
                checkModelSum(ainr_model->model_file[i], ainr_model->model_buf[i], file_size);
            }
        } else {
            LOGE_AINR("read aiisp model file(%s) failed !", tunning->sw_ainrT_model_file);
        }
    }

    pAinrManager->model_max_runcnt = model_max_runcnt;

    return ret;
}

XCamReturn AinrManager_setCalib(AinrManager_t* pAinrManager, CamCalibDbV2Context_t* pCalibDbV2)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    struct rkaiisp_param_info param_info;

    pAinrManager->pCalibDbV2 = pCalibDbV2;

    if (!pAinrManager->is_enable) {
        LOGD_AINR("%s: ainr is disabled.", __func__);
        return XCAM_RETURN_BYPASS;
    }

    readModel(pAinrManager, pCalibDbV2);
    getParamInfo(pAinrManager, &param_info);
    if (param_info.para_size > pAinrManager->param_info.para_size) {
        LOGE_AINR("para_size(%d) in new calib is too big!", param_info.para_size, pAinrManager->param_info.para_size);
        XCAM_ASSERT(false);
    }
    pAinrManager->pCamHw->ainr_setModelInf(pAinrManager->pCamHw, &pAinrManager->mAinrModel);

    return ret;
}

XCamReturn AinrManager_prepare(AinrManager_t* pAinrManager, AiqCamHwBase_t* pCamHw, CamCalibDbV2Context_t* pCalibDbV2,
    uint32_t isp_acq_width, uint32_t isp_acq_height)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ainr_api_attrib_t *ainr_attrib = (ainr_api_attrib_t*)(CALIBDBV2_GET_MODULE_PTR(pCalibDbV2, ainr));
    struct rkaiisp_ispbuf_info *ispbuf_info = &pAinrManager->ispbuf_info;

    pAinrManager->is_enable       = ainr_attrib->en;
    pAinrManager->is_param_update = false;
    pAinrManager->pCamHw          = pCamHw;
    pAinrManager->pCalibDbV2      = pCalibDbV2;
    pAinrManager->isp_acq_width   = isp_acq_width;
    pAinrManager->isp_acq_height  = isp_acq_height;

    if (ainr_attrib->bypass)
        pAinrManager->is_enable = false;

    if (!pAinrManager->is_enable) {
        LOGD_AINR("%s: ainr is disabled.", __func__);
        return XCAM_RETURN_BYPASS;
    }

    readModel(pAinrManager, pCalibDbV2);
    getParamInfo(pAinrManager, &pAinrManager->param_info);

    pAinrManager->mAinrCfg.mode = (AINR_AIISP_MODE == BOTHEVENT_TO_AIQ) ? 1 : 2;
    pAinrManager->mAinrCfg.wr_linecnt = isp_acq_height;
    pAinrManager->mAinrCfg.rd_linecnt = isp_acq_height;
    pAinrManager->pCamHw->ainr_init(pAinrManager->pCamHw, pAinrManager, &pAinrManager->mAinrCfg);
    pAinrManager->pCamHw->ainr_setModelInf(pAinrManager->pCamHw, &pAinrManager->mAinrModel);
    pAinrManager->pCamHw->ainr_setParamInf(pAinrManager->pCamHw, &pAinrManager->param_info);
    pAinrManager->pCamHw->ainr_prepare(pAinrManager->pCamHw);

    for (int i = 0; i < AINR_AIISP_BUF_CNT; i++)
        pAinrManager->aiisp_idx_pool[i] = false;

    LOGD_AINR("iir_width %d, iir_height %d", isp_acq_width, isp_acq_height);
    calcIspBufSize(ispbuf_info, isp_acq_width, isp_acq_height);
    pAinrManager->pCamHw->ainr_setIspBufInf(pAinrManager->pCamHw, ispbuf_info);

    return ret;
}

XCamReturn AinrManager_updateParams(AinrManager_t* pAinrManager, rk_aiq_isp_ainr_params_t *isp_ainr_params, uint32_t frame_id)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AiqV4l2Buffer_t* pV4l2Buf = NULL;

    if (!pAinrManager->is_enable) {
        LOGD_AINR("%s: ainr is disabled.", __func__);
        return XCAM_RETURN_BYPASS;
    }

    pAinrManager->pCamHw->ainr_getParamsBuf(pAinrManager->pCamHw, &pV4l2Buf);
    if (pV4l2Buf) {
        struct rkaiisp_params* aiisp_cfg = (struct rkaiisp_params*)(AiqV4l2Buffer_getBuf(pV4l2Buf)->m.userptr);
        struct rkaiisp_other_cfg *other_cfg = &aiisp_cfg->other_cfg;
        ainr_param_tunning_t *tunning = &isp_ainr_params->ainr_param.dyn.tunning;

        aiisp_cfg->frame_id = frame_id;
        aiisp_cfg->module_update = RKAIISP_OTHER_UPDATE;
        other_cfg->sw_prev_blacklvl  = isp_ainr_params->prev_blacklvl;
        other_cfg->sw_post_blacklvl  = isp_ainr_params->post_blacklvl;
        other_cfg->sw_pos_noiselimit = pow(tunning->sw_ainrT_noiseAdd_limit, 2) * 32767;
        other_cfg->sw_neg_noiselimit = (0xFFFF - other_cfg->sw_pos_noiselimit + 1) | (1 << 15);

        for (int i = 0; i < 33; i++) {
            other_cfg->sw_in_comp_y[i] = isp_ainr_params->sw_in_comp_y[i];
        }

        for (int i = 0; i < 33; i++) {
            other_cfg->sw_out_decomp_y[i] = isp_ainr_params->sw_out_decomp_y[i];
        }

        if (strcmp((char *)tunning->sw_ainrT_model_file, (char *)pAinrManager->last_ainr_params.ainr_param.dyn.tunning.sw_ainrT_model_file) != 0) {
            struct rkaiisp_model_info *model_info;
            int i;

            for (i = 0; i < AINR_ISO_STEP_MAX; i++) {
                if (strcmp(pAinrManager->mAinrModel.model_file[i], tunning->sw_ainrT_model_file) == 0) {
                    model_info = (struct rkaiisp_model_info *)pAinrManager->mAinrModel.model_buf[i];
                    break;
                }
            }

            if (i >= AINR_ISO_STEP_MAX) {
                LOGE_AINR("can not find model file %s", tunning->sw_ainrT_model_file);
                return XCAM_RETURN_ERROR_FAILED;
            }

            aiisp_cfg->model_runcnt = model_info->model_runcnt;
            memcpy(&aiisp_cfg->model_cfg[0], &model_info->model_cfg[0], sizeof(model_info->model_cfg));
            memcpy(&aiisp_cfg->kwt_cfg, &model_info->kwt_cfg, sizeof(model_info->kwt_cfg));
            for (uint32_t i = 0; i < model_info->model_runcnt; i++) {
                uint32_t offset = model_info->kwt_cfg.kwt_offet[i] - sizeof(struct rkaiisp_model_info) + sizeof(struct rkaiisp_params);
                char *pKwtBuffer;

                pKwtBuffer = (char *)aiisp_cfg + offset;
                aiisp_cfg->kwt_cfg.kwt_offet[i] = offset;
                memcpy(pKwtBuffer, (char *)model_info + model_info->kwt_cfg.kwt_offet[i], model_info->kwt_cfg.kwt_pad_size[i]);
            }

            aiisp_cfg->module_update |= RKAIISP_MODEL_UPDATE;
        }

        aiisp_params_print(aiisp_cfg);

        aiqMutex_lock(&pAinrManager->iq_param_mutex);
        pAinrManager->last_ainr_params = *isp_ainr_params;
        aiqMutex_unlock(&pAinrManager->iq_param_mutex);
        pAinrManager->is_param_update = true;
        pAinrManager->pCamHw->ainr_updateParams(pAinrManager->pCamHw, pV4l2Buf);
    } else {
        LOGE_AINR("%s: Can not get param buffer", __func__);
    }

    return ret;
}

XCamReturn AinrManager_start(AinrManager_t* pAinrManager)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (!pAinrManager->is_enable) {
        LOGD_AINR("%s: ainr is disabled.", __func__);
        return XCAM_RETURN_BYPASS;
    }

    pAinrManager->is_start = true;
    pAinrManager->pCamHw->ainr_start(pAinrManager->pCamHw);

    return ret;
}

XCamReturn AinrManager_stop(AinrManager_t* pAinrManager)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (!pAinrManager->is_enable) {
        LOGD_AINR("%s: ainr is disabled.", __func__);
        return XCAM_RETURN_BYPASS;
    }

    pAinrManager->pCamHw->ainr_stop(pAinrManager->pCamHw);
    pAinrManager->is_start = false;

    return ret;
}

XCamReturn AinrManager_deinit(AinrManager_t* pAinrManager)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAinrModelBuf_t *ainr_model = &pAinrManager->mAinrModel;

    for (int i = 0; i < AINR_ISO_STEP_MAX; i++) {
        if (ainr_model->model_buf[i] != NULL) {
            free(ainr_model->model_buf[i]);
            ainr_model->model_buf[i] = NULL;
        }
    }
    aiqMutex_deInit(&pAinrManager->idx_pool_mutex);
    aiqMutex_deInit(&pAinrManager->iq_param_mutex);

    return ret;
}

static bool getValueFromFile(const char* path, int* pos) {
    const char* delim = " ";
    char buffer[16]   = {0};
    int fp;

    fp = open(path, O_RDONLY | O_SYNC);
    if (fp != -1) {
        if (read(fp, buffer, sizeof(buffer)) <= 0) {
            LOGE_AINR("read %s failed!", path);
            goto OUT;
        } else {
            char* p = NULL;

            p = strtok(buffer, delim);
            if (p != NULL) {
                *pos = atoi(p);
            }
        }
        close(fp);
        return true;
    }

OUT:
    return false;
}

static XCamReturn dump_dmabuf(char *filename, u32 dma_fd, u32 size)
{
    FILE *fp;
    char *dump_addr;

    dump_addr = (char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dma_fd, 0);
    if (MAP_FAILED == dump_addr) {
        LOGE_AINR("fd %d, size %d mmap failed", dma_fd, size);
        return XCAM_RETURN_ERROR_FAILED;
    }

    fp = fopen(filename, "wb");
    if (fp) {
        LOGD_AINR("dump buffer to filename %s", filename);
        fwrite(dump_addr, 1, size, fp);
        fclose(fp);
    } else {
        LOGE_AINR("can not open file %s", filename);
    }

    munmap(dump_addr, size);
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn write_file(char *filename, char *addr, u32 size)
{
    FILE *fp;

    fp = fopen(filename, "wb");
    if (fp) {
        LOGD_AINR("dump buffer to filename %s", filename);
        fwrite(addr, 1, size, fp);
        fclose(fp);
    } else {
        LOGE_AINR("can not open file %s", filename);
    }

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn AinrManager_doIspBe(AinrManager_t* pAinrManager, struct rkisp_aiisp_st *isp_in)
{
    struct rkisp_bnr_buf_info *bnr_buf = &pAinrManager->ispbuf_info.bnr_buf;
    rk_aiq_isp_ainr_params_t *isp_ainr_params;
    struct rkaiisp_model_info *model_info;
    struct rkisp_buf_info *buf_inf;
    char dump_file[256];
    u32 frame_id, idx;
    bool dump_flag = false;
    u32 i;

#define AINR_DUMPBUF_DIR        "/data/"
#define AINR_DUMPBUF_DEBUG
#ifdef AINR_DUMPBUF_DEBUG
    int dummy_index = 0;

    if (getValueFromFile("/data/.ainr_dump", &dummy_index) == true) {
        dump_flag = true;
        remove("/data/.ainr_dump");
    }
#endif

    if (dump_flag) {
        frame_id = isp_in->sequence;
        LOGD_AINR("ainr dump frame %d", frame_id);

        idx = isp_in->iir_index;
        buf_inf = &bnr_buf->iir;
        sprintf(dump_file, "%s/%04d_%02d_iir.raw", AINR_DUMPBUF_DIR, frame_id, idx);
        dump_dmabuf(dump_file, buf_inf->buf_fd[idx], buf_inf->buf_size);

        idx = isp_in->gain_index;
        buf_inf = &bnr_buf->u.v35.gain;
        sprintf(dump_file, "%s/%04d_%02d_gain.raw", AINR_DUMPBUF_DIR, frame_id, idx);
        dump_dmabuf(dump_file, buf_inf->buf_fd[idx], buf_inf->buf_size);

        idx = isp_in->aipre_gain_index;
        buf_inf = &bnr_buf->u.v35.aipre_gain;
        sprintf(dump_file, "%s/%04d_%02d_aipre.raw", AINR_DUMPBUF_DIR, frame_id, idx);
        dump_dmabuf(dump_file, buf_inf->buf_fd[idx], buf_inf->buf_size);

        idx = isp_in->vpsl_index;
        buf_inf = &bnr_buf->u.v35.vpsl;
        sprintf(dump_file, "%s/%04d_%02d_yraw%d-%d_%d-%d_%d-%d_sig%d-%d_%d-%d_%d-%d_%d-%d_vpsl.raw",
            AINR_DUMPBUF_DIR, frame_id, idx,
            bnr_buf->u.v35.vpsl_yraw_offs[0],
            bnr_buf->u.v35.vpsl_yraw_stride[0],
            bnr_buf->u.v35.vpsl_yraw_offs[1],
            bnr_buf->u.v35.vpsl_yraw_stride[1],
            bnr_buf->u.v35.vpsl_yraw_offs[2],
            bnr_buf->u.v35.vpsl_yraw_stride[2],
            bnr_buf->u.v35.vpsl_sig_offs[0],
            bnr_buf->u.v35.vpsl_sig_stride[0],
            bnr_buf->u.v35.vpsl_sig_offs[1],
            bnr_buf->u.v35.vpsl_sig_stride[1],
            bnr_buf->u.v35.vpsl_sig_offs[2],
            bnr_buf->u.v35.vpsl_sig_stride[2],
            bnr_buf->u.v35.vpsl_sig_offs[3],
            bnr_buf->u.v35.vpsl_sig_stride[3]);
        dump_dmabuf(dump_file, buf_inf->buf_fd[idx], buf_inf->buf_size);

        idx = isp_in->aiisp_index;
        buf_inf = &bnr_buf->u.v35.aiisp;
        sprintf(dump_file, "%s/%04d_%02d_aiisp.raw", AINR_DUMPBUF_DIR, frame_id, idx);
        dump_dmabuf(dump_file, buf_inf->buf_fd[idx], buf_inf->buf_size);

        aiqMutex_lock(&pAinrManager->iq_param_mutex);
        isp_ainr_params = &pAinrManager->last_ainr_params;
        for (i = 0; i < AINR_ISO_STEP_MAX; i++) {
            if (strcmp(pAinrManager->mAinrModel.model_file[i], isp_ainr_params->ainr_param.dyn.tunning.sw_ainrT_model_file) == 0) {
                model_info = (struct rkaiisp_model_info *)pAinrManager->mAinrModel.model_buf[i];
                break;
            }
        }

        if (i >= AINR_ISO_STEP_MAX) {
            LOGE_AINR("can not find model file %s in dumpbuf", isp_ainr_params->ainr_param.dyn.tunning.sw_ainrT_model_file);
        } else {
            for (i = 0; i < model_info->model_runcnt; i++) {
                char *pKwtBuffer = (char *)model_info + model_info->kwt_cfg.kwt_offet[i];

                sprintf(dump_file, "%s/%04d_%02d_%01d_%04d_kwt.bin",
                    AINR_DUMPBUF_DIR, frame_id, idx, i,
                    model_info->kwt_cfg.kwt_pad_size[i]);
                write_file(dump_file, pKwtBuffer, model_info->kwt_cfg.kwt_pad_size[i]);
            }
        }
        aiqMutex_unlock(&pAinrManager->iq_param_mutex);
    }

#if 0
    {
        char *iir_addr, *aiisp_addr;
        struct rkisp_buf_info *iir_inf, *aiisp_inf;

        idx = isp_in->iir_index;
        iir_inf = &bnr_buf->iir;
        iir_addr = (char*)mmap(NULL, iir_inf->buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, iir_inf->buf_fd[idx], 0);
        if (MAP_FAILED == iir_addr) {
            LOGE_AINR("fd %d, size %d mmap failed", iir_inf->buf_fd[idx], iir_inf->buf_size);
            return XCAM_RETURN_ERROR_FAILED;
        }

        idx = isp_in->aiisp_index;
        aiisp_inf = &bnr_buf->u.v35.aiisp;
        aiisp_addr = (char*)mmap(NULL, aiisp_inf->buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, aiisp_inf->buf_fd[idx], 0);
        if (MAP_FAILED == aiisp_addr) {
            LOGE_AINR("fd %d, size %d mmap failed", aiisp_inf->buf_fd[idx], aiisp_inf->buf_size);
            return XCAM_RETURN_ERROR_FAILED;
        }

        memcpy(aiisp_addr, iir_addr, iir_inf->buf_size);

        munmap(iir_addr, iir_inf->buf_size);
        munmap(aiisp_addr, aiisp_inf->buf_size);
    }
#endif

    LOGD_AINR("ainr_doIspBe, frame id %d, iir_index %d, gain_index %d, aipre_gain_index %d, vpsl_index %d, aiisp_index %d",
        isp_in->sequence,
        isp_in->iir_index,
        isp_in->gain_index,
        isp_in->aipre_gain_index,
        isp_in->vpsl_index,
        isp_in->aiisp_index);
    pAinrManager->pCamHw->ainr_doIspBe(pAinrManager->pCamHw, isp_in);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn AinrManager_hdlEvent(AinrManager_t* pAinrManager, AiqHwAinnEvt_t *event)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    struct rkisp_aiisp_st *isp_idxbuf = &event->isp_idxbuf;
    struct rkisp_bnr_buf_info *bnr_buf = &pAinrManager->ispbuf_info.bnr_buf;
    int i, aiisp_idx = 0;

    if (!pAinrManager->is_enable) {
        LOGD_AINR("%s: ainr is disabled.", __func__);
        return XCAM_RETURN_BYPASS;
    }

    // isp event
    if (event->_base.type == ISP_POLL_AIISP) {
        if (event->_event_id == RKISP_AIISP_WR_LINECNT_ID) {
            // call aiisp ioctrl to do NRNN
            if (isp_idxbuf->iir_index < 0 || isp_idxbuf->iir_index >= bnr_buf->iir.buf_cnt ||
                isp_idxbuf->gain_index < 0 || isp_idxbuf->gain_index >= bnr_buf->u.v35.gain.buf_cnt ||
                isp_idxbuf->aipre_gain_index < 0 || isp_idxbuf->aipre_gain_index >= bnr_buf->u.v35.aipre_gain.buf_cnt ||
                isp_idxbuf->vpsl_index < 0 || isp_idxbuf->vpsl_index >= bnr_buf->u.v35.vpsl.buf_cnt) {
                LOGE_AINR("WR_LINECNT_ID event error index, to check");
                return XCAM_RETURN_ERROR_FAILED;
            }

            aiqMutex_lock(&pAinrManager->idx_pool_mutex);
            for (i = 0; i < AINR_AIISP_BUF_CNT; i++) {
                if (pAinrManager->aiisp_idx_pool[i] == 0) {
                    pAinrManager->aiisp_idx_pool[i] = true;//buf to using
                    aiisp_idx = i;
                    break;
                }
            }
            if (i == AINR_AIISP_BUF_CNT) {
                LOGE_AINR("no free aiisp buf, to check");
                return XCAM_RETURN_ERROR_FAILED;
            }
            aiqMutex_unlock(&pAinrManager->idx_pool_mutex);

            isp_idxbuf->aiisp_index = aiisp_idx;
            pAinrManager->isp_out = event->isp_idxbuf;
            LOGD_AINR("ainr_doNrnn, frame id %d, iir_index %d, gain_index %d, aipre_gain_index %d, vpsl_index %d, aiisp_index %d",
                pAinrManager->isp_out.sequence,
                pAinrManager->isp_out.iir_index,
                pAinrManager->isp_out.gain_index,
                pAinrManager->isp_out.aipre_gain_index,
                pAinrManager->isp_out.vpsl_index,
                pAinrManager->isp_out.aiisp_index);
            pAinrManager->pCamHw->ainr_doNrnn(pAinrManager->pCamHw, &pAinrManager->isp_out);
        } else {
            if (isp_idxbuf->aiisp_index < 0 || isp_idxbuf->aiisp_index >= bnr_buf->u.v35.aiisp.buf_cnt) {
                LOGE_AINR("RD_LINECNT_ID event error index, to check");
                return XCAM_RETURN_ERROR_FAILED;
            }
            aiqMutex_lock(&pAinrManager->idx_pool_mutex);
            pAinrManager->aiisp_idx_pool[isp_idxbuf->aiisp_index] = false;
            aiqMutex_unlock(&pAinrManager->idx_pool_mutex);
        }
    }
    // aiisp event
    else if (event->_base.type == ISP_POLL_AINR_DONE) {
        // inform isp to do BE
        pAinrManager->isp_in = event->isp_idxbuf;
        AinrManager_doIspBe(pAinrManager, &pAinrManager->isp_in);
    }
    // rknn event
    else if (event->_base.type == ISP_POLL_RKNN_DONE) {
        // inform isp to do BE
        pAinrManager->isp_in = event->isp_idxbuf;
        AinrManager_doIspBe(pAinrManager, &pAinrManager->isp_in);
    }

    return ret;
}

bool AinrManager_isNeedRknn(AinrManager_t* pAinrManager)
{
    bool ret = false;

    if (pAinrManager->is_param_update && pAinrManager->is_enable) {
        ret = pAinrManager->last_ainr_params.ainr_param.dyn.tunning.sw_ainrT_doRknn_en;
    }

    // force false
    ret = false;

    return ret;
}

XCamReturn AinrManager_init(AinrManager_t* pAinrManager)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    memset(pAinrManager, 0, sizeof(AinrManager_t));
    aiqMutex_init(&pAinrManager->idx_pool_mutex);
    aiqMutex_init(&pAinrManager->iq_param_mutex);

    return ret;
}
#endif

