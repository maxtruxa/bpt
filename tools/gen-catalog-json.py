import json
from typing import NamedTuple, Tuple, List, Sequence, Union, Optional, Mapping
import sys
import textwrap


class Git(NamedTuple):
    url: str
    ref: str
    auto_lib: Optional[str] = None

    def to_dict(self) -> dict:
        d = {
            'url': self.url,
            'ref': self.ref,
        }
        if self.auto_lib:
            d['auto-lib'] = self.auto_lib
        return d


RemoteInfo = Union[Git]


class Version(NamedTuple):
    version: str
    remote: RemoteInfo
    depends: Mapping[str, str] = {}
    description: str = '(No description provided)'

    def to_dict(self) -> dict:
        ret: dict = {
            'description': self.description,
        }
        ret['depends'] = self.depends
        if isinstance(self.remote, Git):
            ret['git'] = self.remote.to_dict()
        return ret


class Package(NamedTuple):
    name: str
    versions: List[Version]


def many_versions(name: str,
                  versions: Sequence[str],
                  *,
                  tag_fmt: str = '{}',
                  git_url: str,
                  auto_lib: str = None,
                  description='(No description was provided)') -> Package:
    return Package(name, [
        Version(
            ver,
            description='\n'.join(textwrap.wrap(description)),
            remote=Git(
                url=git_url, ref=tag_fmt.format(ver), auto_lib=auto_lib))
        for ver in versions
    ])


