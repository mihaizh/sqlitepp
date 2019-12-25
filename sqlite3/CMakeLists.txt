add_library(sqlite3
	sqlite3.c
	sqlite3.h)

target_include_directories(sqlite3
	PUBLIC
		.)