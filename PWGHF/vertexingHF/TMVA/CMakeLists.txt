# **************************************************************************
# * Copyright(c) 1998-2014, ALICE Experiment at CERN, All rights reserved. *
# *                                                                        *
# * Author: The ALICE Off-line Project.                                    *
# * Contributors are mentioned in the code where appropriate.              *
# *                                                                        *
# * Permission to use, copy, modify and distribute this software and its   *
# * documentation strictly for non-commercial purposes is hereby granted   *
# * without fee, provided that the above copyright notice appears in all   *
# * copies and that both the copyright notice and this permission notice   *
# * appear in the supporting documentation. The authors make no claims     *
# * about the suitability of this software for any purpose. It is          *
# * provided "as is" without express or implied warranty.                  *
# **************************************************************************/

#Module
set(MODULETMVA vertexingHFTMVA)
add_definitions(-D_MODULETMVA_="${MODULETMVA}")

# Module include folder
include_directories(${AliPhysics_SOURCE_DIR}/PWGHF/vertexingHF/TMVA
                    ${AliPhysics_SOURCE_DIR}/PWGHF/vertexingHF)


# Sources - alphabetical order
set(SRCS
  )

set(HDRS
  LHC16i2_TMVAClassification_BDT_BDT_Default_6_12.class.C
  LHC16i2_TMVAClassification_BDT_BDT_Default_4_6.class.C
  BDT_DefaultNode.C
  )



# Generate the dictionary
# It will create G_ARG1.cxx and G_ARG1.h / ARG1 = function first argument
get_directory_property(incdirs INCLUDE_DIRECTORIES)
generate_dictionary("${MODULETMVA}" "${MODULETMVA}LinkDef.h" "${HDRS}" "${incdirs}")

# Add a shared library
add_library_tested(${MODULETMVA} SHARED  ${SRCS} G__${MODULETMVA}.cxx)

# Generate the ROOT map
# Dependecies
generate_rootmap("${MODULETMVA}" "" "${CMAKE_CURRENT_SOURCE_DIR}/${MODULETMVA}LinkDef.h")

# Linking the library
target_link_libraries(${MODULETMVA})

# Public include folders that will be propagated to the dependecies
target_include_directories(${MODULETMVA} PUBLIC ${incdirs})

# System dependent: Modify the way the library is build
if(${CMAKE_SYSTEM} MATCHES Darwin)
    set_target_properties(${MODULETMVA} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(${CMAKE_SYSTEM} MATCHES Darwin)

# Installation
install(TARGETS ${MODULETMVA}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)


