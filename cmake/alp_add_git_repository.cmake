#############################################################################
# Alpine Terrain Renderer
# Copyright (C) 2023 Adam Celarek <family name at cg tuwien ac at>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#############################################################################

find_package(Git 2.22 REQUIRED)

# CMake's FetchContent caches information about the downloads / clones in the build dir.
# Therefore it walks over the clones every time we switch the build type (release, debug, webassembly, android etc),
# which takes forever. Moreover, it messes up changes to subprojects. This function, on the other hand, checks whether
# we are on a branch and in that case only issues a warning. Use origin/main or similar, if you want to stay up-to-date
# with upstream.

function(alp_add_git_repository name)
    set(options DO_NOT_ADD_SUBPROJECT)
    set(oneValueArgs URL COMMITISH DESTINATION_PATH)
    set(multiValueArgs )
    cmake_parse_arguments(PARSE_ARGV 1 PARAM "${options}" "${oneValueArgs}" "${multiValueArgs}")

    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/ThirdParty/extern )
    set(repo_dir ${CMAKE_SOURCE_DIR}/ThirdParty/extern/${name})
    set(short_repo_dir ThirdParty/extern/${name})
    if (DEFINED PARAM_DESTINATION_PATH AND NOT PARAM_DESTINATION_PATH STREQUAL "")
        set(repo_dir ${CMAKE_SOURCE_DIR}/${PARAM_DESTINATION_PATH})
        set(short_repo_dir ${PARAM_DESTINATION_PATH})
    endif()

    set(${name}_SOURCE_DIR "${repo_dir}" PARENT_SCOPE)

    if(EXISTS "${repo_dir}/.git")
        message(STATUS "Updating git repo in ${short_repo_dir}")
        execute_process(COMMAND ${GIT_EXECUTABLE} fetch
            WORKING_DIRECTORY ${repo_dir}
            RESULT_VARIABLE GIT_FETCH_RESULT)
        if (NOT ${GIT_FETCH_RESULT})
            message(STATUS "Fetching ${name} was successfull.")

            execute_process(COMMAND ${GIT_EXECUTABLE} branch --show-current
                WORKING_DIRECTORY ${repo_dir}
                RESULT_VARIABLE GIT_BRANCH_RESULT
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE GIT_BRANCH_OUTPUT)
            if (${GIT_BRANCH_RESULT})
                message(FATAL_ERROR "${repo_dir}: git branch --show-current not successfull")
            endif()
            if (GIT_BRANCH_OUTPUT STREQUAL "")
                execute_process(COMMAND ${GIT_EXECUTABLE} checkout --quiet ${PARAM_COMMITISH}
                    WORKING_DIRECTORY ${repo_dir}
                    RESULT_VARIABLE GIT_CHECKOUT_RESULT)
                if (NOT ${GIT_CHECKOUT_RESULT})
                    message(STATUS "In ${name}, checking out ${PARAM_COMMITISH} was successfull.")
                else()
                    message(FATAL_ERROR "In ${name}, checking out ${PARAM_COMMITISH} was NOT successfull!")
                endif()
            else()
                message(WARNING "${short_repo_dir} is on branch ${GIT_BRANCH_OUTPUT}, leaving it there. "
                    "NOT checking out ${PARAM_COMMITISH}! Use origin/main or similar if you want to stay up-to-date with upstream.")
            endif()
        else()
            message(WARNING "Fetching ${name} was NOT successfull!")
        endif()
    else()
        message(STATUS "Clonging ${PARAM_URL} to ${short_repo_dir}.")
        execute_process(COMMAND ${GIT_EXECUTABLE} clone --recurse-submodules ${PARAM_URL} ${repo_dir}
            RESULT_VARIABLE GIT_CLONE_RESULT)
        if (NOT ${GIT_CLONE_RESULT})
            execute_process(COMMAND ${GIT_EXECUTABLE} checkout --quiet ${PARAM_COMMITISH}
                WORKING_DIRECTORY ${repo_dir}
                RESULT_VARIABLE GIT_CHECKOUT_RESULT)
            if (NOT ${GIT_CHECKOUT_RESULT})
                message(STATUS "Checking out ${PARAM_COMMITISH} was successfull.")
            else()
                message(FATAL_ERROR "In ${name}, checking out ${PARAM_COMMITISH} was NOT successfull!")
            endif()
        else()
            message(FATAL_ERROR "Clonging ${name} was NOT successfull!")
        endif()
    endif()

    if (NOT ${PARAM_DO_NOT_ADD_SUBPROJECT})
        add_subdirectory(${repo_dir} ${CMAKE_BINARY_DIR}/alp_external/${name})
    endif()
endfunction()
