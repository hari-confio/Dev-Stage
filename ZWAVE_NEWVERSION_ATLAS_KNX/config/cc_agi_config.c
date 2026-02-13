/**
 * @file
 * Command Class Association Group Information
 * @remarks This file is auto generated
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <cc_agi_config_api.h>
#include <Assert.h>
#include <SizeOf.h>

static const char GROUP_NAME_ENDPOINT_0_GROUP_2[] = "BTN1 - Basic";
static const ccc_pair_t COMMANDS_ENDPOINT_0_GROUP_2[] = {
  {
    .cmdClass = COMMAND_CLASS_BASIC_V2,
    .cmd = BASIC_SET_V2
  },
};
static const char GROUP_NAME_ENDPOINT_0_GROUP_3[] = "BTN2 - Basic";
static const ccc_pair_t COMMANDS_ENDPOINT_0_GROUP_3[] = {
  {
    .cmdClass = COMMAND_CLASS_BASIC_V2,
    .cmd = BASIC_SET_V2
  },
};static const char GROUP_NAME_ENDPOINT_0_GROUP_4[] = "BTN3 - Basic";
static const ccc_pair_t COMMANDS_ENDPOINT_0_GROUP_4[] = {
  {
    .cmdClass = COMMAND_CLASS_BASIC_V2,
    .cmd = BASIC_SET_V2
  },
};static const char GROUP_NAME_ENDPOINT_0_GROUP_5[] = "BTN4 - Basic";
static const ccc_pair_t COMMANDS_ENDPOINT_0_GROUP_5[] = {
  {
    .cmdClass = COMMAND_CLASS_BASIC_V2,
    .cmd = BASIC_SET_V2
  },
};
//static const char GROUP_NAME_ENDPOINT_0_GROUP_6[] = "BTN5 - Basic";
//static const ccc_pair_t COMMANDS_ENDPOINT_0_GROUP_6[] = {
//  {
//    .cmdClass = COMMAND_CLASS_BASIC_V2,
//    .cmd = BASIC_SET_V2
//  },
//};
static const cc_agi_group_t ROOT_DEVICE_GROUPS[] =
{
  {
    .name = GROUP_NAME_ENDPOINT_0_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_0_GROUP_2),
    .profile = {
      .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL,
      .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY01
    },
    .ccc_pairs = COMMANDS_ENDPOINT_0_GROUP_2,
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_0_GROUP_2),
  },
  {
    .name = GROUP_NAME_ENDPOINT_0_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_0_GROUP_3),
    .profile = {
      .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL,
      .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY02
    },
    .ccc_pairs = COMMANDS_ENDPOINT_0_GROUP_3,
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_0_GROUP_3),
  },
  {
      .name = GROUP_NAME_ENDPOINT_0_GROUP_4,
      .name_length = sizeof(GROUP_NAME_ENDPOINT_0_GROUP_4),
      .profile = {
        .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL,
        .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY03
      },
      .ccc_pairs = COMMANDS_ENDPOINT_0_GROUP_4,
      .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_0_GROUP_4),
    },
    {
        .name = GROUP_NAME_ENDPOINT_0_GROUP_5,
        .name_length = sizeof(GROUP_NAME_ENDPOINT_0_GROUP_5),
        .profile = {
          .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL,
          .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY04
        },
        .ccc_pairs = COMMANDS_ENDPOINT_0_GROUP_5,
        .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_0_GROUP_5),
      },
//      {
//          .name = GROUP_NAME_ENDPOINT_0_GROUP_6,
//          .name_length = sizeof(GROUP_NAME_ENDPOINT_0_GROUP_6),
//          .profile = {
//            .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL,
//            .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY05
//          },
//          .ccc_pairs = COMMANDS_ENDPOINT_0_GROUP_6,
//          .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_0_GROUP_6),
//        },
};

static const cc_agi_config_t config[] = {
  {
    .groups = ROOT_DEVICE_GROUPS,
    .group_count = sizeof_array(ROOT_DEVICE_GROUPS)
  },
};

const cc_agi_config_t * cc_agi_get_config(void)
{
  return config;
}

uint8_t cc_agi_config_get_group_count_by_endpoint(uint8_t endpoint)
{
  if (endpoint > sizeof_array(config)) {
    return 0;
  }
  //return config[endpoint].group_count;
}

const cc_agi_group_t * cc_agi_get_rootdevice_groups(void)
{
  return ROOT_DEVICE_GROUPS;
}
