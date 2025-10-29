#wSenderOpt
set(
    WRECEIVEROPT
    main.cpp
    wReceiverOpt.cpp
)

add_executable(wReceiverOpt ${WRECEIVEROPT})

target_link_libraries(wReceiverOpt PRIVATE cxxopts::cxxopts common spdlog::spdlog Boost::regex)

target_include_directories(wReceiverOpt PRIVATE ${PROJECT_SOURCE_DIR}/common)
