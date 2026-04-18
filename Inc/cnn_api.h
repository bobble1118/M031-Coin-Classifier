/******************************************************************************
 * @file     cnn_api.h
 * @version  V1.00
 * @brief    API header for the CNN inference module.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#ifndef CNN_API_H
#define CNN_API_H

#include <stdint.h>
/* img: Points to a 40x30 (DST_WIDTH x DST_HEIGHT) grayscale image, 0..255 */
void run_cnn_inference_40x30(const uint8_t *img, uint8_t *digit);

#endif /* CNN_API_H */