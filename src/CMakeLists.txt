add_library(${PROJECT_NAME}
	../include/sqlite3_inc.h
	../include/sqlitepp.h
	../include/sqlitepp_db.h
	../include/sqlitepp_stmt.h
	sqlitepp_db.cpp
	sqlitepp_stmt.cpp)
	
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		sqlite3)
		
target_include_directories(${PROJECT_NAME}
	PUBLIC
		../include)
