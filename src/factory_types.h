/* Immutable upstream factory synthesis data. GPL-3.0-or-later adapter;
 * sound data retains the individual pack licences in factory/. */
#define FACTORY_UNITS 10
#define FACTORY_CURVES 8
#define FACTORY_POINTS 32
enum {FA_ON,FA_WAVE,FA_FREQ,FA_LEVEL,FA_PHASE,FA_DENSITY,FA_PITCH,FA_SEED,
      FA_FM,FA_FMK,FA_FILTER_ON,FA_FILTER_TYPE,FA_CUTOFF,FA_Q,
      FA_DRIVE_ON,FA_DRIVE_TYPE,FA_DRIVE,FA_DRIVE_IN,FA_DRIVE_OUT,FA_SCALARS};
enum {FC_AMP,FC_FREQ,FC_PITCH,FC_DENSITY,FC_CUTOFF,FC_Q,FC_DRIVE,FC_VOLUME};
typedef struct {int count,apply;struct gkick_envelope_point_info point[FACTORY_POINTS];} FactoryCurve;
typedef struct {float limiter;float scalar[FACTORY_UNITS][FA_SCALARS];
    FactoryCurve curves[FACTORY_UNITS][FACTORY_CURVES];
    float baseline[GP_COUNT];const char *digest;} FactorySound;
