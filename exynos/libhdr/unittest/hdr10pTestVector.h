#ifndef __HDR10P_TEST_VECTOR_H__
#define __HDR10P_TEST_VECTOR_H__
#include <system/graphics.h>
#include <libhdr_parcel_header.h>
#include <hdrUtil.h>
#include <hdrHwInfo.h>
#include <algorithm>
#include <unistd.h>
#include <string.h>

#include "VendorVideoAPI_hdrTest.h"
class hdr10pTestVector {
private:
    ExynosHdrDynamicInfo __attribute__((unused)) d_meta[4] = {
        { // 0:00:05
            .valid = 0,
            .data.country_code = 181,
            .data.provider_code = 60,
            .data.provider_oriented_code = 1,
            .data.application_identifier = 4,
            .data.num_windows = 1,
            .data.targeted_system_display_maximum_luminance = 500,
            .data.maxscl[0][0] = 8488,
            .data.maxscl[0][1] = 9694,
            .data.maxscl[0][2] = 14439,
            .data.num_maxrgb_percentiles[0] = 9,
            .data.maxrgb_percentages[0][0] =    1,
            .data.maxrgb_percentages[0][1] =    5,
            .data.maxrgb_percentages[0][2] =    10,
            .data.maxrgb_percentages[0][3] =    25,
            .data.maxrgb_percentages[0][4] =    50,
            .data.maxrgb_percentages[0][5] =    75,
            .data.maxrgb_percentages[0][6] =    90,
            .data.maxrgb_percentages[0][7] =    95,
            .data.maxrgb_percentages[0][8] =    99,
            .data.maxrgb_percentages[0][9] =    0,
            .data.maxrgb_percentages[0][10] =    0,
            .data.maxrgb_percentages[0][11] =    0,
            .data.maxrgb_percentages[0][12] =    0,
            .data.maxrgb_percentages[0][13] =    0,
            .data.maxrgb_percentages[0][14] =    0,
            .data.maxrgb_percentiles[0][0] =    0,
            .data.maxrgb_percentiles[0][1] =    377,
            .data.maxrgb_percentiles[0][2] =    98,
            .data.maxrgb_percentiles[0][3] =    3,
            .data.maxrgb_percentiles[0][4] =    4,
            .data.maxrgb_percentiles[0][5] =    8,
            .data.maxrgb_percentiles[0][6] =    22,
            .data.maxrgb_percentiles[0][7] =    71,
            .data.maxrgb_percentiles[0][8] =    1136,
            .data.maxrgb_percentiles[0][9] =    0,
            .data.maxrgb_percentiles[0][10] =    0,
            .data.maxrgb_percentiles[0][11] =    0,
            .data.maxrgb_percentiles[0][12] =    0,
            .data.maxrgb_percentiles[0][13] =    0,
            .data.maxrgb_percentiles[0][14] =    0,

            .data.tone_mapping.tone_mapping_flag[0] = 1,
            .data.tone_mapping.knee_point_x[0] = 0,
            .data.tone_mapping.knee_point_y[0] = 0,
            .data.tone_mapping.num_bezier_curve_anchors[0] = 9,
            .data.tone_mapping.bezier_curve_anchors[0][0] = 102,
            .data.tone_mapping.bezier_curve_anchors[0][1] = 205,
            .data.tone_mapping.bezier_curve_anchors[0][2] = 307,
            .data.tone_mapping.bezier_curve_anchors[0][3] = 410,
            .data.tone_mapping.bezier_curve_anchors[0][4] = 512,
            .data.tone_mapping.bezier_curve_anchors[0][5] = 614,
            .data.tone_mapping.bezier_curve_anchors[0][6] = 717,
            .data.tone_mapping.bezier_curve_anchors[0][7] = 819,
            .data.tone_mapping.bezier_curve_anchors[0][8] = 922,
            .data.tone_mapping.bezier_curve_anchors[0][9] =  0,
            .data.tone_mapping.bezier_curve_anchors[0][10] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][11] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][12] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][13] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][14] = 0,
        },
        { // 0:00:35
            .valid = 0,
            .data.country_code = 181,
            .data.provider_code = 60,
            .data.provider_oriented_code = 1,
            .data.application_identifier = 4,
            .data.num_windows = 1,
            .data.targeted_system_display_maximum_luminance = 500,
            .data.maxscl[0][0] = 36511,
            .data.maxscl[0][1] = 28695,
            .data.maxscl[0][2] = 25596,
            .data.num_maxrgb_percentiles[0] = 9,
            .data.maxrgb_percentages[0][0] =    1,
            .data.maxrgb_percentages[0][1] =    5,
            .data.maxrgb_percentages[0][2] =    10,
            .data.maxrgb_percentages[0][3] =    25,
            .data.maxrgb_percentages[0][4] =    50,
            .data.maxrgb_percentages[0][5] =    75,
            .data.maxrgb_percentages[0][6] =    90,
            .data.maxrgb_percentages[0][7] =    95,
            .data.maxrgb_percentages[0][8] =    99,
            .data.maxrgb_percentages[0][9] =    0,
            .data.maxrgb_percentages[0][10] =    0,
            .data.maxrgb_percentages[0][11] =    0,
            .data.maxrgb_percentages[0][12] =    0,
            .data.maxrgb_percentages[0][13] =    0,
            .data.maxrgb_percentages[0][14] =    0,
            .data.maxrgb_percentiles[0][0] =    29,
            .data.maxrgb_percentiles[0][1] =    15031,
            .data.maxrgb_percentiles[0][2] =    30,
            .data.maxrgb_percentiles[0][3] =    893,
            .data.maxrgb_percentiles[0][4] =    1715,
            .data.maxrgb_percentiles[0][5] =    2455,
            .data.maxrgb_percentiles[0][6] =    3269,
            .data.maxrgb_percentiles[0][7] =    3678,
            .data.maxrgb_percentiles[0][8] =    23472,
            .data.maxrgb_percentiles[0][9] =    0,
            .data.maxrgb_percentiles[0][10] =    0,
            .data.maxrgb_percentiles[0][11] =    0,
            .data.maxrgb_percentiles[0][12] =    0,
            .data.maxrgb_percentiles[0][13] =    0,
            .data.maxrgb_percentiles[0][14] =    0,

            .data.tone_mapping.tone_mapping_flag[0] = 1,
            .data.tone_mapping.knee_point_x[0] = 13,
            .data.tone_mapping.knee_point_y[0] = 64,
            .data.tone_mapping.num_bezier_curve_anchors[0] = 9,
            .data.tone_mapping.bezier_curve_anchors[0][0] = 303,
            .data.tone_mapping.bezier_curve_anchors[0][1] = 631,
            .data.tone_mapping.bezier_curve_anchors[0][2] = 712,
            .data.tone_mapping.bezier_curve_anchors[0][3] = 770,
            .data.tone_mapping.bezier_curve_anchors[0][4] = 818,
            .data.tone_mapping.bezier_curve_anchors[0][5] = 860,
            .data.tone_mapping.bezier_curve_anchors[0][6] = 899,
            .data.tone_mapping.bezier_curve_anchors[0][7] = 934,
            .data.tone_mapping.bezier_curve_anchors[0][8] = 954,
            .data.tone_mapping.bezier_curve_anchors[0][9] =  0,
            .data.tone_mapping.bezier_curve_anchors[0][10] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][11] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][12] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][13] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][14] = 0,
        },
        { // 0:01:42
            .valid = 0,
            .data.country_code = 181,
            .data.provider_code = 60,
            .data.provider_oriented_code = 1,
            .data.application_identifier = 4,
            .data.num_windows = 1,
            .data.targeted_system_display_maximum_luminance = 500,
            .data.maxscl[0][0] = 31159,
            .data.maxscl[0][1] = 27746,
            .data.maxscl[0][2] = 23880,
            .data.num_maxrgb_percentiles[0] = 9,
            .data.maxrgb_percentages[0][0] =    1,
            .data.maxrgb_percentages[0][1] =    5,
            .data.maxrgb_percentages[0][2] =    10,
            .data.maxrgb_percentages[0][3] =    25,
            .data.maxrgb_percentages[0][4] =    50,
            .data.maxrgb_percentages[0][5] =    75,
            .data.maxrgb_percentages[0][6] =    90,
            .data.maxrgb_percentages[0][7] =    95,
            .data.maxrgb_percentages[0][8] =    99,
            .data.maxrgb_percentages[0][9] =    0,
            .data.maxrgb_percentages[0][10] =    0,
            .data.maxrgb_percentages[0][11] =    0,
            .data.maxrgb_percentages[0][12] =    0,
            .data.maxrgb_percentages[0][13] =    0,
            .data.maxrgb_percentages[0][14] =    0,
            .data.maxrgb_percentiles[0][0] =    195,
            .data.maxrgb_percentiles[0][1] =    27702,
            .data.maxrgb_percentiles[0][2] =    4,
            .data.maxrgb_percentiles[0][3] =    1327,
            .data.maxrgb_percentiles[0][4] =    2002,
            .data.maxrgb_percentiles[0][5] =    3051,
            .data.maxrgb_percentiles[0][6] =    4920,
            .data.maxrgb_percentiles[0][7] =    6345,
            .data.maxrgb_percentiles[0][8] =    30871,
            .data.maxrgb_percentiles[0][9] =    0,
            .data.maxrgb_percentiles[0][10] =    0,
            .data.maxrgb_percentiles[0][11] =    0,
            .data.maxrgb_percentiles[0][12] =    0,
            .data.maxrgb_percentiles[0][13] =    0,
            .data.maxrgb_percentiles[0][14] =    0,

            .data.tone_mapping.tone_mapping_flag[0] = 1,
            .data.tone_mapping.knee_point_x[0] = 13,
            .data.tone_mapping.knee_point_y[0] = 64,
            .data.tone_mapping.num_bezier_curve_anchors[0] = 9,
            .data.tone_mapping.bezier_curve_anchors[0][0] = 387,
            .data.tone_mapping.bezier_curve_anchors[0][1] = 622,
            .data.tone_mapping.bezier_curve_anchors[0][2] = 706,
            .data.tone_mapping.bezier_curve_anchors[0][3] = 766,
            .data.tone_mapping.bezier_curve_anchors[0][4] = 817,
            .data.tone_mapping.bezier_curve_anchors[0][5] = 860,
            .data.tone_mapping.bezier_curve_anchors[0][6] = 901,
            .data.tone_mapping.bezier_curve_anchors[0][7] = 936,
            .data.tone_mapping.bezier_curve_anchors[0][8] = 955,
            .data.tone_mapping.bezier_curve_anchors[0][9] =  0,
            .data.tone_mapping.bezier_curve_anchors[0][10] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][11] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][12] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][13] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][14] = 0,
        },
        { // 0:02:04
            .valid = 0,
            .data.country_code = 181,
            .data.provider_code = 60,
            .data.provider_oriented_code = 1,
            .data.application_identifier = 4,
            .data.num_windows = 1,
            .data.targeted_system_display_maximum_luminance = 500,
            .data.maxscl[0][0] = 25707,
            .data.maxscl[0][1] = 18765,
            .data.maxscl[0][2] = 9057,
            .data.num_maxrgb_percentiles[0] = 9,
            .data.maxrgb_percentages[0][0] =    1,
            .data.maxrgb_percentages[0][1] =    5,
            .data.maxrgb_percentages[0][2] =    10,
            .data.maxrgb_percentages[0][3] =    25,
            .data.maxrgb_percentages[0][4] =    50,
            .data.maxrgb_percentages[0][5] =    75,
            .data.maxrgb_percentages[0][6] =    90,
            .data.maxrgb_percentages[0][7] =    95,
            .data.maxrgb_percentages[0][8] =    99,
            .data.maxrgb_percentages[0][9] =    0,
            .data.maxrgb_percentages[0][10] =    0,
            .data.maxrgb_percentages[0][11] =    0,
            .data.maxrgb_percentages[0][12] =    0,
            .data.maxrgb_percentages[0][13] =    0,
            .data.maxrgb_percentages[0][14] =    0,
            .data.maxrgb_percentiles[0][0] =    10,
            .data.maxrgb_percentiles[0][1] =    11430,
            .data.maxrgb_percentiles[0][2] =    21,
            .data.maxrgb_percentiles[0][3] =    398,
            .data.maxrgb_percentiles[0][4] =    3247,
            .data.maxrgb_percentiles[0][5] =    6393,
            .data.maxrgb_percentiles[0][6] =    8137,
            .data.maxrgb_percentiles[0][7] =    9435,
            .data.maxrgb_percentiles[0][8] =    21916,
            .data.maxrgb_percentiles[0][9] =    0,
            .data.maxrgb_percentiles[0][10] =    0,
            .data.maxrgb_percentiles[0][11] =    0,
            .data.maxrgb_percentiles[0][12] =    0,
            .data.maxrgb_percentiles[0][13] =    0,
            .data.maxrgb_percentiles[0][14] =    0,

            .data.tone_mapping.tone_mapping_flag[0] = 1,
            .data.tone_mapping.knee_point_x[0] = 14,
            .data.tone_mapping.knee_point_y[0] = 64,
            .data.tone_mapping.num_bezier_curve_anchors[0] = 9,
            .data.tone_mapping.bezier_curve_anchors[0][0] = 286,
            .data.tone_mapping.bezier_curve_anchors[0][1] = 433,
            .data.tone_mapping.bezier_curve_anchors[0][2] = 556,
            .data.tone_mapping.bezier_curve_anchors[0][3] = 655,
            .data.tone_mapping.bezier_curve_anchors[0][4] = 709,
            .data.tone_mapping.bezier_curve_anchors[0][5] = 752,
            .data.tone_mapping.bezier_curve_anchors[0][6] = 813,
            .data.tone_mapping.bezier_curve_anchors[0][7] = 884,
            .data.tone_mapping.bezier_curve_anchors[0][8] = 940,
            .data.tone_mapping.bezier_curve_anchors[0][9] =  0,
            .data.tone_mapping.bezier_curve_anchors[0][10] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][11] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][12] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][13] = 0,
            .data.tone_mapping.bezier_curve_anchors[0][14] = 0,
        }
    };

    struct hdr10Node {
        int max_luminance;
        std::vector<struct hdr_dat_node> coef_packed;
    };
    struct hdr10Module {
        std::vector<struct hdr10Node>        hdr10NodeTable;
        void sort_ascending(void) {
            std::sort(hdr10NodeTable.begin(), hdr10NodeTable.end(),
                    [] (struct hdr10Node &op1, struct hdr10Node &op2) {
                        return op1.max_luminance < op2.max_luminance;
                    });
        }
    };
    hdrHwInfo *hwInfo = NULL;
    std::string filename = "/vendor/etc/dqe/hdr10pLut.xml";
    std::unordered_map<int, struct hdr10Module> layerToHdr10Mod;
    std::vector<std::unordered_map<int, struct hdr_dat_node>> group_list[HDR_HW_MAX];
    void parse(std::vector<struct supportedHdrHw> *list);
    void __parse__(int hw_id);
    void parse_hdr10Mods(
        int hw_id,
        xmlDocPtr xml_doc,
        xmlNodePtr xml_module);
    void parse_hdr10Mod(
        int hw_id,
        int module_id,
        struct hdr10Module &hdr10_module,
        xmlDocPtr xml_doc,
        xmlNodePtr xml_module);
    void parse_hdr10Node(
        int hw_id,
        int module_id,
        struct hdr10Node __attribute__((unused)) &hdr10_node,
        xmlDocPtr __attribute__((unused)) xml_doc,
        xmlNodePtr xml_hdr10node);

    std::unordered_map<int,struct hdr_dat_node*> hdrTV_map;
public:
    std::string testimage = "/data/hdr_test_1.png";
    void init(hdrHwInfo *hwInfo);
    void setDynamicMeta(ExynosHdrDynamicInfo *d_meta, int index);
    std::unordered_map<int,struct hdr_dat_node*>& getTestVector(
            int hw_id, int layer_index,
            ExynosHdrStaticInfo *s_meta,
            ExynosHdrDynamicInfo *d_meta);
    float getSrcMaxLuminance(ExynosHdrDynamicInfo *dyn_meta);

};

#endif
