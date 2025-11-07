#add_libaray(rv1126b_config INTERFACE)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
add_compile_options(-fopenmp -ftree-vectorize -mfpu=neon -mfloat-abi=hard -funroll-loops)
add_link_options(-fopenmp -ftree-vectorize -mfpu=neon -mfloat-abi=hard -funroll-loops)

