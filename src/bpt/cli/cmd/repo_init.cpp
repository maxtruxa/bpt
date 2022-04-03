#include "../options.hpp"

#include <bpt/crs/error.hpp>
#include <bpt/crs/repo.hpp>
#include <bpt/error/marker.hpp>
#include <bpt/error/try_catch.hpp>
#include <bpt/util/fs/shutil.hpp>

namespace dds::cli::cmd {

namespace {

int _repo_init(const options& opts) {
    auto try_create = [&] {
        auto repo = dds::crs::repository::create(opts.repo.repo_dir, opts.repo.init.name);
        dds_log(info,
                "Created a new CRS repository '{}' in [{}]",
                repo.name(),
                repo.root().string());
        return 0;
    };
    return dds_leaf_try { return try_create(); }
    dds_leaf_catch(dds::crs::e_repo_already_init) {
        switch (opts.if_exists) {
        case if_exists::ignore:
            dds_log(info,
                    "Directory [{}] already contains a CRS repository",
                    opts.repo.repo_dir.string());
            return 0;
        case if_exists::replace:
            dds_log(info,
                    "Removing existing repository database [{}]",
                    opts.repo.repo_dir.string());
            dds::ensure_absent(opts.repo.repo_dir / "repo.db").value();
            return try_create();
        case if_exists::fail:
            throw;
        }
        neo::unreachable();
    };
}
}  // namespace

int repo_init(const options& opts) {
    return dds_leaf_try { return _repo_init(opts); }
    dds_leaf_catch(dds::crs::e_repo_already_init, dds::crs::e_repo_open_path dirpath) {
        dds_log(error,
                "Failed to initialize a new repostiory at [{}]: Repository already exists",
                dirpath.value.string());
        write_error_marker("repo-init-already-init");
        return 1;
    }
    dds_leaf_catch(std::error_code ec, dds::crs::e_repo_open_path dirpath) {
        dds_log(error,
                "Error while initializing new repository at [{}]: {}",
                dirpath.value.string(),
                ec.message());
        return 1;
    };
}

}  // namespace dds::cli::cmd
