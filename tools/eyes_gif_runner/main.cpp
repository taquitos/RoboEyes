#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>

#include "eyes/animator.h"
#include "eyes/emotion.h"
#include "eyes/gif/file_byte_sink.h"
#include "eyes/gif/gif_frame_sink.h"
#include "eyes/player.h"
#include "eyes/renderer.h"
#include "eyes/sequencer.h"

namespace {

struct Args {
  enum class Scenario {
    None = 0,
    HappinessHalfToFallingAsleepSnoringWakingUpCuriosity = 1,
    PlayfulPongToDizzy = 2,
    AllEmotionsInterruptSweep = 3,
    AllEmotionsRandomTransitions = 4,
    Curiosity2sPlayfulPong10sFallingAsleep = 5,
  };

  std::string out_dir = "out/eyes_gifs";
  bool all = true;
  eyes::Emotion emotion = eyes::Emotion::Neutral;
  Scenario scenario = Scenario::None;
  int fps = 30;
  int width = 280;
  int height = 240;
  uint32_t transition_ms = 420;
  uint32_t duration_ms = 0;  // 0 uses clip default
  uint32_t variation_seed = 0; // 0 = deterministic default
};

static void usage(const char *argv0) {
  printf("Usage: %s [--out <dir>] [--emotion <name|all>] [--scenario <name>] [--fps <n>] "
         "[--width <n>] [--height <n>] [--duration-ms <n>] [--transition-ms <n>] "
         "[--variation-seed <u32>]\n",
         argv0);
  printf("Scenarios:\n");
  printf("  happiness_half_to_falling_asleep_snoring_waking_up_curiosity\n");
  printf("  playful_pong_to_dizzy\n");
  printf("  all_emotions_interrupt_sweep\n");
  printf("  all_emotions_random_transitions\n");
  printf("  curiosity_2s_to_playful_pong_10s_to_falling_asleep\n");
}

static bool mkdir_p(const std::string &path) {
  if (path.empty()) {
    return false;
  }

  // Simple recursive mkdir -p for POSIX.
  std::string cur;
  cur.reserve(path.size());
  for (size_t i = 0; i < path.size(); ++i) {
    char c = path[i];
    cur.push_back(c);
    if (c == '/' || i + 1 == path.size()) {
      if (cur.size() == 1 && cur[0] == '/') {
        continue;
      }
      if (::mkdir(cur.c_str(), 0755) != 0) {
        // EEXIST is ok.
        struct stat st;
        if (::stat(cur.c_str(), &st) != 0) {
          return false;
        }
        if (!S_ISDIR(st.st_mode)) {
          return false;
        }
      }
    }
  }
  return true;
}

static bool parse_int(const char *s, int *out) {
  if (!s || !out) {
    return false;
  }
  char *end = nullptr;
  long v = strtol(s, &end, 10);
  if (!end || *end != '\0') {
    return false;
  }
  if (v < INT32_MIN || v > INT32_MAX) {
    return false;
  }
  *out = static_cast<int>(v);
  return true;
}

static bool parse_u32(const char *s, uint32_t *out) {
  if (!s || !out) {
    return false;
  }
  char *end = nullptr;
  unsigned long v = strtoul(s, &end, 10);
  if (!end || *end != '\0') {
    return false;
  }
  if (v > 0xFFFFFFFFu) {
    return false;
  }
  *out = static_cast<uint32_t>(v);
  return true;
}

static bool parse_args(int argc, char **argv, Args *out) {
  if (!out) {
    return false;
  }
  Args a;

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      usage(argv[0]);
      exit(0);
    }
    if (strcmp(arg, "--out") == 0 && i + 1 < argc) {
      a.out_dir = argv[++i];
      continue;
    }
    if (strcmp(arg, "--emotion") == 0 && i + 1 < argc) {
      const char *name = argv[++i];
      if (strcmp(name, "all") == 0) {
        a.all = true;
      } else {
        eyes::Emotion e;
        if (!eyes::emotion_from_string(name, &e)) {
          fprintf(stderr, "Unknown emotion: %s\n", name);
          return false;
        }
        a.all = false;
        a.emotion = e;
      }
      continue;
    }
    if (strcmp(arg, "--scenario") == 0 && i + 1 < argc) {
      const char *name = argv[++i];
      if (strcmp(name, "happiness_half_to_falling_asleep_snoring_waking_up_curiosity") == 0 ||
          strcmp(name, "happiness_half_to_sleepy_snoring_waking_up_curiosity") == 0) {
        a.scenario = Args::Scenario::HappinessHalfToFallingAsleepSnoringWakingUpCuriosity;
      } else if (strcmp(name, "playful_pong_to_dizzy") == 0) {
        a.scenario = Args::Scenario::PlayfulPongToDizzy;
      } else if (strcmp(name, "all_emotions_interrupt_sweep") == 0 ||
                 strcmp(name, "all_emotions_interruption_sweep") == 0) {
        a.scenario = Args::Scenario::AllEmotionsInterruptSweep;
      } else if (strcmp(name, "all_emotions_random_transitions") == 0 ||
                 strcmp(name, "all_emotions_random_transition") == 0) {
        a.scenario = Args::Scenario::AllEmotionsRandomTransitions;
      } else if (strcmp(name, "curiosity_2s_to_playful_pong_10s_to_falling_asleep") == 0 ||
                 strcmp(name, "curiosity_to_pong_to_falling_asleep") == 0 ||
                 strcmp(name, "curious_to_pong_to_falling_asleep") == 0) {
        a.scenario = Args::Scenario::Curiosity2sPlayfulPong10sFallingAsleep;
      } else {
        fprintf(stderr, "Unknown scenario: %s\n", name);
        return false;
      }
      continue;
    }
    if (strcmp(arg, "--fps") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a.fps)) {
        return false;
      }
      continue;
    }
    if (strcmp(arg, "--width") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a.width)) {
        return false;
      }
      continue;
    }
    if (strcmp(arg, "--height") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a.height)) {
        return false;
      }
      continue;
    }
    if (strcmp(arg, "--duration-ms") == 0 && i + 1 < argc) {
      if (!parse_u32(argv[++i], &a.duration_ms)) {
        return false;
      }
      continue;
    }
    if (strcmp(arg, "--transition-ms") == 0 && i + 1 < argc) {
      if (!parse_u32(argv[++i], &a.transition_ms)) {
        return false;
      }
      continue;
    }
    if (strcmp(arg, "--variation-seed") == 0 && i + 1 < argc) {
      if (!parse_u32(argv[++i], &a.variation_seed)) {
        return false;
      }
      continue;
    }

    fprintf(stderr, "Unknown option: %s\n", arg);
    return false;
  }

  if (a.fps <= 0 || a.fps > 120) {
    fprintf(stderr, "--fps must be 1..120\n");
    return false;
  }
  if (a.width <= 0 || a.height <= 0) {
    fprintf(stderr, "--width/--height must be > 0\n");
    return false;
  }
  if (a.transition_ms == 0) {
    fprintf(stderr, "--transition-ms must be > 0\n");
    return false;
  }

  *out = a;
  return true;
}

