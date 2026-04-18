/******************************************************************************
 * @file     cnn_model.c
 * @version  V1.00
 * @brief    CNN inference model implementation.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#include <math.h>
#include <stdint.h>
#include "cnn_api.h"
#include "cnn_model_weights.h"

#ifndef BN_EPSILON
#define BN_EPSILON 1e-3f
#endif

/* ===================== Model Dimensions ===================== */

/* Input: 80x60 RGB */
#define INPUT_H 60
#define INPUT_W 80
#define INPUT_C 3 

/* Layer 1: Conv2D (Stride=2) 
 * In: 80x60 -> Out: 40x30
 */
#define CONV1_OUT_H 30  // 60 / 2
#define CONV1_OUT_W 40  // 80 / 2
#define CONV1_OUT_C 4

/* Layer 1 Pool: 40x30 -> 20x15 */
#define POOL1_OUT_H 15
#define POOL1_OUT_W 20
#define POOL1_OUT_C 4

/* Layer 2 Conv: 20x15 -> 20x15 (Padding=Same) */
#define CONV2_OUT_H 15
#define CONV2_OUT_W 20
#define CONV2_OUT_C 4

/* Layer 2 Pool: 20x15 -> 10x7 */
#define POOL2_OUT_H 7   // 15 / 2 = 7
#define POOL2_OUT_W 10  // 20 / 2 = 10
#define POOL2_OUT_C 4

/* Layer 3 Conv: 10x7 -> 10x7 (Padding=Same) */
#define CONV3_OUT_H 7
#define CONV3_OUT_W 10
#define CONV3_OUT_C 8

/* Layer 3 Pool: 10x7 -> 5x3 (Max Pool) */
#define POOL3_OUT_H 3   // 7 / 2 = 3
#define POOL3_OUT_W 5   // 10 / 2 = 5
#define POOL3_OUT_C 8

/* Dense Layers */
#define DENSE1_IN  120 // 3 * 5 * 8 (Flattened, fits inside M032 SRAM)
#define DENSE1_OUT 12
#define DENSE2_OUT 6 

/* ===================== Buffers (Ping-Pong Strategy) ===================== */

/* Avoid allocating large img_norm (which is 57KB).
 * Compute normalization on the fly during the first layer.
 */

// Buffer 1: Conv1 output map (CONV1_OUT = 30*40*4 = 4800 floats)
static float shared_buf_1[CONV1_OUT_H * CONV1_OUT_W * CONV1_OUT_C];

// Buffer 2: Pool output map (POOL1_OUT = 15*20*4 = 1200 floats)
static float shared_buf_2[POOL1_OUT_H * POOL1_OUT_W * POOL1_OUT_C];

static float dense1_buf[DENSE1_OUT];
static float result_logits[DENSE2_OUT];

/* ===================== Utils ===================== */

#define IDX3(y,x,c,W,C) (((y)*(W)*(C)) + ((x)*(C)) + (c))

static inline float relu(float x)
{
    return x > 0.0f ? x : 0.0f;
}

/* ===================== First Layer Block: Uint8 Input + Stride 2 ===================== */
/* Optimize for memory map:
 * 1. Directly read uint8_t image array from main.c
 * 2. Cast to float and scale (/255.0)
 * 3. Evaluate stride=2 operation
 */
static void conv_forward_strided_u8(
    const uint8_t *input, float *output,
    int OUT_H, int OUT_W, int OUT_C,
    const float *bias,
    int IN_C, int IN_H, int IN_W,
    const float *weights,
    int stride_y, int stride_x)
{
    for (int oy = 0; oy < OUT_H; oy++) {
        for (int ox = 0; ox < OUT_W; ox++) {
            for (int oc = 0; oc < OUT_C; oc++) {

                float sum = bias[oc];

                // Calculate center (with stride)
                int center_y = oy * stride_y;
                int center_x = ox * stride_x;

                for (int ky = 0; ky < 3; ky++) {
                    for (int kx = 0; kx < 3; kx++) {
                        // Padding 'same' logic with stride
                        int iy = center_y + ky - 1;
                        int ix = center_x + kx - 1;

                        if (iy < 0 || iy >= IN_H || ix < 0 || ix >= IN_W)
                            continue;

                        for (int ic = 0; ic < IN_C; ic++) {
                            int in_idx = IDX3(iy, ix, ic, IN_W, IN_C);
                            
                            // [Optimize] Read uint8 directly to save memory footprint
                            float pixel_val = (float)input[in_idx] * 0.00392156862f; // * 1/255

                            int w_idx = (((ky*3 + kx)*IN_C + ic)*OUT_C + oc);
                            sum += pixel_val * weights[w_idx];
                        }
                    }
                }
                output[IDX3(oy, ox, oc, OUT_W, OUT_C)] = relu(sum);
            }
        }
    }
}

