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
#include "rk_aiq_types_priv_c.h"

#include "algo_types_priv.h"
#include "ainr_types_prvt.h"
#include "xcam_log.h"

#include "RkAiqCalibDbTypes.h"
#include "RkAiqCalibDbTypesV2.h"
#include "RkAiqCalibDbV2Helper.h"
#include "interpolation.h"
#include "c_base/aiq_base.h"
#include "newStruct/algo_common.h"
#include "common/rk-aiisp-config.h"

#include "rk_info_utils.h"

static void printParam(AinrContext_t *pAinrCtx, ainr_param_t* out, int iso)
{
    ainr_param_dyn_t *dyn = &out->dyn;
    ainr_param_calib_t *calib = &dyn->calib;
    ainr_param_tunning_t *tunning = &dyn->tunning;

    LOGD_AINR("algo_ainr: working mode=%d iso=%d mean luma=%f",
              pAinrCtx->working_mode, pAinrCtx->pre_iso, pAinrCtx->mean_luma);

    LOGD_AINR("sw_ainrC_blkPrev_val=%f sw_ainrC_blkPost_val=%f sw_ainrC_shotNoise_val=%f sw_ainrC_readNoise_val=%f sw_ainrC_fixedNoise_val=%f",
              calib->sw_ainrC_blkPrev_val, calib->sw_ainrC_blkPost_val, calib->sw_ainrC_shotNoise_val,
              calib->sw_ainrC_readNoise_val, calib->sw_ainrC_fixedNoise_val);

    LOGD_AINR("sw_ainrT_totalNr_strg=%f sw_ainrT_shotNr_strg=%f sw_ainrT_readNr_strg=%f sw_ainrT_fixedNr_strg=%f",
              tunning->sw_ainrT_totalNr_strg, tunning->sw_ainrT_shotNr_strg,
              tunning->sw_ainrT_readNr_strg, tunning->sw_ainrT_fixedNr_strg);

    LOGD_AINR("sw_ainrT_motionBias_offset=%f sw_ainrT_motionGain_strg=%f sw_ainrT_noiseAdd_ratio=%f sw_ainrT_noiseAdd_limit=%f",
              tunning->sw_ainrT_motionBias_offset, tunning->sw_ainrT_motionGain_strg,
              tunning->sw_ainrT_noiseAdd_ratio, tunning->sw_ainrT_noiseAdd_limit);

    LOGD_AINR("sw_ainrT_noiseAddMot_offset=%f sw_ainrT_noiseAddMot_coeff=%f sw_ainrT_noiseAddLuma_offset=%f sw_ainrT_noiseAddLuma_coeff=%f",
              tunning->sw_ainrT_noiseAddMot_offset, tunning->sw_ainrT_noiseAddMot_coeff,
              tunning->sw_ainrT_noiseAddLuma_offset, tunning->sw_ainrT_noiseAddLuma_coeff);

    LOGD_AINR("sw_ainrT_noiseAddLumaClip_th=%f sw_ainrT_noiseAddLumaStatic_th=%f sw_ainrT_nonLinear_scale=%f sw_ainrT_nonLinear_ratio=%f",
              tunning->sw_ainrT_noiseAddLumaClip_th, tunning->sw_ainrT_noiseAddLumaStatic_th,
              tunning->sw_ainrT_nonLinear_scale, tunning->sw_ainrT_nonLinear_ratio);

    LOGD_AINR("sw_ainrT_model_file=%s sw_ainrT_doRknn_en=%d",
            tunning->sw_ainrT_model_file, tunning->sw_ainrT_doRknn_en);
}

