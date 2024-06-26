# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/swyang/Redis/redis-similarity/src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/swyang/Redis/redis-similarity/src/build

# Include any dependencies generated for this target.
include CMakeFiles/mros.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/mros.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/mros.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mros.dir/flags.make

CMakeFiles/mros.dir/mros/mros.c.o: CMakeFiles/mros.dir/flags.make
CMakeFiles/mros.dir/mros/mros.c.o: ../mros/mros.c
CMakeFiles/mros.dir/mros/mros.c.o: CMakeFiles/mros.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/swyang/Redis/redis-similarity/src/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/mros.dir/mros/mros.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/mros.dir/mros/mros.c.o -MF CMakeFiles/mros.dir/mros/mros.c.o.d -o CMakeFiles/mros.dir/mros/mros.c.o -c /home/swyang/Redis/redis-similarity/src/mros/mros.c

CMakeFiles/mros.dir/mros/mros.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mros.dir/mros/mros.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/swyang/Redis/redis-similarity/src/mros/mros.c > CMakeFiles/mros.dir/mros/mros.c.i

CMakeFiles/mros.dir/mros/mros.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mros.dir/mros/mros.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/swyang/Redis/redis-similarity/src/mros/mros.c -o CMakeFiles/mros.dir/mros/mros.c.s

CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o: CMakeFiles/mros.dir/flags.make
CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o: ../murmur2/MurmurHash2.c
CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o: CMakeFiles/mros.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/swyang/Redis/redis-similarity/src/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o -MF CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o.d -o CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o -c /home/swyang/Redis/redis-similarity/src/murmur2/MurmurHash2.c

CMakeFiles/mros.dir/murmur2/MurmurHash2.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mros.dir/murmur2/MurmurHash2.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/swyang/Redis/redis-similarity/src/murmur2/MurmurHash2.c > CMakeFiles/mros.dir/murmur2/MurmurHash2.c.i

CMakeFiles/mros.dir/murmur2/MurmurHash2.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mros.dir/murmur2/MurmurHash2.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/swyang/Redis/redis-similarity/src/murmur2/MurmurHash2.c -o CMakeFiles/mros.dir/murmur2/MurmurHash2.c.s

# Object files for target mros
mros_OBJECTS = \
"CMakeFiles/mros.dir/mros/mros.c.o" \
"CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o"

# External object files for target mros
mros_EXTERNAL_OBJECTS =

libmros.a: CMakeFiles/mros.dir/mros/mros.c.o
libmros.a: CMakeFiles/mros.dir/murmur2/MurmurHash2.c.o
libmros.a: CMakeFiles/mros.dir/build.make
libmros.a: CMakeFiles/mros.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/swyang/Redis/redis-similarity/src/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C static library libmros.a"
	$(CMAKE_COMMAND) -P CMakeFiles/mros.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mros.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mros.dir/build: libmros.a
.PHONY : CMakeFiles/mros.dir/build

CMakeFiles/mros.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mros.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mros.dir/clean

CMakeFiles/mros.dir/depend:
	cd /home/swyang/Redis/redis-similarity/src/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/swyang/Redis/redis-similarity/src /home/swyang/Redis/redis-similarity/src /home/swyang/Redis/redis-similarity/src/build /home/swyang/Redis/redis-similarity/src/build /home/swyang/Redis/redis-similarity/src/build/CMakeFiles/mros.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mros.dir/depend

