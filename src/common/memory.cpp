#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "mkl_dnn.h"

#include "c_types_map.hpp"
#include "engine.hpp"
#include "utils.hpp"
#include "type_helpers.hpp"

using namespace mkl_dnn::impl;
using namespace mkl_dnn::impl::status;
using namespace mkl_dnn::impl::memory_format;

status_t mkl_dnn_tensor_desc_init(tensor_desc_t *tensor_desc,
        uint32_t ndims_batch, uint32_t ndims_channels, uint32_t ndims_spatial,
        const dims_t dims) {
    if (any_null(tensor_desc, dims))
        return invalid_arguments;
    tensor_desc_t td = {
        .ndims_batch = ndims_batch,
        .ndims_channels = ndims_channels,
        .ndims_spatial = ndims_spatial};
    const uint32_t ndims = types::ndims(td);
    if (ndims > TENSOR_MAX_DIMS)
        return invalid_arguments;
    array_copy(td.dims, dims, ndims);
    *tensor_desc = td;
    return success;
}

status_t mkl_dnn_memory_desc_init(memory_desc_t *memory_desc,
        const tensor_desc_t *tensor, memory_format_t format) {
    if (any_null(memory_desc, tensor))
        return invalid_arguments;
    memory_desc_t md = {
        .tensor_desc = *tensor,
        .format = format };

    status_t status = success;
    switch (format) {
    case any_f32:
        break;
    /* semidefined blocked format */
    case n_f32:
    case nchw_f32:
    case nhwc_f32:
        status = types::compute_blocking(md);
        break;
    /* no enough information */
    case blocked_f32:
    default:
        return invalid_arguments;
    }

    if (status == success)
        *memory_desc = md;
    return status;
}

status_t mkl_dnn_memory_primitive_desc_init(
        memory_primitive_desc_t *memory_primitive_desc,
        const memory_desc_t *memory_desc, const engine *engine) {
    if (any_null(memory_primitive_desc, memory_desc, engine))
        return invalid_arguments;
    if (memory_desc->format == any_f32 || !engine->is_ok())
        return invalid_arguments;

    for (auto init = engine->get_memory_inits(); *init; ++init) {
        status_t status = (*init)(
                reinterpret_cast<primitive_desc_t*>(memory_primitive_desc),
                static_cast<const_op_desc_t>(memory_desc), *engine);
        if (status == success)
            return success;
    }

    return unimplemented;
}

int mkl_dnn_memory_primitive_desc_equal(const memory_primitive_desc_t *lhs,
        const memory_primitive_desc_t *rhs) {
    return types::operator==(*lhs, *rhs);
}

status_t mkl_dnn_memory_create(primitive **memory,
        const memory_primitive_desc_t *memory_primitive_desc,
        void *data_ptr) {
    // XXX: is this ok?
    primitive_at_t inputs[] = {
        { static_cast<const primitive*>(data_ptr), 0 } };
    primitive *outputs[] = {
        static_cast<primitive*>(data_ptr) };
    return mkl_dnn_primitive_create(memory, memory_primitive_desc, inputs,
            outputs);
}

// vim: et ts=4 sw=4 cindent cino^=l0,\:0
