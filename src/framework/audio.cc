
#include <optional>

#include <AL/al.h>
#include <AL/alc.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <stb/stb_vorbis.h>

#include <framework/audio.h>
#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fw {

namespace {

namespace fs = boost::filesystem;

// Strings from alcGetString come as an array of nul-terminated string where the last string is empty. So something
// like "foo\0bar\0baz\0\0". This will split those strings into a vector of strings.
std::vector<std::string> split_alc_string(const ALCchar* str) {
  std::vector<std::string> strings;
  if (str == nullptr) {
    return strings;
  }

  const char* ptr = str;
  while (strlen(ptr) > 0) {
    strings.push_back(std::string(ptr));
    ptr += strlen(ptr) + 1;
  }
  return strings;
}

std::string describe_error(ALenum error) {
  switch (error) {
  case AL_NO_ERROR:
    return "AL_NO_ERROR";
  case AL_INVALID_NAME:
    return "AL_INVALID_NAME";
  case AL_INVALID_ENUM:
    return "AL_INVALID_ENUM";
  case AL_INVALID_VALUE:
    return "AL_INVALID_VALUE";
  case AL_INVALID_OPERATION:
    return "AL_INVALID_OPERATION";
  case AL_OUT_OF_MEMORY:
    return "AL_OUT_OF_MEMORY";
  default:
    return (boost::format("UNKNOWN:%1%") % error).str();
  }
}

struct PcmData {
  std::string filename;
  int channels = 0;
  int sample_rate = 0;
  int samples = 0;
  int16_t* pcm_data = nullptr;

  PcmData() = default;
  PcmData(const PcmData&) = delete;
  PcmData& operator=(const PcmData&) = delete;

  ~PcmData() {
    if (pcm_data != nullptr) {
      free(pcm_data);
    }
  }
};

std::optional<std::unique_ptr<PcmData>> decode_ogg(const fs::path& path) {
  auto pcm_data = std::make_unique<PcmData>();
  pcm_data->filename = path.string();

  pcm_data->samples =
    stb_vorbis_decode_filename(
      pcm_data->filename.c_str(), &pcm_data->channels, &pcm_data->sample_rate, &pcm_data->pcm_data);
  if (pcm_data->samples < 0) {
    fw::debug << " ERROR decoding file: " << path << std::endl;
  }

  return std::move(pcm_data);
}

}  // namespace

void AudioManager::initialize() {
  fw::debug << "OpenAL devices:" << std::endl;
  for (std::string device : split_alc_string(alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER))) {
    fw::debug << "  " << device << std::endl;
  }
  fw::debug << "default device: " << alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER) << std::endl;

  // TODO: allow the user to choose the output device (and change it at runtime).
  device_ = alcOpenDevice(nullptr);
  if (device_ == nullptr) {
    fw::debug << " ERROR initializing audio subsystem: " << describe_error(alGetError()) << std::endl;
    return;
  }

  context_ = alcCreateContext(device_, nullptr);
  if (context_ == nullptr) {
    fw::debug << " ERROR creating context: " << describe_error(alGetError()) << std::endl;
    alcCloseDevice(device_);
    device_ = nullptr;
    return;
  }

  alcMakeContextCurrent(context_);

  auto version = alGetString(AL_VERSION);
  fw::debug << "OpenAL version: " << version << std::endl;
}

void AudioManager::destroy() {
  if (device_ != nullptr) {
    alcCloseDevice(device_);
    device_ = nullptr;
  }
}

void AudioManager::update(float dt) {
  // first, remove any dead audio sources
  audio_sources_.erase(
    std::remove_if(
      audio_sources_.begin(),
      audio_sources_.end(),
      [](const std::weak_ptr<AudioSource>& s) {
        // If the underlying shared_ptr is gone, we remove it.
        return s.expired();
      }), audio_sources_.end());

  // Now update them all.
  for (auto s : audio_sources_) {
    if (auto source = s.lock()) {
      source->update(dt);
    }
  }
}

