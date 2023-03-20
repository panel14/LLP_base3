cmake_minimum_required(VERSION 3.16)
project(LLP3 C)

set(CMAKE_C_STANDARD 99)

include_directories(analyzer)

include_directories(pblib)
include_directories(pblib/nanopb)

add_executable(Client
        pblib/message.pb.c
        pblib/nanopb/pb_common.c
        pblib/nanopb/pb_decode.c
        pblib/nanopb/pb_encode.c
        analyzer/lex.yy.c
        analyzer/y.tab.c
        analyzer/common.c analyzer/include/common.h)

add_executable(Server server.c
        analyzer/common.c
        pblib/message.pb.c
        pblib/nanopb/pb_decode.c
        pblib/nanopb/pb_encode.c
        storage/OSFileAPI/osFile.c
        storage/DataFileAPI/dataFile.c
        storage/StorageFileInnerAPI/storageBaseApi.c
        storage/StorageFileInnerAPI/storageFileInner.c
        storage/Utils/initializer.c
        storage/Utils/parser.c
        storage/Utils/sstring.c
        storage/StorageFilePublicAPI/storageFilePublic.c
        storage/handler.c)