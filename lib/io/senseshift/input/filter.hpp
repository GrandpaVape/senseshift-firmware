#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include <senseshift/core/helpers.hpp>

namespace SenseShift::Input {
    template<typename Tp>
    class ISimpleSensor;

    template<typename Tp>
    class Sensor;

    template<typename Tp>
    class IFilter {
      public:
        using ValueType = Tp;

        virtual auto filter(ISimpleSensor<ValueType>* sensor, ValueType value) -> ValueType = 0;
    };

    template<typename Tp>
    class AddFilter : public IFilter<Tp> {
      public:
        explicit AddFilter(Tp offset) : offset_(offset){};

        auto filter(ISimpleSensor<Tp>* /*sensor*/, Tp value) -> Tp override
        {
            return value + this->offset_;
        }

      private:
        Tp offset_;
    };

    template<typename Tp>
    class IFiltered {
      public:
        using ValueType = Tp;

        virtual void addFilter(IFilter<ValueType>* filter) = 0;

        virtual void addFilters(std::vector<IFilter<ValueType>*> filters) = 0;

        virtual void setFilters(std::vector<IFilter<ValueType>*> filters) = 0;

        void clearFilters() = 0;
    };

    template<typename Tp>
    class SubtractFilter : public IFilter<Tp> {
      public:
        explicit SubtractFilter(Tp offset) : offset_(offset){};

        auto filter(ISimpleSensor<Tp>* /*sensor*/, Tp value) -> Tp override
        {
            return value - this->offset_;
        }

      private:
        Tp offset_;
    };

    template<typename Tp>
    class MultiplyFilter : public IFilter<Tp> {
      public:
        explicit MultiplyFilter(Tp factor) : factor_(factor){};

        auto filter(ISimpleSensor<Tp>* /*sensor*/, Tp value) -> Tp override
        {
            return value * this->factor_;
        }

    private:
        Tp factor_;
    };

    class VoltageDividerFilter : public MultiplyFilter<float> {
      public:
        /// \param r1 The resistance in Ohms of the first resistor in the voltage divider.
        /// Example: 27000.0F.
        /// \param r2 The resistance in Ohms of the second resistor in the voltage divider.
        /// Example: 100000.0F.
        ///
        /// \example
        /// \code
        /// new VoltageDividerFilter(27000.0F, 100000.0F);
        /// \endcode
        explicit VoltageDividerFilter(float r1, float r2) : MultiplyFilter<float>(r2 / (r1 + r2)){};
    };

    template<typename Tp>
    class ClampFilter : public IFilter<Tp> {
      public:
        ClampFilter(Tp min, Tp max) : min_(min), max_(max){};

        auto filter(ISimpleSensor<Tp>* /*source*/, Tp value) -> Tp override
        {
            return std::clamp(value, this->min_, this->max_);
        }

      private:
        Tp min_;
        Tp max_;
    };

    /// An alias for ClampFilter.
    template<typename Tp>
    using MinMaxFilter = ClampFilter<Tp>;

    /// An alias for ClampFilter.
    template<typename Tp>
    using RangeFilter = ClampFilter<Tp>;

    template<typename Tp>
    class LambdaFilter : public IFilter<Tp> {
      public:
        using Lambda = std::function<Tp(Tp)>;

        explicit LambdaFilter(Lambda filter) : filter_(std::move(filter)) {};

        auto filter(ISimpleSensor<Tp>* /*sensor*/, Tp value) -> Tp override
        {
            return this->filter_(value);
        }

      private:
        Lambda filter_;
    };

    /// Average filter. Reads the value from the sensor and returns the average of the N values.
    template<typename Tp, typename Sensor>
    class SampleAverageFilter : public IFilter<Tp> {
        static_assert(std::is_arithmetic_v<Tp>, "SampleAverageFilter only supports arithmetic types");
        static_assert(std::is_same_v<typename Sensor::ValueType, Tp>, "Sensor type must match filter type");
        static_assert(std::is_same_v<decltype(&Sensor::readRawValue), void (Sensor::*)()>, "Can only use sensors with readRawValue()");

      public:
        explicit SampleAverageFilter(std::size_t size) : size_(size){};

