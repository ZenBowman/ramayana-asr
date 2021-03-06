#include "portaudio.h"
#include <iostream>
#include <vector>
#include <pocketsphinx.h>
#include <string>
#include <zmq.hpp>
#include <signal.h>

const std::string base_dir =
    "/home/psamtani/development/pocketsphinx-0.8/model";
const std::string hmm = base_dir + "/hmm/en_US/hub4wsj_sc_8k";
const std::string lm = "lm/ramayana.lm";
const std::string dict = "lm/ramayana.dic";

constexpr unsigned int bufferSize = 512;
constexpr unsigned int sampleRate = 16000;
constexpr unsigned int numChannels = 1;

short samples[bufferSize];

std::vector<short> history;

long numcalls;
bool recorded = false;
bool done = false;

struct AsrServiceData {
  ps_decoder_t *ps;
  zmq::socket_t *publisher;
};

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags, void *userData) {

  numcalls++;
  AsrServiceData *sdata = (AsrServiceData*)userData;
  ps_decoder_t *ps = sdata->ps;
  short max = 0;
  short thisval;

  short *data = (short *)inputBuffer;
  for (unsigned int i = 0; i < bufferSize; i++) {
    thisval = data[i];
    if (thisval > max) {
      max = thisval;
    }
  }

  history.push_back(max);
  if ((history.size() % 150) == 140) {
    char const *uttid;
    int32 score;
    int rv = ps_end_utt(ps);
    const char *hyp1 = ps_get_hyp(ps, &score, &uttid);
    int32 prob = ps_get_prob(ps, &uttid);
    std::cout << "Recognized: " << uttid << " = " << hyp1 << "(" << prob
              << ")" << std::endl;
    
    zmq::message_t response(strlen(hyp1));
    memcpy((void *) response.data(), hyp1, strlen(hyp1));
    sdata->publisher->send(response);
    std::cout << "Send message via zeromq" << std::endl;
    
    return paContinue;
  }

  if ((history.size() % 150) == 1) {
    ps_start_utt(ps, "foo");
  }

  if (((history.size() % 150) > 1) && ((history.size() % 150) < 140)){
    ps_process_raw(ps, data, 512, FALSE, FALSE);
  }

  std::cout << max << " ";
  return paContinue;
}

 ps_decoder_t *initializePocketsphinx() {
  cmd_ln_t *config =
      cmd_ln_init(NULL, ps_args(), TRUE, "-hmm", hmm.c_str(), "-lm", lm.c_str(),
                  "-dict", dict.c_str(), NULL);
  if (config == NULL) {
    return nullptr;
  }

  ps_decoder_t *ps = ps_init(config);
  return ps;
}

void finalizePocketsphinx(ps_decoder_t *ps) { ps_free(ps); }

PaStream *initializeRecordingStream(void *userData) {
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    std::cout << "Unable to initialize portaudio";
    Pa_Terminate();
    return nullptr;
  }

  PaStreamParameters inputParameters;
  inputParameters.device = Pa_GetDefaultInputDevice();
  if (inputParameters.device == paNoDevice) {
    std::cout << "Unable to capture input device";
    Pa_Terminate();
    return nullptr;
  }

  inputParameters.channelCount = 1;
  inputParameters.sampleFormat = paInt16;
  inputParameters.suggestedLatency =
      Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = nullptr;

  PaStream *stream;
  err = Pa_OpenStream(&stream, &inputParameters, nullptr, sampleRate,
                      bufferSize, paClipOff, recordCallback, userData);

  if (err != paNoError) {
    std::cout << "Unable to open stream";
    Pa_Terminate();
    return nullptr;
  }

  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cout << "Unable to start stream";
    Pa_Terminate();
    return nullptr;
  }

  return stream;
}

void finalizeRecordingStream(PaStream *stream) {
  Pa_CloseStream(stream);
  Pa_Terminate();
}

void term(int signum) {
  std::cout << "Received SIGTERM, exiting" << std::endl;
  done = true;
}

int main(int argc, char *argv[]) {
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGINT, &action, NULL);

  char const *hyp, *uttid;
  int32 score;
  PaError err;
  AsrServiceData asrServiceData;

  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  publisher.bind("ipc://asr.ipc");
  publisher.bind("tcp://*:5556");

  asrServiceData.ps = initializePocketsphinx();
  asrServiceData.publisher = &publisher;

  if (asrServiceData.ps == nullptr){
    return 1;
  }

  PaStream *stream = initializeRecordingStream(&asrServiceData); 
  if (stream == nullptr) {
    return 1;
  }

  std::cout << "Recording started";
  std::cout.flush();

  while (((err = Pa_IsStreamActive(stream)) == 1) && !done) {
    Pa_Sleep(1000);
  }
  
  finalizeRecordingStream(stream);
  finalizePocketsphinx(asrServiceData.ps);

  return 0;
}
