import json
from pathlib import Path

import pytest
from dds_ci.dds import DDSWrapper

from dds_ci.testing import RepoServer, Project, ProjectOpener
from dds_ci.testing import repo
from dds_ci.testing.error import expect_error_marker
from dds_ci.testing.fs import DirRenderer, TempPathFactory
from dds_ci.testing.repo import CRSRepo, CRSRepoFactory, RepoCloner, clone_repo, make_simple_crs
from dds_ci import proc, toolchain


@pytest.fixture(scope='session')
def bd_test_repo(crs_repo_factory: CRSRepoFactory, dir_renderer: DirRenderer) -> CRSRepo:
    repo = crs_repo_factory('bd-repo')
    pkg_1 = dir_renderer.get_or_render(
        'pkg_1', {
            'pkg.json': json.dumps(make_simple_crs('foo', '1.2.3')),
            'src': {
                'foo.hpp':
                '#pragma once\nextern void foo_fun();',
                'foo.cpp':
                r'''
                    #include <iostream>

                    #include "./foo.hpp"

                    void foo_fun() {
                        std::cout << "Hello, from foo library!\n";
                    }
                '''
            }
        })
    repo.import_dir(pkg_1)
    pkg_2 = dir_renderer.get_or_render(
        'pkg_2', {
            'pkg.json': json.dumps(make_simple_crs('foo', '2.0.0')),
            'src': {
                'foo.hpp':
                '#pragma once\n extern void bar_fun();',
                'foo.cpp':
                r'''
                    #include <iostream>
                    #include "./foo.hpp"

                    void bar_fun() {
                        std::cout << "New foo libary!\n";
                    }
                '''
            }
        })
    repo.import_dir(pkg_2)
    pkg_3 = dir_renderer.get_or_render(
        'pkg_3', {
            'pkg.json': json.dumps(make_simple_crs('foo', '1.2.8')),
            'src': {
                'foo.hpp':
                '#pragma once\n extern void foo_fun();',
                'foo.cpp':
                r'''
                    #include <iostream>
                    #include "./foo.hpp"

                    void foo_fun() {
                        std::cout << "foo 1.2.8!\n";
                    }
                '''
            }
        })
    repo.import_dir(pkg_3)
    pkg_4 = dir_renderer.get_or_render(
        'pkg_4', {
            'pkg.json': json.dumps(make_simple_crs('foo', '1.3.4')),
            'src': {
                'foo.hpp':
                '#pragma once\n extern void foo_fun();',
                'foo.cpp':
                r'''
                    #include <iostream>
                    #include "./foo.hpp"

                    void foo_fun() {
                        std::cout << "foo 1.3.4!\n";
                    }
                '''
            }
        })
    repo.import_dir(pkg_4)
    return repo


@pytest.fixture()
def bd_project(tmp_project: Project) -> Project:
    tmp_project.dds.crs_cache_dir = tmp_project.root / '_crs'
    return tmp_project


def test_cli(bd_test_repo: CRSRepo, bd_project: Project) -> None:
    bd_project.dds.build_deps(['foo@1.2.3'], repos=[bd_test_repo.path])
    assert bd_project.root.joinpath('_deps/foo@1.2.3').is_dir()
    assert bd_project.root.joinpath('_deps/_libman/foo.lmp').is_file()
    assert bd_project.root.joinpath('_deps/_libman/foo/foo.lml').is_file()
    assert bd_project.root.joinpath('INDEX.lmi').is_file()


def test_cli_missing(bd_test_repo: CRSRepo, bd_project: Project) -> None:
    with expect_error_marker('no-dependency-solution'):
        bd_project.dds.build_deps(['no-such-pkg@4.5.6'])


def test_from_file(bd_test_repo: CRSRepo, bd_project: Project) -> None:
    """build-deps using a file listing deps"""
    bd_project.write('deps.json5', json.dumps({'depends': ['foo+0.3.0']}))
    bd_project.dds.build_deps(['-d', 'deps.json5'], repos=[bd_test_repo.path])
    assert bd_project.root.joinpath('_deps/foo@1.2.3').is_dir()
    assert bd_project.root.joinpath('_deps/_libman/foo.lmp').is_file()
    assert bd_project.root.joinpath('_deps/_libman/foo/foo.lml').is_file()
    assert bd_project.root.joinpath('INDEX.lmi').is_file()


