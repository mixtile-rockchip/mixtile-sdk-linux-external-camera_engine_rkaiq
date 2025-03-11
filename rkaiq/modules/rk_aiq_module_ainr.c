#include "common/rk-aiisp-config.h"
#include "common/rk-isp35-config.h"

#define AIBNR_PYRAMID_LAYER_NUM_4                    4
#define AIBNR_PREV_LUT_BIT_5                         5
#define AIBNR_POST_LUT_BIT_5                         5
#define AIBNR_SIGMA_LUT_ROW_BIT_3                    3
#define AIBNR_SIGMA_LUT_COL_BIT_3                    3
#define AIBNR_RAW_DATA_BIT_16                        16
#define AIBNR_NAR_MAP_BIT_7                          7
#define AIBNR_GAIN_DOWN_SCALE_ROW_2                  2
#define AIBNR_GAIN_DOWN_SCALE_COL_8                  8
#define AIBNR_NPU_FEATURE_BIT_10                     10

#define AIBNR_PREV_LUT_LEN_33                        ((1 << AIBNR_PREV_LUT_BIT_5) + 1)
#define AIBNR_POST_LUT_LEN_33                        ((1 << AIBNR_POST_LUT_BIT_5) + 1)
#define AIBNR_SIGMA_LUT_ROW_LEN_9                    ((1<<AIBNR_SIGMA_LUT_ROW_BIT_3) + 1)
#define AIBNR_SIGMA_LUT_COL_LEN_9                    ((1<<AIBNR_SIGMA_LUT_COL_BIT_3) + 1)
#define AIBNR_RAW_DATA_MAX_65535                     ((1 << AIBNR_RAW_DATA_BIT_16) - 1)
#define AIBNR_NPU_FEATURE_MAX_1023                   ((1 << AIBNR_NPU_FEATURE_BIT_10) - 1)
#define AIBNR_NOISE_LIMIT_RANGE_32767                32767

static float nonLinear(float x, int flag, float r, float scale)
{
    float R2 = (1 + r)*(1 + r) + r * r;
    float y = 0;

    if (flag == 0)
    {
        x = CLIP(x, 0, 1);
        float X2 = (ABS(x) - (1 + r))*(ABS(x) - (1 + r));
        y = (sqrt(R2 - X2) - r) * scale;
    }
    else
    {
        x = CLIP(x / scale, 0, 1);
        float X2 = (ABS(x) + r)*(ABS(x) + r);
        y = (r + 1) - sqrt(R2 - X2);
    }
    return y;
}

float Linear(float x, float scale, int flag)
{
    float y = 0;

    if (flag == 0)
    {
        y = x * scale;
        y = CLIP(y, 0, 1);
    }
    else
    {
        y = x / scale;
        y = CLIP(y, 0, 1);
    }
    return y;
}

float logTransf(float x, int flag, float offset)
{
    float y = 0;

    if (flag == 0)
    {
        x = x * (1 << 16);

        y = log2(x + offset) - log2(offset);
        y = y * (1 << 12);

        y = y / (1 << 16);
    }
    else
    {
        x = x * (1 << 16);

        x = x / (1 << 12);
        y = x + log2(offset);
        y = pow(2, y) - offset;

        y = y / (1 << 16);
    }

    return y;
}

