#ifndef _AINR_ALGO_API_H_
#define _AINR_ALGO_API_H_

#include "rk_aiq_algo_des.h"
#include "isp/rk_aiq_isp_ainr.h"
#include "algos/rk_aiq_api_types_ainr.h"

RKAIQ_BEGIN_DECLARE

XCamReturn Ainr_processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams, int iso);

extern RkAiqAlgoDescription g_RkIspAlgoDescAinr;
RKAIQ_END_DECLARE

#endif
