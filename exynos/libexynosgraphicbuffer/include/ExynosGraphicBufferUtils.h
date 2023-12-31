#include "exynos_format.h"
#include <system/graphics.h>

static bool is_semi_planar(uint32_t format) {
    switch (format) {
        // HAL FORMAT
        case HAL_PIXEL_FORMAT_YCBCR_422_SP:
        case HAL_PIXEL_FORMAT_YCRCB_420_SP:
        case HAL_PIXEL_FORMAT_YCBCR_P010:
        // EXYNOS FORMAT
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_SP:
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L50:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L75:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80:
            return true;
    }
    return false;
}
