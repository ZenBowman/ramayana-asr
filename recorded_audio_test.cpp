#include <pocketsphinx.h>
#include <string>

const std::string base_dir = "/home/psamtani/development/pocketsphinx-0.8/model";
const std::string hmm = base_dir + "/hmm/en_US/hub4wsj_sc_8k";
const std::string lm = base_dir + "/lm/en/turtle.DMP";
const std::string dict = base_dir + "/lm/en/turtle.dic";

constexpr int sampleRate = 16000;
constexpr int framesPerBuffer = 512;


int main(int argc, char *argv[]) {
  ps_decoder_t *ps;
  
  cmd_ln_t *config;
  FILE *fh;
  char const *hyp, *uttid;
  int rv;
  int32 score;
  
  config = cmd_ln_init(NULL, ps_args(), TRUE,
		       "-hmm", hmm.c_str(),
		       "-lm", lm.c_str(),
		       "-dict", dict.c_str(),
		       NULL);
  if (config == NULL) {
    return 1;
  }
  ps = ps_init(config);
  if (ps == NULL) {
    return 1;
  }
  
  fh = fopen("goforward.raw", "rb");
  if (fh == NULL) {
    perror("Failed to open goforward.raw");
    return 1;
  }

  rv = ps_decode_raw(ps, fh, "goforward", -1);
  if (rv < 0) {
    return 1;
  }
  hyp = ps_get_hyp(ps, &score, &uttid);
  if (hyp == NULL) {
    return 1;
  }
  printf("Recognized: %s\n", hyp);
  return 0;
}