static uint32_t lcg_next_u32(uint32_t *state) {
  if (!state) {
    return 0;
  }
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static uint32_t lcg_range_u32(uint32_t *state, uint32_t min_value, uint32_t max_value) {
  if (max_value <= min_value) {
    return min_value;
  }
  const uint32_t span = max_value - min_value + 1u;
  return min_value + (lcg_next_u32(state) % span);
}

static void shuffle_emotions(eyes::Emotion *emotions, size_t count, uint32_t *state) {
  if (!emotions || !state || count < 2) {
    return;
  }

  for (size_t i = count - 1; i > 0; --i) {
    const size_t j = static_cast<size_t>(lcg_next_u32(state) % static_cast<uint32_t>(i + 1));
    const eyes::Emotion tmp = emotions[i];
    emotions[i] = emotions[j];
    emotions[j] = tmp;
  }
}

class PersistentFrameSink final : public eyes::IFrameSink {
public:
  explicit PersistentFrameSink(eyes::IFrameSink &inner) : inner_(inner) {
  }

  bool begin(int width, int height, int fps) override {
    if (!started_) {
      if (!inner_.begin(width, height, fps)) {
        return false;
      }
      started_ = true;
      width_ = width;
      height_ = height;
      fps_ = fps;
      return true;
    }
    return width == width_ && height == height_ && fps == fps_;
  }

  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override {
    if (!started_ || finalized_) {
      return false;
    }
    return inner_.frame(grayscale, stride, duration_ms);
  }

  bool end() override {
    // EyesClipPlayer calls end() after every clip. We defer real finalization so multiple clips can
    // be stitched into one GIF.
    return true;
  }

  bool finalize() {
    if (!started_ || finalized_) {
      return true;
    }
    finalized_ = true;
    return inner_.end();
  }

private:
  eyes::IFrameSink &inner_;
  bool started_ = false;
  bool finalized_ = false;
  int width_ = 0;
  int height_ = 0;
  int fps_ = 0;
};

static bool export_one(const Args &args, eyes::Emotion emotion) {
  if (!mkdir_p(args.out_dir)) {
    fprintf(stderr, "Failed to create out dir: %s\n", args.out_dir.c_str());
    return false;
  }

  std::string path = args.out_dir + "/" + std::string(eyes::emotion_name(emotion)) + ".gif";

  eyes::gif::FileByteSink file;
  if (!file.open(path.c_str())) {
    fprintf(stderr, "Failed to open: %s\n", path.c_str());
    return false;
  }

  eyes::gif::GifEncoder enc(file);
  eyes::gif::GifFrameSink gif_sink(enc);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, gif_sink);

  eyes::ClipOptions opt;
  opt.size = {args.width, args.height};
  opt.fps = args.fps;
  opt.duration_ms_override = args.duration_ms;
  opt.variation_seed = args.variation_seed;

  if (!player.play(emotion, opt)) {
    fprintf(stderr, "Export failed: %s\n", eyes::emotion_name(emotion));
    return false;
  }

  printf("Wrote %s (%ums @ %dfps)\n", path.c_str(),
         static_cast<unsigned>(opt.duration_ms_override ? opt.duration_ms_override
                                                        : eyes::emotion_clip(emotion).duration_ms),
         opt.fps);
  return true;
}

