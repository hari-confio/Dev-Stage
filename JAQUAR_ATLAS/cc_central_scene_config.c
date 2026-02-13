/**
 * @file
 * Command Class Central Scene Configuration
 * @remarks This file is auto generated
 * @copyright 2023 Silicon Laboratories Inc.
 */

#include <stdint.h>
#include <stdbool.h>
#include <CC_CentralScene.h>
#include <SizeOf.h>
#include <Assert.h>


/*
 * This array must be sorted by scene numbers. The Z-Wave Command Class Configurator (Z3C) will sort
 * the array, but if modified manually after generation, make sure to keep it sorted by scene numbers.
 */
static cc_central_scene_t scenes_attributes [] = {
  {
     .scene_number = 1,
     .scene_attributes = (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2 ) | (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_RELEASED_V2 ) | (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_HELD_DOWN_V2)
  },
};

STATIC_ASSERT((sizeof_array(scenes_attributes) > 0), STATIC_ASSERT_FAILED_size_mismatch);

cc_central_scene_t * cc_central_scene_config_get_scenes_attributes(void) {
  return scenes_attributes;
}

uint8_t cc_central_scene_config_get_number_of_scenes() {
  return 1;
}

uint8_t cc_central_scene_config_get_attribute_bitmask(uint8_t scene_number) {
  ASSERT(scene_number <= sizeof_array(scenes_attributes));
  return scenes_attributes[scene_number - 1].scene_attributes;
}

uint8_t cc_central_scene_config_get_identical() {
  return 1;
}
