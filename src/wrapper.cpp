#include "blackhole/wrapper.hpp"

namespace blackhole {

wrapper_t::wrapper_t(logger_t& log, attributes_t attributes):
    inner(log),
    storage(std::move(attributes))
{
    for (const auto& attribute : storage) {
        attributes_view.emplace_back(attribute);
    }
}

auto
wrapper_t::log(int severity, string_view pattern) -> void {
    attribute_pack pack{attributes()};
    inner.log(severity, pattern, pack);
}

auto
wrapper_t::log(int severity, string_view pattern, attribute_pack& pack) -> void {
    pack.push_back(attributes());
    inner.log(severity, pattern, pack);
}

auto
wrapper_t::log(int severity, string_view pattern, attribute_pack& pack, const format_t& fn) -> void {
    pack.push_back(attributes());
    inner.log(severity, pattern, pack, fn);
}

}  // namespace blackhole