        auto filter(ISimpleSensor<Tp>* sensor, Tp value) -> Tp override
        {
            auto sum = value;

            // Read the acc_ from the sensor N-1 times and sum them up.
            // We read it N-1 times because we already have the first acc_.
            for (std::size_t i = 0; i < this->size_ - 1; i++) {
                sum += sensor->readRawValue();
            }

            // Return the average of the values.
            return sum / this->size_;
        }

      private:
        std::size_t size_;
    };

    template<std::size_t N, typename Tp, typename Sensor>
    class SampleMedianFilter : public IFilter<Tp> {
        static_assert(N % 2 == 1, "SampleMedianFilter only supports odd sample sizes");
        static_assert(std::is_same_v<typename Sensor::ValueType, Tp>, "Sensor type must match filter type");
        static_assert(std::is_same_v<decltype(&Sensor::readRawValue), void (Sensor::*)()>, "Can only use sensors with readRawValue()");

      public:
        explicit SampleMedianFilter() = default;

        auto filter(ISimpleSensor<Tp>* sensor, Tp value) -> Tp override
        {
            this->values = { value };

            // Read the acc_ from the sensor N-1 times and put them in the array.
            // We read it N-1 times because we already have the first acc_.
            for (std::size_t i = 1; i <= this->size_ - 1; i++) {
                this->values[i] = sensor->readRawValue();
            }

            // Sort the array.
            std::sort(this->values.begin(), this->values.end());

            // Return the median of the values.
            return this->values[this->size_ / 2];
        }

      private:
        std::array<Tp, N> values_;
    };

    template<typename Tp>
    class SlidingWindowMovingAverageFilter : public IFilter<Tp> {
        static_assert(std::is_arithmetic_v<Tp>, "SlidingWindowAverageFilter only supports arithmetic types");

      public:
        explicit SlidingWindowMovingAverageFilter(size_t window_size) : window_size_(window_size) { };

        auto filter(ISimpleSensor<Tp>* /*sensor*/, Tp value) -> Tp override
        {
            while (this->queue_.size() >= this->window_size_) {
                this->queue_.pop_front();
            }
            this->queue_.push_back(value);

            return this->getAverage();
        }

      private:
        std::size_t window_size_;
        std::deque<Tp> values_;

        [[nodiscard]] auto getAverage() const -> Tp
        {
            Tp sum = 0;
            for (auto value : this->queue_) {
                sum += value;
            }
            return sum / this->queue_.size();
        }
    };

    template<typename Tp>
    class ExponentialMovingAverageFilter : public IFilter<Tp> {
        static_assert(std::is_arithmetic_v<Tp>, "ExponentialMovingAverageFilter only supports arithmetic types");

      public:
        explicit ExponentialMovingAverageFilter(float alpha) : alpha_(alpha) { };

        auto filter(ISimpleSensor<Tp>* /*sensor*/, Tp value) -> Tp override
        {
            this->acc_ = this->alpha_ * value + (1 - this->alpha_) * this->acc_;

            return this->acc_;
        }

      private:
        float alpha_;
        Tp acc_;
    };

    /// Deadzone filter. Clamps acc_ to center if it is within the deadzone.
    /// Usually used to filter out noise in the joystick.
    class CenterDeadzoneFilter : public IFilter<float>
    {
      public:
        explicit CenterDeadzoneFilter(float deadzone) : deadzone_(deadzone){};

        auto filter(ISimpleSensor<float>* /*sensor*/, float value) -> float override
        {
            float const deviation = std::abs(CENTER - value);
            return deviation < deadzone_ ? CENTER : value;
        }

    private:
        static constexpr float CENTER = 0.5F;

        float deadzone_;
    };

    /// \tparam Container Type of the lookup table container.
    template <typename Tp, typename Container>
    class LookupTableInterpolationFilter : public IFilter<Tp>
    {
        static_assert(std::is_same_v<typename Container::value_type, Tp>);
        static_assert(std::is_arithmetic_v<Tp>, "LookupTableInterpolationFilter only supports arithmetic types");

      public:
        explicit LookupTableInterpolationFilter(Container const& lookup_table) : lookup_table_(lookup_table) {};

        auto filter(ISimpleSensor<float>* /*sensor*/, Tp value) -> Tp override
        {
            return SenseShift::lookup_interpolate<Tp, Container>(this->lookup_table_, value);
        }

      private:
        Container const& lookup_table_;
    };
} // namespace SenseShift::Input