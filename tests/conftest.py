from typing import Any
from pathlib import Path

import pytest
from _pytest.config import Config as PyTestConfig

# Ensure the fixtures are registered with PyTest:
from bpt_ci.testing.fs import *  # pylint: disable=wildcard-import,unused-wildcard-import
from bpt_ci.testing.fixtures import *  # pylint: disable=wildcard-import,unused-wildcard-import
from bpt_ci.testing.http import *  # pylint: disable=wildcard-import,unused-wildcard-import
from bpt_ci.testing.repo import *  # pylint: disable=wildcard-import,unused-wildcard-import


def pytest_addoption(parser: Any) -> None:
    parser.addoption('--test-deps',
                     action='store_true',
                     default=False,
                     help='Run the exhaustive and intensive bpt-deps tests')
    parser.addoption('--bpt-exe', help='Path to the bpt executable under test', type=Path)
    parser.addoption('--git-exe', help='Path to the git executable', type=Path)


def pytest_configure(config: Any) -> None:
    config.addinivalue_line('markers', 'deps_test: Deps tests are slow. Enable with --test-deps')


def pytest_collection_modifyitems(config: PyTestConfig, items: Any) -> None:
    if config.getoption('--test-deps'):
        return
    for item in items:
        if 'deps_test' not in item.keywords:
            continue
        item.add_marker(
            pytest.mark.skip(
                reason='Exhaustive deps tests are slow and perform many Git clones. Use --test-deps to run them.'))