static bool export_scenario_happiness_half_to_falling_asleep_snoring_waking_up_curiosity(
    const Args &args) {
  if (!mkdir_p(args.out_dir)) {
    fprintf(stderr, "Failed to create out dir: %s\n", args.out_dir.c_str());
    return false;
  }

  const std::string filename = "happiness_half_to_falling_asleep_snoring_waking_up_curiosity.gif";
  std::string path = args.out_dir + "/" + filename;

  eyes::gif::FileByteSink file;
  if (!file.open(path.c_str())) {
    fprintf(stderr, "Failed to open: %s\n", path.c_str());
    return false;
  }

  eyes::gif::GifEncoder enc(file);
  eyes::gif::GifFrameSink gif_sink(enc);
  PersistentFrameSink stitched_sink(gif_sink);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, stitched_sink);

  eyes::SequencerOptions seq_opt;
  seq_opt.enable_transitions = true;
  seq_opt.transition_ms = args.transition_ms;
  eyes::EyesEmotionSequencer sequencer(player, seq_opt);

  eyes::ClipOptions opt;
  opt.size = {args.width, args.height};
  opt.fps = args.fps;
  uint32_t variation_seed = args.variation_seed;

  const eyes::Emotion sequence[] = {
      eyes::Emotion::Happiness,
      eyes::Emotion::FallingAsleep,
      eyes::Emotion::Snoring,
      eyes::Emotion::WakingUp,
      eyes::Emotion::Curiosity,
  };
  uint32_t total_ms = 0;

  for (size_t i = 0; i < sizeof(sequence) / sizeof(sequence[0]); ++i) {
    const eyes::Emotion emotion = sequence[i];
    if (emotion == eyes::Emotion::Happiness) {
      opt.duration_ms_override = eyes::emotion_clip(emotion).duration_ms / 2;
    } else {
      opt.duration_ms_override = 0;
    }

    const uint32_t duration_ms =
        (opt.duration_ms_override != 0) ? opt.duration_ms_override : eyes::emotion_clip(emotion).duration_ms;
    total_ms += duration_ms;
    if (variation_seed != 0) {
      variation_seed = variation_seed * 1664525u + 1013904223u;
      opt.variation_seed = variation_seed;
    } else {
      opt.variation_seed = 0;
    }

    if (!sequencer.play(emotion, opt)) {
      fprintf(stderr, "Scenario export failed at %s\n", eyes::emotion_name(emotion));
      (void)stitched_sink.finalize();
      return false;
    }
  }

  if (!stitched_sink.finalize()) {
    fprintf(stderr, "Scenario export finalize failed\n");
    return false;
  }

  printf("Wrote %s (%ums @ %dfps)\n", path.c_str(), static_cast<unsigned>(total_ms), args.fps);
  return true;
}