XCamReturn AinrSelectParam
(
    AinrContext_t *pAinrCtx,
    ainr_param_t* out,
    int iso)
{
    ainr_param_auto_t *paut = &pAinrCtx->ainr_attrib->stAuto;
    ainr_param_dyn_t *dyn = &out->dyn;
    ainr_param_calib_t *calib = &dyn->calib;
    ainr_param_tunning_t *tunning = &dyn->tunning;
    int ilow = 0, ihigh = 0;
    float ratio = 0.0f;
    float mean_luma = 0.0f;

    if (paut == NULL || out == NULL) {
        LOGE_AINR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    pre_interp(iso, NULL, 0, &ilow, &ihigh, &ratio);
    LOGD_AINR("%s: iso %d, ilow %d, ihigh %d, ratio %f", __FUNCTION__, iso, ilow, ihigh, ratio);

    tunning->sw_ainrT_doRknn_en = paut->dyn[ilow].tunning.sw_ainrT_doRknn_en;
    if (ratio < 0.5)
        strcpy(tunning->sw_ainrT_model_file, paut->dyn[ilow].tunning.sw_ainrT_model_file);
    else
        strcpy(tunning->sw_ainrT_model_file, paut->dyn[ihigh].tunning.sw_ainrT_model_file);

    calib->sw_ainrC_blkPrev_val = interpolation_f32(
                    paut->dyn[ilow].calib.sw_ainrC_blkPrev_val, paut->dyn[ihigh].calib.sw_ainrC_blkPrev_val, ratio);
    calib->sw_ainrC_blkPost_val = interpolation_f32(
                    paut->dyn[ilow].calib.sw_ainrC_blkPost_val, paut->dyn[ihigh].calib.sw_ainrC_blkPost_val, ratio);
    calib->sw_ainrC_shotNoise_val = interpolation_f32(
                    paut->dyn[ilow].calib.sw_ainrC_shotNoise_val, paut->dyn[ihigh].calib.sw_ainrC_shotNoise_val, ratio);
    calib->sw_ainrC_readNoise_val = interpolation_f32(
                    paut->dyn[ilow].calib.sw_ainrC_readNoise_val, paut->dyn[ihigh].calib.sw_ainrC_readNoise_val, ratio);
    calib->sw_ainrC_fixedNoise_val = interpolation_f32(
                    paut->dyn[ilow].calib.sw_ainrC_fixedNoise_val, paut->dyn[ihigh].calib.sw_ainrC_fixedNoise_val, ratio);
    tunning->sw_ainrT_totalNr_strg = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_totalNr_strg, paut->dyn[ihigh].tunning.sw_ainrT_totalNr_strg, ratio);
    tunning->sw_ainrT_shotNr_strg = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_shotNr_strg, paut->dyn[ihigh].tunning.sw_ainrT_shotNr_strg, ratio);
    tunning->sw_ainrT_readNr_strg = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_readNr_strg, paut->dyn[ihigh].tunning.sw_ainrT_readNr_strg, ratio);
    tunning->sw_ainrT_fixedNr_strg = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_fixedNr_strg, paut->dyn[ihigh].tunning.sw_ainrT_fixedNr_strg, ratio);

    tunning->sw_ainrT_motionBias_offset = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_motionBias_offset, paut->dyn[ihigh].tunning.sw_ainrT_motionBias_offset, ratio);
    tunning->sw_ainrT_motionGain_strg = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_motionGain_strg, paut->dyn[ihigh].tunning.sw_ainrT_motionGain_strg, ratio);
    tunning->sw_ainrT_noiseAdd_ratio = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAdd_ratio, paut->dyn[ihigh].tunning.sw_ainrT_noiseAdd_ratio, ratio);
    tunning->sw_ainrT_noiseAdd_limit = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAdd_limit, paut->dyn[ihigh].tunning.sw_ainrT_noiseAdd_limit, ratio);
    tunning->sw_ainrT_noiseAddMot_offset = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAddMot_offset, paut->dyn[ihigh].tunning.sw_ainrT_noiseAddMot_offset, ratio);
    tunning->sw_ainrT_noiseAddMot_coeff = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAddMot_coeff, paut->dyn[ihigh].tunning.sw_ainrT_noiseAddMot_coeff, ratio);
    tunning->sw_ainrT_noiseAddLuma_offset = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAddLuma_offset, paut->dyn[ihigh].tunning.sw_ainrT_noiseAddLuma_offset, ratio);
    tunning->sw_ainrT_noiseAddLuma_coeff = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAddLuma_coeff, paut->dyn[ihigh].tunning.sw_ainrT_noiseAddLuma_coeff, ratio);
    tunning->sw_ainrT_noiseAddLumaClip_th = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAddLumaClip_th, paut->dyn[ihigh].tunning.sw_ainrT_noiseAddLumaClip_th, ratio);
    tunning->sw_ainrT_noiseAddLumaStatic_th = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_noiseAddLumaStatic_th, paut->dyn[ihigh].tunning.sw_ainrT_noiseAddLumaStatic_th, ratio);
    tunning->sw_ainrT_nonLinear_scale = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_nonLinear_scale, paut->dyn[ihigh].tunning.sw_ainrT_nonLinear_scale, ratio);
    tunning->sw_ainrT_nonLinear_ratio = interpolation_f32(
                    paut->dyn[ilow].tunning.sw_ainrT_nonLinear_ratio, paut->dyn[ihigh].tunning.sw_ainrT_nonLinear_ratio, ratio);

    printParam(pAinrCtx, out, iso);
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
create_context(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;
    CamCalibDbV2Context_t *pCalibDbV2 = cfg->calibv2;

    AinrContext_t* ctx = aiq_mallocz(sizeof(AinrContext_t));

    if (ctx == NULL) {
        LOGE_AINR("%s create context failed!", __func__);
        return XCAM_RETURN_ERROR_MEM;
    }
    ctx->isReCal_ = true;
    ctx->ainr_attrib =
        (ainr_api_attrib_t*)(CALIBDBV2_GET_MODULE_PTR(pCalibDbV2, ainr));

    *context = (RkAiqAlgoContext* )ctx;
    LOGV_AINR("%s: Ainr (exit)\n", __FUNCTION__ );

    *context = (RkAiqAlgoContext*)ctx;
    return result;
}