static void rk_aiq_ainr_params_print(struct isp35_ai_cfg* ai_cfg)
{
    int i;

    LOGD_AINR("aipre:");
    LOGD_AINR("aiisp_raw12_msb = 0x%x", ai_cfg->aiisp_raw12_msb);
    LOGD_AINR("aiisp_gain_mode = 0x%x", ai_cfg->aiisp_gain_mode);
    LOGD_AINR("aiisp_curve_en = 0x%x", ai_cfg->aiisp_curve_en);
    LOGD_AINR("aipre_iir_en = 0x%x", ai_cfg->aipre_iir_en);
    LOGD_AINR("aipre_iir2ddr_en = 0x%x", ai_cfg->aipre_iir2ddr_en);
    LOGD_AINR("aipre_gain_en = 0x%x", ai_cfg->aipre_gain_en);
    LOGD_AINR("aipre_gain2ddr_en = 0x%x", ai_cfg->aipre_iir_en);
    LOGD_AINR("aipre_luma2gain_dis = 0x%x", ai_cfg->aipre_luma2gain_dis);
    LOGD_AINR("aipre_nl_ddr_mode = 0x%x", ai_cfg->aipre_nl_ddr_mode);
    LOGD_AINR("aipre_yraw_sel = 0x%x", ai_cfg->aipre_yraw_sel);
    LOGD_AINR("aipre_gain_bypass = 0x%x", ai_cfg->aipre_gain_bypass);
    LOGD_AINR("aipre_gain_mode = 0x%x", ai_cfg->aipre_gain_mode);
    LOGD_AINR("aipre_narmap_inv = 0x%x", ai_cfg->aipre_narmap_inv);

    for (i = 0; i < ISP35_AI_SIGMA_NUM; i++) {
        LOGD_AINR("aiisp_sigma_y[%d] = 0x%x", i, ai_cfg->aiisp_sigma_y[i]);
    }

    LOGD_AINR("aipre_scale = 0x%x", ai_cfg->aipre_scale);
    LOGD_AINR("aipre_zp = 0x%x", ai_cfg->aipre_zp);
    LOGD_AINR("aipre_black_lvl = 0x%x", ai_cfg->aipre_black_lvl);

    LOGD_AINR("aipre_gain_alpha = 0x%x", ai_cfg->aipre_gain_alpha);
    LOGD_AINR("aipre_global_gain = 0x%x", ai_cfg->aipre_global_gain);
    LOGD_AINR("aipre_gain_ratio = 0x%x", ai_cfg->aipre_gain_ratio);

    for (i = 0; i < ISP35_AI_SIGMA_NUM; i++) {
        LOGD_AINR("aipre_sigma_y[%d] = 0x%x", i, ai_cfg->aipre_sigma_y[i]);
    }

    LOGD_AINR("aipre_noise_mot_offset = 0x%x", ai_cfg->aipre_noise_mot_offset);
    LOGD_AINR("aipre_noise_mot_gain = 0x%x", ai_cfg->aipre_noise_mot_gain);
    LOGD_AINR("aipre_noise_luma_offset = 0x%x", ai_cfg->aipre_noise_luma_offset);

    LOGD_AINR("aipre_noise_luma_gain = 0x%x", ai_cfg->aipre_noise_luma_gain);
    LOGD_AINR("aipre_noise_luma_clip = 0x%x", ai_cfg->aipre_noise_luma_clip);
    LOGD_AINR("aipre_noise_luma_static = 0x%x", ai_cfg->aipre_noise_luma_static);

    LOGD_AINR("aipre_nar_manual = 0x%x", ai_cfg->aipre_nar_manual);
    LOGD_AINR("aipre_nar_manual_alpha = 0x%x", ai_cfg->aipre_nar_manual_alpha);

    LOGD_AINR("pyr_yraw_mode = 0x%x", ai_cfg->pyr_yraw_mode);
    LOGD_AINR("pyr_sigma_en = 0x%x", ai_cfg->pyr_sigma_en);
    LOGD_AINR("pyr_yraw_sel = 0x%x", ai_cfg->pyr_yraw_sel);
    LOGD_AINR("pyr_gain_leftshift = 0x%x", ai_cfg->pyr_gain_leftshift);
    LOGD_AINR("pyr_blacklvl_sig = 0x%x", ai_cfg->pyr_blacklvl_sig);
    for (i = 0; i < ISP35_VPSL_SIGMA_NUM; i++) {
        LOGD_AINR("pyr_sigma_y[%d] = 0x%x", i, ai_cfg->pyr_sigma_y[i]);
    }
}

