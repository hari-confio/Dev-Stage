/**
 * @file
 * Command Class Binary Switch Configuration
 * @remarks This file is auto generated
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <cc_binary_switch_config.h>
#include <CC_BinarySwitch.h>
#include <ZW_typedefs.h>
#include <SizeOf.h>
#include <Assert.h>

static cc_binary_switch_t binary_switches[] = {
    {
      .endpoint = 1,
      .default_duration = CC_BINARY_SWITCH_CONFIG_DEFAULT_DURATION,
      .callback = cc_binary_switch_handler
    },
    {
      .endpoint = 2,
      .default_duration = CC_BINARY_SWITCH_CONFIG_DEFAULT_DURATION,
      .callback = cc_binary_switch_handler
    },
    {
      .endpoint = 3,
      .default_duration = CC_BINARY_SWITCH_CONFIG_DEFAULT_DURATION,
      .callback = cc_binary_switch_handler
    },
//    {
//      .endpoint = 4,
//      .default_duration = CC_BINARY_SWITCH_CONFIG_DEFAULT_DURATION,
//      .callback = cc_binary_switch_handler
//    },
//    {
//      .endpoint = 5,
//      .default_duration = CC_BINARY_SWITCH_CONFIG_DEFAULT_DURATION,
//      .callback = cc_binary_switch_handler
//    },
//    {
//      .endpoint = 6,
//      .default_duration = CC_BINARY_SWITCH_CONFIG_DEFAULT_DURATION,
//      .callback = cc_binary_switch_handler
//    }
};

STATIC_ASSERT((sizeof_array(binary_switches) > 0), STATIC_ASSERT_FAILED_size_mismatch);

cc_binary_switch_t * cc_binary_switch_get_config(void)
{
  return binary_switches;
}

uint8_t cc_binary_switch_get_config_length(void)
{
  return sizeof_array(binary_switches);
}

ZAF_CC_REGISTER_CONFIG(COMMAND_CLASS_SWITCH_BINARY, &binary_switches[0], 0);
ZAF_CC_REGISTER_CONFIG(COMMAND_CLASS_SWITCH_BINARY, &binary_switches[1], 1);
ZAF_CC_REGISTER_CONFIG(COMMAND_CLASS_SWITCH_BINARY, &binary_switches[2], 2);
//ZAF_CC_REGISTER_CONFIG(COMMAND_CLASS_SWITCH_BINARY, &binary_switches[3], 3);
//ZAF_CC_REGISTER_CONFIG(COMMAND_CLASS_SWITCH_BINARY, &binary_switches[4], 4);
//ZAF_CC_REGISTER_CONFIG(COMMAND_CLASS_SWITCH_BINARY, &binary_switches[5], 5);
//ZAF_CC_REGISTER_CONFIG(COMMAND_CLASS_SWITCH_BINARY, &binary_switches[6], 6);
