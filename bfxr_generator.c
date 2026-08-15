#include "bfxr_generator.h"
#include "bfxr_params.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

static double rand_uniform(void) {
    return (double)rand() / RAND_MAX * 2.0 - 1.0;
}

BfxrWave bfxr_generate_wave(const double params[NUM_PARAMS]) {
    double p[NUM_PARAMS];
    memcpy(p, params, sizeof(double) * NUM_PARAMS);

    int wt = (int)p[0];
    if (wt < 0) wt = 0;
    if (wt > 11) wt = 11;

    double master_vol = p[1];
    double attack_t = p[2];
    double sustain_t = p[3];
    double sust_punch = p[4];
    double decay_t = p[5];
    double start_freq = p[6];
    double min_freq = p[7];
    double slide = p[8];
    double delta_slide = p[9];
    double vib_depth = p[10];
    double vib_speed = p[11];
    double chg_amt = p[12];
    double chg_spd = p[13];
    double chg_amt2 = p[14];
    double chg_spd2 = p[15];
    double chg_repeat = p[16];
    double sq_duty = p[17];
    double duty_sweep = p[18];
    double repeat_spd = p[19];
    double flng_offset = p[20];
    double flng_sweep = p[21];
    double lp_cutoff = p[22];
    double lp_cut_sweep = p[23];
    double lp_res = p[24];
    double hp_cutoff = p[25];
    double hp_cut_sweep = p[26];
    double bit_crush = p[27];
    double bit_crush_sw = p[28];
    double compression = p[29];
    double overtones = p[30];
    double ot_falloff = p[31];

    double mv2 = master_vol * master_vol;

    int env0 = (int)(attack_t * attack_t * 100000.0);
    int env1 = (int)(sustain_t * sustain_t * 100000.0);
    int env2 = (int)(decay_t * decay_t * 100000.0) + 10;
    int total = env0 + env1 + env2;
    int min_len = (int)(0.18 * 44100);
    if (total < min_len) {
        double scale = (double)min_len / (total > 0 ? total : 1);
        env0 = (int)(env0 * scale);
        env1 = (int)(env1 * scale);
        env2 = (int)(env2 * scale);
        total = env0 + env1 + env2;
    }

    double period = 100.0 / (start_freq * start_freq + 0.001);
    double max_period = min_freq > 0.0 ? 100.0 / (min_freq * min_freq + 0.001) : 1e9;

    double sl = 1.0 - slide * slide * slide * 0.01;
    double dsl = -delta_slide * delta_slide * delta_slide * 0.000001;

    double sq_d = 0.5 - sq_duty * 0.5;
    double dsq = -duty_sweep * 0.00005;

    double vib_phase = 0.0;
    double vib_spd = vib_speed * vib_speed * 0.01;
    double vib_amp = vib_depth * 0.5;

    double ca, ca2;
    if (chg_amt > 0.0) ca = 1.0 - chg_amt * chg_amt * 0.9;
    else ca = 1.0 + chg_amt * chg_amt * 10.0;
    if (chg_amt2 > 0.0) ca2 = 1.0 - chg_amt2 * chg_amt2 * 0.9;
    else ca2 = 1.0 + chg_amt2 * chg_amt2 * 10.0;

    int cl, cl2;
    if (chg_spd == 1.0) cl = 0;
    else cl = (int)((1.0 - chg_spd) * (1.0 - chg_spd) * 20000) + 32;
    if (chg_spd2 == 1.0) cl2 = 0;
    else cl2 = (int)((1.0 - chg_spd2) * (1.0 - chg_spd2) * 20000) + 32;

    int chg_period = (int)(((1.0 - chg_repeat) + 0.1) / 1.1 * 20000) + 32;
    cl = (int)(cl * ((1.0 - chg_repeat + 0.1) / 1.1));
    cl2 = (int)(cl2 * ((1.0 - chg_repeat + 0.1) / 1.1));

    int repeat_limit;
    if (repeat_spd == 0.0) repeat_limit = 0;
    else repeat_limit = (int)((1.0 - repeat_spd) * (1.0 - repeat_spd) * 20000) + 32;

    double flng_off = flng_offset * flng_offset * 1020.0;
    if (flng_offset < 0.0) flng_off = -flng_off;
    double flng_doff = flng_sweep * flng_sweep * flng_sweep * 0.2;
    int use_flng = (flng_offset != 0.0 || flng_sweep != 0.0);

    double lp_cut = lp_cutoff * lp_cutoff * lp_cutoff * 0.1;
    double lp_dcut = 1.0 + lp_cut_sweep * 0.0001;
    double lp_damp = 5.0 / (1.0 + lp_res * lp_res * 20.0) * (0.01 + lp_cut);
    if (lp_damp > 0.8) lp_damp = 0.8;
    lp_damp = 1.0 - lp_damp;
    int lp_on = (lp_cutoff != 1.0);
    int use_filt = (lp_cutoff != 1.0 || hp_cutoff != 0.0);

    double hp_cut = hp_cutoff * hp_cutoff * 0.1;
    double hp_dcut = 1.0 + hp_cut_sweep * 0.0003;

    double bc_freq = 1.0 - pow(fmax(bit_crush, 1e-9), 1.0/3.0);
    double bc_sweep = -bit_crush_sw * 0.000015;
    double bc_phase = 0.0;
    double bc_last = 0.0;

    double comp_factor = 1.0 / (1.0 + 4.0 * compression);

    int ot_count = (int)(overtones * 10);

    double noise_buf[32];
    for (int n = 0; n < 32; n++) noise_buf[n] = rand_uniform();

    double pb[7] = {0};
    double pink_buf[32];
    for (int n = 0; n < 32; n++) {
        double w = rand_uniform();
        pb[0] = 0.99886*pb[0] + w*0.0555179;
        pb[1] = 0.99332*pb[1] + w*0.0750759;
        pb[2] = 0.96900*pb[2] + w*0.1538520;
        pb[3] = 0.86650*pb[3] + w*0.3104856;
        pb[4] = 0.55000*pb[4] + w*0.5329522;
        pb[5] = -0.7616*pb[5] - w*0.0168980;
        pink_buf[n] = (pb[0]+pb[1]+pb[2]+pb[3]+pb[4]+pb[5]+pb[6]+w*0.5362)*0.11;
        pb[6] = w * 0.115926;
    }

    double lores_buf[32];
    lores_buf[0] = rand_uniform();
    for (int n = 1; n < 32; n++) {
        if (n % 8 == 0) lores_buf[n] = rand_uniform();
        else lores_buf[n] = lores_buf[n-1];
    }

    double flng_buf[1024];
    memset(flng_buf, 0, sizeof(flng_buf));
    int flng_pos = 0;
    int flng_int = ((int)fabs(flng_off)) & 1023;

    double lp_pos = 0.0, lp_dpos = 0.0, hp_pos = 0.0;
    int one_bit_state = 1 << 14;
    double one_bit_noise = 0.0;
    int buzz_state = 1 << 14;
    double buzz_val = 0.0;

    int phase = 0;
    int env_stage = 0;
    int env_time = 0;
    double env_vol = 0.0;
    int env_cur = env0;
    double oe0 = env0 > 0 ? 1.0 / env0 : 0.0;
    double oe1 = env1 > 0 ? 1.0 / env1 : 0.0;
    double oe2 = env2 > 0 ? 1.0 / env2 : 0.0;

    int chg_time = 0, chg_reached = 0;
    int chg_time2 = 0, chg_reached2 = 0;
    int chg_period_time = 0;
    int repeat_time = 0;
    int muted = 0;

    double start_period = period;
    double start_sl = sl;
    double start_dsl = dsl;
    double start_sq_d = sq_d;
    double start_dsq = dsq;

    float* out = (float*)malloc(sizeof(float) * total);
    int finished = 0;

    for (int i = 0; i < total && !finished; i++) {
        if (repeat_limit > 0) {
            repeat_time++;
            if (repeat_time >= repeat_limit) {
                repeat_time = 0;
                period = start_period;
                sl = start_sl;
                dsl = start_dsl;
                sq_d = start_sq_d;
                dsq = start_dsq;
                chg_time = 0; chg_reached = 0;
                chg_time2 = 0; chg_reached2 = 0;
                chg_period_time = 0;
            }
        }

        chg_period_time++;
        if (chg_period_time >= chg_period) {
            chg_period_time = 0;
            chg_time = 0; chg_time2 = 0;
            if (chg_reached) { period /= ca; chg_reached = 0; }
            if (chg_reached2) { period /= ca2; chg_reached2 = 0; }
        }

        if (!chg_reached) {
            chg_time++;
            if (cl == 0 || chg_time >= cl) { chg_reached = 1; period *= ca; }
        }
        if (!chg_reached2) {
            chg_time2++;
            if (cl2 == 0 || chg_time2 >= cl2) { chg_reached2 = 1; period *= ca2; }
        }

        sl += dsl;
        period *= sl;
        if (period > max_period) {
            period = max_period;
            if (min_freq > 0.0) muted = 1;
        }

        double period_temp = period;
        if (vib_amp > 0.0) {
            vib_phase += vib_spd;
            period_temp = period * (1.0 + sin(vib_phase) * vib_amp);
        }

        int period_int = (int)period_temp;
        if (period_int < 8) period_int = 8;

        if (wt == 0) {
            sq_d += dsq;
            if (sq_d < 0.0) sq_d = 0.0;
            if (sq_d > 0.5) sq_d = 0.5;
        }

        env_time++;
        if (env_time > env_cur) {
            env_time = 0;
            env_stage++;
            if (env_stage == 1) env_cur = env1;
            else if (env_stage == 2) env_cur = env2;
        }

        if (env_stage == 0) env_vol = env_time * oe0;
        else if (env_stage == 1) env_vol = 1.0 + (1.0 - env_time * oe1) * 2.0 * sust_punch;
        else if (env_stage == 2) env_vol = 1.0 - env_time * oe2;
        else { env_vol = 0.0; finished = 1; }

        if (use_flng) {
            flng_off += flng_doff;
            flng_int = (int)fabs(flng_off);
            if (flng_int > 1023) flng_int = 1023;
        }

        if (use_filt) {
            hp_cut *= hp_dcut;
            if (hp_cut < 0.00001) hp_cut = 0.00001;
            if (hp_cut > 0.1) hp_cut = 0.1;
        }

        double supersample = 0.0;
        for (int j = 0; j < 8; j++) {
            phase++;
            if (phase >= period_int) {
                phase -= period_int;
                if (wt == 3) {
                    for (int n = 0; n < 32; n++) noise_buf[n] = rand_uniform();
                } else if (wt == 5) {
                    for (int n = 0; n < 32; n++) {
                        double w = rand_uniform();
                        pb[0] = 0.99886*pb[0] + w*0.0555179;
                        pb[1] = 0.99332*pb[1] + w*0.0750759;
                        pb[2] = 0.96900*pb[2] + w*0.1538520;
                        pb[3] = 0.86650*pb[3] + w*0.3104856;
                        pb[4] = 0.55000*pb[4] + w*0.5329522;
                        pb[5] = -0.7616*pb[5] - w*0.0168980;
                        pink_buf[n] = (pb[0]+pb[1]+pb[2]+pb[3]+pb[4]+pb[5]+pb[6]+w*0.5362)*0.11;
                        pb[6] = w * 0.115926;
                    }
                } else if (wt == 6) {
                    for (int n = 0; n < 32; n++) {
                        if (n % 8 == 0) lores_buf[n] = rand_uniform();
                        else lores_buf[n] = lores_buf[n-1];
                    }
                } else if (wt == 9) {
                    int fb = ((one_bit_state >> 1) & 1) ^ (one_bit_state & 1);
                    one_bit_state = (one_bit_state >> 1) | (fb << 14);
                    one_bit_noise = (double)(~one_bit_state & 1) - 0.5;
                } else if (wt == 11) {
                    int fb = ((buzz_state >> 3) & 1) ^ (buzz_state & 1);
                    buzz_state = (buzz_state >> 1) | (fb << 14);
                    buzz_val = (double)(~buzz_state & 1) - 0.5;
                }
            }

            double sample = 0.0;
            double ot_str = 1.0;
            for (int k = 0; k <= ot_count; k++) {
                int tp = (phase * (k + 1)) % period_int;
                int cur_wt = wt;
                if (cur_wt == 10) cur_wt = (phase / 4) % 10;

                if (cur_wt == 0) {
                    sample += ot_str * (0.5 - ((tp / (double)period_int) < sq_d ? -0.5 : 0.5));
                } else if (cur_wt == 1) {
                    sample += ot_str * (1.0 - (tp / (double)period_int) * 2.0);
                } else if (cur_wt == 2) {
                    double pos = tp / (double)period_int;
                    if (pos > 0.5) pos = (pos - 1.0) * 6.28318531;
                    else pos = pos * 6.28318531;
                    double ts = pos < 0.0 ? 1.27323954*pos + 0.405284735*pos*pos : 1.27323954*pos - 0.405284735*pos*pos;
                    double sv = ts < 0.0 ? 0.225*(ts*-ts - ts) + ts : 0.225*(ts*ts - ts) + ts;
                    sample += ot_str * sv;
                } else if (cur_wt == 3) {
                    int idx = (int)(tp * 32 / (double)period_int) % 32;
                    sample += ot_str * noise_buf[idx];
                } else if (cur_wt == 4) {
                    sample += ot_str * (fabs(1.0 - (tp / (double)period_int) * 2.0) - 1.0);
                } else if (cur_wt == 5) {
                    int idx = (int)(tp * 32 / (double)period_int) % 32;
                    sample += ot_str * pink_buf[idx];
                } else if (cur_wt == 6) {
                    double v = tp / (double)period_int;
                    double arg = 3.14159265 * v;
                    if (arg > 1.56) arg = 1.56;
                    sample += ot_str * tan(arg);
                } else if (cur_wt == 7) {
                    double pos = tp / (double)period_int;
                    if (pos > 0.5) pos = (pos-1.0)*6.28318531;
                    else pos = pos*6.28318531;
                    double ts = pos < 0.0 ? 1.27323954*pos+0.405284735*pos*pos : 1.27323954*pos-0.405284735*pos*pos;
                    double sv = ts < 0.0 ? 0.225*(ts*-ts-ts)+ts : 0.225*(ts*ts-ts)+ts;
                    double v = 0.75 * sv;
                    double pos2 = ((tp*20) % period_int) / (double)period_int;
                    if (pos2 > 0.5) pos2 = (pos2-1.0)*6.28318531;
                    else pos2 = pos2*6.28318531;
                    double ts2 = pos2 < 0.0 ? 1.27323954*pos2+0.405284735*pos2*pos2 : 1.27323954*pos2-0.405284735*pos2*pos2;
                    double sv2 = ts2 < 0.0 ? 0.225*(ts2*-ts2-ts2)+ts2 : 0.225*(ts2*ts2-ts2)+ts2;
                    v += 0.25 * sv2;
                    sample += ot_str * v;
                } else if (cur_wt == 8) {
                    double amp = tp / (double)period_int;
                    sample += ot_str * (fabs(1.0 - amp*amp*2.0) - 1.0);
                } else if (cur_wt == 9) {
                    sample += ot_str * one_bit_noise;
                } else if (cur_wt == 11) {
                    sample += ot_str * buzz_val;
                }
                ot_str *= (1.0 - ot_falloff);
            }

            if (use_filt) {
                double lp_old = lp_pos;
                lp_cut *= lp_dcut;
                if (lp_cut < 0.0) lp_cut = 0.0;
                if (lp_cut > 0.1) lp_cut = 0.1;
                if (lp_on) {
                    lp_dpos += (sample - lp_pos) * lp_cut;
                    lp_dpos *= lp_damp;
                } else {
                    lp_pos = sample;
                    lp_dpos = 0.0;
                }
                lp_pos += lp_dpos;
                hp_pos += lp_pos - lp_old;
                hp_pos *= (1.0 - hp_cut);
                sample = hp_pos;
            }

            if (use_flng) {
                flng_buf[flng_pos & 1023] = sample;
                sample += flng_buf[(flng_pos - flng_int + 1024) & 1023];
                flng_pos = (flng_pos + 1) & 1023;
            }

            supersample += sample;
        }

        if (supersample > 8.0) supersample = 8.0;
        if (supersample < -8.0) supersample = -8.0;
        supersample = mv2 * env_vol * supersample * 0.125;

        bc_phase += bc_freq;
        if (bc_phase > 1.0) { bc_phase = 0.0; bc_last = supersample; }
        bc_freq += bc_sweep;
        if (bc_freq < 0.0) bc_freq = 0.0;
        if (bc_freq > 1.0) bc_freq = 1.0;
        supersample = bc_last;

        if (supersample > 0.0) supersample = pow(supersample, comp_factor);
        else if (supersample < 0.0) supersample = -pow(-supersample, comp_factor);

        if (muted) supersample = 0.0;

        out[i] = (float)supersample;
    }

    double peak = 0.0;
    for (int i = 0; i < total; i++) {
        double v = fabs(out[i]);
        if (v > peak) peak = v;
    }
    if (peak > 1e-9) {
        double scale = 0.9 / peak;
        for (int i = 0; i < total; i++) out[i] *= (float)scale;
    }

    BfxrWave wave;
    wave.num_samples = total;
    wave.samples = (int16_t*)malloc(sizeof(int16_t) * total);
    for (int i = 0; i < total; i++) wave.samples[i] = (int16_t)(out[i] * 32767.0);
    free(out);
    return wave;
}

