# IR Sender basic tests

uncomment IR_USE_NEC_PROTOCOL
test with `irReceiver_NEC`
test with `irReceiver_raw`      IR_MODE = 0
test with `irReceiver_custom`   IR_MODE = 0

comment IR_USE_NEC_PROTOCOL
test with `irReceiver_raw`      IR_MODE = 1
test with `irReceiver_custom`   IR_MODE = 1

do the same tests for IR_USE_TIM1_PWM enabled and disabled