static XCamReturn
destroy_context(RkAiqAlgoContext *context)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;
    AinrContext_t* pAinrCtx = (AinrContext_t*)context;
    aiq_free(pAinrCtx);
    return result;
}

static XCamReturn prepare(RkAiqAlgoCom* params)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;
    AinrContext_t* pAinrCtx = (AinrContext_t *)params->ctx;

    if(!!(params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB )) {
        // just update calib ptr
        if (params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB_PTR) {
            pAinrCtx->ainr_attrib =
                (ainr_api_attrib_t*)(CALIBDBV2_GET_MODULE_PTR(params->u.prepare.calibv2, ainr));
            pAinrCtx->iso_list = params->u.prepare.calibv2->sensor_info->iso_list;
            return XCAM_RETURN_NO_ERROR;
        }
    }

    pAinrCtx->working_mode = params->u.prepare.working_mode;
    pAinrCtx->ainr_attrib =
        (ainr_api_attrib_t*)(CALIBDBV2_GET_MODULE_PTR(params->u.prepare.calibv2, ainr));
    pAinrCtx->iso_list = params->u.prepare.calibv2->sensor_info->iso_list;
    pAinrCtx->isReCal_ = true;

    return result;
}

XCamReturn Ainr_processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams, int iso)
{
    AinrContext_t* pAinrCtx = (AinrContext_t *)inparams->ctx;
    ainr_api_attrib_t* ainr_attrib = pAinrCtx->ainr_attrib;
    ainr_param_t* ainr_res = outparams->algoRes;
    AlgoRstShared_t* xCamAePreRes = inparams->u.proc.res_comb->ae_pre_res_c;

    RkAiqAlgoPreResAe* ae_pre_res_int = NULL;
    if (xCamAePreRes) {
        ae_pre_res_int = (RkAiqAlgoPreResAe*)xCamAePreRes->_data;
        if (!ae_pre_res_int) {
            LOGW_AINR("ae pre result is null");
        }
        if (ae_pre_res_int) {
            if (pAinrCtx->working_mode == RK_AIQ_WORKING_MODE_NORMAL)
                pAinrCtx->mean_luma = ae_pre_res_int->ae_pre_res_rk.MeanLuma[0];
            else
                pAinrCtx->mean_luma = ae_pre_res_int->ae_pre_res_rk.MeanLuma[1];
        }
    }

    if (ainr_attrib->opMode != RK_AIQ_OP_MODE_AUTO) {
        LOGE_AINR("mode is %d, not auto mode, ignore", ainr_attrib->opMode);
        return XCAM_RETURN_NO_ERROR;
    }

    if (ainr_attrib->en == false) {
        if (inparams->u.proc.is_attrib_update) {
            outparams->cfg_update = true;
            outparams->en = ainr_attrib->en;
            outparams->bypass = ainr_attrib->bypass;
        }
        LOGD_AINR("AINR not enable, skip!");
        return XCAM_RETURN_NO_ERROR;
    }
    bool need_recal = pAinrCtx->isReCal_;

    bool init = inparams->u.proc.init;
    if (inparams->u.proc.is_attrib_update || inparams->u.proc.init) {
        need_recal = true;
    }

    int delta_iso = abs(iso - pAinrCtx->pre_iso);
    if (delta_iso > 0.01 || init) {
        pAinrCtx->pre_iso = iso;
        need_recal = true;
    }

    outparams->cfg_update = false;
    if (need_recal) {
        AinrSelectParam(pAinrCtx, ainr_res, iso);
        pAinrCtx->cur_param = *ainr_res;

        outparams->cfg_update = true;
        outparams->en = ainr_attrib->en;
        outparams->bypass = ainr_attrib->bypass;
        LOGD_AINR("AINR processing: iso %d, ainr en:%d, bypass:%d", iso, outparams->en, outparams->bypass);

        pAinrCtx->isReCal_ = false;
    }

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    int iso = inparams->u.proc.iso;
    Ainr_processing(inparams, outparams, iso);

    LOGV_AINR("%s: Ainr (exit)\n", __FUNCTION__ );
    return XCAM_RETURN_NO_ERROR;
}