static bool export_scenario_playful_pong_to_dizzy(const Args &args) {
  if (!mkdir_p(args.out_dir)) {
    fprintf(stderr, "Failed to create out dir: %s\n", args.out_dir.c_str());
    return false;
  }

  const std::string filename = "playful_pong_to_dizzy.gif";
  std::string path = args.out_dir + "/" + filename;

  eyes::gif::FileByteSink file;
  if (!file.open(path.c_str())) {
    fprintf(stderr, "Failed to open: %s\n", path.c_str());
    return false;
  }

  eyes::gif::GifEncoder enc(file);
  eyes::gif::GifFrameSink gif_sink(enc);
  PersistentFrameSink stitched_sink(gif_sink);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, stitched_sink);

  eyes::SequencerOptions seq_opt;
  seq_opt.enable_transitions = true;
  seq_opt.transition_ms = args.transition_ms;
  eyes::EyesEmotionSequencer sequencer(player, seq_opt);

  eyes::ClipOptions opt;
  opt.size = {args.width, args.height};
  opt.fps = args.fps;
  opt.duration_ms_override = 0;
  uint32_t variation_seed = args.variation_seed;

  const eyes::Emotion sequence[] = {
      eyes::Emotion::PlayfulPong,
      eyes::Emotion::Dizzy,
  };
  uint32_t total_ms = 0;

  for (size_t i = 0; i < sizeof(sequence) / sizeof(sequence[0]); ++i) {
    const eyes::Emotion emotion = sequence[i];
    const uint32_t duration_ms = eyes::emotion_clip(emotion).duration_ms;
    total_ms += duration_ms;
    if (variation_seed != 0) {
      variation_seed = variation_seed * 1664525u + 1013904223u;
      opt.variation_seed = variation_seed;
    } else {
      opt.variation_seed = 0;
    }

    if (!sequencer.play(emotion, opt)) {
      fprintf(stderr, "Scenario export failed at %s\n", eyes::emotion_name(emotion));
      (void)stitched_sink.finalize();
      return false;
    }
  }

  if (!stitched_sink.finalize()) {
    fprintf(stderr, "Scenario export finalize failed\n");
    return false;
  }

  printf("Wrote %s (%ums @ %dfps)\n", path.c_str(), static_cast<unsigned>(total_ms), args.fps);
  return true;
}

static bool export_scenario_all_emotions_interrupt_sweep(const Args &args) {
  if (!mkdir_p(args.out_dir)) {
    fprintf(stderr, "Failed to create out dir: %s\n", args.out_dir.c_str());
    return false;
  }

  const std::string filename = "all_emotions_interrupt_sweep.gif";
  std::string path = args.out_dir + "/" + filename;

  eyes::gif::FileByteSink file;
  if (!file.open(path.c_str())) {
    fprintf(stderr, "Failed to open: %s\n", path.c_str());
    return false;
  }

  eyes::gif::GifEncoder enc(file);
  eyes::gif::GifFrameSink gif_sink(enc);
  PersistentFrameSink stitched_sink(gif_sink);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, stitched_sink);

  eyes::SequencerOptions seq_opt;
  seq_opt.enable_transitions = true;
  seq_opt.transition_ms = args.transition_ms;
  eyes::EyesEmotionSequencer sequencer(player, seq_opt);

  eyes::ClipOptions opt;
  opt.size = {args.width, args.height};
  opt.fps = args.fps;

  const eyes::Emotion emotions[] = {
      eyes::Emotion::Neutral,     eyes::Emotion::Happiness,   eyes::Emotion::Curiosity,
      eyes::Emotion::Exhaustion,  eyes::Emotion::FallingAsleep, eyes::Emotion::WakingUp,
      eyes::Emotion::Snoring,     eyes::Emotion::Dizzy,       eyes::Emotion::PlayfulPong,
  };
  const float interruption_factors[] = {0.22f, 0.41f, 0.63f, 0.79f};

  uint32_t variation_seed = args.variation_seed;
  uint32_t total_ms = 0;

  for (size_t pass = 0; pass < sizeof(interruption_factors) / sizeof(interruption_factors[0]); ++pass) {
    const float factor = interruption_factors[pass];
    for (size_t i = 0; i < sizeof(emotions) / sizeof(emotions[0]); ++i) {
      const eyes::Emotion emotion = emotions[i];
      uint32_t clip_ms = eyes::emotion_clip(emotion).duration_ms;
      uint32_t interrupted_ms = static_cast<uint32_t>(static_cast<float>(clip_ms) * factor + 0.5f);
      if (interrupted_ms == 0) {
        interrupted_ms = 1;
      }
      opt.duration_ms_override = interrupted_ms;
      total_ms += interrupted_ms;

      if (variation_seed != 0) {
        variation_seed = variation_seed * 1664525u + 1013904223u;
        opt.variation_seed = variation_seed;
      } else {
        opt.variation_seed = 0;
      }

      if (!sequencer.play(emotion, opt)) {
        fprintf(stderr, "Scenario export failed at pass=%u emotion=%s\n",
                static_cast<unsigned>(pass), eyes::emotion_name(emotion));
        (void)stitched_sink.finalize();
        return false;
      }
    }
  }

  if (!stitched_sink.finalize()) {
    fprintf(stderr, "Scenario export finalize failed\n");
    return false;
  }

  printf("Wrote %s (%ums @ %dfps)\n", path.c_str(), static_cast<unsigned>(total_ms), args.fps);
  return true;
}

