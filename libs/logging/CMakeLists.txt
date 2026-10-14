add_library(logging STATIC
    src/log.c)

target_include_directories(logging PUBLIC
    include)
# SDK здесь намеренно не подключается!