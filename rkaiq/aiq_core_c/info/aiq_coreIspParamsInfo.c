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

#include "aiq_coreIspParamsInfo.h"

#include "aiq_algo_handler.h"
#include "rk_info_utils.h"

void core_isp_params_dump_mod_param(AiqCore_t* self, st_string* result) {
    char buffer[MAX_LINE_LENGTH] = {0};

    aiq_info_dump_title(result, "module param");

    snprintf(buffer, MAX_LINE_LENGTH, "%-9s", "phy_chn");
    string_printf(result, buffer);
    string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "%-9d", self->mAlogsComSharedParams.mCamPhyId);

    string_printf(result, buffer);
    string_printf(result, "\n\n");
}

void core_isp_params_dump_ldc_params(AiqCore_t* self, st_string* result) {
    AiqAlgoHandler_t* handler = self->mAlgoHandleMaps[RK_AIQ_ALGO_TYPE_ALDC];
    if (!handler || !AiqAlgoHandler_getEnable(handler)) return;

    char buffer[MAX_LINE_LENGTH] = {0};

    aiq_info_dump_title(result, "ldc mod params");

    snprintf(buffer, MAX_LINE_LENGTH, "%-8s%-11s%-9s%-8s", "mod", "is_update", "en", "buf_fd");
    string_printf(result, buffer);
    string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    aiq_params_base_t* base = self->mAiqCurParams->pParamsArray[RESULT_TYPE_LDC_PARAM];
    ldc_param_t* ldc_param  = (ldc_param_t*)(base->_data);
    snprintf(buffer, MAX_LINE_LENGTH, "%-8s%-11s%-9s%-8d", "ldch", base->is_update ? "Y" : "N",
             ldc_param->sta.ldchCfg.en ? "Y" : "N",
             ldc_param->sta.ldchCfg.lutMapCfg.sw_ldcT_lutMapBuf_fd[0]);
    string_printf(result, buffer);
    string_printf(result, "\n");
#if defined(ISP_HW_V39)
    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "%-8s%-11s%-9s%-8d", "ldcv", base->is_update ? "Y" : "N",
             ldc_param->sta.ldcvCfg.en ? "Y" : "N",
             ldc_param->sta.ldcvCfg.lutMapCfg.sw_ldcT_lutMapBuf_fd[0]);
    string_printf(result, buffer);
    string_printf(result, "\n\n");
#endif
    string_printf(result, "\n");
}

int core_isp_params_dump(void* self, st_string* result, int argc, void* argv[]) {
    core_isp_params_dump_mod_param((AiqCore_t*)self, result);
    core_isp_params_dump_ldc_params((AiqCore_t*)self, result);

    return 0;
}
