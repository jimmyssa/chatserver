#定义了一个SRC_LIST变量，包含了所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
add_executable(ChatServer ${SRC_LIST} ${DB_LIST})
#指定可执行文件所依赖的库
target_link_libraries(ChatServer pthread muduo_net muduo_base mysqlclient)