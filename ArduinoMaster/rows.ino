// static qualifiers seem to do very little in the Arduino world

#define PEDAL_MIN 40    // anything below this counts as normal speed w/o interpolation
#define PEDAL_MAX 1024  // 10-bit ADC
#define ADVANCE_MAX 4.0 // limit of 15.0

#define FIELDS 18
#define DACS 16
#define ENCODER_COUNTS 0x1756

// scale-factor calculation
#define SCALE (uint16_t)(ONE * (ADVANCE_MAX - 1.0) / PEDAL_MAX)   // coarse but fast (0.25 ADVANCE_MAX granularity w/ 10-bit ADC)

static ProfileRow row[2];    // ping-pong buffers
static ProfileRow *next = &row[0];
//uint16_t processed[FIELDS];

// fixed-point 4.12 format
#define STEP_INTEGER_BITS 4
#define ONE 0x1000                // 1.0 (0x10000UL >> STEP_INTEGER_BITS)
static uint16_t row_step        = ONE;   // never less than ONE
static uint16_t row_accumulator = 0;

//#define current() (next == &row[0] ? &row[1] : &row[0])
static ProfileRow* current() {
  return next == &row[0] ? &row[1] : &row[0];
}

void row_init() {
  profile_read(current(), sizeof(ProfileRow));
  profile_read(next, sizeof(ProfileRow));
  row_accumulator = 0;
}

static void row_read_next() {
  next = current();
  profile_read(next, sizeof(ProfileRow));
}

void row_advance() {
  row_accumulator += row_step;
  do {  // always advance at least one row
    row_read_next();
    row_accumulator -= ONE;
  } while(row_accumulator >= ONE);
}

static uint16_t interpolate_angle(uint16_t a, uint16_t b, uint16_t fraction) {
  int16_t delta, weight, result;
  
  delta = (int16_t)(b - a);
  if(delta < -ENCODER_COUNTS/2) {
    delta += ENCODER_COUNTS;
  } else if(delta >= ENCODER_COUNTS/2) {
    delta -= ENCODER_COUNTS;
  }
  
  weight = (int16_t)((delta * (int32_t)fraction) >> 16);
  
  result = weight + a;
  if(result < 0) {
    result += ENCODER_COUNTS;
  } else if(result >= ENCODER_COUNTS) {
    result -= ENCODER_COUNTS;
  }
  
  return (uint16_t)result;
}

static uint16_t interpolate_simple(uint16_t a, uint16_t b, uint16_t fraction) {
  int16_t delta, weight;
  delta = (int16_t)(b - a);
  weight = (int16_t)((delta * (int32_t)fraction) >> 16);
  return (uint16_t)(weight + a);
}

static void interpolate(uint16_t *interpolated, ProfileRow *a, ProfileRow *b, uint16_t fraction) {    // fraction shall be left-justified unsigned 16-bit (i.e. fixed-point 0.16 format)
  int i;
  interpolated[0] = interpolate_angle(a->left,  b->left,  fraction);
  interpolated[1] = interpolate_angle(a->right, b->right, fraction);
  for(i = 0; i < DACS; i++) {
    interpolated[i + 2] = interpolate_simple(a->dac[i], b->dac[i], fraction);
  }
}

void row_next(uint16_t *processed) {
  interpolate(processed, current(), next, row_accumulator << 4);
  row_advance();
}

void row_calculate_step(uint16_t pedal) {
  if(pedal <= PEDAL_MIN) {
    row_step = ONE;
    row_accumulator = 0;
  } else {
    row_step = pedal * SCALE + ONE;
  }
}
