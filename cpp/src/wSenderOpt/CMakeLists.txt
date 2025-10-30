#wSenderOpt
set(
    WSENDEROPT
    main.cpp
    wSenderOpt.cpp
)

add_executable(wSenderOpt ${WSENDEROPT})

target_link_libraries(wSenderOpt PRIVATE cxxopts::cxxopts common spdlog::spdlog Boost::regex)

target_include_directories(wSender PRIVATE ${PROJECT_SOURCE_DIR}/common)
