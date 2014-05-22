cmake_minimum_required(VERSION 2.8)

SET (CMAKE_C_COMPILER  "clang")
SET (CMAKE_CXX_COMPILER  "clang++")

project(RecordedAudioTest)
add_definitions(-std=c++11)

include_directories(
  /usr/include/sphinxbase
  /usr/include/pocketsphinx
)

add_executable( 
  RecordedAudioTest
  recorded_audio_test.cpp 
)

target_link_libraries(
  RecordedAudioTest
  pocketsphinx
  sphinxbase
  sphinxad
)

project(PortAudioRecordTest)
add_definitions(-std=c++11)

include_directories(
  /usr/include/sphinxbase
  /usr/include/pocketsphinx
)

add_executable(
  PortAudioRecordTest
  portaudio_recorder.cpp
)

target_link_libraries(
  PortAudioRecordTest
  portaudio
  pocketsphinx
  sphinxbase
  sphinxad
  zmq
)