#pragma once

#include "senseshift_interface.hpp"
#include <abstract_actuator.hpp>
#include <point2.hpp>

#include <map>

typedef uint8_t oh_output_path_t;
typedef OH_OUTPUT_COORD_T oh_output_coord_t;
typedef OH::Point2b oh_output_point_t;

namespace OH {
    struct OutputData {
        oh_output_point_t point;
        oh_output_intensity_t intensity;
    };

    struct OutputState {
        oh_output_intensity_t intensity;
    };
} // namespace OH

typedef OH::OutputData oh_output_data_t;
typedef OH::OutputState oh_output_state_t;
typedef std::map<oh_output_point_t, OH::AbstractActuator*> oh_output_actuators_map_t;
