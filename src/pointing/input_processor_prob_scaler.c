/*
 * Copyright (c) 2025 Erik Tollerud
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_prob_scaler

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct prob_scaler_config {
    uint8_t type;
    size_t codes_len;
    uint16_t codes[];
    bool float_divmul;
    float_t exponent;
};

static int scale_val(struct input_event *event, uint32_t mul, uint32_t div,
                     float_t exponent) {
    int16_t value_mul = event->value * (int16_t)mul;
    int16_t scaled = value_mul / (int16_t)div;

    LOG_DBG("scaled %d with %d/%d to %d with remainder %d", event->value, mul, div, scaled,
            (state && state->remainder) ? *state->remainder : 0);

    event->value = scaled;

    return 0;
}

static int prob_scaler_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    const struct prob_scaler_config *cfg = dev->config;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    for (int i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] == event->code) {
            return scale_val(event, param1, param2, cfg->exponent);
        }
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api scaler_driver_api = {
    .handle_event = prob_scaler_handle_event,


#define PROB_SCALER_INST(n)                                                                        \
    static const struct prob_scaler_config prob_scaler_config_##n = {                              \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .codes = DT_INST_PROP(n, codes),                                                           \
        .float_divmul = DT_INST_PROP(n, float_divmul),                                             \
        .exponent = DT_INST_PROP(n, exponent),                                                     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &prob_scaler_config_##n, POST_KERNEL,               \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &scaler_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PROB_SCALER_INST)
