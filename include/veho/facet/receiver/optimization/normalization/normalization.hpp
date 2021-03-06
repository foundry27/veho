/*
* Created by Mark Johnson on 3/3/2018.
*/

#ifndef VEHO_NORMALIZATION_HPP
#define VEHO_NORMALIZATION_HPP

#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>

#include <veho/facet/receiver/optimization/pass_traits.hpp>

#include "veho/facet/receiver/optimization/preprocessing/unify_identically_filtered_callbacks_pass.hpp"
#include "append_exact_filter_callback_to_range_filter_callback_when_exact_within_range_pass.hpp"
#include "append_range_filter_callback_to_exact_filter_callback_when_exact_within_range_pass.hpp"
#include "append_overlapping_range_filter_segment_callbacks_pass.hpp"


namespace veho {
    namespace facet {
        namespace receiver {
            namespace optimization {
                namespace normalization {
                    namespace detail {
                        template <typename Controller, typename TypeMap, typename Callbacks, typename PassList, std::size_t PassListSize = boost::mp11::mp_size<PassList>::value>
                        struct callback_normalizer_impl {
                            using current_quoted_pass = boost::mp11::mp_front<PassList>;

                            using current_pass = boost::mp11::mp_invoke<current_quoted_pass, Controller, TypeMap, Callbacks>;

                            using current_pass_type_map_type = typename current_pass::updated_type_map_type;

                            using current_pass_callbacks_type = typename current_pass::updated_callbacks_type;

                            using next_stage = callback_normalizer_impl<
                                    Controller,
                                    current_pass_type_map_type, current_pass_callbacks_type,
                                    boost::mp11::mp_pop_front<PassList>
                            >;

                            constexpr explicit callback_normalizer_impl(Callbacks&& callbacks)
                                    : new_callbacks(next_stage(current_pass(std::forward<Callbacks>(callbacks)).new_callbacks).new_callbacks) {}

                            using updated_type_map_type = typename next_stage::updated_type_map_type;

                            using updated_callbacks_type = typename next_stage::updated_callbacks_type;

                            updated_callbacks_type new_callbacks;
                        };

                        template <typename Controller, typename TypeMap, typename Callbacks, typename PassList>
                        struct callback_normalizer_impl<Controller, TypeMap, Callbacks, PassList, 0> {
                            constexpr explicit callback_normalizer_impl(Callbacks&& callbacks)
                                    : new_callbacks(std::forward<Callbacks>(callbacks)) {}

                            using updated_type_map_type = TypeMap;

                            using updated_callbacks_type = Callbacks;

                            updated_callbacks_type new_callbacks;
                        };
                    }

                    template <typename Controller, typename TypeMap, typename Callbacks>
                    struct callback_normalizer {
                    private:
                        using normalization_pass_list = boost::mp11::mp_list<
                                boost::mp11::mp_quote<append_range_filter_callback_to_exact_filter_callback_when_exact_within_range_pass>,
                                boost::mp11::mp_quote<append_exact_filter_callback_to_range_filter_callback_when_exact_within_range_pass>,
                                boost::mp11::mp_quote<append_overlapping_range_filter_segment_callbacks_pass>
                        >;

                        using impl = detail::callback_normalizer_impl<Controller, TypeMap, Callbacks, normalization_pass_list>;

                    public:
                        constexpr explicit callback_normalizer(Callbacks&& callbacks)
                                : new_callbacks(impl(std::forward<Callbacks>(callbacks)).new_callbacks) {}

                        using new_type_map_type = typename impl::updated_type_map_type;

                        using new_callbacks_type = typename impl::updated_callbacks_type;

                        new_callbacks_type new_callbacks;
                    };
                }
            }
        }
    }
}

#endif //VEHO_NORMALIZATION_HPP
