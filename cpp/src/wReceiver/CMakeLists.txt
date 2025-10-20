#wSender
set(
    WRECEIVER
    main.cpp
    wReceiver.cpp
)

add_executable(wReceiver ${WRECEIVER})

target_link_libraries(wReceiver PRIVATE cxxopts::cxxopts common spdlog::spdlog Boost::regex)

target_include_directories(wReceiver PRIVATE ${PROJECT_SOURCE_DIR}/common)
