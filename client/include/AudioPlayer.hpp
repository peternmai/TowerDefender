#pragma once
#include <AL/al.h>
#include <AL/alc.h>

#include <vector>
#include <array>
#include <unordered_map>
#include <string>

#define BUFFER_SIZE 4096

class AudioPlayer
{
private:

    struct WaveFileData {
        uint8_t         riffHeader[4];
        uint32_t        chunkSize;
        uint8_t         waveHeader[4];

        uint8_t         fmtHeader[4];
        uint32_t        subchunk1Size;
        uint16_t        audioFormat;
        uint16_t        numOfChannel;
        uint32_t        samplingFrequency;
        uint32_t        bytesPerSec;
        uint16_t        blockAlign;
        uint16_t        bitsPerSample;

        uint8_t         dataString[4];
        uint32_t        dataSize;
    };

    struct AudioFileData {
        std::string filename;
        std::vector<uint8_t> data;
        ALenum audioFormat;
        ALsizei audioSize;
        ALsizei audioFreq;
    };

    struct AudioLibraryData {
        ALuint audioSourceID;
        ALuint audioBufferID;
    };

    ALCdevice * audioDevice;
    ALCcontext * audioContext;
    std::unordered_map<std::string, AudioLibraryData> audioLibrary;

    AudioFileData loadWAVFile(const std::string & filename);

public:
    AudioPlayer();
    ~AudioPlayer();

    bool loadAudioFile(const std::string & filename);
    bool unloadAudioFile(const std::string & filename);
    bool play(const std::string & filename, bool loop = false);
    bool pause(const std::string & filename);
    bool stop(const std::string & filename);
};