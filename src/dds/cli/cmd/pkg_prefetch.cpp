#include "../options.hpp"

#include <dds/crs/cache.hpp>
#include <dds/crs/cache_db.hpp>
#include <dds/crs/info/pkg_id.hpp>
#include <dds/crs/repo.hpp>
#include <dds/util/url.hpp>

namespace dds::cli::cmd {

int pkg_prefetch(const options& opts) {
    auto cache
        = dds::crs::cache::open(opts.crs_cache_dir.value_or(dds::crs::cache::default_path()));
    auto& meta_db = cache.metadata_db();
    for (auto& r : opts.use_repos) {
        auto url = dds::guess_url_from_string(r);
        meta_db.sync_remote(url);
        meta_db.enable_remote(url);
    }

    for (auto& pkg_str : opts.pkg.prefetch.pkgs) {
        auto pid = crs::pkg_id::parse(pkg_str);
        cache.prefetch(pid);
    }
    return 0;
}

}  // namespace dds::cli::cmd
