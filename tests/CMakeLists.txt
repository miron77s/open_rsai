cmake_minimum_required(VERSION 3.5)

install(DIRECTORY ./
       DESTINATION bin/tests/
       FILES_MATCHING PATTERN "*.sh")