std::shared_ptr<AudioBuffer> AudioManager::get_audio_buffer(std::string const &name) {
  auto it = loaded_buffers_.find(name);
  if (it != loaded_buffers_.end()) {
    return it->second;
  }

  auto buffer = std::make_shared<AudioBuffer>(this, name);
  loaded_buffers_[name] = buffer;
  return buffer;
}

std::shared_ptr<AudioSource> AudioManager::create_audio_source() {
  std::shared_ptr<AudioSource> audio_source = std::make_shared<AudioSource>(*this);
  audio_sources_.push_back(audio_source);
  return audio_source;
}

//-----------------------------------------------------------------------------

AudioBuffer::AudioBuffer(AudioManager *mgr, std::string const &name) {
  fs::path path = fw::resolve(name);
  auto pcm_data = decode_ogg(path);
  if (!pcm_data) {
    fw::debug << "error loading sound " << path << std::endl;
  }

  alGenBuffers(1, &id_);
  int err = alGetError();
  if (err != AL_NO_ERROR) {
    fw::debug << "error creating buffer " << path << ": " << describe_error(err);
    return;
  }

  int format;
  if ((*pcm_data)->channels == 2) {
    format = AL_FORMAT_STEREO16;
  } else if ((*pcm_data)->channels == 1) {
    format = AL_FORMAT_MONO16;
  } else {
    fw::debug << "error in " << path << ": unexpected number of channels: " << (*pcm_data)->channels << std::endl;
    alDeleteBuffers(1, &id_);
    id_ = 0;
    return;
  }

  int buffer_size = (*pcm_data)->samples * (*pcm_data)->channels * sizeof(uint16_t);

  alBufferData(id_, format, (*pcm_data)->pcm_data, buffer_size, (*pcm_data)->sample_rate);
  err = alGetError();
  if (err != AL_NO_ERROR) {
    fw::debug << "error uploading OpenAL buffer " << path << ": " << describe_error(err);
    alDeleteBuffers(1, &id_);
    id_ = 0;
    return;
  }

  fw::debug << "loaded sound: " << path << " " << static_cast<float>((*pcm_data)->sample_rate) / 1000.0f << "kHz, "
    << (*pcm_data)->channels << " channels, "
    << static_cast<float>((*pcm_data)->samples) / (*pcm_data)->sample_rate << " seconds" << std::endl;
}

AudioBuffer::~AudioBuffer() {
  if (id_ > 0) {
    alDeleteBuffers(1, &id_);
  }
}

//-----------------------------------------------------------------------------
AudioSource::AudioSource(const AudioManager& manager)
  : manager_(manager) {
}

AudioSource::~AudioSource() {
  // TODO: free the play states.
}

// Called regularly by the AudioManager to update playback.
void AudioSource::update(float dt) {
  // TODO: remove play states if there's a bunch free/unused. 
}

AudioSource::PlayState& AudioSource::find_free_playstate() {
  for (auto& state : playing_sounds_) {
    int playing_state = 0;
    alGetSourcei(state.id, AL_SOURCE_STATE, &playing_state);
    if (playing_state != AL_PLAYING) {
      return state;
    }
  }

  // Need to create a new source.
  PlayState& state = *playing_sounds_.insert(playing_sounds_.end(), PlayState());
  alGenSources(1, &state.id);
  int err = alGetError();
  if (err != AL_NO_ERROR) {
    fw::debug << "error generating OpenAL source: " << describe_error(err);
    return state; // TODO: what to do about errors?
  }

  return state;
}


bool AudioSource::play(std::shared_ptr<AudioBuffer> audio) {
  PlayState& state = find_free_playstate();

  alSourcei(state.id, AL_BUFFER, audio->id_);
  int err = alGetError();
  if (err != AL_NO_ERROR) {
    fw::debug << "error playing sound: " << describe_error(err);
    return false; // TODO: what to do about errors?
  }

  alSourcePlay(state.id);
  err = alGetError();
  if (err != AL_NO_ERROR) {
    fw::debug << "error playing sound: " << describe_error(err);
    return false; // TODO: what to do about errors?
  }

  return true;
}

//-----------------------------------------------------------------------------
std::string to_string(audio_error_info const &err_info) {
  std::string msg = err_info.value();
  return msg;
}

}
