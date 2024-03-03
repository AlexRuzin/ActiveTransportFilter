#pragma once

#include <Windows.h>

#include "unpacker.h"

#include <string>

//
// Get the install path
//
int32_t GetTemporaryFilePath(std::string &out);

//
// Extract resource to target path
//
int32_t ExtractResourceToPath(int resourceId, const std::string &resourceName, const std::string &absPath);