static bool export_scenario_all_emotions_random_transitions(const Args &args) {
  if (!mkdir_p(args.out_dir)) {
    fprintf(stderr, "Failed to create out dir: %s\n", args.out_dir.c_str());
    return false;
  }

  const std::string filename = "all_emotions_random_transitions.gif";
  std::string path = args.out_dir + "/" + filename;

  eyes::gif::FileByteSink file;
  if (!file.open(path.c_str())) {
    fprintf(stderr, "Failed to open: %s\n", path.c_str());
    return false;
  }

  eyes::gif::GifEncoder enc(file);
  eyes::gif::GifFrameSink gif_sink(enc);
  PersistentFrameSink stitched_sink(gif_sink);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, stitched_sink);

  eyes::SequencerOptions seq_opt;
  seq_opt.enable_transitions = true;
  seq_opt.transition_ms = args.transition_ms;
  eyes::EyesEmotionSequencer sequencer(player, seq_opt);

  eyes::ClipOptions opt;
  opt.size = {args.width, args.height};
  opt.fps = args.fps;
  opt.duration_ms_override = 0;

  const uint32_t kMinSegmentMs = 1500;
  eyes::Emotion emotions[] = {
      eyes::Emotion::Neutral,       eyes::Emotion::Happiness,    eyes::Emotion::Curiosity,
      eyes::Emotion::Exhaustion,    eyes::Emotion::FallingAsleep, eyes::Emotion::WakingUp,
      eyes::Emotion::Snoring,       eyes::Emotion::Dizzy,        eyes::Emotion::PlayfulPong,
  };

  // If caller did not provide a seed, use a stable default so tests remain deterministic.
  uint32_t rng_state = (args.variation_seed != 0) ? args.variation_seed : 0x31415926u;
  uint32_t variation_seed = args.variation_seed;
  uint32_t total_ms = 0;

  shuffle_emotions(emotions, sizeof(emotions) / sizeof(emotions[0]), &rng_state);

  for (size_t i = 0; i < sizeof(emotions) / sizeof(emotions[0]); ++i) {
    const eyes::Emotion emotion = emotions[i];
    const uint32_t clip_ms = eyes::emotion_clip(emotion).duration_ms;
    const uint32_t max_ms = (clip_ms > kMinSegmentMs) ? clip_ms : kMinSegmentMs;
    opt.duration_ms_override = lcg_range_u32(&rng_state, kMinSegmentMs, max_ms);
    total_ms += opt.duration_ms_override;

    if (variation_seed != 0) {
      variation_seed = variation_seed * 1664525u + 1013904223u;
      opt.variation_seed = variation_seed;
    } else {
      opt.variation_seed = 0;
    }

    if (!sequencer.play(emotion, opt)) {
      fprintf(stderr, "Scenario export failed at emotion=%s\n", eyes::emotion_name(emotion));
      (void)stitched_sink.finalize();
      return false;
    }
  }

  if (!stitched_sink.finalize()) {
    fprintf(stderr, "Scenario export finalize failed\n");
    return false;
  }

  printf("Wrote %s (%ums @ %dfps)\n", path.c_str(), static_cast<unsigned>(total_ms), args.fps);
  return true;
}