packages = [
    many_versions(
        'range-v3',
        (
            '0.5.0',
            '0.9.0',
            '0.9.1',
            '0.10.0',
        ),
        git_url='https://github.com/ericniebler/range-v3.git',
        auto_lib='range-v3/range-v3',
        description=
        'Range library for C++14/17/20, basis for C++20\'s std::ranges',
    ),
    many_versions(
        'nlohmann-json',
        (
            '3.0.0',
            '3.0.1',
            '3.1.0',
            '3.1.1',
            '3.1.2',
            '3.2.0',
            '3.3.0',
            '3.4.0',
            '3.5.0',
            '3.6.0',
            '3.6.1',
            '3.7.0',
            '3.7.1',
            '3.7.2',
            '3.7.3',
        ),
        git_url='https://github.com/vector-of-bool/json.git',
        tag_fmt='dds/{}',
        description='JSON for Modern C++',
    ),
    Package('ms-wil', [
        Version(
            '2020.03.16',
            description='The Windows Implementation Library',
            remote=Git('https://github.com/vector-of-bool/wil.git',
                       'dds/2020.03.16'))
    ]),
    many_versions(
        'neo-sqlite3',
        (
            '0.1.0',
            '0.2.0',
            '0.2.1',
            '0.2.2',
            '0.2.3',
        ),
        description='A modern and low-level C++ SQLite API',
        git_url='https://github.com/vector-of-bool/neo-sqlite3.git',
    ),
    many_versions(
        'neo-fun',
        (
            '0.1.0',
            '0.1.1',
            '0.2.0',
            '0.2.1',
            '0.3.0',
            '0.3.1',
            '0.3.2',
        ),
        description='Some library fundamentals that you might find useful',
        git_url='https://github.com/vector-of-bool/neo-fun.git',
    ),
    many_versions(
        'neo-concepts',
        (
            '0.1.0',
            '0.2.0',
            '0.2.1',
            '0.2.2',
            '0.3.0',
            '0.3.1',
            '0.3.2',
        ),
        description=
        'Minimal C++ concepts library. Contains many definitions from C++20.',
        git_url='https://github.com/vector-of-bool/neo-concepts.git',
    ),
    Package('semver', [
        Version(
            '0.2.1',
            description=
            'A C++ library that implements Semantic Versioning parsing, emitting, '
            'types, ordering, and operations. See https://semver.org/',
            remote=Git('https://github.com/vector-of-bool/semver.git',
                       '0.2.1')),
        Version(
            '0.2.2',
            description=
            'A C++ library that implements Semantic Versioning parsing, emitting, '
            'types, ordering, and operations. See https://semver.org/',
            remote=Git('https://github.com/vector-of-bool/semver.git',
                       '0.2.2')),
    ]),
    many_versions(
        'pubgrub',
        (
            '0.1.2',
            '0.2.0',
            '0.2.1',
        ),
        description=
        'A C++ implementation of the Pubgrub version solving algorithm',
        git_url='https://github.com/vector-of-bool/pubgrub.git',
    ),
    many_versions(
        'vob-json5',
        ('0.1.5', ),
        description='A C++ implementation of a JSON5 parser',
        git_url='https://github.com/vector-of-bool/json5.git',
    ),
    Package('vob-semester', [
        Version(
            '0.1.0',
            description='A C++ library to process recursive dynamic data',
            remote=Git('https://github.com/vector-of-bool/semester.git',
                       '0.1.0'),
            depends={
                'neo-fun': '^0.1.0',
                'neo-concepts': '^0.2.1',
            }),
        Version(
            '0.1.1',
            description='A C++ library to process recursive dynamic data',
            remote=Git('https://github.com/vector-of-bool/semester.git',
                       '0.1.1'),
            depends={
                'neo-fun': '^0.1.1',
                'neo-concepts': '^0.2.2',
            }),
        Version(
            '0.2.0',
            description='A C++ library to process recursive dynamic data',
            remote=Git('https://github.com/vector-of-bool/semester.git',
                       '0.2.0'),
            depends={
                'neo-fun': '^0.3.2',
                'neo-concepts': '^0.3.2',
            }),
        Version(
            '0.2.1',
            description='A C++ library to process recursive dynamic data',
            remote=Git('https://github.com/vector-of-bool/semester.git',
                       '0.2.1'),
            depends={
                'neo-fun': '^0.3.2',
                'neo-concepts': '^0.3.2',
            }),
    ]),
    Package('ctre', [
        Version(
            '2.7.0',
            description=
            'A compile-time PCRE (almost) compatible regular expression matcher',
            remote=Git(
                'https://github.com/hanickadot/compile-time-regular-expressions.git',
                'v2.7',
                auto_lib='hanickadot/ctre',
            ))
    ]),
    many_versions(
        'spdlog',
        (
            '0.9.0',
            '0.10.0',
            '0.11.0',
            '0.12.0',
            '0.13.0',
            '0.14.0',
            '0.16.0',
            '0.16.1',
            '0.16.2',
            '0.17.0',
            '1.0.0',
            '1.1.0',
            '1.2.0',
            '1.2.1',
            '1.3.0',
            '1.3.1',
            '1.4.0',
            '1.4.1',
            '1.4.2',
        ),
        git_url='https://github.com/gabime/spdlog.git',
        tag_fmt='v{}',
        auto_lib='spdlog/spdlog',
        description='Fast C++ logging library',
    ),
    many_versions(
        'fmt',
        (
            '0.8.0',
            '0.9.0',
            '0.10.0',
            '0.12.0',
            '1.0.0',
            '1.1.0',
            '2.0.0',
            '2.0.1',
            '2.1.0',
            '2.1.1',
            '3.0.0',
            '3.0.1',
            '3.0.2',
            '4.0.0',
            '4.1.0',
            '5.0.0',
            '5.1.0',
            '5.2.0',
            '5.2.1',
            '5.3.0',
            '6.0.0',
            '6.1.0',
            '6.1.1',
            '6.1.2',
        ),
        git_url='https://github.com/fmtlib/fmt.git',
        auto_lib='fmt/fmt',
        description='A modern formatting library : https://fmt.dev/',
    ),
]

data = {
    'version': 1,
    'packages': {
        pkg.name: {ver.version: ver.to_dict()
                   for ver in pkg.versions}
        for pkg in packages
    }
}

print(json.dumps(data, indent=2, sort_keys=True))