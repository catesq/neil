#ifndef LANTERNFISH_UTILS_HPP
#define LANTERNFISH_UTILS_HPP

namespace lanternfish {
  float *load_to_float_mono(short *in, int channels, int bytes, int samples);
  void add_signals(float *s1, float *s2, int n);
  void add_signals(float scalar, float *signal, int n);
  void mul_signals(float *s1, float *s2, int n);
  void mul_signals(float scalar, float *signal, int n);
  void scale_signal(float *signal, float min, float max, int n);
  void const_signal(float *out, float value, int n);
  void signal_copy(float *in, float *out, int n);
  float note_to_freq(int note);
  float *make_sine_table(int size);
  float rms(float *buffer, int n);

  class DcFilter {
  public:
      DcFilter(float pole = 0.995);
      void process(float *left_input, float *left_output, float *right_in, float *right_output, unsigned sample_count);
  private:
      float pole, itl1, otl1, itr1, otr1;
  };
}

#endif // LANTERNFISH_UTILS_HPP