static bool export_scenario_curiosity_2s_to_playful_pong_10s_to_falling_asleep(const Args &args) {
  if (!mkdir_p(args.out_dir)) {
    fprintf(stderr, "Failed to create out dir: %s\n", args.out_dir.c_str());
    return false;
  }

  const std::string filename = "curiosity_2s_to_playful_pong_10s_to_falling_asleep.gif";
  std::string path = args.out_dir + "/" + filename;

  eyes::gif::FileByteSink file;
  if (!file.open(path.c_str())) {
    fprintf(stderr, "Failed to open: %s\n", path.c_str());
    return false;
  }

  eyes::gif::GifEncoder enc(file);
  eyes::gif::GifFrameSink gif_sink(enc);
  PersistentFrameSink stitched_sink(gif_sink);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, stitched_sink);

  eyes::SequencerOptions seq_opt;
  seq_opt.enable_transitions = true;
  seq_opt.transition_ms = args.transition_ms;
  eyes::EyesEmotionSequencer sequencer(player, seq_opt);

  eyes::ClipOptions opt;
  opt.size = {args.width, args.height};
  opt.fps = args.fps;
  opt.sustain.enabled = true;
  opt.sustain.play_outro_on_end = true;

  struct Step {
    eyes::Emotion emotion;
    uint32_t duration_ms;
  };
  const Step sequence[] = {
      {eyes::Emotion::Curiosity, 2000},
      {eyes::Emotion::PlayfulPong, 10000},
      {eyes::Emotion::FallingAsleep, 5000},
  };

  uint32_t variation_seed = args.variation_seed;
  uint32_t total_ms = 0;

  for (size_t i = 0; i < sizeof(sequence) / sizeof(sequence[0]); ++i) {
    const Step &step = sequence[i];
    opt.duration_ms_override = step.duration_ms;
    total_ms += step.duration_ms;

    if (variation_seed != 0) {
      variation_seed = variation_seed * 1664525u + 1013904223u;
      opt.variation_seed = variation_seed;
    } else {
      opt.variation_seed = 0;
    }

    if (!sequencer.play(step.emotion, opt)) {
      fprintf(stderr, "Scenario export failed at %s\n", eyes::emotion_name(step.emotion));
      (void)stitched_sink.finalize();
      return false;
    }
  }

  if (!stitched_sink.finalize()) {
    fprintf(stderr, "Scenario export finalize failed\n");
    return false;
  }

  printf("Wrote %s (%ums @ %dfps)\n", path.c_str(), static_cast<unsigned>(total_ms), args.fps);
  return true;
}

static bool export_scenario(const Args &args) {
  switch (args.scenario) {
    case Args::Scenario::HappinessHalfToFallingAsleepSnoringWakingUpCuriosity:
      return export_scenario_happiness_half_to_falling_asleep_snoring_waking_up_curiosity(args);
    case Args::Scenario::PlayfulPongToDizzy:
      return export_scenario_playful_pong_to_dizzy(args);
    case Args::Scenario::AllEmotionsInterruptSweep:
      return export_scenario_all_emotions_interrupt_sweep(args);
    case Args::Scenario::AllEmotionsRandomTransitions:
      return export_scenario_all_emotions_random_transitions(args);
    case Args::Scenario::Curiosity2sPlayfulPong10sFallingAsleep:
      return export_scenario_curiosity_2s_to_playful_pong_10s_to_falling_asleep(args);
    case Args::Scenario::None:
      break;
  }
  return false;
}

}  // namespace

int main(int argc, char **argv) {
  Args args;
  if (!parse_args(argc, argv, &args)) {
    usage(argv[0]);
    return 2;
  }

  if (args.scenario != Args::Scenario::None) {
    return export_scenario(args) ? 0 : 1;
  }

  if (args.all) {
    const eyes::Emotion emotions[] = {
        eyes::Emotion::Neutral,      eyes::Emotion::Happiness,  eyes::Emotion::Curiosity,
        eyes::Emotion::Exhaustion,   eyes::Emotion::FallingAsleep, eyes::Emotion::WakingUp,
        eyes::Emotion::Snoring,      eyes::Emotion::Dizzy,
        eyes::Emotion::PlayfulPong,
    };
    for (size_t i = 0; i < sizeof(emotions) / sizeof(emotions[0]); ++i) {
      if (!export_one(args, emotions[i])) {
        return 1;
      }
    }
    return 0;
  }

  return export_one(args, args.emotion) ? 0 : 1;
}