#if RKAIQ_HAVE_DUMPSYS
static int dump(const RkAiqAlgoCom* self, st_string* result)
{
    AinrContext_t* pAinrCtx = (AinrContext_t *)self->ctx;
    ainr_api_attrib_t* ainr_attrib = pAinrCtx->ainr_attrib;
    ainr_param_t *cur_param = &pAinrCtx->cur_param;
    ainr_param_dyn_t *dyn = &cur_param->dyn;
    ainr_param_calib_t *calib = &dyn->calib;
    ainr_param_tunning_t *tunning = &dyn->tunning;
    char buffer[MAX_LINE_LENGTH] = {0};

    aiq_info_dump_title(result, "ainr status");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "Cam%d FrmId=%d working mode=%d iso=%d mean luma=%f",
             self->cid, self->frame_id, pAinrCtx->working_mode, pAinrCtx->pre_iso,
             pAinrCtx->mean_luma);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrC_blkPrev_val=%f",
            calib->sw_ainrC_blkPrev_val);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrC_blkPost_val=%f",
            calib->sw_ainrC_blkPost_val);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrC_shotNoise_val=%f",
            calib->sw_ainrC_shotNoise_val);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrC_readNoise_val=%f",
            calib->sw_ainrC_readNoise_val);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrC_fixedNoise_val=%f",
            calib->sw_ainrC_fixedNoise_val);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_totalNr_strg=%f",
            tunning->sw_ainrT_totalNr_strg);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_shotNr_strg=%f",
            tunning->sw_ainrT_shotNr_strg);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_readNr_strg=%f",
            tunning->sw_ainrT_readNr_strg);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_fixedNr_strg=%f",
            tunning->sw_ainrT_fixedNr_strg);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_motionBias_offset=%f",
            tunning->sw_ainrT_motionBias_offset);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_motionGain_strg=%f",
            tunning->sw_ainrT_motionGain_strg);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAdd_ratio=%f",
            tunning->sw_ainrT_noiseAdd_ratio);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAdd_limit=%f",
            tunning->sw_ainrT_noiseAdd_limit);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAddMot_offset=%f",
            tunning->sw_ainrT_noiseAddMot_offset);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAddMot_coeff=%f",
            tunning->sw_ainrT_noiseAddMot_coeff);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAddLuma_offset=%f",
            tunning->sw_ainrT_noiseAddLuma_offset);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAddLuma_coeff=%f",
            tunning->sw_ainrT_noiseAddLuma_coeff);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAddLumaClip_th=%f",
            tunning->sw_ainrT_noiseAddLumaClip_th);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_noiseAddLumaStatic_th=%f",
            tunning->sw_ainrT_noiseAddLumaStatic_th);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_nonLinear_scale=%f",
            tunning->sw_ainrT_nonLinear_scale);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_nonLinear_ratio=%f",
            tunning->sw_ainrT_nonLinear_ratio);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_model_file=%s",
            tunning->sw_ainrT_model_file);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    memset(buffer, 0, MAX_LINE_LENGTH);
    snprintf(buffer, MAX_LINE_LENGTH, "sw_ainrT_doRknn_en=%d",
            tunning->sw_ainrT_doRknn_en);
    aiq_string_printf(result, buffer);
    aiq_string_printf(result, "\n");

    return 0;
}
#endif

#define RKISP_ALGO_AINR_VERSION     "v0.0.9"
#define RKISP_ALGO_AINR_VENDOR      "Rockchip"
#define RKISP_ALGO_AINR_DESCRIPTION "Rockchip ainr algo for ISP35"

RkAiqAlgoDescription g_RkIspAlgoDescAinr = {
    .common = {
        .version = RKISP_ALGO_AINR_VERSION,
        .vendor  = RKISP_ALGO_AINR_VENDOR,
        .description = RKISP_ALGO_AINR_DESCRIPTION,
        .type    = RK_AIQ_ALGO_TYPE_AINR,
        .id      = 0,
        .create_context  = create_context,
        .destroy_context = destroy_context,
    },
    .prepare = prepare,
    .pre_process = NULL,
    .processing = processing,
    .post_process = NULL,
#if RKAIQ_HAVE_DUMPSYS
    .dump = dump,
#endif
};