BfxrWave bfxr_generate_wave_blended(const double params[NUM_PARAMS], int wave_type_a, int wave_type_b, double blend_t) {
    double p_a[NUM_PARAMS], p_b[NUM_PARAMS];
    memcpy(p_a, params, sizeof(double) * NUM_PARAMS);
    memcpy(p_b, params, sizeof(double) * NUM_PARAMS);
    p_a[0] = (double)wave_type_a;
    p_b[0] = (double)wave_type_b;

    BfxrWave wave_a = bfxr_generate_wave(p_a);
    BfxrWave wave_b = bfxr_generate_wave(p_b);

    int len_a = wave_a.num_samples;
    int len_b = wave_b.num_samples;
    int n = len_a > len_b ? len_a : len_b;

    BfxrWave result;
    result.num_samples = n;
    result.samples = (int16_t*)malloc(sizeof(int16_t) * n);

    for (int i = 0; i < n; i++) {
        double sa = i < len_a ? wave_a.samples[i] / 32767.0 : 0.0;
        double sb = i < len_b ? wave_b.samples[i] / 32767.0 : 0.0;
        result.samples[i] = (int16_t)((sa + blend_t * (sb - sa)) * 32767.0);
    }

    bfxr_wave_free(&wave_a);
    bfxr_wave_free(&wave_b);
    return result;
}

void bfxr_wave_free(BfxrWave* wave) {
    if (wave->samples) {
        free(wave->samples);
        wave->samples = NULL;
        wave->num_samples = 0;
    }
}
