/**
 * @file
 * Auto-generated configuration file for Command Class Multi Channel.
 *
 * This file defines the number of endpoints in a multi channel device and the generic type and
 * specific type for each endpoint.
 *
 * This array must be sorted by endpoint.
 */
#include "cc_multi_channel_config_api.h"
#include "ZW_classcmd.h"
#include "Assert.h"
#include "SizeOf.h"

static const cc_multi_channel_config_t endpoints[] = {
  {
    // Endpoint 1
    .generic_type  = GENERIC_TYPE_SWITCH_BINARY,
    .specific_type = SPECIFIC_TYPE_NOT_USED
  },
  {
    // Endpoint 2
    .generic_type  = GENERIC_TYPE_SWITCH_BINARY,
    .specific_type = SPECIFIC_TYPE_NOT_USED
  },
  {
    // Endpoint 3
    .generic_type  = GENERIC_TYPE_SWITCH_BINARY,
    .specific_type = SPECIFIC_TYPE_NOT_USED
  },
//  {
//    // Endpoint 4
//    .generic_type  = GENERIC_TYPE_SWITCH_BINARY,
//    .specific_type = SPECIFIC_TYPE_NOT_USED
//  },
//  {
//    // Endpoint 5
//    .generic_type  = GENERIC_TYPE_SWITCH_BINARY,
//    .specific_type = SPECIFIC_TYPE_NOT_USED
//  },
//  {
//    // Endpoint 6
//    .generic_type  = GENERIC_TYPE_SWITCH_BINARY,
//    .specific_type = SPECIFIC_TYPE_NOT_USED
//  }
};

cc_multi_channel_config_t const * cc_multi_channel_get_config(void)
{
  return endpoints;
}

cc_multi_channel_config_t const * cc_multi_channel_get_config_endpoint(uint8_t endpoint)
{
  ASSERT(sizeof_array(endpoints) > 0);
  ASSERT(endpoint <= sizeof_array(endpoints));
  /*
   * For backwards compatibility, the Root Device MUST
   * mirror the application functionality of End Point 1.
   */
  if (endpoint == 0) {
    return &endpoints[0];
  } else {
    return &endpoints[endpoint - 1];
  }
}
