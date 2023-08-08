#pragma once

#include <haptic_body.hpp>
#include <string>

namespace BH {
    void plainOutputTransformer(
      OH::HapticBody* output,
      std::string& value,
      const oh_output_point_t* layout[],
      const size_t layoutSize,
      const oh_output_target_t path
    );
    void vestOutputTransformer(
      OH::HapticBody* output, std::string& value, const oh_output_point_t* layout[], const size_t layoutSize
    );
    void vestX16OutputTransformer(
      OH::HapticBody* output,
      std::string& value,
      const oh_output_point_t* layout[],
      const size_t layoutSize,
      const uint8_t layoutGroups[],
      const size_t layoutGroupsSize
    );
} // namespace BH
