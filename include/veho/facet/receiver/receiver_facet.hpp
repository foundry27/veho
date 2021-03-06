/*
* Created by Mark Johnson on 3/14/2018.
*/

#ifndef VEHO_RECEIVER_FACET_HPP
#define VEHO_RECEIVER_FACET_HPP

#include <utility>
#include <tuple>
#include <type_traits>

#include <boost/mp11/list.hpp>
#include <boost/mp11/integral.hpp>

#include <veho/frame.hpp>
#include <veho/bus_template_builder_fwd.hpp>
#include <veho/bus_template.hpp>

#include <veho/facet/builder_facet.hpp>
#include <veho/facet/template_facet.hpp>

#include <veho/config/config_traits.hpp>
#include <veho/controller/controller_traits.hpp>

#include <veho/controller/capabilities.hpp>

#include <veho/detail/instantiation_utils.hpp>
#include <veho/detail/tuple_utils.hpp>

#include "receiver_facet_compiletime_capability_data.hpp"
#include "receiver_facet_runtime_capability_data.hpp"
#include "receiver_facet_config_postprocessor.hpp"

namespace veho {
    namespace facet {
        namespace receiver {
            namespace detail {
                template <typename RuntimeConfig, typename... Dependencies>
                struct update_runtime_config_after_adding_dependencies {
                    constexpr explicit update_runtime_config_after_adding_dependencies(RuntimeConfig&& runtime_config, Dependencies&&... dependencies)
                            : updated_config(veho::facet::receiver::receiver_runtime_capability_data<std::tuple<Dependencies...>>(
                                    std::make_tuple(std::forward<Dependencies>(dependencies)...))) {}

                    using updated_config_type = decltype(
                            std::declval<RuntimeConfig>().template add_capability_data<veho::controller::receive_capability>(
                                    std::declval<veho::facet::receiver::receiver_runtime_capability_data<std::tuple<Dependencies...>>>()
                            )
                    );

                    updated_config_type updated_config;
                };

                template <typename Config, typename Matcher, typename Callback,
                        bool FacetAlreadyPresent = veho::config::config_traits<Config>::template has_capability<veho::controller::receive_capability>::value>
                struct update_config_after_adding_listener {
                    constexpr update_config_after_adding_listener(Config&& config, Matcher&& matcher, Callback&& callback)
                            : updated_config(
                            config.template replace_capability_data<veho::controller::receive_capability>(
                                    config.template capability_data_for<veho::controller::receive_capability>().push_back(
                                            std::forward<Matcher>(matcher),
                                            std::forward<Callback>(callback)
                                    )
                            )) {}

                    using updated_config_type = decltype(
                            std::declval<Config>().template replace_capability_data<veho::controller::receive_capability>(
                                    std::declval<Config>().template capability_data_for<veho::controller::receive_capability>().push_back(
                                            std::declval<Matcher>(),
                                            std::declval<Callback>()
                                    )
                            )
                    );

                    updated_config_type updated_config;
                };

                template <typename Config, typename Matcher, typename Callback>
                struct update_config_after_adding_listener<Config, Matcher, Callback, false> {
                private:
                    using controller = typename config::config_traits<Config>::controller_type;

                    using updated_capability_data = decltype(
                    make_receiver_compiletime_capability_data<controller>().push_back(std::declval<Matcher>(), std::declval<Callback>())
                    );

                public:
                    constexpr update_config_after_adding_listener(Config&& config, Matcher&& matcher,
                                                                  Callback&& callback)
                            : updated_config(config.template add_capability_data<veho::controller::receive_capability>(
                            make_receiver_compiletime_capability_data<controller>().push_back(std::forward<Matcher>(matcher),
                                                                      std::forward<Callback>(callback)))) {
                    }

                    using updated_config_type = decltype(
                            std::declval<Config>().template add_capability_data<veho::controller::receive_capability>(
                                    std::declval<updated_capability_data>()
                            )
                    );