/* ===================== Standard Conv2D (Float Input) ===================== */

static void conv_forward(
    const float *input, float *output,
    int OUT_H, int OUT_W, int OUT_C,
    const float *bias,
    int IN_C, int IN_H, int IN_W,
    const float *weights)
{
    for (int oy = 0; oy < OUT_H; oy++) {
        for (int ox = 0; ox < OUT_W; ox++) {
            for (int oc = 0; oc < OUT_C; oc++) {
                float sum = bias[oc];
                for (int ky = 0; ky < 3; ky++) {
                    for (int kx = 0; kx < 3; kx++) {
                        int iy = oy + ky - 1;
                        int ix = ox + kx - 1;
                        if (iy < 0 || iy >= IN_H || ix < 0 || ix >= IN_W) continue;
                        for (int ic = 0; ic < IN_C; ic++) {
                            int in_idx = IDX3(iy, ix, ic, IN_W, IN_C);
                            int w_idx = (((ky*3 + kx)*IN_C + ic)*OUT_C + oc);
                            sum += input[in_idx] * weights[w_idx];
                        }
                    }
                }
                output[IDX3(oy, ox, oc, OUT_W, OUT_C)] = relu(sum);
            }
        }
    }
}

/* ===================== MaxPool 2x2 ===================== */

static void maxpool2x2(
    const float *input, int in_h, int in_w, int in_c,
    float *output)
{
    int out_h = in_h / 2;
    int out_w = in_w / 2;
    for (int oy = 0; oy < out_h; oy++) {
        for (int ox = 0; ox < out_w; ox++) {
            for (int c = 0; c < in_c; c++) {
                int iy = oy * 2;
                int ix = ox * 2;
                float m = input[IDX3(iy, ix, c, in_w, in_c)];
                float v;
                if (iy+1 < in_h) { v = input[IDX3(iy+1, ix, c, in_w, in_c)]; if(v>m) m=v; }
                if (ix+1 < in_w) { v = input[IDX3(iy, ix+1, c, in_w, in_c)]; if(v>m) m=v; }
                if (iy+1 < in_h && ix+1 < in_w) { v = input[IDX3(iy+1, ix+1, c, in_w, in_c)]; if(v>m) m=v; }
                output[IDX3(oy, ox, c, out_w, in_c)] = m;
            }
        }
    }
}

/* ===================== Dense & BN ===================== */

static void dense_forward(const float *input, float *output, int OUT, int IN, const float *bias, const float *weights, int use_relu)
{
    for (int o = 0; o < OUT; o++) {
        float sum = bias[o];
        for (int i = 0; i < IN; i++) sum += input[i] * weights[i * OUT + o];
        output[o] = use_relu ? relu(sum) : sum;
    }
}

static void bnd(float *data, int h, int w, int c, const float *gamma, const float *beta, const float *mean, const float *var)
{
    for (int ch = 0; ch < c; ch++) {
        float scale = gamma[ch] / sqrtf(var[ch] + BN_EPSILON);
        float shift = beta[ch] - mean[ch] * scale;
        for (int i = 0; i < h*w; i++) {
            // Data array format is (y, x, c), 
            // So we iterate and add scale/shift per corresponding channel.
             data[i*c + ch] = data[i*c + ch] * scale + shift;
        }
    }
}

static void softmax(float *x, int n)
{
    float maxv = x[0];
    for (int i = 1; i < n; i++) if (x[i] > maxv) maxv = x[i];
    float sum = 0.0f;
    for (int i = 0; i < n; i++) { x[i] = expf(x[i] - maxv); sum += x[i]; }
    float inv = 1.0f / sum;
    for (int i = 0; i < n; i++) x[i] *= inv;
}

