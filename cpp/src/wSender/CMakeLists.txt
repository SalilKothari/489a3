#wSender
set(
    WSENDER
    main.cpp
    wServer.cpp
)

add_executable(wSender ${WSENDER})

target_link_libraries(wSender PRIVATE cxxopts::cxxopts common spdlog::spdlog Boost::regex)

target_include_directories(wSender PRIVATE ${PROJECT_SOURCE_DIR}/common)