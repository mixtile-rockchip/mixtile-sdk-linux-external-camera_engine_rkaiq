#ifndef _RK_AIQ_ISP35_MODULES_H_
#define _RK_AIQ_ISP35_MODULES_H_

#include "rk_aiq_isp32_modules.h"

#include "rk_aiq_module_btnr_common.h"
#include "rk_aiq_module_common.h"
#include "common/rk-isp33-config.h"
#include "common/rk-isp35-config.h"

RKAIQ_BEGIN_DECLARE
void rk_aiq_merge23_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t* cvtinfo);
void rk_aiq_drc41_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t* cvtinfo, bool drc_en);
void rk_aiq_hsv11_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t* cvtinfo);
void rk_aiq_dm25_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t* cvtinfo);
void rk_aiq_btnr42_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t* cvtinfo, btnr_cvt_info_t* pBtnrInfo);
void rk_aiq_ynr41_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t *cvtinfo);
void rk_aiq_sharp41_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t *cvtinfo, btnr_cvt_info_t* pBtnrInfo);
void rk_aiq_ainr_params_cvt(void* attr, isp_params_t* isp_params, common_cvt_info_t *cvtinfo);
RKAIQ_END_DECLARE

#endif