/* ===================== Inference ===================== */

void run_cnn_inference_40x30(const uint8_t *img, uint8_t *digit)
{
    /* Buffer mapping (Ping-Pong approach)
     * shared_buf_1: stores the Feature Map (Conv Out)
     * shared_buf_2: stores the Feature Map (Pool Out)
     */
    float *buf_large = shared_buf_1; 
    float *buf_small = shared_buf_2; 

    // ------------------------------------------------------------------
    // Layer 1: Conv2D (Stride 2) -> 80x60 uint8 input, 40x30 float map
    // ------------------------------------------------------------------
    // In: img (uint8, 80x60x3)
    // Out: buf_large (float, 40x30x4)
    conv_forward_strided_u8(img, buf_large,
        CONV1_OUT_H, CONV1_OUT_W, CONV1_OUT_C,
        conv_0_bias,
        INPUT_C, INPUT_H, INPUT_W,
        (const float *)conv_0_weights,
        2, 2); // Stride = (2, 2)

    bnd(buf_large, CONV1_OUT_H, CONV1_OUT_W, CONV1_OUT_C,
        bn_0_gamma, bn_0_beta, bn_0_mean, bn_0_var);

    // Pool: 40x30 -> 20x15
    // In: buf_large, Out: buf_small
    maxpool2x2(buf_large, CONV1_OUT_H, CONV1_OUT_W, CONV1_OUT_C, buf_small);


    // ------------------------------------------------------------------
    // Layer 2: Conv2D
    // ------------------------------------------------------------------
    // In: buf_small (20x15x4)
    // Out: buf_large (20x15x4) - Output back to buf_large
    conv_forward(buf_small, buf_large,
        CONV2_OUT_H, CONV2_OUT_W, CONV2_OUT_C,
        conv_1_bias,
        POOL1_OUT_C, POOL1_OUT_H, POOL1_OUT_W,
        (const float *)conv_1_weights);

    bnd(buf_large, CONV2_OUT_H, CONV2_OUT_W, CONV2_OUT_C,
        bn_1_gamma, bn_1_beta, bn_1_mean, bn_1_var);

    // Pool: 20x15 -> 10x7
    // In: buf_large, Out: buf_small - Output back to buf_small
    maxpool2x2(buf_large, CONV2_OUT_H, CONV2_OUT_W, CONV2_OUT_C, buf_small);


    // ------------------------------------------------------------------
    // Layer 3: Conv2D
    // ------------------------------------------------------------------
    // In: buf_small (10x7x4)
    // Out: buf_large (10x7x8) - Output back to buf_large
    // Note: Use output channel of 8. The buf_large size (4800 elements) ensures enough room to hold this (560 elements).
    conv_forward(buf_small, buf_large,
        CONV3_OUT_H, CONV3_OUT_W, CONV3_OUT_C,
        conv_2_bias,
        POOL2_OUT_C, POOL2_OUT_H, POOL2_OUT_W,
        (const float *)conv_2_weights);

    bnd(buf_large, CONV3_OUT_H, CONV3_OUT_W, CONV3_OUT_C,
        bn_2_gamma, bn_2_beta, bn_2_mean, bn_2_var);

    // Pool: 10x7 -> 5x3
    // In: buf_large, Out: buf_small
    maxpool2x2(buf_large, CONV3_OUT_H, CONV3_OUT_W, CONV3_OUT_C, buf_small);


    // ------------------------------------------------------------------
    // Dense & Output
    // ------------------------------------------------------------------
    // In: buf_small (5x3x8 = 120 features)
    dense_forward(buf_small, dense1_buf,
        DENSE1_OUT, DENSE1_IN,
        dense_0_bias, (const float *)dense_0_weights, 1);

    dense_forward(dense1_buf, result_logits,
        DENSE2_OUT, DENSE1_OUT,
        dense_1_bias, (const float *)dense_1_weights, 0);

    softmax(result_logits, DENSE2_OUT);

    /* Get Result */
    uint8_t best = 0;
    float best_v = result_logits[0];
    for (int i = 1; i < DENSE2_OUT; i++)
        if (result_logits[i] > best_v) {
            best_v = result_logits[i];
            best = i;
        }

    *digit = best;
}