def test_from_file_missing(bd_project: Project) -> None:
    bd_project.write('deps.json5', json.dumps({'depends': ['no-such-pkg@9.3.1']}))
    with expect_error_marker('no-dependency-solution'):
        bd_project.dds.build_deps(['-d', 'deps.json5'])


def test_multiple_deps(bd_test_repo: CRSRepo, bd_project: Project) -> None:
    """build-deps with multiple deps resolves to a single version"""
    bd_project.dds.build_deps(['foo@1.2.3', 'foo@1.2.6'], repos=[bd_test_repo.path])
    assert bd_project.root.joinpath('_deps/foo@1.2.8').is_dir()
    assert bd_project.root.joinpath('_deps/_libman/foo.lmp').is_file()
    assert bd_project.root.joinpath('_deps/_libman/foo/foo.lml').is_file()
    assert bd_project.root.joinpath('INDEX.lmi').is_file()


def test_cmake_simple(project_opener: ProjectOpener, bd_test_repo: CRSRepo) -> None:
    proj = project_opener.open('projects/simple-cmake')
    proj.build_root.mkdir(exist_ok=True, parents=True)
    proj.dds.crs_cache_dir = proj.build_root / '_crs'
    proj.dds.build_deps(
        [
            'foo@1.2.3',
            f'--cmake={proj.build_root}/libraries.cmake',
        ],
        repos=[bd_test_repo.path],
    )

    try:
        proc.check_run(['cmake', '-S', proj.root, '-B', proj.build_root])
    except FileNotFoundError:
        assert False, 'Running the integration tests requires a CMake executable'
    proc.check_run(['cmake', '--build', proj.build_root])


def test_cmake_transitive(bd_project: Project, tmp_crs_repo: CRSRepo, dir_renderer: DirRenderer) -> None:
    # yapf: disable
    libs = dir_renderer.get_or_render(
        'libs',
        {
            'foo': {
                'pkg.json': json.dumps({
                    'crs_version': 1,
                    'name': 'foo',
                    'namespace': 'foo',
                    'version': '1.2.3',
                    'meta_version': 1,
                    'libraries': [{
                        'name': 'foo',
                        'uses': [],
                        'path': '.',
                        'depends': [],
                    }]
                }),
                'src': {
                    'foo.hpp': r'''
                        #pragma once
                        namespace foo{int value();}
                        ''',
                    'foo.cpp': r'''
                        #include "./foo.hpp"
                        int foo::value(){ return 42; }
                        ''',
                }
            },
            'bar': {
                'pkg.json': json.dumps({
                    'crs_version': 1,
                    'name': 'bar',
                    'namespace': 'bar',
                    'version': '1.2.3',
                    'meta_version': 1,
                    'libraries': [{
                        'name': 'bar',
                        'uses': [],
                        'path': '.',
                        'depends': [{
                            'name': 'foo',
                            'versions': [{'low': '1.2.3', 'high': '1.2.4'}],
                            'uses': ['foo'],
                            'for': 'lib',
                        }],
                    }]
                }),
                'src': {
                    'bar.hpp': r'''
                        #pragma once
                        namespace bar{int value();}
                        ''',
                    'bar.cpp': r'''
                        #include "./bar.hpp"
                        #include <foo.hpp>
                        int bar::value() { return foo::value() * 2; }
                        ''',
                }
            }
        },
    )
    # yapf: enable
    tmp_crs_repo.import_dir(libs / 'foo')
    tmp_crs_repo.import_dir(libs / 'bar')

    bd_project.write(
        'CMakeLists.txt', r'''
        cmake_minimum_required(VERSION 3.0)
        project(TestProject)

        include(CTest)
        include(libraries.cmake)
        add_executable(app app.cpp)
        target_link_libraries(app PRIVATE bar::bar)
        add_test(app app)
        ''')
    bd_project.write(
        'app.cpp', r'''
        #include <bar.hpp>
        #include <iostream>
        #include <cassert>

        int main() {
            std::cout << "Bar value is " << bar::value() << '\n';
            assert(bar::value() == 84);
        }
        ''')

    bd_project.dds.build_deps(['bar@1.2.3', f'--cmake=libraries.cmake'], repos=[tmp_crs_repo.path])
    proc.check_run(['cmake', '-S', bd_project.root, '-B', bd_project.build_root])
    proc.check_run(['cmake', '--build', bd_project.build_root])
    proc.check_run(['cmake', '--build', bd_project.build_root, '--target', 'test'])
