/*
 * Copyright (c) 2012-2015 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef _VX_KHR_DOT_H_
#define _VX_KHR_DOT_H_

#define OPENVX_KHR_DOT  "vx_khr_dot"

#include <VX/vx.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Exports a single graph to a dotfile.
 * \param [in] graph The graph to export.
 * \param [in] dotfile The name of the file to write to.
 * \param [in] showData If true, data objects will be listed in the graph too.
 * \see http://www.graphviz.com
 */
vx_status vxExportGraphToDot(vx_graph g, const vx_char dotfile[], vx_bool showData);

#ifdef __cplusplus
}
#endif

#endif

