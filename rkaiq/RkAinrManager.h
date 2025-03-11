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

#ifndef _RK_AINR_MANAGER_H_
#define _RK_AINR_MANAGER_H_

#include "hwi_c/aiq_CamHwBase.h"
#include "rk-aiisp-config.h"
#include "c_base/aiq_mutex.h"

RKAIQ_BEGIN_DECLARE

#define AINR_IIR_BUF_CNT                3
#define AINR_GAIN_BUF_CNT               4
#define AINR_AIPRE_BUF_CNT              3
#define AINR_VPSL_BUF_CNT               3
#define AINR_AIISP_BUF_CNT              3
#define AINR_AIISP_MODE                 BOTHEVENT_TO_AIQ

#if RKAIQ_HAVE_AINR

typedef struct AinrManager_s AinrManager_t;
typedef struct AinrManager_s {
    bool is_enable;
    bool is_bypass;
    bool is_start;
    bool is_param_update;
    bool is_parambuf_prepare;
    uint32_t model_max_runcnt;
    uint32_t isp_acq_width;
    uint32_t isp_acq_height;
    rk_aiq_isp_ainr_params_t last_ainr_params;
    struct rkaiisp_param_info param_info;
    struct rkaiisp_ispbuf_info ispbuf_info;
    struct rkisp_aiisp_st isp_out;
    struct rkisp_aiisp_st isp_in;
    struct rkisp_aiisp_cfg mAinrCfg;
    RkAiqAinrModelBuf_t mAinrModel;
    CamCalibDbV2Context_t* pCalibDbV2;
    AiqCamHwBase_t* pCamHw;

    bool aiisp_idx_pool[AINR_AIISP_BUF_CNT];
    AiqMutex_t idx_pool_mutex;
    AiqMutex_t iq_param_mutex;

} AinrManager_t;

XCamReturn AinrManager_init(AinrManager_t* pAinrManager);
XCamReturn AinrManager_setCalib(AinrManager_t* pAinrManager, CamCalibDbV2Context_t* pCalibDbV2);
XCamReturn AinrManager_deinit(AinrManager_t* pAinrManager);
XCamReturn AinrManager_prepare(AinrManager_t* pAinrManager, AiqCamHwBase_t* pCamHw, CamCalibDbV2Context_t* pCalibDbV2,
                               uint32_t isp_acq_width, uint32_t isp_acq_height);
XCamReturn AinrManager_start(AinrManager_t* pAinrManager);
XCamReturn AinrManager_stop(AinrManager_t* pAinrManager);
XCamReturn AinrManager_updateParams(AinrManager_t* pAinrManager, rk_aiq_isp_ainr_params_t *ainr_params, uint32_t frame_id);
XCamReturn AinrManager_hdlEvent(AinrManager_t* pAinrManager, AiqHwAinnEvt_t *event);
XCamReturn AinrManager_setCalib(AinrManager_t* pAinrManager, CamCalibDbV2Context_t* pCalibDbV2);
bool AinrManager_isNeedRknn(AinrManager_t* pAinrManager);

#endif

RKAIQ_END_DECLARE

#endif
