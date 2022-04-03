import time

from bpt_ci.testing import Project, error


def test_compile_file_missing(tmp_project: Project) -> None:
    with error.expect_error_marker('nonesuch-compile-file'):
        tmp_project.compile_file('src/answer.cpp')


def test_simple_compile_file(tmp_project: Project) -> None:
    """
    Check that changing a source file will update the resulting application.
    """
    tmp_project.write('src/answer.cpp', 'int get_answer() { return 42; }')
    # No error:
    tmp_project.compile_file('src/answer.cpp')
    # Fail:
    time.sleep(1)  # Sleep long enough to register a file change
    tmp_project.write('src/answer.cpp', 'int get_answer() { return "How many roads must a man walk down?"; }')
    with error.expect_error_marker('compile-file-failed'):
        tmp_project.compile_file('src/answer.cpp')
