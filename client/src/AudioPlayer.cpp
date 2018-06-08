#include "AudioPlayer.hpp"
#include <iostream>

AudioPlayer::AudioPlayer()
{
    // Open the preferred device
    this->audioDevice = alcOpenDevice(NULL);
    if (this->audioDevice == NULL)
        throw std::runtime_error("Unable to open audio device");

    // Create context for which we'll play audio from
    this->audioContext = alcCreateContext(this->audioDevice, NULL);
    if (alcMakeContextCurrent(this->audioContext) == NULL)
        throw std::runtime_error("Unable to create audio context");
}


AudioPlayer::~AudioPlayer()
{
    // Deallocate all source files
    for (auto libraryContent = this->audioLibrary.begin(); libraryContent != this->audioLibrary.end(); libraryContent++) {
        alDeleteSources(1, &libraryContent->second.audioSourceID);
        alDeleteBuffers(1, &libraryContent->second.audioBufferID);
    }

    // Destroy the audio session
    alcMakeContextCurrent(NULL);
    alcDestroyContext(this->audioContext);
    alcCloseDevice(this->audioDevice);
}

AudioPlayer::AudioFileData AudioPlayer::loadWAVFile(const std::string & filename)
{
    // Attempt to open audio file for read
    FILE * audioFile = NULL;
    if (fopen_s(&audioFile, filename.c_str(), "rb") != 0)
        throw std::runtime_error("Unable to open audio file");

    // Read in the header file
    WaveFileData waveFileData;
    size_t bytesRead = fread(&waveFileData, 1, sizeof(WaveFileData), audioFile);
    if (bytesRead != sizeof(WaveFileData))
        throw std::runtime_error("Error parsing WAV header");

    // Read in the audio data
    std::vector<uint8_t> data;
    std::array<uint8_t, BUFFER_SIZE> buffer;
    while ((bytesRead = fread(&buffer[0], sizeof(buffer[0]), BUFFER_SIZE / sizeof(buffer[0]), audioFile)) > 0)
        for (int byteIndex = 0; byteIndex < bytesRead; byteIndex++)
            data.push_back(buffer[byteIndex]);
    fclose(audioFile);

    // Check if we read in all the data
    if (data.size() != waveFileData.dataSize)
        throw std::runtime_error("Failed to read in entire WAV file");

    // Store the requested data
    AudioPlayer::AudioFileData audioFileData;
    audioFileData.filename = filename;
    audioFileData.data = data;
    audioFileData.audioSize = waveFileData.dataSize;
    audioFileData.audioFreq = waveFileData.samplingFrequency;
    bool stereoAudio = (waveFileData.numOfChannel > 1);
    if (waveFileData.bitsPerSample == 16)
        audioFileData.audioFormat = (stereoAudio) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    else if (waveFileData.bitsPerSample == 8)
        audioFileData.audioFormat = (stereoAudio) ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
    else
        audioFileData.audioFormat = -1;

    return audioFileData;
}

bool AudioPlayer::loadAudioFile(const std::string & filename)
{
    // Check to see if file was already loaded
    if (this->audioLibrary.find(filename) != this->audioLibrary.end())
        return true;

    // Attempt to load in the audio file
    // TODO: Ability to load more than just WAV files
    std::cout << "Loading in \"" << filename << "\"... ";
    AudioPlayer::AudioFileData audioFileData;
    try {
        audioFileData = this->loadWAVFile(filename);
    } catch (const std::exception) { 
        std::cerr << "FAILED - Reading Audio File" << std::endl;
        return false;
    }

    // Generate a buffer for the audio data
    ALuint audioBufferID;
    alGenBuffers(1, &audioBufferID);
    if (alGetError() != AL_NO_ERROR) {
        std::cerr << "FAILED - Audio Buffer Generation" << std::endl;
        return false;
    }

    // Store audio file into buffer
    alBufferData(audioBufferID, audioFileData.audioFormat, &audioFileData.data[0],
        audioFileData.audioSize, audioFileData.audioFreq);
    if (alGetError() != AL_NO_ERROR) {
        alDeleteBuffers(1, &audioBufferID);
        std::cerr << "FAILED - Storing Audio Buffer" << std::endl;
        return false;
    }
    
    // Generate source
    ALuint audioSourceID;
    alGenSources(1, &audioSourceID);
    if (alGetError() != AL_NO_ERROR) {
        alDeleteBuffers(1, &audioBufferID);
        std::cerr << "FAILED - Audio Source Generation" << std::endl;
        return false;
    }

    // Set the source
    alSourcei(audioSourceID, AL_BUFFER, audioBufferID);
    if (alGetError() != AL_NO_ERROR) {
        alDeleteBuffers(1, &audioBufferID);
        alDeleteSources(1, &audioSourceID);
        std::cerr << "FAILED - Setting Audio Source" << std::endl;
        return false;
    }

    this->audioLibrary[filename] = { audioSourceID, audioBufferID };
    std::cout << "Successful" << std::endl;
    return true;
}


bool AudioPlayer::unloadAudioFile(const std::string & filename)
{
    if (this->audioLibrary.find(filename) != this->audioLibrary.end()) {
        alSourceStop(this->audioLibrary[filename].audioSourceID);
        alDeleteBuffers(1, &this->audioLibrary[filename].audioBufferID);
        alDeleteSources(1, &this->audioLibrary[filename].audioSourceID);
        this->audioLibrary.erase(filename);
    }
}

bool AudioPlayer::play(const std::string & filename, bool loop){
    if (this->audioLibrary.find(filename) != this->audioLibrary.end()) {
        alSourcei(this->audioLibrary[filename].audioSourceID, AL_LOOPING, ((loop) ? 1 : 0));
        alSourcePlay(this->audioLibrary[filename].audioSourceID);
        return true;
    }
    return false;
}

bool AudioPlayer::pause(const std::string & filename) {
    if (this->audioLibrary.find(filename) != this->audioLibrary.end()) {
        alSourcePause(this->audioLibrary[filename].audioSourceID);
        return true;
    }
    return false;
}

bool AudioPlayer::stop(const std::string & filename) {
    if (this->audioLibrary.find(filename) != this->audioLibrary.end()) {
        alSourceStop(this->audioLibrary[filename].audioSourceID);
        return true;
    }
    return false;
}
