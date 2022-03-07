# - Find INIReader
# Find the native INIReader includes and library
#
#  INIREADER_INCLUDES    - where to find fftw3.h
#  INIREADER_LIBRARIES   - List of libraries when using FFTW.
#  INIREADER_FOUND       - True if INIReader found.

if (INIREADER_INCLUDES)
  # Already in cache, be silent
  set (INIREADER_FIND_QUIETLY TRUE)
endif (INIREADER_INCLUDES)

find_path (INIREADER_INCLUDES INIReader.h)

find_library (INIREADER_LIBRARIES NAMES inih)

# handle the QUIETLY and REQUIRED arguments and set INIREADER_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (INIReader DEFAULT_MSG INIREADER_LIBRARIES INIREADER_INCLUDES)

mark_as_advanced (INIREADER_LIBRARIES INIREADER_INCLUDES)