void rk_aiq_ainr_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t *cvtinfo, bool bypass)
{
    struct isp35_ai_cfg* ai_cfg = &isp_params->isp_cfg->others.ai_cfg;
    RkAiqAinrModelBuf_t *pAinrModel = &cvtinfo->ainrModel;
    struct rkaiisp_model_info *model_info;
    int i;

    rk_aiq_isp_ainr_params_t *isp_ainr_param = (rk_aiq_isp_ainr_params_t *) attr;
    ainr_param_t *ainr_param = &isp_ainr_param->ainr_param;
    ainr_param_calib_t *calib = &ainr_param->dyn.calib;
    ainr_param_tunning_t *tunning = &ainr_param->dyn.tunning;

    if (!cvtinfo->use_aiisp) {
        memset(ai_cfg, 0, sizeof(*ai_cfg));
        LOGD_AINR("aiisp is disabled");
        return;
    }

    isp_ainr_param->is_bypass = bypass;

    float hdrLS_ratio = cvtinfo->L2S_Ratio;
    float preDgain = cvtinfo->preDGain_drc;
    int btnrCfg_pixDomain_mode = cvtinfo->btnrCfg_pixDomain_mode;
    int hw_btnrCfg_trans_mode = cvtinfo->btnrCfg_trans_mode;
    int hw_btnrCfg_trans_offset = cvtinfo->btnrCfg_trans_offset;
    float sw_ainrC_blkPrev_val = cvtinfo->isp_ob_offset1 + calib->sw_ainrC_blkPrev_val;
    float sw_ainrC_blkPost_val = cvtinfo->isp_ob_offset1 + calib->sw_ainrC_blkPost_val;

    if (preDgain < 1.0)
        preDgain = 1.0;

    sw_ainrC_blkPrev_val = CLIP(sw_ainrC_blkPrev_val, 0, 511);
    sw_ainrC_blkPost_val = CLIP(sw_ainrC_blkPost_val, 0, 511);

    LOGD_AINR("%s: frameNum %d, L2S_Ratio %f, preDGain %f, pixDomain_mode %d, trans_mode %d, trans_offset %d, isp_ob_offset %d, isp_ob_offset1 %d, sw_ainrC_blkPrev_val %f, sw_ainrC_blkPost_val %f",
              __func__,
              cvtinfo->frameNum, cvtinfo->L2S_Ratio, preDgain, cvtinfo->btnrCfg_pixDomain_mode,
              cvtinfo->btnrCfg_trans_mode, cvtinfo->btnrCfg_trans_offset,
              cvtinfo->isp_ob_offset, cvtinfo->isp_ob_offset1, sw_ainrC_blkPrev_val, sw_ainrC_blkPost_val);

    if (cvtinfo->frameNum > 1) {
        if (btnrCfg_pixDomain_mode != btnr_pixLog2Domain_mode) {
            LOGE_AINR("btnrCfg_pixDomain_mode(%d) is not log2Domain in hdr mode", btnrCfg_pixDomain_mode);
        }
        if (hw_btnrCfg_trans_mode != btnr_pixInBw20b_mode) {
            LOGE_AINR("hw_btnrCfg_trans_mode(%d) is not btnr_pixInBw20b_mode in hdr mode", hw_btnrCfg_trans_mode);
        }
    } else {
        hdrLS_ratio = 1;
        if (preDgain > 1.0) {
            if (btnrCfg_pixDomain_mode != btnr_pixLog2Domain_mode) {
                LOGE_AINR("btnrCfg_pixDomain_mode(%d) is not log2Domain when preDgain(%f) > 1", btnrCfg_pixDomain_mode, preDgain);
            }
        }
    }

    ai_cfg->aiisp_raw12_msb     = 1;
    ai_cfg->aiisp_gain_mode     = 0;
    ai_cfg->aipre_iir_en        = 1;
    ai_cfg->aipre_iir2ddr_en    = 1;
    ai_cfg->aipre_gain_en       = 1;
    ai_cfg->aipre_gain2ddr_en   = 1;
    ai_cfg->aipre_yraw_sel      = 1;
    ai_cfg->aipre_nl_ddr_mode   = 1;
    ai_cfg->aipre_gain_bypass   = 0;
    ai_cfg->aipre_gain_mode     = 0;
    ai_cfg->aipre_luma2gain_dis = (cvtinfo->rawWidth % 8 != 0) ? 1 : 0;

    /* VPSL_PYR_CTRL */
    ai_cfg->pyr_yraw_mode = 1;
    ai_cfg->pyr_sigma_en  = 1;
    ai_cfg->pyr_yraw_sel  = 1;

    ai_cfg->aiisp_curve_en = 0;
    for (i = 0; i < 33; i++)
        ai_cfg->aiisp_sigma_y[i] = 0;           // 12bit to 12bit convertion curve

    float logMax          = 0;
    float logScale        = 1;
    float log_offset      = hw_btnrCfg_trans_offset;
    float nonlinear_scale = tunning->sw_ainrT_nonLinear_scale;
    float nonlinear_ratio = tunning->sw_ainrT_nonLinear_ratio;
    if (btnrCfg_pixDomain_mode == btnr_pixLog2Domain_mode) {
        logMax     = (log2(log_offset + (1 << 12) * hdrLS_ratio * preDgain) - log2(log_offset)) * (1 << 12) / (1 << 16);
        logScale = nonlinear_scale / logMax;

        ai_cfg->aipre_black_lvl = (log2(log_offset + sw_ainrC_blkPrev_val) - log2(log_offset)) * (1 << 12);
        ai_cfg->pyr_blacklvl_sig = ai_cfg->aipre_black_lvl / 256;
        isp_ainr_param->prev_blacklvl = ai_cfg->aipre_black_lvl;
        isp_ainr_param->post_blacklvl = (log2(log_offset + sw_ainrC_blkPost_val) - log2(log_offset)) * (1 << 12);
    } else {
        ai_cfg->aipre_black_lvl       = sw_ainrC_blkPrev_val * (1 << 4);
        ai_cfg->pyr_blacklvl_sig      = sw_ainrC_blkPrev_val * (1 << 4) / 256;
        isp_ainr_param->prev_blacklvl = ai_cfg->aipre_black_lvl;
        isp_ainr_param->post_blacklvl = sw_ainrC_blkPost_val * (1 << 4);
    }
    ai_cfg->aipre_black_lvl           = ai_cfg->aipre_black_lvl >> 4;
    ai_cfg->aipre_gain_ratio          = tunning->sw_ainrT_noiseAdd_ratio * (1 << AIBNR_NAR_MAP_BIT_7) * 255;

    ai_cfg->aipre_noise_mot_offset    = sqrt(1.0 / tunning->sw_ainrT_noiseAddMot_offset) * 255.0;
    ai_cfg->aipre_noise_mot_gain      = tunning->sw_ainrT_noiseAddMot_coeff * (1 << 3);
    ai_cfg->aipre_noise_mot_gain      = CLIP(ai_cfg->aipre_noise_mot_gain, -64, 63);

    ai_cfg->aipre_noise_luma_offset   = tunning->sw_ainrT_noiseAddLuma_offset;
    ai_cfg->aipre_noise_luma_gain     = tunning->sw_ainrT_noiseAddLuma_coeff;
    ai_cfg->aipre_noise_luma_clip     = tunning->sw_ainrT_noiseAddLumaClip_th* (1 << 10);
    ai_cfg->aipre_noise_luma_clip     = MIN(ai_cfg->aipre_noise_luma_clip, 0x3FF);

    uint16_t gain_tmp                 = sqrt(1.0 / tunning->sw_ainrT_noiseAddLumaStatic_th) * 255.0;
    uint16_t ext_tmp                  = (MAX((int)gain_tmp - ai_cfg->aipre_noise_mot_offset, 0) * ai_cfg->aipre_noise_mot_gain) >> 3;
    gain_tmp                          = MIN(gain_tmp + ext_tmp, 2047);
    ai_cfg->aipre_noise_luma_static   = MIN(255 * ai_cfg->aipre_noise_luma_gain / gain_tmp, 128);

    ai_cfg->aipre_gain_alpha          = 8;
    ai_cfg->aipre_global_gain         = 128;

    ai_cfg->aipre_nar_manual          = 0;
    ai_cfg->aipre_nar_manual_alpha    = 0;
    ai_cfg->aipre_narmap_inv          = 0;
    ai_cfg->pyr_gain_leftshift        = 0;

    for (int i = 0; i < AIBNR_PREV_LUT_LEN_33; i++) {
        float x = (float)i / (AIBNR_PREV_LUT_LEN_33 - 1);

        if (btnrCfg_pixDomain_mode == btnr_pixLinearDomain_mode)
        {
            x = nonLinear(x, 0, nonlinear_ratio, nonlinear_scale);
        }
        else
        {
            x = Linear(x, logScale, 0);
        }

        ai_cfg->aipre_sigma_y[i] = round(x * AIBNR_NPU_FEATURE_MAX_1023);
        isp_ainr_param->sw_in_comp_y[i] = ai_cfg->aipre_sigma_y[i];
    }

    for (int i = 0; i < AIBNR_POST_LUT_LEN_33; i++)
    {
        float x = (float)i / (AIBNR_POST_LUT_LEN_33 - 1);

        if (btnrCfg_pixDomain_mode == btnr_pixLinearDomain_mode)
        {
            x = nonLinear(x, 1, nonlinear_ratio, nonlinear_scale);
        }
        else
        {
            x = Linear(x, logScale, 1);
        }

        isp_ainr_param->sw_out_decomp_y[i] = round(x * AIBNR_RAW_DATA_MAX_65535);
    }

    float shot_val  = calib->sw_ainrC_shotNoise_val  * tunning->sw_ainrT_shotNr_strg  * tunning->sw_ainrT_totalNr_strg / 1000;
    float read_val  = calib->sw_ainrC_readNoise_val  * tunning->sw_ainrT_readNr_strg  * tunning->sw_ainrT_totalNr_strg / 1000;
    float fixed_val = calib->sw_ainrC_fixedNoise_val * tunning->sw_ainrT_fixedNr_strg * tunning->sw_ainrT_totalNr_strg / 1000;

    float motion_bias_offset = tunning->sw_ainrT_motionBias_offset;
    float motion_gain_strg = tunning->sw_ainrT_motionGain_strg;
    float quant_val = 0;

    for (i = 0; i < AINR_ISO_STEP_MAX; i++) {
        if (strcmp(pAinrModel->model_file[i], tunning->sw_ainrT_model_file) == 0) {
            model_info = (struct rkaiisp_model_info *)pAinrModel->model_buf[i];
            quant_val = model_info->model_qr;
            break;
        }
    }

    if (i >= AINR_ISO_STEP_MAX) {
        LOGE_AINR("can not find model file %s", tunning->sw_ainrT_model_file);
        XCAM_ASSERT(false);
    }

    for (int i = 0; i < AIBNR_SIGMA_LUT_ROW_LEN_9; i++) {
        for (int j = 0; j < AIBNR_SIGMA_LUT_COL_LEN_9; j++) {
            float wgt = (float)i / (AIBNR_SIGMA_LUT_ROW_LEN_9 - 1);
            float lum = (float)j / (AIBNR_SIGMA_LUT_COL_LEN_9 - 1);
            float dul = 1;
            float ratio = 1;

            if (btnrCfg_pixDomain_mode == btnr_pixLinearDomain_mode)
            {
                if ((lum - 0.01) > 0) {
                    dul = (nonLinear(lum, 0, nonlinear_ratio, nonlinear_scale) - nonLinear(lum - 0.01, 0, nonlinear_ratio, nonlinear_scale)) * 100;
                } else {
                    dul = (nonLinear(lum + 0.01, 0, nonlinear_ratio, nonlinear_scale) - nonLinear(lum, 0, nonlinear_ratio, nonlinear_scale)) * 100;
                }
            }
            else
            {
                lum = logTransf(lum, 1, log_offset);

                if ((lum - 0.01) > 0)
                {
                    dul = (logTransf(lum, 0, log_offset) - logTransf(lum - 0.01, 0, log_offset)) * 100;
                }
                else
                {
                    dul = (logTransf(lum + 0.01, 0, log_offset) - logTransf(lum, 0, log_offset)) * 100;
                }
                ratio = 1 << 4; //12bit -> 16bit
            }

            float motion_strg = pow(motion_bias_offset / MIN(motion_bias_offset, 1 / (wgt * wgt)), motion_gain_strg);
            float sig = MAX(shot_val * lum / preDgain * ratio + read_val, 0) * wgt * wgt * motion_strg + fixed_val;
            sig = sqrt(sig) * quant_val * dul * preDgain / ratio * logScale;
            sig = MIN(sig, 1);

            ai_cfg->pyr_sigma_y[i * AIBNR_SIGMA_LUT_COL_LEN_9 + j] = sig * 255;
        }
    }

    ai_cfg->aipre_scale = 0;                  // 8 bits, [0, 255]
    ai_cfg->aipre_zp = 0;                     // 8 bits, [-128, 127]

    rk_aiq_ainr_params_print(ai_cfg);

    return;
}

