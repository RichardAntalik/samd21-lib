#include "Chip.h"
#include "SerialConsole.h"

// ---- EMA low-pass filter ---------------------------------------------------
class Filter {
public:
    explicit Filter(float a) : a_(a), v_(0.0f), ok_(false) {}
    void prime(float v) { v_ = v; ok_ = true; }
    float update(float raw) {
        if (!ok_) { v_ = raw; ok_ = true; }
        else v_ = a_ * raw + (1.0f - a_) * v_;
        return v_;
    }
    float value() const { return v_; }
private:
    float a_, v_;
    bool ok_;
};

// ---- INA219: PV bus voltage + shunt current + power ------------------------
class INA219 {
public:
    explicit INA219(I2C& bus) : bus_(bus) {}
    float bus_v()   { return (reg(0x02) >> 3) * 0.004f; }
    float current() { return (int16_t)reg(0x01) * -0.00142857f; }
    float power() {
        float v = bus_v(), i = current();
        return (v < 0.0f || i <= -999.0f) ? -9999.0f : v * i;
    }
private:
    static constexpr uint8_t ADDR = 0x4F;
    I2C& bus_;
    uint16_t reg(uint8_t r) {
        uint8_t b[2];
        if (bus_.send(ADDR, r) != Status::OK) return 0;
        if (bus_.receive(ADDR, b, 2) != Status::OK) return 0;
        return (uint16_t)((b[0] << 8) | b[1]);
    }
};

I2C i2c(chip.PA23, chip.PA22, 50000);
INA219 ina(i2c);
Filter adc_f(0.05f), pwr_f(0.10f);

// ---- state -----------------------------------------------------------------
static float duty = 0.40f, mppt_prev = 0.0f;
static int   mppt_dir = 1;
static bool  manual = false;
static float manual_duty = 0.0f;

// Battery voltage: AIN5 (PA05) through a 20.043x divider.
float batt_v() {
    return adc_f.update(chip.PA05.adc_read() / 4095.0f * 20.043f);
}

// ---- status line -------------------------------------------------------------
void print_status() {
    Serial.printf("Batt %.2fV  PV %.2fV  P %.1fW  duty %.3f\n",
                  batt_v(), ina.bus_v(), pwr_f.value(), duty);
}

// ---- MPPT with a hard 14.1 V battery ceiling (200 ms period) ---------------
void control() {
    float pv = ina.bus_v();
    float p  = ina.power();
    if (p < 0.0f) p = 0.0f;
    p = pwr_f.update(p);
    float batt = batt_v();

    if (pv < 6.0f) { duty = 0.0f; mppt_prev = 0.0f; return; }  // night

    if (batt >= 14.1f) {                                        // at ceiling
        duty -= 0.02f;
        mppt_prev = p;
    } else {                                                    // perturb & observe
        float dp = p - mppt_prev;
        if ((dp < 0.0f ? -dp : dp) > 0.25f && dp < 0.0f) mppt_dir = -mppt_dir;
        duty += 0.05f * mppt_dir;
        mppt_prev = p;
    }

    if (duty > 0.95f) duty = 0.95f;
    if (duty < 0.30f) duty = 0.30f;
    if (manual) duty = manual_duty;

    chip.PA08.duty(duty);
}

auto console = createConsole(
    "pwm",    [](float duty){manual = true; manual_duty = duty;},    "<0.0-1.0> manual duty",
    "resume", [](){manual = false;}, "back to auto MPPT"
);

void setup() {
    delay(2000);
    chip.PA07.use_out();          // buck control
    chip.PA10.use_out();          // heartbeat LED
    chip.PA08.use_pwm(150470).duty(0.0f);
    chip.PA05.use_adc();
    adc_f.prime(12.0f);
    pwr_f.prime(0.0f);

    chip.scheduler.every(500,[](){ chip.PA10.toggle(); });
    chip.scheduler.every(500,print_status);
    chip.scheduler.every(200,control);
}

void loop() {
    chip.scheduler.tick();
    console.handleInput();
}
