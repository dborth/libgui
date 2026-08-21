#pragma once

#include "AudioDriver.h"
#include "VideoDriver.h"
#include "InputDriver.h"
#include "FileSystemDriver.h"

class AudioDriver;
class VideoDriver;
class InputDriver;
class FileSystemDriver;

class Platform
{
public:
    virtual ~Platform() = default;

    virtual void init() = 0;
    virtual void shutdown() = 0;

    virtual AudioDriver* getAudio() = 0;
    virtual VideoDriver* getVideo() = 0;
    virtual InputDriver* getInput() = 0;
    virtual FileSystemDriver* getFileSystem() = 0;
};

//! The globally accessible platform instance
extern Platform* platform;
