echo off

protoc --cpp_out=.. controller.proto byte_dict.proto pose.proto devices.proto message.proto