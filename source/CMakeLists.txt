add_executable(pen_file)

set(LOCAL_FILES
    main.c
    server.c
    server.h
)
target_sources(pen_file PRIVATE ${LOCAL_FILES})

install(TARGETS pen_file)

