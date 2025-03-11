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

#ifndef _RK_AIQ_PARAM_AINR_H_
#define _RK_AIQ_PARAM_AINR_H_

#define AINR_MODEL_FILENAME_LEN    64

typedef struct ainr_param_calib_s {
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrC_blkPrev_val),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(-511.0, 511.0),
        M4_DEFAULT(256.0),
        M4_DIGIT_EX(6),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(0),
        M4_NOTES(The value of black level.\n
        Freq of use: High))  */
    float sw_ainrC_blkPrev_val;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrC_blkPost_val),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(-511.0, 511.0),
        M4_DEFAULT(256.0),
        M4_DIGIT_EX(6),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(1),
        M4_NOTES(The value of black level.\n
        Freq of use: High))  */
    float sw_ainrC_blkPost_val;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrC_shotNoise_val),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 128.0),
        M4_DEFAULT(16.0),
        M4_DIGIT_EX(6),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(2),
        M4_NOTES(The value of shot noise.\n
        Freq of use: High))  */
    float sw_ainrC_shotNoise_val;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrC_readNoise_val),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 128.0),
        M4_DEFAULT(1.6),
        M4_DIGIT_EX(6),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(3),
        M4_NOTES(The value of read noise.\n
        Freq of use: High))  */
    float sw_ainrC_readNoise_val;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrC_fixedNoise_val),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 128.0),
        M4_DEFAULT(0.16),
        M4_DIGIT_EX(6),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(4),
        M4_NOTES(The value of fixed noise.\n
        Freq of use: High))  */
    float sw_ainrC_fixedNoise_val;
} ainr_param_calib_t;

typedef struct ainr_param_tunning_s {
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_totalNr_strg),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 8.0),
        M4_DEFAULT(1.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(4),
        M4_NOTES(The value of total noise reduce strength.\n
        Freq of use: High))  */
    float sw_ainrT_totalNr_strg;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_shotNr_strg),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 8.0),
        M4_DEFAULT(1.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(5),
        M4_NOTES(The value of shot noise reduce strength.\n
        Freq of use: High))  */
    float sw_ainrT_shotNr_strg;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_readNr_strg),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 8.0),
        M4_DEFAULT(1.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(6),
        M4_NOTES(The value of read noise reduce strength.\n
        Freq of use: High))  */
    float sw_ainrT_readNr_strg;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_fixedNr_strg),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 8.0),
        M4_DEFAULT(1.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(7),
        M4_NOTES(The value of fixed noise reduce strength.\n
        Freq of use: High))  */
    float sw_ainrT_fixedNr_strg;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_motionBias_offset),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(1.0, 32.0),
        M4_DEFAULT(20.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(8),
        M4_NOTES(The value of motion bias offset.\n
        Freq of use: High))  */
    float sw_ainrT_motionBias_offset;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_motionGain_strg),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 4.0),
        M4_DEFAULT(0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(9),
        M4_NOTES(The value of motion gain strength.\n
        Freq of use: High))  */
    float sw_ainrT_motionGain_strg;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAdd_ratio),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 1.0),
        M4_DEFAULT(0.05),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(10),
        M4_NOTES(The value of noise add ratio.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAdd_ratio;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAdd_limit),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 1.0),
        M4_DEFAULT(0.5),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(11),
        M4_NOTES(The value of noise add limit.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAdd_limit;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAddMot_offset),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(1.0, 32.0),
        M4_DEFAULT(10.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(12),
        M4_NOTES(The value of motion offset for adding noise.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAddMot_offset;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAddMot_coeff),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(-8, 8),
        M4_DEFAULT(0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(13),
        M4_NOTES(The value of motion coefficient for adding noise.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAddMot_coeff;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAddLuma_offset),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 255.0),
        M4_DEFAULT(114),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(14),
        M4_NOTES(The value of luma offset for adding noise.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAddLuma_offset;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAddLuma_coeff),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 32.0),
        M4_DEFAULT(32),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(15),
        M4_NOTES(The value of luma coefficient for adding noise.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAddLuma_coeff;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAddLumaClip_th),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 1.0),
        M4_DEFAULT(1.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(16),
        M4_NOTES(The value of luma clip threshold for adding noise.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAddLumaClip_th;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_noiseAddLumaStatic_th),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(1.0, 32.0),
        M4_DEFAULT(1.0),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(17),
        M4_NOTES(The value of luma static threshold for adding noise.\n
        Freq of use: High))  */
    float sw_ainrT_noiseAddLumaStatic_th;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_nonLinear_scale),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 1.0),
        M4_DEFAULT(0.7),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(18),
        M4_NOTES(Non linear scale value.\n
        Freq of use: High))  */
    float sw_ainrT_nonLinear_scale;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_nonLinear_ratio),
        M4_TYPE(f32),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0.0, 5.0),
        M4_DEFAULT(0.5),
        M4_DIGIT_EX(3),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(19),
        M4_NOTES(Non linear ratio value.\n
        Freq of use: High))  */
    float sw_ainrT_nonLinear_ratio;
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_model_file),
        M4_TYPE(string),
        M4_SIZE_EX(1,1),
        M4_RANGE_EX(0, 64),
        M4_DEFAULT("/etc/iqfiles/model.bin"),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(20),
        M4_NOTES(Model info file.\n
        Freq of use: High))  */
    char sw_ainrT_model_file[AINR_MODEL_FILENAME_LEN];
    /* M4_GENERIC_DESC(
        M4_ALIAS(sw_ainrT_doRknn_en),
        M4_TYPE(bool),
        M4_DEFAULT(0),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(21),
        M4_NOTES(The enable bit of the rknn after AINR.\n
        Freq of use: low))  */
    bool sw_ainrT_doRknn_en;
} ainr_param_tunning_t;

typedef struct ainr_param_dyn_s {
    /* M4_GENERIC_DESC(
        M4_ALIAS(calib),
        M4_TYPE(struct),
        M4_UI_MODULE(normal_ui_style),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(0),
        M4_NOTES(TODO))  */
    ainr_param_calib_t   calib;
    /* M4_GENERIC_DESC(
        M4_ALIAS(tunning),
        M4_TYPE(struct),
        M4_UI_MODULE(normal_ui_style),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(1),
        M4_NOTES(TODO))  */
    ainr_param_tunning_t tunning;
} ainr_param_dyn_t;

typedef struct ainr_param_s {
    /* M4_GENERIC_DESC(
        M4_ALIAS(dyn),
        M4_TYPE(struct),
        M4_UI_MODULE(dynamic_ui),
        M4_HIDE_EX(0),
        M4_RO(0),
        M4_ORDER(0),
        M4_NOTES(TODO))  */
    ainr_param_dyn_t dyn;
} ainr_param_t;

#endif