                    updated_config_type updated_config;
                };

                template <std::size_t NumMailboxes, std::size_t NumTransmitters>
                struct ensure_transmitters_do_not_occupy_all_available_mailboxes_impl {
                };

                template <std::size_t NumTransmitters>
                struct ensure_transmitters_do_not_occupy_all_available_mailboxes_impl<NumTransmitters, NumTransmitters> {
                    static_assert(veho::detail::false_if_instantiated<boost::mp11::mp_int<NumTransmitters>>::value,
                                  "There are no available mailboxes for listeners");
                };

                template <typename Config, bool HasTransmitterCountBeenSpecified = veho::config::config_traits<Config>::template has_capability<veho::controller::transmit_capability>::value>
                struct ensure_transmitters_do_not_occupy_all_available_mailboxes {
                };

                template <typename Config>
                struct ensure_transmitters_do_not_occupy_all_available_mailboxes<Config, true>
                        : ensure_transmitters_do_not_occupy_all_available_mailboxes_impl<
                                veho::controller::controller_traits<typename veho::config::config_traits<Config>::controller_type>::num_mailboxes,
                                veho::config::config_traits<Config>::template capability_data_type<veho::controller::transmit_capability>::num_transmitters
                        > {
                };
            }
        }

        template <typename Config>
        struct builder_facet<Config, veho::controller::receive_capability> {
        private:
            template <typename Matcher, typename Callback>
            using config_updater = receiver::detail::update_config_after_adding_listener<Config, Matcher, Callback>;

        public:
            template <typename Matcher, typename Callback, typename... ListenerPolicies>
            constexpr veho::bus_template_builder<typename config_updater<Matcher, Callback>::updated_config_type>
            on(Matcher&& matcher, Callback&& callback, ListenerPolicies... listener_policies) {
                return (receiver::detail::ensure_transmitters_do_not_occupy_all_available_mailboxes<Config>(),
                        veho::bus_template_builder<typename config_updater<Matcher, Callback>::updated_config_type>(
                                config_updater<Matcher, Callback>(
                                        std::move(
                                                const_cast<veho::bus_template_builder<Config>*>(static_cast<const veho::bus_template_builder<Config>*>(this))->config
                                        ),
                                        std::forward<Matcher>(matcher),
                                        std::forward<Callback>(callback)
                                ).updated_config
                        ));
            }
        };

        template <typename CompiletimeConfig, typename RuntimeConfig>
        struct template_facet<CompiletimeConfig, RuntimeConfig, veho::controller::receive_capability> {
        private:
            template <typename... Dependencies>
            using runtime_config_updater = receiver::detail::update_runtime_config_after_adding_dependencies<RuntimeConfig, Dependencies...>;

            template <typename... Dependencies>
            using updated_runtime_config = typename runtime_config_updater<Dependencies...>::updated_config_type;

        public:
            template <typename... Dependencies>
            inline veho::bus_template<
                    CompiletimeConfig,
                    updated_runtime_config<Dependencies...>
            > with(Dependencies&&... dependencies) const {
                return veho::bus_template<CompiletimeConfig, updated_runtime_config<Dependencies...>>(
                        std::move(
                                const_cast<veho::bus_template<CompiletimeConfig, RuntimeConfig>*>(
                                        static_cast<const veho::bus_template<CompiletimeConfig, RuntimeConfig>*>(this))->compiletime_config
                        ),
                        runtime_config_updater<Dependencies...>(
                                std::move(
                                        const_cast<veho::bus_template<CompiletimeConfig, RuntimeConfig>*>(
                                                static_cast<const veho::bus_template<CompiletimeConfig, RuntimeConfig>*>(this))->runtime_config
                                ),
                                std::forward<Dependencies>(dependencies)...
                        ).updated_config
                );
            }
        };
    }
}

#endif //VEHO_RECEIVER_FACET_HPP
