FILE(GLOB SOURCE_1 "${PROJECT_ROOT_PATH}/src/*.cpp")
FILE(GLOB SOURCE_2 "${PROJECT_ROOT_PATH}/thirdparty/src/*.cpp")
ADD_LIBRARY(nemodel STATIC ${SOURCE_1} ${SOURCE_2} )
TARGET_LINK_LIBRARIES(nemodel ictlap_core_ne  litecrf stdc++46 pthread glog)
